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

   TODO:

     - implement proper animation loading (now it loops thru loaded frames
       and does not stop when loading is in progress and we are out of frames)
     - look at gdk-pixbuf to make gdk_pixbuf_compose work (look also
       on gk_pixbuf_render_to_drawable_alpha)
     - take care about all the frame->action values

*/

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "htmlobject.h"
#include "htmlimage.h"
#include "htmlengine.h"
#include "htmlpainter.h"
#include "gtkhtml-private.h"


/* HTMLImageFactory stuff.  */

struct _HTMLImageFactory {
	HTMLEngine *engine;
	GHashTable *loaded_images;
};


#define DEFAULT_SIZE 48

#if 1
#define gdk_pixbuf_animation_get_width animation_actual_width
#define gdk_pixbuf_animation_get_height animation_actual_height
#endif


HTMLImageClass html_image_class;
static HTMLObjectClass *parent_class = NULL;

static HTMLImageAnimation *html_image_animation_new     (HTMLImage *image);
static void                html_image_animation_destroy (HTMLImageAnimation *anim);
static HTMLImagePointer   *html_image_pointer_new       (const char *filename, HTMLImageFactory *factory);
static void                html_image_pointer_destroy   (HTMLImagePointer *ip);

static void render_cur_frame (HTMLImage *image, gint nx, gint ny);



static guint
get_actual_width (HTMLImage *image,
		  HTMLPainter *painter)
{
	GdkPixbuf *pixbuf = image->image_ptr->pixbuf;
	GdkPixbufAnimation *anim = image->image_ptr->animation;

	gint width;

	if (image->specified_width > 0) {
		width = image->specified_width * html_painter_get_pixel_size (painter);
	} else if (HTML_OBJECT (image)->percent > 0) {
		width = (HTML_OBJECT (image)->max_width * HTML_OBJECT (image)->percent) / 100;
	} else if (image->image_ptr == NULL || pixbuf == NULL) {
		width = DEFAULT_SIZE * html_painter_get_pixel_size (painter);
	} else {
		width = ((anim) ? gdk_pixbuf_animation_get_width (anim) : gdk_pixbuf_get_width (pixbuf))
			* html_painter_get_pixel_size (painter);
	}

	return width;
}


/* FIXME these will be replaced with the proper functions from gdk-pixbuf soon */
gint
animation_actual_height (GdkPixbufAnimation *ganim) 
{
	GdkPixbufFrame    *frame;
	GList *cur = gdk_pixbuf_animation_get_frames (ganim);
	gint h = 0, nh;

	while (cur) {
		frame = (GdkPixbufFrame *) cur->data;

		nh = gdk_pixbuf_get_height (frame->pixbuf) + frame->y_offset;
		h = MAX (h, nh);
		cur = cur->next;
	}
}

gint
animation_actual_width (GdkPixbufAnimation *ganim) 
{
	GdkPixbufFrame    *frame;
	GList *cur = gdk_pixbuf_animation_get_frames (ganim);
	gint w = 0, nw;

	while (cur) {
		frame = (GdkPixbufFrame *) cur->data;

		nw = gdk_pixbuf_get_width (frame->pixbuf) + frame->x_offset;
		w = MAX (w, nw);
		cur = cur->next;
	}
}

static guint
get_actual_height (HTMLImage *image,
		   HTMLPainter *painter)
{
	GdkPixbuf *pixbuf = image->image_ptr->pixbuf;
	GdkPixbufAnimation *anim = image->image_ptr->animation;
	gint height;

	if (image->specified_height > 0) {
		height = image->specified_height * html_painter_get_pixel_size (painter);
	} else if (image->image_ptr == NULL || pixbuf == NULL) {
		height = DEFAULT_SIZE * html_painter_get_pixel_size (painter);
	} else {
		height = (((anim) ? gdk_pixbuf_animation_get_height (anim) : gdk_pixbuf_get_height (pixbuf))
			  * html_painter_get_pixel_size (painter));
	}

	return height;
}


/* HTMLObject methods.  */

