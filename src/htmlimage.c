/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999 Red Hat Software
   Copyright (C) 1999, 2000 Helix Code, Inc.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "htmlobject.h"
#include "htmlimage.h"
#include "htmlengine.h"
#include "htmlpainter.h"
#include "gtkhtml-private.h"

static void html_image_update_scaled_pixbuf (HTMLImage *image);


HTMLImageClass html_image_class;


static GdkPixbuf *
generate_pixbuf_for_selection (HTMLImage *image,
			       HTMLPainter *painter,
			       gint clip_x, gint clip_y,
			       gint clip_width, gint clip_height)
{
	const GdkColor *selection_color;
	GdkPixbuf *src;
	GdkPixbuf *pixbuf;
	guchar *sp, *dp, *s, *d;
	guint has_alpha;
	guint n_channels;
	guint bits_per_sample;
	guint src_rowstride, dest_rowstride;
	guint src_width, src_height;
	guint x, y;

	selection_color = html_painter_get_default_highlight_color (painter);

	if (image->scaled_pixbuf != NULL)
		src = image->scaled_pixbuf;
	else
		src = image->image_ptr->pixbuf;

	src_width = gdk_pixbuf_get_width (src);
	src_height = gdk_pixbuf_get_height (src);

	if (clip_width < 0)
		clip_width = src_width;
	if (clip_height < 0)
		clip_height = src_height;

	if (clip_x < 0 || clip_x >= src_width)
		return NULL;
	if (clip_y < 0 || clip_y >= src_height)
		return NULL;

	if (src_width < clip_x + clip_width)
		clip_width = src_width - clip_x;
	if (src_height < clip_y + clip_height)
		clip_height = src_height - clip_y;

	if (clip_width == 0 || clip_height == 0)
		return NULL;

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	n_channels = gdk_pixbuf_get_n_channels (src);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample (src);

	pixbuf = gdk_pixbuf_new (ART_PIX_RGB, has_alpha, bits_per_sample, clip_width, clip_height);

	src_rowstride = gdk_pixbuf_get_rowstride (src);
	dest_rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	sp = gdk_pixbuf_get_pixels (src) + clip_y * src_rowstride;
	dp = gdk_pixbuf_get_pixels (pixbuf);

	/* This is *very* unoptimized.  */

	for (y = 0; y < clip_height; y++) {
		s = sp, d = dp;

		for (x = 0; x < clip_width; x++) {
			gint r, g, b;

			r = (s[0] + (selection_color->red >> 8)) / 2 ;
			g = (s[1] + (selection_color->green >> 8)) / 2 ;
			b = (s[2] + (selection_color->blue >> 8)) / 2 ;

			d[0] = r;
			d[1] = g;
			d[2] = b;

			if (has_alpha)
				d[3] = s[3];

			s += n_channels, d += n_channels;
		}

		sp += src_rowstride, dp += dest_rowstride;
	}

	return pixbuf;
}


/* HTMLObject methods.  */

/* FIXME: We should close the stream here, too.  But in practice we cannot
   because the stream pointer might be invalid at this point, and there is no
   way to set it to NULL when the stream is closed.  This clearly sucks and
   must be fixed.  */
static void
destroy (HTMLObject *image)
{
	if (HTML_IMAGE (image)->scaled_pixbuf)
		gdk_pixbuf_unref (HTML_IMAGE (image)->scaled_pixbuf);

	html_image_factory_unregister (HTML_IMAGE (image)->image_ptr->factory,
				       HTML_IMAGE (image)->image_ptr, HTML_IMAGE (image));
}

