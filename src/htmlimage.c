/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#include "htmlobject.h"
#include "htmlimage.h"
#include "htmlengine.h"
#include "htmlpainter.h"

static void html_image_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);
static void html_image_set_max_width (HTMLObject *o, gint max_width);
static gint html_image_calc_min_width (HTMLObject *o);
static void html_image_init (HTMLImage *image);
static gint html_image_calc_preferred_width (HTMLObject *o);
static void html_image_calc_size (HTMLObject *o, HTMLObject *parent);

HTMLObject *
html_image_new (HTMLEngine *e, gchar *filename, gint max_width, 
		gint width, gint height, gint percent, gint border)
{
	HTMLImage *image = g_new0 (HTMLImage, 1);
	HTMLObject *object = HTML_OBJECT (image);
	html_object_init (object, Image);

	/* HTMLObject functions */
	object->draw = html_image_draw;
	object->set_max_width = html_image_set_max_width;
	object->calc_min_width = html_image_calc_min_width;
	object->calc_preferred_width = html_image_calc_preferred_width;
	object->calc_size = html_image_calc_size;

	image->engine = e;
	image->url = filename;
	
	image->predefinedWidth = (width < 0 && !percent) ? FALSE : TRUE;
	image->predefinedHeight = height < 0 ? FALSE : TRUE;

	image->border = border;

	object->percent = percent;
	object->max_width = max_width;
	object->ascent = height + border;
	object->descent = border;

	if (percent > 0) {
		object->width = (gint)max_width * (gint)percent / 100;
		object->width += border * 2;
	}
	else {
		object->width = width + border * 2;
	}

	/* Always assume local file */
	image->pixmap = gdk_pixbuf_new_from_file (image->url);

	if (image->pixmap)
		html_image_init (image);

	/* Is the image available? */
	if (!image->pixmap) {
		if (!image->predefinedWidth && !object->percent)
			object->width = 32;
		if (!image->predefinedHeight)
			object->ascent = 32;
	}

	return object;
}

static void
html_image_init (HTMLImage *image) 
{
	HTMLObject *object = HTML_OBJECT (image);

	if (object->percent > 0) {
		object->width = (gint)(object->max_width) * (gint)(object->percent) / 100;
		if (!image->predefinedHeight)
			object->ascent = image->pixmap->art_pixbuf->height * object->width / image->pixmap->art_pixbuf->width;
		object->flags &= ~FixedWidth;
	}
	else {
		if (!image->predefinedWidth)
			object->width = image->pixmap->art_pixbuf->width;

		if (!image->predefinedHeight)
			object->ascent = image->pixmap->art_pixbuf->height;
		
		g_print ("ascent is %d\n", object->ascent);
	}
	
	if (!image->predefinedWidth)
		object->width += image->border * 2;
	if (!image->predefinedHeight)
		object->ascent += image->border;
}

static void
html_image_set_max_width (HTMLObject *o, gint max_width)
{
	if (o->percent > 0)
		o->max_width = max_width;

	if (!HTML_IMAGE (o)->pixmap)
		return;

	if (o->percent > 0) {
		o->width = (gint)(o->max_width) * (gint)(o->percent) / 100;
		if (!HTML_IMAGE (o)->predefinedHeight)
			o->ascent = HTML_IMAGE (o)->pixmap->art_pixbuf->height * o->width /
				HTML_IMAGE (o)->pixmap->art_pixbuf->width + HTML_IMAGE (o)->border;
		o->width += HTML_IMAGE (o)->border * 2;
	}
}

gint
html_image_calc_min_width (HTMLObject *o)
{
	if (o->percent > 0)
		return 1;
	return o->width;
}

static void
html_image_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	/* FIXME: Should be removed */
	
	if (!HTML_IMAGE (o)->pixmap) {
		/*
		if (!HTML_IMAGE (o)->predefinedWidth || 
		!HTML_IMAGE (o)->predefinedHeight)*/ {
		  html_painter_draw_rect (p, 
					  o->x + tx, 
					  o->y - o->ascent + ty, 
					  o->width, o->ascent);
		}
	} 
	else {
		html_painter_draw_pixmap (p, 
					  o->x + tx + HTML_IMAGE (o)->border, 
					  o->y - o->ascent + ty + HTML_IMAGE (o)->border, 
					  HTML_IMAGE (o)->pixmap);
	}
}

static gint
html_image_calc_preferred_width (HTMLObject *o)
{
	return o->width;
}

static void
html_image_calc_size (HTMLObject *o, HTMLObject *parent)
{
	/* Do nothing */
}
