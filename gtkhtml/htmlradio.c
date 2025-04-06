/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
 *
 *  Copyright (C) 2000 Jonas Borgstr�m <jonas_b@bitsmart.com>.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <gtk/gtk.h>
#include "htmlform.h"
#include "htmlradio.h"
#include <string.h>


HTMLRadioClass html_radio_class;
static HTMLEmbeddedClass *parent_class = NULL;


/* HTMLObject methods.  */
static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_RADIO (dest)->default_checked = HTML_RADIO (self)->default_checked;
}


/* HTMLEmbedded methods.  */
static void
reset (HTMLEmbedded *e)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (e->widget), HTML_RADIO (e)->default_checked);
}

static gchar *
encode (HTMLEmbedded *e,
        const gchar *codepage)
{
	GString *encoding = g_string_new ("");
	gchar *ptr;

	if (strlen (e->name) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (e->widget))) {

		ptr = html_embedded_encode_string (e->name, codepage);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);

		encoding = g_string_append_c (encoding, '=');

		ptr = html_embedded_encode_string (e->value, codepage);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
	}

	ptr = g_string_free (encoding, FALSE);

	return ptr;
}


void
html_radio_type_init (void)
{
	html_radio_class_init (&html_radio_class, HTML_TYPE_RADIO, sizeof (HTMLRadio));
}

void
html_radio_class_init (HTMLRadioClass *klass,
                       HTMLType type,
                       guint object_size)
{
	HTMLEmbeddedClass *element_class;
	HTMLObjectClass *object_class;

	element_class = HTML_EMBEDDED_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_embedded_class_init (element_class, type, object_size);

	/* HTMLObject methods.  */
	object_class->copy = copy;

	/* HTMLEmbedded methods.   */
	element_class->reset = reset;
	element_class->encode = encode;

	parent_class = &html_embedded_class;
}

void
html_radio_init (HTMLRadio *radio,
                 HTMLRadioClass *klass,
                 GtkWidget *parent,
                 gchar *name,
                 gchar *value,
                 gboolean checked,
                 HTMLForm *form)
{
	HTMLEmbedded *element;
	GtkWidget *widget;

	element = HTML_EMBEDDED (radio);

	if (value == NULL)
		value = g_strdup ("on");

	html_embedded_init (element, HTML_EMBEDDED_CLASS (klass), parent, name, value);

	widget = gtk_radio_button_new (NULL);
	html_embedded_set_widget (element, widget);
	html_form_add_radio (form, name, GTK_RADIO_BUTTON (widget));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), checked);
	radio->default_checked = checked;
}

HTMLObject *
html_radio_new (GtkWidget *parent,
                gchar *name,
                gchar *value,
                gboolean checked,
                HTMLForm *form)
{
	HTMLRadio *radio;

	radio = g_new0 (HTMLRadio, 1);
	html_radio_init (radio, &html_radio_class, parent, name, value, checked, form);

	return HTML_OBJECT (radio);
}