static void
set_max_width (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	if (o->percent > 0)
		o->max_width = max_width;

	if (HTML_IMAGE (o)->image_ptr == NULL || HTML_IMAGE (o)->image_ptr->pixbuf == NULL)
		return;

	if (o->percent > 0) {
		o->width = (gint) (o->max_width) * (gint) (o->percent) / 100;
		if (!HTML_IMAGE (o)->predefinedHeight)
			o->ascent = HTML_IMAGE (o)->image_ptr->pixbuf->art_pixbuf->height * o->width /
				HTML_IMAGE (o)->image_ptr->pixbuf->art_pixbuf->width + HTML_IMAGE (o)->border;
		o->width += HTML_IMAGE (o)->border * 2;
	}

	if (HTML_IMAGE (o)->scaled)
		html_image_update_scaled_pixbuf (HTML_IMAGE (o));
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	if (o->percent > 0)
		return 1;

	return o->width * html_painter_get_pixel_size (painter);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	int base_x, base_y, clip_x, clip_y, clip_width, clip_height;

	/* FIXME horizontal check? */
	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	/*
	 * We need three coords, all screen-relative:
	 * base_x/base_y
	 * clipx/clipy
	 * clipwidth/clipheight
	 * 
	 * We have three coords:
	 * tx/ty - add this to object-relative coords to convert to screen coords
	 * x/y - object-relative clip coordinates
	 * width/height - clip width/height
	 *
	 */
	base_x = o->x + tx + HTML_IMAGE (o)->border;
	base_y = o->y - o->ascent + ty + HTML_IMAGE (o)->border;

	clip_x = base_x + x;
	clip_y = base_y + y;

	clip_width = width;
	clip_height = height;

	if (HTML_IMAGE (o)->image_ptr->pixbuf == NULL) {
		html_painter_draw_panel (p, 
					 o->x + tx, o->y + ty - o->ascent, 
					 o->width, o->ascent,
					 TRUE, 1);
	} else {
		GdkPixbuf *pixbuf;

		if (o->selected) {
			/* FIXME something is wrong with the coordinates here,
                           so I just generate the whole thing.  Also, it might
                           be a performance problem.  */
			pixbuf = generate_pixbuf_for_selection (HTML_IMAGE (o),
								p,
								0, 0,
								-1, -1);
		} else if (HTML_IMAGE (o)->scaled) {
			pixbuf = gdk_pixbuf_ref (HTML_IMAGE (o)->scaled_pixbuf);
		} else {
			pixbuf = gdk_pixbuf_ref (HTML_IMAGE (o)->image_ptr->pixbuf);
		}

		if (pixbuf != NULL) {
			html_painter_draw_pixmap (p, base_x, base_y,
						  pixbuf,
						  clip_x, clip_y,
						  clip_width, clip_height);
			gdk_pixbuf_unref (pixbuf);
		}
	}
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	return o->width;
}

static void
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
{
}

static const gchar *
get_url (HTMLObject *o)
{
	HTMLImage *image;

	image = HTML_IMAGE (o);
	return image->url;
}

static const gchar *
get_target (HTMLObject *o)
{
	HTMLImage *image;

	image = HTML_IMAGE (o);
	return image->target;
}

static gboolean
accepts_cursor (HTMLObject *o)
{
	return TRUE;
}


void
html_image_type_init (void)
{
	html_image_class_init (&html_image_class, HTML_TYPE_IMAGE);
}

void
html_image_class_init (HTMLImageClass *image_class,
		       HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (image_class);

	html_object_class_init (object_class, type);

	object_class->draw = draw;
	object_class->destroy = destroy;
	object_class->set_max_width = set_max_width;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_size = calc_size;
	object_class->get_url = get_url;
	object_class->get_target = get_target;
	object_class->accepts_cursor = accepts_cursor;
}

void
html_image_init (HTMLImage *image,
		 HTMLImageClass *klass,
		 HTMLImageFactory *imf,
		 gchar *filename,
		 const gchar *url,
		 const gchar *target,
		 gint max_width, 
		 gint width, gint height,
		 gint percent, gint border)
{
	HTMLObject *object;

	object = HTML_OBJECT (image);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	image->predefinedWidth = (width < 0 && !percent) ? FALSE : TRUE;
	image->predefinedHeight = height < 0 ? FALSE : TRUE;

	image->url = g_strdup (url);
	image->target = g_strdup (target);

	image->border = border;

	object->percent = percent;
	object->max_width = max_width;
	object->ascent = height + border;
	object->descent = border;

	if (percent > 0) {
		object->width = (gint)max_width * (gint)percent / 100;
		object->width += border * 2;
	} else {
		object->width = width + border * 2;
	}

	if (!image->predefinedWidth && !object->percent)
		object->width = 32;
	if (!image->predefinedHeight)
		object->ascent = 32;

	image->scaled = FALSE;
	image->scaled_pixbuf = NULL;

	image->image_ptr = html_image_factory_register (imf, image, filename);
}

