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
#include "gtkhtml-search.h"
#include "gtkhtml-input.h"
#include "htmlsearch.h"

struct _GtkHTMLISearch {
	GtkHTML *html;
	gboolean forward;
};
typedef struct _GtkHTMLISearch GtkHTMLISearch;

static void
changed (GtkEntry *entry, GtkHTMLISearch *data)
{
	HTMLEngine *e;
	/* printf ("isearch changed\n"); */

	e = data->html->engine;

	if (e->search_info)
		html_engine_search_incremental (e, gtk_entry_get_text (entry));
	else
		html_engine_search (e, gtk_entry_get_text (entry), FALSE, data->forward, FALSE);
}

static gint
key_press (GtkWidget *widget, GdkEventKey *event, GtkHTMLISearch *data)
{
	HTMLEngine *e = data->html->engine;
	gint rv = FALSE;

	if (event->state == GDK_CONTROL_MASK && event->keyval == GDK_s) {
		html_search_set_forward (e->search_info, TRUE);
		html_engine_search_next (e);
		rv = TRUE;
	} else if (event->state == GDK_CONTROL_MASK && event->keyval == GDK_r) {
		html_search_set_forward (e->search_info, FALSE);
		html_engine_search_next (e);
		rv = TRUE;
	}

	return rv;
}

void
gtk_html_isearch (GtkHTML *html, gboolean forward)
{
	GtkHTMLISearch *data = g_new (GtkHTMLISearch, 1);

	/* printf ("isearch\n"); */

	data->html = html;
	data->forward = forward;
	gtk_html_input_line_activate (html->input_line, "isearch", changed, NULL, GTK_SIGNAL_FUNC (key_press), g_free, data);
}
