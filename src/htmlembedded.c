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
#include <string.h>
#include <stdio.h>
#include "htmlembedded.h"

HTMLEmbeddedClass html_embedded_class;

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{

	HTMLEmbedded *element = HTML_EMBEDDED(o);
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
	HTMLEmbedded *element;

	element = HTML_EMBEDDED (o);

	if(element->name)
		g_free(element->name);
	if(element->value)
		g_free(element->value);
	if(element->widget)
		gtk_widget_destroy(element->widget);

	HTML_OBJECT_CLASS (&html_object_class)->destroy (o);
}

static void
reset (HTMLEmbedded *e)
{
}

void
html_embedded_reset (HTMLEmbedded *e)
{
	HTML_EMBEDDED_CLASS (HTML_OBJECT (e)->klass)->reset (e);
}

static gchar *
encode (HTMLEmbedded *e)
{
	return g_strdup("");
}

gchar *
html_embedded_encode (HTMLEmbedded *e)
{
	return HTML_EMBEDDED_CLASS (HTML_OBJECT (e)->klass)->encode (e);
}

void
html_embedded_set_form (HTMLEmbedded *e, HTMLForm *form)
{
	e->form = form;
}

gchar *
html_embedded_encode_string (gchar *str)
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
html_embedded_type_init (void)
{
	html_embedded_class_init (&html_embedded_class, HTML_TYPE_EMBEDDED, sizeof (HTMLEmbedded));
}

void
html_embedded_class_init (HTMLEmbeddedClass *klass, 
			  HTMLType type,
			  guint size)
{
	HTMLObjectClass *object_class;

	g_return_if_fail (klass != NULL);

	object_class = HTML_OBJECT_CLASS (klass);
	html_object_class_init (object_class, type, size);

	/* HTMLEmbedded methods.   */
	klass->reset = reset;
	klass->encode = encode;

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
	object_class->draw = draw;
}

void
html_embedded_init (HTMLEmbedded *element, 
		   HTMLEmbeddedClass *klass, 
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

void
html_embedded_size_recalc(HTMLEmbedded *em)
{
	HTMLObject *o;
	GtkRequisition req;
	GtkHTMLEmbedded *eb;

	o = HTML_OBJECT (em);

	eb = (GtkHTMLEmbedded *)em->widget;

	gtk_widget_size_request((GtkWidget *)eb, &req);
	o->width = req.width;
	if (GTK_IS_HTML_EMBEDDED(eb)) {
		o->descent = eb->descent;
	} else {
		o->descent = 0;
	}
	o->ascent = req.height - o->descent;
}

static gboolean
html_embedded_grab_cursor(GtkWidget *eb, GdkEvent *event)
{
	/* Keep the focus! Fight the power */
	printf("button pressed\n");
	return TRUE;
}

HTMLEmbedded *
html_embedded_new_widget(GtkWidget *parent, GtkHTMLEmbedded *eb)
{
	HTMLEmbedded *em;

	em = g_new0(HTMLEmbedded, 1);
	html_embedded_init (em, HTML_EMBEDDED_CLASS (&html_embedded_class), parent, eb->name, "");

	em->widget = (GtkWidget *)eb;
	html_embedded_size_recalc(em);
	gtk_signal_connect(GTK_OBJECT(eb), "button_press_event",
			   GTK_SIGNAL_FUNC(html_embedded_grab_cursor), NULL);

	return em;
}