HTMLObject *
html_image_new (HTMLImageFactory *imf,
		gchar *filename,
		const gchar *url,
		const gchar *target,
		gint max_width,
		gint width,
		gint height,
		gint percent,
		gint border)
{
	HTMLImage *image;

	image = g_new(HTMLImage, 1);
	html_image_init (image, &html_image_class, imf, filename,
			 url, target, max_width, width, height, percent, border);

	return HTML_OBJECT (image);
}


/* FIXME this was formerly known as `html_image_init()'.  I am not sure what this is for.  */

static void
html_image_update_scaled_pixbuf (HTMLImage *image) 
{
	HTMLObject *object = HTML_OBJECT (image);

	g_assert (image->scaled);

	if (image->scaled_pixbuf)
		gdk_pixbuf_unref (image->scaled_pixbuf);

	image->scaled_pixbuf = gdk_pixbuf_scale_simple (image->image_ptr->pixbuf, 
							object->width - 2 * image->border,
							object->ascent - image->border,
							ART_FILTER_TILES);
}

static void
html_image_setup (HTMLImage *image)
{
	HTMLObject *object = HTML_OBJECT (image);

	if (object->percent > 0) {
		object->width = (gint) (object->max_width) * (gint) (object->percent) / 100;
		if (!image->predefinedHeight && image->image_ptr)
			object->ascent = image->image_ptr->pixbuf->art_pixbuf->height *
				object->width / image->image_ptr->pixbuf->art_pixbuf->width;
		object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
	}

	if (image->image_ptr && image->image_ptr->pixbuf) {

		if (image->predefinedWidth && object->width != 
		    image->image_ptr->pixbuf->art_pixbuf->width)
			image->scaled = TRUE;
		else if (image->predefinedHeight && object->ascent != 
			 image->image_ptr->pixbuf->art_pixbuf->height)
			image->scaled = TRUE;

		if (!image->predefinedWidth)
			object->width = image->image_ptr->pixbuf->art_pixbuf->width;

		if (!image->predefinedHeight)
			object->ascent = image->image_ptr->pixbuf->art_pixbuf->height;

		if (image->scaled)
			html_image_update_scaled_pixbuf (image);
	}
	
	if (!image->predefinedWidth)
		object->width += image->border * 2;
	if (!image->predefinedHeight)
		object->ascent += image->border;

}


/* HTMLImageFactory stuff.  */

struct _HTMLImageFactory {
	HTMLEngine *engine;
	GHashTable *loaded_images;
};

static void html_image_factory_end_pixbuf    (GtkHTMLStreamHandle handle,
					      GtkHTMLStreamStatus status,
					      gpointer user_data);
static void html_image_factory_write_pixbuf  (GtkHTMLStreamHandle handle,
					      const guchar *buffer, size_t size,
					      gpointer user_data);
static void html_image_factory_area_prepared (GdkPixbufLoader *loader, HTMLImagePointer *ip);
static void html_image_factory_area_updated  (GdkPixbufLoader *loader,
					      guint x, guint y, guint width, guint height,
					      HTMLImagePointer *ip);

static void
html_image_factory_end_pixbuf (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, gpointer user_data)
{
	HTMLImagePointer *ip = user_data;
	GSList *cur;

	if (ip->pixbuf) {
		for (cur = ip->interests; cur; cur = cur->next){
			if (cur->data){
				HTMLImage *i = HTML_IMAGE (cur->data);
				if (i->scaled)
					html_image_update_scaled_pixbuf (i);
			}
		}
	}
	html_engine_schedule_update (ip->factory->engine);
	
	gdk_pixbuf_loader_close (ip->loader);
	ip->loader = NULL;
}

static void
html_image_factory_write_pixbuf (GtkHTMLStreamHandle handle, const guchar *buffer,
				size_t size, gpointer user_data)
{
	HTMLImagePointer *p = user_data;
	/* FIXME ! Check return value */
	gdk_pixbuf_loader_write (p->loader, buffer, size);
}

static void
html_image_factory_area_updated  (GdkPixbufLoader *loader,
				  guint x, guint y, guint width, guint height,
				  HTMLImagePointer *ip)
{
	GSList *cur;
	
	for (cur = ip->interests; cur; cur = cur->next){
		if (cur->data){
			HTMLImage *i = HTML_IMAGE (cur->data);
			if (i->scaled)
				html_image_update_scaled_pixbuf (i);
		}
	}
	/* XXX fixme - only if we are onscreen, and even then we just need to do something to redraw ourselves
	   rather than the whole stupid area. Also should only redraw if a significant new amount of info has come in */

#if 0
	html_engine_schedule_update (e);
#endif
}

