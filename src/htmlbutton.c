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
#include "htmlform.h"

HTMLButtonClass html_button_class;

static HTMLEmbeddedClass *parent_class = NULL;


static void
clicked_event (GtkWidget *widget, gpointer data)
{
	HTMLButton *b = HTML_BUTTON (data);
	HTMLEmbedded *e = HTML_EMBEDDED (data);

	switch (b->type) {
	case BUTTON_SUBMIT:
		html_form_submit (HTML_FORM (e->form));
		break;

	case BUTTON_RESET:
		html_form_reset (HTML_FORM (e->form));
		break;
	default:
		return;
	}
}


void
html_button_type_init (void)
{
	html_button_class_init (&html_button_class, HTML_TYPE_BUTTON, sizeof (HTMLButton));
}

void
html_button_class_init (HTMLButtonClass *klass,
			HTMLType type,
			guint object_size)
{
	HTMLEmbeddedClass *element_class;
	HTMLObjectClass *object_class;

	element_class = HTML_EMBEDDED_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_embedded_class_init (element_class, type, object_size);

	parent_class = &html_embedded_class;
}

void
html_button_init (HTMLButton *button, 
		  HTMLButtonClass *klass, 
		  GtkWidget *parent, 
		  gchar *name, gchar *value,
		  HTMLButtonType type)
{
	HTMLEmbedded *element;
	HTMLObject *object;
	GtkRequisition req;

	element = HTML_EMBEDDED (button);
	object = HTML_OBJECT (button);

	html_embedded_init (element, HTML_EMBEDDED_CLASS (klass), parent, name, value);
	
	if( strlen (element->value))
		element->widget = gtk_button_new_with_label (element->value);
	else {
		switch(type) {
		case BUTTON_NORMAL:
			element->widget = gtk_button_new ();
			break;
		case BUTTON_SUBMIT:
			element->widget = gtk_button_new_with_label ("Submit Query");
			break;
		case BUTTON_RESET:
			element->widget = gtk_button_new_with_label ("Reset");
			break;
		}
	}

	gtk_signal_connect (GTK_OBJECT (element->widget), "clicked",
                            GTK_SIGNAL_FUNC (clicked_event), button);

	gtk_widget_size_request(element->widget, &req);

	object->descent = 0;
	object->width = req.width;
	object->ascent = req.height;

	button->type = type;

	/*	gtk_widget_show(element->widget);
		gtk_layout_put(GTK_LAYOUT(parent), element->widget, 0, 0);*/
}

HTMLObject *
html_button_new (GtkWidget *parent, 
		 gchar *name, 
		 gchar *value, 
		 HTMLButtonType type)
{
	HTMLButton *button;

	button = g_new0 (HTMLButton, 1);
	html_button_init (button, &html_button_class, parent, name, value, type);

	return HTML_OBJECT (button);
}
