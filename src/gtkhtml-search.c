/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright 1999, 2000 Helix Code, Inc.
    Authors:             Radek Doulik (rodo@helixcode.com)
    
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

#include <gdk/gdkkeysyms.h>
#include "gtkhtml.h"
#include "gtkhtml-search.h"
#include "htmlengine-search.h"
#include "htmlsearch.h"
#include "htmlselection.h"

struct _GtkHTMLISearch {
	GtkHTML  *html;
	GtkEntry *entry;
	gboolean forward;
	gboolean changed;
	gchar *last_text;
};
typedef struct _GtkHTMLISearch GtkHTMLISearch;

static void
changed (GtkEntry *entry, GtkHTMLISearch *data)
{
	/* printf ("isearch changed to '%s'\n", gtk_entry_get_text (entry)); */
	if (*gtk_entry_get_text (entry))
		html_engine_search_incremental (data->html->engine, gtk_entry_get_text (entry), data->forward);
	else
		html_engine_unselect_all (data->html->engine);
	data->changed = TRUE;
}

static void
continue_search (GtkHTMLISearch *data, gboolean forward)
{
	HTMLEngine *e = data->html->engine;

	if (!data->changed && data->last_text && *data->last_text) {
		gtk_entry_set_text (data->entry, data->last_text);
		html_engine_search_incremental (data->html->engine, data->last_text, forward);
		data->changed = TRUE;
	} else {
		if (e->search_info)
			html_search_set_forward (e->search_info, forward);
		html_engine_search_next (e);
	}
	data->forward = forward;
}

static gint
key_press (GtkWidget *widget, GdkEventKey *event, GtkHTMLISearch *data)
{
	gint rv = TRUE;

	if (event->state == GDK_CONTROL_MASK && event->keyval == GDK_s) {
		continue_search (data, TRUE);
	} else if (event->state == GDK_CONTROL_MASK && event->keyval == GDK_r) {
		continue_search (data, FALSE);
	} else if (!event->state && event->keyval == GDK_Escape) {
		gtk_grab_remove (GTK_WIDGET (data->entry));
		gtk_widget_destroy (GTK_WIDGET (data->entry));
		gtk_widget_grab_focus (GTK_WIDGET (data->html));
		g_free (data->last_text);
		g_free (data);
	} else
		rv = FALSE;

	return rv;
}

void
gtk_html_isearch (GtkHTML *html, gboolean forward)
{
	GtkHTMLISearch *data;
	GtkEntry *entry;

	if (!html->editor_api->create_input_line)
		return;
	entry  = (*html->editor_api->create_input_line) (html, html->editor_data);
	if (!entry)
		return;

	data = g_new (GtkHTMLISearch, 1);

	data->html      = html;
	data->forward   = forward;
	data->entry     = entry;
	data->changed   = FALSE;
	data->last_text = NULL;

	if (html->engine->search_info) {
		data->last_text = g_strdup (html->engine->search_info->text);
		html_search_set_text (html->engine->search_info, "");
	}

	gtk_signal_connect (GTK_OBJECT (entry), "key_press_event", GTK_SIGNAL_FUNC (key_press), data);
	gtk_signal_connect (GTK_OBJECT (entry), "changed",         GTK_SIGNAL_FUNC (changed),   data);

	gtk_widget_grab_focus (GTK_WIDGET (entry));
}