static void
html_image_factory_area_prepared (GdkPixbufLoader *loader, HTMLImagePointer *ip)
{
	GSList *cur;
	gboolean need_redraw = FALSE;
	
	ip->pixbuf = gdk_pixbuf_loader_get_pixbuf (ip->loader);
	g_assert (ip->pixbuf);
	
	for (cur = ip->interests; cur; cur = cur->next){
		if (cur->data){
			HTMLImage *i = HTML_IMAGE (cur->data);
			if (!i->predefinedWidth && !i->predefinedHeight && !HTML_OBJECT (i)->percent)
				need_redraw = TRUE;
			html_image_setup (i);
		} else {
			/* Document background pixmap */
			need_redraw = TRUE;
		}
	}
	
	if (need_redraw)
		html_engine_schedule_update (ip->factory->engine);
}

HTMLImageFactory *
html_image_factory_new (HTMLEngine *e)
{
	HTMLImageFactory *retval;
	retval = g_new (HTMLImageFactory, 1);
	retval->engine = e;
	retval->loaded_images = g_hash_table_new (g_str_hash, g_str_equal);

	return retval;
}

static gboolean
cleanup_images (gpointer key, gpointer value, gpointer user_data)
{
	HTMLImagePointer *ptr;
	gboolean retval = FALSE;

	ptr = value;

	if (user_data){
		g_slist_free (ptr->interests);
		ptr->interests = NULL;
	}

	if (!ptr->interests){
		retval = TRUE;
		g_free (ptr->url);
		if (ptr->loader)
			gdk_pixbuf_loader_close (ptr->loader);
		if (ptr->pixbuf)
			gdk_pixbuf_unref (ptr->pixbuf);
		g_free (ptr);
	}

	return retval;
}

void
html_image_factory_cleanup (HTMLImageFactory *factory)
{
	g_return_if_fail (factory);

	g_hash_table_foreach_remove (factory->loaded_images, cleanup_images, NULL);
}

void
html_image_factory_free (HTMLImageFactory *factory)
{
	g_return_if_fail (factory);

	g_hash_table_foreach_remove (factory->loaded_images, cleanup_images, factory);
	g_hash_table_destroy (factory->loaded_images);
	g_free (factory);
}

HTMLImagePointer *
html_image_factory_register (HTMLImageFactory *factory, HTMLImage *i, const char *filename)
{
	HTMLImagePointer *retval;

	g_return_val_if_fail (factory, NULL);
	g_return_val_if_fail (filename, NULL);

	retval = g_hash_table_lookup (factory->loaded_images, filename);

	if (!retval){
		GtkHTMLStreamHandle handle;

		retval = g_new (HTMLImagePointer, 1);
		retval->url = g_strdup (filename);
		retval->loader = gdk_pixbuf_loader_new ();
		retval->pixbuf = NULL;
		retval->interests = NULL;
		retval->factory = factory;

		gtk_signal_connect (GTK_OBJECT (retval->loader), "area_prepared",
				    GTK_SIGNAL_FUNC (html_image_factory_area_prepared),
				    retval);
		
		gtk_signal_connect (GTK_OBJECT (retval->loader), "area_updated",
				    GTK_SIGNAL_FUNC (html_image_factory_area_updated),
				    retval);
		
		handle = gtk_html_stream_new (GTK_HTML (factory->engine->widget),
					      retval->url,
					      html_image_factory_write_pixbuf,
					      html_image_factory_end_pixbuf,
					      retval);

		g_hash_table_insert (factory->loaded_images, retval->url, retval);

		/* This is a bit evil, I think.  But it's a lot better here
		   than in the HTMLImage object.  FIXME anyway -- ettore  */
		
		gtk_signal_emit_by_name (GTK_OBJECT (factory->engine), "url_requested", filename,
					 handle);

	} else if (i){

		i->image_ptr = retval;
		html_image_setup (i);
	}
	retval->interests = g_slist_prepend (retval->interests, i);

	return retval;
}

void
html_image_factory_unregister (HTMLImageFactory *factory, HTMLImagePointer *pointer, HTMLImage *i)
{
	pointer->interests = g_slist_remove (pointer->interests, i);
}
