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
destroy (HTMLObject *o)
{
	HTMLTextInput *ti;

	ti = HTML_TEXTINPUT (o);

	if (ti->default_text)
		g_free (ti->default_text);

	HTML_OBJECT_CLASS (&html_embedded_class)->destroy (o);
}

static void
reset (HTMLEmbedded *e)
{
	gtk_entry_set_text (GTK_ENTRY(e->widget), HTML_TEXTINPUT(e)->default_text);
}

static gchar *
encode (HTMLEmbedded *e)
{
	GString *encoding = g_string_new ("");
	gchar *ptr;

	if(strlen (e->name)) {
		ptr = html_embedded_encode_string (e->name);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);

		encoding = g_string_append_c (encoding, '=');

		ptr = html_embedded_encode_string (gtk_entry_get_text (GTK_ENTRY (e->widget)));
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
	}

	ptr = encoding->str;
	g_string_free(encoding, FALSE);

	return ptr;
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
	HTMLEmbeddedClass *element_class;
	HTMLObjectClass *object_class;


	element_class = HTML_EMBEDDED_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_embedded_class_init (element_class, type);

	/* HTMLEmbedded methods.   */
	element_class->reset = reset;
	element_class->encode = encode;

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
}

void
html_text_input_init (HTMLTextInput *ti,
		      HTMLTextInputClass *klass,
		      GtkWidget *parent,
		      gchar *name,
		      gchar *value,
		      gint size,
		      gint maxlen,
		      gboolean password)
{

	HTMLEmbedded *element;
	HTMLObject *object;
	GtkRequisition req;

	element = HTML_EMBEDDED (ti);
	object = HTML_OBJECT (ti);

	html_embedded_init (element, HTML_EMBEDDED_CLASS (klass),
			   parent, name, value);

	element->widget = gtk_entry_new();
	gtk_widget_size_request(element->widget, &req);

	if(strlen (element->value))	
		gtk_entry_set_text(GTK_ENTRY(element->widget), element->value);

	ti->default_text = g_strdup (element->value);

	if(maxlen != -1)
		gtk_entry_set_max_length(GTK_ENTRY(element->widget), maxlen);

	gtk_entry_set_visibility (GTK_ENTRY(element->widget), !password);
	
	req.width = gdk_char_width(element->widget->style->font, '0') * size + 8;

	gtk_widget_set_usize(element->widget, req.width, req.height);

	object->descent = 0;
	object->width = req.width;
	object->ascent = req.height;

	/*	gtk_widget_show(element->widget);
		gtk_layout_put(GTK_LAYOUT(parent), element->widget, 0, 0);*/

	ti->size = size;
	ti->maxlen = maxlen;
}

HTMLObject *
html_text_input_new (GtkWidget *parent,
		     gchar *name,
		     gchar *value,
		     gint size,
		     gint maxlen,
		     gboolean password)
{
	HTMLTextInput *ti;

	ti = g_new0 (HTMLTextInput, 1);
	html_text_input_init (ti, &html_text_input_class,
			      parent, name, value, size,
			      maxlen, password);

	return HTML_OBJECT (ti);
}