/* FIXME: We should close the stream here, too.  But in practice we cannot
   because the stream pointer might be invalid at this point, and there is no
   way to set it to NULL when the stream is closed.  This clearly sucks and
   must be fixed.  */
static void
destroy (HTMLObject *o)
{
	HTMLImage *image = HTML_IMAGE (o);

	html_image_factory_unregister (image->image_ptr->factory,
				       image->image_ptr, HTML_IMAGE (image));
	if (image->animation)
		html_image_animation_destroy (image->animation);

	HTML_OBJECT_CLASS (parent_class)->destroy (o);
}

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	/* FIXME not sure this is all correct.  */

	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_IMAGE (dest)->image_ptr = HTML_IMAGE (self)->image_ptr;
	HTML_IMAGE (dest)->border = HTML_IMAGE (self)->border;
	HTML_IMAGE (dest)->specified_width = HTML_IMAGE (self)->specified_width;
	HTML_IMAGE (dest)->specified_height = HTML_IMAGE (self)->specified_height;
	HTML_IMAGE (dest)->url = HTML_IMAGE (self)->url;
	HTML_IMAGE (dest)->target = HTML_IMAGE (self)->target;
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	HTMLImage *image = HTML_IMAGE (o);
	guint pixel_size;
	guint min_width;

	pixel_size = html_painter_get_pixel_size (painter);

	if (o->percent > 0)
		min_width = pixel_size;
	else
		min_width = get_actual_width (HTML_IMAGE (o), painter);

	min_width += image->border * 2 * pixel_size + 2*image->hspace;

	return min_width;
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	HTMLImage *image = HTML_IMAGE (o);
	guint pixel_size;
	guint width;

	pixel_size = html_painter_get_pixel_size (painter);
	width = get_actual_width (HTML_IMAGE (o), painter)
		+ image->border * 2 * pixel_size + 2*image->hspace;

	return width;
}

static gboolean
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
{
	HTMLImage *image;
	guint pixel_size;
	guint width, height;
	gint old_width, old_ascent, old_descent;

	old_width = o->width;
	old_ascent = o->ascent;
	old_descent = o->descent;

	image = HTML_IMAGE (o);

	pixel_size = html_painter_get_pixel_size (painter);

	width = get_actual_width (image, painter);
	height = get_actual_height (image, painter);

	o->width  = width + image->border * 2 * pixel_size + 2 * image->hspace;
	o->ascent = height + 2 * image->border * pixel_size + 2 * image->vspace;
	o->descent = 0;

	if (o->descent != old_descent
	    || o->ascent != old_ascent
	    || o->width != old_width)
		return TRUE;

	return FALSE;
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

	image = HTML_IMAGE (o);

	pixbuf = image->image_ptr->pixbuf;

	if (pixbuf == NULL) {
		html_painter_draw_panel (painter, 
					 o->x + tx + image->hspace,
					 o->y + ty + image->vspace - o->ascent,
					 o->width - 2*image->hspace, o->ascent + o->descent - 2 * image->vspace,
					 TRUE, 1);
		return;
	}

	pixel_size = html_painter_get_pixel_size (painter);

	base_x = o->x + tx + image->border * pixel_size + image->hspace;
	base_y = o->y + ty + image->border * pixel_size + image->vspace - o->ascent;

	scale_width = get_actual_width (image, painter);
	scale_height = get_actual_height (image, painter);

	if (o->selected)
		color = html_painter_get_default_highlight_color (painter);
	else
		color = NULL;

	if (image->animation) {
		image->animation->active = TRUE;
		image->animation->x = base_x;
		image->animation->y = base_y;
		image->animation->ex = image->image_ptr->factory->engine->x_offset;
		image->animation->ey = image->image_ptr->factory->engine->y_offset;

		render_cur_frame (image, base_x, base_y);
	} else {
		html_painter_draw_pixmap (painter, pixbuf,
					  base_x, base_y,
					  scale_width, scale_height,
					  color);
	}
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

static HTMLVAlignType
get_valign (HTMLObject *self)
{
	HTMLImage *image;

	image = HTML_IMAGE (self);

	return image->valign;
}


void
html_image_type_init (void)
{
	html_image_class_init (&html_image_class, HTML_TYPE_IMAGE, sizeof (HTMLImage));
}

void
html_image_class_init (HTMLImageClass *image_class,
		       HTMLType type,
		       guint size)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (image_class);

	html_object_class_init (object_class, type, size);

	/* FIXME destroy, dammit!!!  */

	object_class->copy = copy;
	object_class->draw = draw;
	object_class->destroy = destroy;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_size = calc_size;
	object_class->get_url = get_url;
	object_class->get_target = get_target;
	object_class->accepts_cursor = accepts_cursor;
	object_class->get_valign = get_valign;

	parent_class = &html_object_class;
}

