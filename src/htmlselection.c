/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "htmlcursor.h"
#include "htmlinterval.h"
#include "htmlselection.h"
#include "htmlengine-edit.h"

static void
select_object (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	HTMLInterval *i = (HTMLInterval *) data;

	// printf ("select_object %p %p <%d,%d>\n", o, e, html_interval_get_start (i, o), html_interval_get_length (i, o));
	html_object_select_range (o, e, html_interval_get_start (i, o), html_interval_get_length (i, o), TRUE);
}

void
html_engine_select_region (HTMLEngine *e,
			   gint x1, gint y1,
			   gint x2, gint y2)
{
	HTMLPoint *a, *b;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (e->clue == NULL)
		return;

	a = html_engine_get_point_at (e, x1, y1, TRUE);
	b = html_engine_get_point_at (e, x2, y2, TRUE);

	if (a && b) {
		HTMLInterval *new_selection;

		new_selection = html_interval_new_from_points (a ,b);
		html_interval_validate (new_selection);
		if (e->selection && html_interval_eq (e->selection, new_selection))
			html_interval_destroy (new_selection);
		else {
			printf ("set selection\n");
			html_engine_unselect_all (e);
			e->selection = new_selection;
			html_interval_forall (e->selection, e, select_object, e->selection);
		}
	}

	if (a) html_point_destroy (a);
	if (b) html_point_destroy (b);
}

static void
unselect_object (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	// printf ("unselect_object %p\n", o);
	html_object_select_range (o, e, 0, 0, TRUE);
}

void
html_engine_unselect_all (HTMLEngine *e)
{
	/* FIXME handle iframes */
	if (e->selection) {
		html_interval_forall (e->selection, e, unselect_object, e->selection);
		html_interval_destroy (e->selection);
		e->selection = NULL;
	}
}

void
html_engine_disable_selection (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (e->editable) {
		if (e->mark == NULL)
			return;

		html_cursor_destroy (e->mark);
		e->mark = NULL;
	}

	html_engine_unselect_all (e);
}

static gboolean
line_interval (HTMLEngine *e, HTMLCursor *begin, HTMLCursor *end)
{
	return html_cursor_beginning_of_line (begin, e) && html_cursor_end_of_line (end, e);
}

static gboolean
word_interval (HTMLEngine *e, HTMLCursor *begin, HTMLCursor *end)
{
	/* move to the begin of word */
	while (html_is_in_word (html_cursor_get_prev_char (begin)))
		html_cursor_backward (begin, e);
	/* move to the end of word */
	while (html_is_in_word (html_cursor_get_current_char (end)))
		html_cursor_forward (end, e);

	return (begin->object && end->object);
}

static void
selection_helper (HTMLEngine *e, gboolean (*get_interval)(HTMLEngine *e, HTMLCursor *begin, HTMLCursor *end))
{
	HTMLCursor *cursor, *begin, *end;
	HTMLInterval *i;

	html_engine_unselect_all (e);
	cursor = html_engine_get_cursor (e);

	if (cursor->object) {
		begin  = html_cursor_dup (cursor);
		end    = html_cursor_dup (cursor);

		if ((*get_interval) (e, begin, end)) {
			i = html_interval_new_from_cursor (begin, end);
			html_interval_select (i, e);
		}

		html_cursor_destroy (begin);
		html_cursor_destroy (end);
	}
	html_cursor_destroy (cursor);	
}

void
html_engine_select_word (HTMLEngine *e)
{
	selection_helper (e, word_interval);
}

void
html_engine_select_line (HTMLEngine *e)
{
	selection_helper (e, line_interval);
}

gboolean
html_engine_is_selection_active (HTMLEngine *e)
{
	return e->selection ? TRUE : FALSE;
}
