/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1999, 2000 Helix Code, Inc.

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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/


#include <config.h>
#include <ctype.h>
#include <glib.h>

#include "htmlobject.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmllinktext.h"
#include "htmltext.h"
#include "htmltextslave.h"
#include "htmlimage.h"
#include "htmlinterval.h"
#include "htmlselection.h"
#include "htmlundo.h"

#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlengine-edit.h"


void
html_engine_undo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);
	g_return_if_fail (e->editable);

	html_engine_unselect_all (e);

	undo = e->undo;
	html_undo_do_undo (undo, e);
}

void
html_engine_redo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);

	html_engine_unselect_all (e);

	undo = e->undo;
	html_undo_do_redo (undo, e);
}


void
html_engine_set_mark (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->editable);

	if (e->mark != NULL)
		html_engine_unselect_all (e);

	e->mark = html_cursor_dup (e->cursor);

	html_engine_edit_selection_updater_reset (e->selection_updater);
	html_engine_edit_selection_updater_schedule (e->selection_updater);
}

void
html_engine_clipboard_push (HTMLEngine *e)
{
	e->clipboard_stack = g_list_prepend (e->clipboard_stack, GUINT_TO_POINTER (e->clipboard_len));
	e->clipboard_stack = g_list_prepend (e->clipboard_stack, e->clipboard);
	e->clipboard       = NULL;
}

void
html_engine_clipboard_pop (HTMLEngine *e)
{
	g_assert (e->clipboard_stack);

	e->clipboard       = HTML_OBJECT (e->clipboard_stack->data);
	e->clipboard_stack = g_list_remove (e->clipboard_stack, e->clipboard_stack->data);
	e->clipboard_len   = GPOINTER_TO_UINT (e->clipboard_stack->data);
	e->clipboard_stack = g_list_remove (e->clipboard_stack, e->clipboard_stack->data);
}

void
html_engine_selection_push (HTMLEngine *e)
{
	if (html_engine_is_selection_active (e)) {
		e->selection_stack
			= g_list_prepend (e->selection_stack, GINT_TO_POINTER (html_cursor_get_position (e->mark)));
		e->selection_stack
			= g_list_prepend (e->selection_stack, GINT_TO_POINTER (html_cursor_get_position (e->cursor)));
		e->selection_stack = g_list_prepend (e->selection_stack, GINT_TO_POINTER (TRUE));
	} else {
		e->selection_stack = g_list_prepend (e->selection_stack, GINT_TO_POINTER (FALSE));
	}
}

void
html_engine_selection_pop (HTMLEngine *e)
{
	gboolean selection;

	g_assert (e->selection_stack);

	selection = GPOINTER_TO_INT (e->selection_stack->data);
	e->selection_stack = g_list_remove (e->selection_stack, e->selection_stack->data);

	html_engine_disable_selection (e);

	if (selection) {
		gint cursor, mark;

		cursor = GPOINTER_TO_INT (e->selection_stack->data);
		e->selection_stack = g_list_remove (e->selection_stack, e->selection_stack->data);
		mark = GPOINTER_TO_INT (e->selection_stack->data);
		e->selection_stack = g_list_remove (e->selection_stack, e->selection_stack->data);

		html_cursor_jump_to_position (e->cursor, e, mark);
		html_engine_set_mark (e);
		html_cursor_jump_to_position (e->cursor, e, cursor);
	}
	html_engine_edit_selection_updater_update_now (e->selection_updater);
}

void
html_engine_cut_and_paste_begin (HTMLEngine *e, gchar *op_name)
{
	html_engine_hide_cursor (e);
	html_engine_selection_push (e);
	html_engine_clipboard_push (e);
	html_undo_level_begin (e->undo, op_name);
	html_engine_cut (e);
}

void
html_engine_cut_and_paste_end (HTMLEngine *e)
{
	if (e->clipboard) {
		html_engine_paste (e);
		html_engine_clipboard_clear (e);
	}
	html_undo_level_end (e->undo);
	html_engine_clipboard_pop (e);
	html_engine_selection_pop (e);
	html_engine_show_cursor (e);
}

