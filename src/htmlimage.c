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


#define DEFAULT_SIZE 48


HTMLImageClass html_image_class;


static guint
get_actual_width (HTMLImage *image)
{
	if (image->specified_width > 0) {
		return image->specified_width;
	} else if (HTML_OBJECT (image)->percent > 0) {
		return (HTML_OBJECT (image)->max_width * HTML_OBJECT (image)->percent) / 100;
	} else if (image->image_ptr == NULL || image->image_ptr->pixbuf == NULL) {
		return DEFAULT_SIZE;
	} else {
		return gdk_pixbuf_get_width (image->image_ptr->pixbuf);
	}
}

static guint
get_actual_height (HTMLImage *image)
{
	if (image->specified_height > 0) {
		return image->specified_height;
	} else if (image->image_ptr == NULL || image->image_ptr->pixbuf == NULL) {
		return DEFAULT_SIZE;
	} else {
		return gdk_pixbuf_get_height (image->image_ptr->pixbuf);
	}
}

#if 0
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
#endif


/* HTMLObject methods.  */

/* FIXME: We should close the stream here, too.  But in practice we cannot
   because the stream pointer might be invalid at this point, and there is no
   way to set it to NULL when the stream is closed.  This clearly sucks and
   must be fixed.  */
static void
destroy (HTMLObject *image)
{
	html_image_factory_unregister (HTML_IMAGE (image)->image_ptr->factory,
				       HTML_IMAGE (image)->image_ptr, HTML_IMAGE (image));
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	guint pixel_size;
	guint min_width;

	pixel_size = html_painter_get_pixel_size (painter);

	if (o->percent > 0)
		min_width = 1;
	else
		min_width = get_actual_width (HTML_IMAGE (o));

	min_width += HTML_IMAGE (o)->border * 2;

	return pixel_size * min_width;
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	guint pixel_size;
	guint width;

	pixel_size = html_painter_get_pixel_size (painter);
	width = get_actual_width (HTML_IMAGE (o)) + HTML_IMAGE (o)->border * 2;

	return pixel_size * width;
}

static void
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
{
	HTMLImage *image;
	guint pixel_size;
	guint width, height;

	image = HTML_IMAGE (o);

	pixel_size = html_painter_get_pixel_size (painter);

	width = get_actual_width (image);
	height = get_actual_height (image);

	o->width = (width + image->border * 2) * pixel_size;
	o->descent = image->border * pixel_size;
	o->ascent = (height + image->border) * pixel_size;
}

static void
draw (HTMLObject *o,
      HTMLPainter *painter,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLImage *image;
	GdkPixbuf *pixbuf;
	gint base_x, base_y;
	gint scale_width, scale_height;
	const GdkColor *color;
	guint pixel_size;

#if 0
	if (y + height < o->y - o->ascent || y > o->y + o->descent
	    || x + width < o->x || x > o->x + o->width)
		return;
#endif

	image = HTML_IMAGE (o);
	pixbuf = image->image_ptr->pixbuf;

	if (pixbuf == NULL) {
		html_painter_draw_panel (painter, 
					 o->x + tx, o->y + ty - o->ascent, 
					 o->width, o->ascent + o->descent,
					 TRUE, 1);
		return;
	}

	pixel_size = html_painter_get_pixel_size (painter);

	base_x = o->x + tx + image->border * pixel_size;
	base_y = o->y - o->ascent + ty + image->border * pixel_size;

	scale_width = get_actual_width (image);
	scale_height = get_actual_height (image);

	color = NULL;		/* FIXME */

	html_painter_draw_pixmap (painter, pixbuf,
				  base_x, base_y,
				  scale_width, scale_height,
				  color);
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
		 gint width, gint height,
		 gint percent, gint border)
{
	HTMLObject *object;

	object = HTML_OBJECT (image);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	image->url = g_strdup (url);
	image->target = g_strdup (target);

	image->specified_width = width;
	image->specified_height = height;
	image->border = border;

	object->percent = percent;

	image->image_ptr = html_image_factory_register (imf, image, filename);
}

HTMLObject *
html_image_new (HTMLImageFactory *imf,
		gchar *filename,
		const gchar *url,
		const gchar *target,
		gint width, gint height,
		gint percent, gint border)
{
	HTMLImage *image;

	image = g_new(HTMLImage, 1);

	html_image_init (image, &html_image_class, imf, filename,
			 url, target, width, height, percent, border);

	return HTML_OBJECT (image);
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

static void
html_image_factory_end_pixbuf (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, gpointer user_data)
{
	HTMLImagePointer *ip = user_data;

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
html_image_factory_area_prepared (GdkPixbufLoader *loader, HTMLImagePointer *ip)
{
	ip->pixbuf = gdk_pixbuf_loader_get_pixbuf (ip->loader);
	g_assert (ip->pixbuf);

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
	} else if (i) {
		i->image_ptr = retval;
	}

	retval->interests = g_slist_prepend (retval->interests, i);

	return retval;
}

void
html_image_factory_unregister (HTMLImageFactory *factory, HTMLImagePointer *pointer, HTMLImage *i)
{
	pointer->interests = g_slist_remove (pointer->interests, i);
}
