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
#include "htmlbutton.h"

HTMLButtonClass html_button_class;

static void
draw (HTMLObject *o, 
      HTMLPainter *p, 
      HTMLCursor *cursor, 
      gint x, gint y, 
      gint width, gint height, 
      gint tx, gint ty)
{
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
html_button_type_init (void)
{
	html_button_class_init (&html_button_class, HTML_TYPE_BUTTON);
}

void
html_button_class_init (HTMLButtonClass *klass,
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
html_button_init (HTMLButton *button, 
		  HTMLButtonClass *klass, 
		  GtkWidget *parent, 
		  gchar *name, gchar *value)
{
	HTMLElement *element;
	HTMLObject *object;
	GtkRequisition req;

	element = HTML_ELEMENT (button);
	object = HTML_OBJECT (button);

	html_element_init (element, HTML_ELEMENT_CLASS (klass), parent, name, value);
	
	if(value)
		element->widget = gtk_button_new_with_label (value);
	else
		element->widget = gtk_button_new ();

	gtk_widget_size_request(element->widget, &req);

	object->descent = 0;
	object->width = req.width;
	object->ascent = req.height;

	gtk_widget_show(element->widget);
	gtk_layout_put(GTK_LAYOUT(parent), element->widget, 0, 0);
}

HTMLObject *
html_button_new (GtkWidget *parent, 
		 gchar *name, 
		 gchar *value)
{
	HTMLButton *button;

	button = g_new0 (HTMLButton, 1);
	html_button_init (button, &html_button_class, parent, name, value);

	return HTML_OBJECT (button);
}
