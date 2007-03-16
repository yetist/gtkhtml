/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) 1999 Helix Code, Inc.

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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include "htmlelement.h"

HTMLElementClass html_element_class;
static HTMLObjectClass *parent_class = NULL;


static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	/* FIXME this cannot work.  */

	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_ELEMENT (dest)->name = g_strdup (HTML_ELEMENT (self)->name);
	HTML_ELEMENT (dest)->value = g_strdup (HTML_ELEMENT (self)->value);
	HTML_ELEMENT (dest)->form = HTML_ELEMENT (self)->form;

	HTML_ELEMENT (dest)->widget = NULL;
	HTML_ELEMENT (dest)->parent = NULL;

	HTML_ELEMENT (dest)->abs_x = HTML_ELEMENT (self)->abs_x;
	HTML_ELEMENT (dest)->abs_y = HTML_ELEMENT (self)->abs_y;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{

	HTMLElement *element = HTML_ELEMENT(o);
	gint new_x, new_y;

	if (element->widget) {

		new_x = GTK_LAYOUT (element->parent)->hadjustment->value + o->x + tx;
		new_y = GTK_LAYOUT (element->parent)->vadjustment->value + o->y + ty - o->ascent;
		
		if(element->abs_x == -1 && element->abs_y == -1) {
			gtk_layout_put(GTK_LAYOUT(element->parent), element->widget,
					new_x, new_y);

			gtk_widget_show (element->widget);			
		}
		else if(new_x != element->abs_x || new_y != element->abs_y) {
			
			gtk_layout_move(GTK_LAYOUT(element->parent), element->widget,
					new_x, new_y);
		}
		element->abs_x = new_x;
		element->abs_y = new_y;
	}
}

static void
destroy (HTMLObject *o)
{
	HTMLElement *element;

	element = HTML_ELEMENT (o);

	if(element->name)
		g_free(element->name);
	if(element->value)
		g_free(element->value);

	if(element->widget)
		gtk_widget_destroy(element->widget);

	HTML_OBJECT_CLASS (parent_class)->destroy (o);
}

static void
reset (HTMLElement *e)
{
}

void
html_element_reset (HTMLElement *e)
{
	HTML_ELEMENT_CLASS (HTML_OBJECT (e)->klass)->reset (e);
}

static gchar *
encode (HTMLElement *e)
{
	return g_strdup("");
}

gchar *
html_element_encode (HTMLElement *e)
{
	return HTML_ELEMENT_CLASS (HTML_OBJECT (e)->klass)->encode (e);
}

void
html_element_set_form (HTMLElement *e, HTMLForm *form)
{
	e->form = form;
}

gchar *
html_element_encode_string (gchar *str)
{
        static gchar *safe = "$-._!*(),"; /* RFC 1738 */
        unsigned pos = 0;
        GString *encoded = g_string_new ("");
        gchar buffer[5], *ptr;
	guchar c;
	
        while ( pos < strlen(str) ) {

		c = (unsigned char) str[pos];
			
		if ( (( c >= 'A') && ( c <= 'Z')) ||
		     (( c >= 'a') && ( c <= 'z')) ||
		     (( c >= '0') && ( c <= '9')) ||
		     (strchr(safe, c))
		     )
			{
				encoded = g_string_append_c (encoded, c);
			}
		else if ( c == ' ' )
			{
				encoded = g_string_append_c (encoded, '+');
			}
		else if ( c == '\n' )
			{
				encoded = g_string_append (encoded, "%0D%0A");
			}
		else if ( c != '\r' )
			{
				sprintf( buffer, "%%%02X", (int)c );
				encoded = g_string_append (encoded, buffer);
				}
		pos++;
	}
	
	ptr = encoded->str;

	g_string_free (encoded, FALSE);

        return ptr;
}


void
html_element_type_init (void)
{
	html_element_class_init (&html_element_class, HTML_TYPE_ELEMENT, sizeof (HTMLElement));
}

void
html_element_class_init (HTMLElementClass *klass, 
			 HTMLType type,
			 guint size)
{
	HTMLObjectClass *object_class;

	g_return_if_fail (klass != NULL);

	object_class = HTML_OBJECT_CLASS (klass);
	html_object_class_init (object_class, type, size);

	/* HTMLElement methods.   */
	klass->reset = reset;
	klass->encode = encode;

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
	object_class->copy = copy;
	object_class->draw = draw;

	parent_class = &html_object_class;
}

void
html_element_init (HTMLElement *element, 
		   HTMLElementClass *klass, 
		   GtkWidget *parent, 
		   gchar *name, 
		   gchar *value)
{
	HTMLObject *object;

	object = HTML_OBJECT (element);
	html_object_init (object, HTML_OBJECT_CLASS (klass));

	element->form = NULL;
	if (name)
		element->name = g_strdup(name);
	else
		element->name = g_strdup("");
	if (value)
		element->value = g_strdup(value);
	else
		element->value = g_strdup("");
	element->widget = NULL;
	element->parent = parent;

	element->abs_x = element->abs_y = -1;
}