void
html_image_init (HTMLImage *image,
		 HTMLImageClass *klass,
		 HTMLImageFactory *imf,
		 gchar *filename,
		 const gchar *url,
		 const gchar *target,
		 gint16 width, gint16 height,
		 gint8 percent, gint8 border,
		 HTMLVAlignType valign)
{
	HTMLObject *object;

	object = HTML_OBJECT (image);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	image->url = g_strdup (url);
	image->target = g_strdup (target);

	image->specified_width = width;
	image->specified_height = height;
	image->border = border;
	image->animation = NULL;

	image->hspace = 0;
	image->vspace = 0;

	if (valign == HTML_VALIGN_NONE)
		valign = HTML_VALIGN_BOTTOM;
	image->valign = valign;

	object->percent = percent;

	image->image_ptr = html_image_factory_register (imf, image, filename);
}

HTMLObject *
html_image_new (HTMLImageFactory *imf,
		gchar *filename,
		const gchar *url,
		const gchar *target,
		gint16 width, gint16 height,
		gint8 percent, gint8 border,
		HTMLVAlignType valign)
{
	HTMLImage *image;

	image = g_new(HTMLImage, 1);

	html_image_init (image, &html_image_class,
			 imf,
			 filename,
			 url,
			 target,
			 width, height,
			 percent, border,
			 valign);

	return HTML_OBJECT (image);
}

void
html_image_set_spacing (HTMLImage *image, gint hspace, gint vspace)
{
	image->hspace = hspace;
	image->vspace = vspace;
}



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
	ip->pixbuf    = gdk_pixbuf_loader_get_pixbuf (ip->loader);
	g_assert (ip->pixbuf);

	html_engine_schedule_update (ip->factory->engine);
}

static void
render_cur_frame (HTMLImage *image, gint nx, gint ny)
{
	GdkPixbufFrame    *frame;
	HTMLPainter       *painter;
	HTMLImageAnimation *anim = image->animation;
	GdkPixbufAnimation *ganim = image->image_ptr->animation;
	GList *cur = gdk_pixbuf_animation_get_frames (ganim);
	gint w, h;

	painter = image->image_ptr->factory->engine->painter;

	frame = (GdkPixbufFrame *) anim->cur_frame->data;
	/* printf ("w: %d h: %d action: %d\n", w, h, frame->action); */

	do {
		frame = (GdkPixbufFrame *) cur->data;
		w = gdk_pixbuf_get_width (frame->pixbuf);
		h = gdk_pixbuf_get_height (frame->pixbuf);
		if (anim->cur_frame == cur) {
			html_painter_draw_pixmap (painter, frame->pixbuf,
						  nx + frame->x_offset,
						  ny + frame->y_offset,
						  w, h,
						  NULL);
			break;
		} else if (frame->action == GDK_PIXBUF_FRAME_RETAIN) {
			html_painter_draw_pixmap (painter, frame->pixbuf,
						  nx + frame->x_offset,
						  ny + frame->y_offset,
						  w, h,
						  NULL);
		} 
		cur = cur->next;
	} while (1);
}

