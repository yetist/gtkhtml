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
#include "htmltextinput.h"

HTMLTextInputClass html_text_input_class;

static void
draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor, gint x, gint y, gint width, gint height, gint tx, gint ty) {

	HTMLElement *element = HTML_ELEMENT(o);
	gint new_x, new_y;

	new_x = GTK_LAYOUT (element->parent)->hadjustment->value + o->x + tx;
	new_y = GTK_LAYOUT (element->parent)->vadjustment->value + o->y + ty - o->ascent;

	if(new_x != element->abs_x || new_y != element->abs_y) {

		gtk_layout_move(GTK_LAYOUT(element->parent), element->widget, new_x, new_y);

		element->abs_x = new_x;
		element->abs_y = new_y;
	}
}

void
html_text_input_type_init (void)
{
	html_text_input_class_init (&html_text_input_class, HTML_TYPE_TEXTINPUT);
}

void
html_text_input_class_init (HTMLTextInputClass *klass,
			HTMLType type)
{
	HTMLElementClass *element_class;
	HTMLObjectClass *object_class;


	element_class = HTML_ELEMENT_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_element_class_init (element_class, type);

	/* HTMLObject methods.   */
	object_class->draw = draw;
}

void
html_text_input_init (HTMLTextInput *ti, HTMLTextInputClass *klass, GtkWidget *parent, gchar *name, gchar *value, gint size, gint maxlen, gboolean password) {

	HTMLElement *element;
	HTMLObject *object;
	GtkRequisition req;

	element = HTML_ELEMENT (ti);
	object = HTML_OBJECT (ti);

	html_element_init (element, HTML_ELEMENT_CLASS (klass), parent, name, value);

	element->widget = gtk_entry_new();
	gtk_widget_size_request(element->widget, &req);

	if(value)
		gtk_entry_set_text(GTK_ENTRY(element->widget), value);
	if(maxlen != -1)
		gtk_entry_set_max_length(GTK_ENTRY(element->widget), maxlen);
	
	req.width = gdk_char_width(element->widget->style->font, 'W') * size + 4;

	gtk_widget_set_usize(element->widget, req.width, req.height);

	object->descent = 0;
	object->width = req.width;
	object->ascent = req.height;

	gtk_widget_show(element->widget);
	gtk_layout_put(GTK_LAYOUT(parent), element->widget, 0, 0);

	ti->size = size;
	ti->maxlen = maxlen;
	ti->password = password;
}

HTMLObject *
html_text_input_new (GtkWidget *parent, gchar *name, gchar *value, gint size, gint maxlen, gboolean password)
{
	HTMLTextInput *ti;

	ti = g_new0 (HTMLTextInput, 1);
	html_text_input_init (ti, &html_text_input_class, parent, name, value, size, maxlen, password);

	return HTML_OBJECT (ti);
}
