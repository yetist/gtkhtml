/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML widget.
 *
 *  Copyright (C) 2000 Jonas Borgstr�m <jonas_b@bitsmart.com>
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,    Boston, MA 02110-1301, USA.
 *
*/

#include <config.h>
#include <string.h>
#include "htmlform.h"
#include "htmlengine.h"


HTMLForm *
html_form_new (HTMLEngine *engine,
               const gchar *_action,
               const gchar *_method)
{
	HTMLForm *new;

	new = g_new (HTMLForm, 1);
	new->action = g_strdup (_action);
	new->method = g_strdup (_method);

	new->elements = NULL;
	new->hidden = NULL;
	html_form_set_engine (new, engine);

	new->radio_group = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

	return new;
}

void
html_form_add_element (HTMLForm *form,
                       HTMLEmbedded *element)
{
	form->elements = g_list_append (form->elements, element);

	html_embedded_set_form (element, form);
}

void
html_form_add_hidden (HTMLForm *form,
                      HTMLHidden *hidden)
{
	html_form_add_element (form, HTML_EMBEDDED (hidden));

	form->hidden = g_list_append (form->hidden, hidden);
}

void
html_form_add_radio (HTMLForm *form,
                     const gchar *name,
                     GtkRadioButton *button)
{
	GtkWidget *master;
	GSList *group;
	const gchar *key = name;

	/*
	 * FIXME a null name makes them all share the same "" group.  I doubt this
	 *  is the correct behaviour but I'm not sure what is.  The spec doesn't seem clear.
	 */
	if (key == NULL) key = "";

	master = g_hash_table_lookup (form->radio_group, key);
	if (master == NULL) {
		/* if there is no entry we dup the key because the table will own it */
		g_hash_table_insert (
			form->radio_group, g_strdup (key),
			g_object_ref (button));
	} else {
		group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (master));
		gtk_radio_button_set_group (button, group);
	}
}

static void
destroy_hidden (gpointer o,
                gpointer data)
{
	html_object_destroy (HTML_OBJECT (o));
}

static void
reset_element (gpointer o,
               gpointer data)
{
	html_embedded_reset (HTML_EMBEDDED (o));
}

void
html_form_destroy (HTMLForm *form)
{
	g_list_foreach (form->hidden, destroy_hidden, NULL);
	g_list_free (form->elements);
	g_list_free (form->hidden);

	g_hash_table_destroy (form->radio_group);

	g_free (form->action);
	g_free (form->method);
	g_free (form);
}

void
html_form_submit (HTMLForm *form)
{
	GString *encoding = g_string_new ("");
	gint first = TRUE;
	GList *i = form->elements;
	gchar *ptr;

	const gchar * codepage = html_engine_get_content_type (form->engine);

	while (i) {
		ptr = html_embedded_encode (HTML_EMBEDDED (i->data), codepage);
		if (strlen (ptr)) {
			if (!first)
				encoding = g_string_append_c (encoding, '&');
			else
				first = FALSE;

			encoding = g_string_append (encoding, ptr);
			g_free (ptr);
		}
		i = g_list_next (i);
	}

	html_engine_form_submitted (form->engine, form->method, form->action, encoding->str);

	g_string_free (encoding, TRUE);
}

void
html_form_set_engine (HTMLForm *form,
                      HTMLEngine *engine)
{
	g_return_if_fail (HTML_IS_ENGINE (engine));
	form->engine = engine;
}

void
html_form_reset (HTMLForm *form)
{
	g_list_foreach (form->elements, reset_element, NULL);
}

