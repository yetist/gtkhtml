/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Jonas Borgström <jonas_b@bitsmart.com>.

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
#include "htmlimageinput.h"
#include "htmlform.h"

HTMLImageInputClass html_imageinput_class;

static void
destroy (HTMLObject *o)
{
	html_object_destroy (HTML_OBJECT (HTML_IMAGEINPUT (o)->image));

	HTML_OBJECT_CLASS (&html_element_class)->destroy (o);
}

static HTMLObject *
mouse_event (HTMLObject *o,
	     gint x, gint y, gint button, gint state)
{
	if ( x >= o->x && x < o->x + o->width )
		if ( y > o->y - o->ascent && y < o->y + o->descent + 1 ) {
			
			if(button == 1 && state == 256) { /* Fix me Use define */

				HTML_IMAGEINPUT(o)->m_x = x - o->x - 1;
				HTML_IMAGEINPUT(o)->m_y = y - o->y + o->ascent - 1;
				html_form_submit (HTML_FORM (HTML_ELEMENT(o)->form));
			}

			return o;
		}
	return NULL;
}


static void
draw (HTMLObject *o,
      HTMLPainter *p,
      HTMLCursor *cursor,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTML_OBJECT (HTML_IMAGEINPUT (o)->image)->x = o->x;
	HTML_OBJECT (HTML_IMAGEINPUT (o)->image)->y = o->y;

	o->width = HTML_OBJECT (HTML_IMAGEINPUT (o)->image)->width;

	o->ascent = HTML_OBJECT (HTML_IMAGEINPUT (o)->image)->ascent;

	html_object_draw (HTML_OBJECT (HTML_IMAGEINPUT (o)->image), p, cursor, x, y, width, height, tx, ty);
}

static gchar *
encode (HTMLElement *e)
{
	GString *encoding = g_string_new ("");
	gchar *ptr;

	if(strlen (e->name)) {
		ptr = html_element_encode_string (e->name);
		encoding = g_string_assign (encoding, ptr);
		g_free (ptr);

		ptr = g_strdup_printf(".x=%d&", HTML_IMAGEINPUT(e)->m_x);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);

		ptr = html_element_encode_string (e->name);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);

		ptr = g_strdup_printf(".y=%d", HTML_IMAGEINPUT(e)->m_y);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
	}

	ptr = encoding->str;
	g_string_free(encoding, FALSE);

	return ptr;
}

void
html_imageinput_type_init (void)
{
	html_imageinput_class_init (&html_imageinput_class, HTML_TYPE_IMAGEINPUT);
}

void
html_imageinput_class_init (HTMLImageInputClass *klass,
			    HTMLType type)
{
	HTMLElementClass *element_class;
	HTMLObjectClass *object_class;


	element_class = HTML_ELEMENT_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_element_class_init (element_class, type);

	/* HTMLElement methods.  */
	element_class->encode = encode;

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
	object_class->draw = draw;
	object_class->mouse_event = mouse_event;
}

void
html_imageinput_init (HTMLImageInput *img, 
		      HTMLImageInputClass *klass,
		      HTMLImageFactory *imf,
		      gchar *name, gchar *url)
{
	HTMLElement *element;
	HTMLObject *object;

	element = HTML_ELEMENT (img);
	object = HTML_OBJECT (img);

	html_element_init (element, HTML_ELEMENT_CLASS (klass), NULL, name, NULL);

	object->width = object->ascent = 32;

	img->image = HTML_IMAGE (html_image_new (imf, 
				     url, NULL, NULL, 
				     0, -1, -1, 0, 0));

	object->ascent = 32;
	object->width = 0;
	object->descent = 0;
}

HTMLObject *
html_imageinput_new (HTMLImageFactory *imf,
		     gchar *name, 
		     gchar *url)
{
	HTMLImageInput *img;

	img = g_new0 (HTMLImageInput, 1);
	html_imageinput_init (img, &html_imageinput_class, imf, name, url);

	return HTML_OBJECT (img);
}