static gint
html_image_animation_timeout (HTMLImage *image)
{
	HTMLImageAnimation *anim = image->animation;
	GdkPixbufAnimation *ganim = image->image_ptr->animation;
	GdkPixbufFrame    *frame;
	HTMLPainter       *painter;
	/* HTMLObject        *o = HTML_OBJECT (image); */
	HTMLEngine        *engine;
	gint nx, ny, nex, ney;
	gint w, h;

	anim->cur_frame = anim->cur_frame->next;
	if (!anim->cur_frame)
		anim->cur_frame = gdk_pixbuf_animation_get_frames (image->image_ptr->animation);

	frame = (GdkPixbufFrame *) anim->cur_frame->data;
	
	/* FIXME - use gdk_pixbuf_composite instead of render_cur_frame
	   to be more efficient */
	/* render this step to helper buffer */
	/*
	  w = gdk_pixbuf_get_width (frame->pixbuf);
	  h = gdk_pixbuf_get_height (frame->pixbuf);
	
	  if (anim->cur_frame != image->image_ptr->animation->frames) {
		gdk_pixbuf_composite (frame->pixbuf,
				      anim->pixbuf,
				      frame->x_offset, frame->y_offset,
				      w, h,
				      0.0, 0.0, 1.0, 1.0,
				      ART_FILTER_NEAREST, 255);
	} else {
		gdk_pixbuf_copy_area (frame->pixbuf,
				      0, 0, w, h,
				      anim->pixbuf,
				      frame->x_offset, frame->y_offset);
				      } */

	/* draw only if animation is active - onscreen */

	engine = image->image_ptr->factory->engine;

	nex = engine->x_offset;
	ney = engine->y_offset;

	nx = anim->x - (nex - anim->ex);
	ny = anim->y - (ney - anim->ey);
	
	if (anim->active) {
		gint aw, ah;

		aw = gdk_pixbuf_animation_get_width (ganim);
		ah = gdk_pixbuf_animation_get_height (ganim);

		if (MAX(0, nx) < MIN(engine->width, nx+aw)
		    && MAX(0, ny) < MIN(engine->height, ny+ah)) {
			html_engine_draw (engine,
					  nx, ny,
					  aw, ah);
		}
		
	}

	anim->timeout = gtk_timeout_add (10 * (frame->delay_time ? frame->delay_time : 1),
					 (GtkFunction) html_image_animation_timeout, (gpointer) image);

	return FALSE;
}

static void
html_image_animation_start (HTMLImage *image)
{
	HTMLImageAnimation *anim = image->animation;

	if (anim && gdk_pixbuf_animation_get_num_frames (image->image_ptr->animation) > 1) {
		if (anim->timeout == 0) {
			GList *frames = gdk_pixbuf_animation_get_frames (image->image_ptr->animation);

			anim->cur_frame = frames->next;
			anim->cur_n = 1;
			anim->timeout = gtk_timeout_add (10*((GdkPixbufFrame *)
							     frames->data)->delay_time,
							 (GtkFunction) html_image_animation_timeout, (gpointer) image);
		}
	}
}

static void
html_image_animation_stop (HTMLImage *image)
{
	HTMLImageAnimation *anim = image->animation;

	if (anim->timeout) {
		gtk_timeout_remove (anim->timeout);
		anim->timeout = 0;
	}
	anim->active  = 0;
}

static void
html_image_factory_frame_done (GdkPixbufLoader *loader, HTMLImagePointer *ip)
{
	if (!ip->animation) {
		ip->animation = gdk_pixbuf_loader_get_animation (loader);
	}
	g_assert (ip->animation);

	if (gdk_pixbuf_animation_get_num_frames (ip->animation) > 1) {
		GSList *cur = ip->interests;
		HTMLImage *image;

		while (cur) {
			if (cur->data) {
				image = HTML_IMAGE (cur->data);
				if (!image->animation) {
					image->animation = html_image_animation_new (image);
				}
				html_image_animation_start (image);
			}
			cur = cur->next;
		}
	}
}

