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
#include "htmlelement.h"

HTMLElementClass html_element_class;

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

	HTML_OBJECT_CLASS (&html_object_class)->destroy (o);
   
	g_warning("Element->distroy()\n");
}

void
html_element_type_init (void)
{
	html_element_class_init (&html_element_class, HTML_TYPE_ELEMENT);
}

void
html_element_class_init (HTMLElementClass *klass, HTMLType type) {

	HTMLObjectClass *object_class;

	g_return_if_fail (klass != NULL);

	object_class = HTML_OBJECT_CLASS (klass);
	html_object_class_init (object_class, type);

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
}

void
html_element_init (HTMLElement *element, HTMLElementClass *klass, GtkWidget *parent, gchar *name, gchar *value) {
	HTMLObject *object;

	object = HTML_OBJECT (element);
	html_object_init (object, HTML_OBJECT_CLASS (klass));

	element->form = NULL;
	element->name = g_strdup(name);
	element->value = g_strdup(value);
	element->widget = NULL;
	element->parent = parent;

	element->abs_x = element->abs_y = 0;
}