void
html_engine_cut_and_paste (HTMLEngine *e, gchar *op_name, HTMLObjectForallFunc iterator, gpointer data)
{
	html_engine_edit_selection_updater_update_now (e->selection_updater);
	html_engine_cut_and_paste_begin (e, op_name);
	if (e->clipboard)
		html_object_forall (e->clipboard, e, iterator, data);
	html_engine_cut_and_paste_end (e);
}

static void
spell_check_object (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	if (HTML_OBJECT_TYPE (o) == HTML_TYPE_CLUEFLOW)
		html_clueflow_spell_check (HTML_CLUEFLOW (o), e, (HTMLInterval *) data);
}

void
html_engine_spell_check_range (HTMLEngine *e, HTMLCursor *begin, HTMLCursor *end)
{
	HTMLInterval *i;

	begin = html_cursor_dup (begin);
	end   = html_cursor_dup (end);

	while (html_is_in_word (html_cursor_get_prev_char (begin)))
		if (html_cursor_backward (begin, e));
	while (html_is_in_word (html_cursor_get_current_char (end)))
		if (html_cursor_forward (end, e));

	i = html_interval_new_from_cursor (begin, end);
	if (begin->object->parent != end->object->parent)
		html_interval_forall (i, e, spell_check_object, i);
	else
		html_clueflow_spell_check (HTML_CLUEFLOW (begin->object->parent), e, i);
	html_interval_destroy (i);
	html_cursor_destroy (begin);
	html_cursor_destroy (end);
}

gboolean
html_is_in_word (unicode_char_t uc)
{
	/* printf ("test %d %c => %d\n", uc, uc, unicode_isalnum (uc) || uc == '\''); */
	return unicode_isalpha (uc) || uc == '\'';
}

void
html_engine_select_word_editable (HTMLEngine *e)
{
	while (html_is_in_word (html_cursor_get_prev_char (e->cursor)))
		html_cursor_backward (e->cursor, e);
	html_engine_set_mark (e);
	while (html_is_in_word (html_cursor_get_current_char (e->cursor)))
		html_cursor_forward (e->cursor, e);
}

void
html_engine_select_line_editable (HTMLEngine *e)
{
	html_engine_beginning_of_line (e);
	html_engine_set_mark (e);
	html_engine_end_of_line (e);
}

struct SetData {
	HTMLType      object_type;
	const gchar  *key;
	gpointer      value;
};

static void
set_data (HTMLObject *o, HTMLEngine *e, gpointer p)
{
	struct SetData *data = (struct SetData *) p;

	if (HTML_OBJECT_TYPE (o) == data->object_type) {
		/* printf ("set data %s --> %p\n", data->key, data->value); */
		html_object_set_data (o, data->key, data->value);
	}
}

void
html_engine_set_data_by_type (HTMLEngine *e, HTMLType object_type, const gchar *key, gpointer value)
{
	struct SetData *data = g_new (struct SetData, 1);

	/* printf ("html_engine_set_data_by_type %s\n", key); */

	data->object_type = object_type;
	data->key         = key;
	data->value       = value;

	html_object_forall (e->clue, NULL, set_data, data);

	g_free (data);
}

void
html_engine_clipboard_clear (HTMLEngine *e)
{
	if (e->clipboard) {
		html_object_destroy (e->clipboard);
		e->clipboard = NULL;
	}
}

HTMLObject *
html_engine_new_text (HTMLEngine *e, const gchar *text, gint len)
{
	return e->insertion_url
		? html_link_text_new_with_len (text, len, e->insertion_font_style, e->insertion_color,
					       e->insertion_url, e->insertion_target)
		: html_text_new_with_len      (text, len, e->insertion_font_style, e->insertion_color);
}

HTMLObject *
html_engine_new_text_empty (HTMLEngine *e)
{
	return html_engine_new_text (e, "", 0);
}