static void
html_image_factory_animation_done (GdkPixbufLoader *loader, HTMLImagePointer *ip)
{
	printf ("animation done\n");
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

	/* user data means: NULL only clean, non-NULL free */
	if (user_data){
		g_slist_free (ptr->interests);
		ptr->interests = NULL;
	}

	/* clean only if this image is not used anymore */
	if (!ptr->interests){
		retval = TRUE;
		html_image_pointer_destroy (ptr);
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

static HTMLImageAnimation *
html_image_animation_new (HTMLImage *image)
{
	HTMLImageAnimation *animation;

	animation = g_new (HTMLImageAnimation, 1);
	animation->cur_frame = gdk_pixbuf_animation_get_frames (image->image_ptr->animation);
	animation->cur_n = 0;
	animation->timeout = 0;
	animation->pixbuf = gdk_pixbuf_new (ART_PIX_RGB, TRUE, 8,
					    gdk_pixbuf_animation_get_width (image->image_ptr->animation),
					    gdk_pixbuf_animation_get_height (image->image_ptr->animation));
	animation->active = FALSE;

	return animation;
}

static void
html_image_animation_destroy (HTMLImageAnimation *anim)
{
	gdk_pixbuf_unref (anim->pixbuf);
}

static HTMLImagePointer *
html_image_pointer_new (const char *filename, HTMLImageFactory *factory)
{
	HTMLImagePointer *retval;

	retval = g_new (HTMLImagePointer, 1);
	retval->url = g_strdup (filename);
	retval->loader = gdk_pixbuf_loader_new ();
	retval->pixbuf = NULL;
	retval->animation = NULL;
	retval->interests = NULL;
	retval->factory = factory;

	return retval;
}

static void
html_image_pointer_destroy (HTMLImagePointer *ip)
{
	g_free (ip->url);
	if (ip->loader) {
		gtk_object_unref (GTK_OBJECT (ip->loader));
		gdk_pixbuf_loader_close (ip->loader);
	}
	if (ip->pixbuf)
		gdk_pixbuf_unref (ip->pixbuf);

	g_free (ip);
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

		retval = html_image_pointer_new (filename, factory);
		gtk_signal_connect (GTK_OBJECT (retval->loader), "area_prepared",
				    GTK_SIGNAL_FUNC (html_image_factory_area_prepared),
				    retval);

		gtk_signal_connect (GTK_OBJECT (retval->loader), "frame_done",
				    GTK_SIGNAL_FUNC (html_image_factory_frame_done),
				    retval);

		gtk_signal_connect (GTK_OBJECT (retval->loader), "animation_done",
				    GTK_SIGNAL_FUNC (html_image_factory_animation_done),
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
	}

	/* we add also NULL ptrs, as we dont want these to be cleaned out */
	retval->interests = g_slist_prepend (retval->interests, i);

	if (i) {
		i->image_ptr      = retval;

		if (retval->animation && gdk_pixbuf_animation_get_num_frames (retval->animation) > 1) {
			i->animation = html_image_animation_new (i);
			html_image_animation_start (i);
		}
	}

	return retval;
}

void
html_image_factory_unregister (HTMLImageFactory *factory, HTMLImagePointer *pointer, HTMLImage *i)
{
	pointer->interests = g_slist_remove (pointer->interests, i);
}

static void
stop_anim (gpointer key, gpointer value, gpointer user_data)
{
	HTMLImagePointer *ip = value;
	GSList *cur = ip->interests;
	HTMLImage *image;

	while (cur) {
		if (cur->data) {
			image = (HTMLImage *) cur->data;
			if (image->animation) {
				html_image_animation_stop (image);
			}
		}
		cur = cur->next;
	}
}

void
html_image_factory_stop_animations (HTMLImageFactory *factory)
{
	g_hash_table_foreach (factory->loaded_images, stop_anim, NULL);
}

static void
deactivate_anim (gpointer key, gpointer value, gpointer user_data)
{
	HTMLImagePointer *ip = value;
	GSList *cur = ip->interests;
	HTMLImage *image;

	while (cur) {
		if (cur->data) {
			image = (HTMLImage *) cur->data;
			if (image->animation) {
				image->animation->active = 0;
			}
		}
		cur = cur->next;
	}
}

void
html_image_factory_deactivate_animations (HTMLImageFactory *factory)
{
	g_hash_table_foreach (factory->loaded_images, deactivate_anim, NULL);
}
