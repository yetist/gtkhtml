/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmltext.h"

#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-paste.h"
#include "htmlengine-cutbuffer.h"


/* Undo/redo support.  */

struct _ActionData {
	guint ref_count;
	GList *buffer;
	gboolean backwards;
};
typedef struct _ActionData ActionData;

static void  closure_destroy  (gpointer closure);
static void  do_redo          (HTMLEngine *engine, gpointer closure);
static void  do_undo          (HTMLEngine *engine, gpointer closure);
static void  setup_undo       (HTMLEngine *engine, ActionData *data);
static void  setup_redo       (HTMLEngine *engine, ActionData *data);

static void
closure_destroy (gpointer closure)
{
	ActionData *data;

	data = (ActionData *) closure;

	g_assert (data->ref_count > 0);
	data->ref_count--;

	if (data->ref_count > 0)
		return;

	html_engine_cut_buffer_destroy (data->buffer);

	g_free (data);
}

static void
do_redo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;
	guint count;

	data = (ActionData *) closure;

	count = html_engine_cut_buffer_count (data->buffer);

	html_engine_delete (engine, count, FALSE, data->backwards);

	setup_undo (engine, data);
}

static void
setup_redo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *undo_action;

	data->ref_count ++;

	/* FIXME i18n */
	undo_action = html_undo_action_new ("paste",
					    do_redo,
					    closure_destroy,
					    data,
					    html_cursor_get_position (engine->cursor));

	html_undo_add_redo_action (engine->undo, undo_action);
}

static void
do_undo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;

	data = (ActionData *) closure;

	html_engine_paste_buffer (engine, data->buffer);

	/* FIXME: Instead of this ugly hackish way, there should be a
           flag so that we can prevent `html_engine_paste_buffer' from
           skipping the pasted text.  */
	if (! data->backwards)
		html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_LEFT,
					 html_engine_cut_buffer_count (data->buffer));

	setup_redo (engine, data);
}

static void
setup_undo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *undo_action;

	data->ref_count ++;

	/* FIXME i18n */
	undo_action = html_undo_action_new ("paste",
					    do_undo,
					    closure_destroy,
					    data,
					    html_cursor_get_position (engine->cursor));

	html_undo_add_undo_action (engine->undo, undo_action);
}

static ActionData *
create_action_data (GList *buffer,
		    gboolean backwards)
{
	ActionData *data;

	data = g_new (ActionData, 1);
	data->ref_count = 0;
	data->buffer = buffer;
	data->backwards = backwards;

	return data;
}


static void
append_to_buffer (GList **buffer,
		  GList **buffer_tail,
		  HTMLObject *object)
{
	HTMLObject *last_object;

	if (*buffer == NULL) {
		*buffer = *buffer_tail = g_list_append (NULL, object);
		return;
	}

	g_assert (*buffer_tail != NULL);

	last_object = HTML_OBJECT ((*buffer_tail)->data);

	if (html_object_is_text (object)
	    && html_object_is_text (last_object)
	    && html_text_check_merge (HTML_TEXT (object), HTML_TEXT (last_object))) {
		html_text_merge (HTML_TEXT (last_object), HTML_TEXT (object), FALSE);
		return;
	}

	*buffer_tail = g_list_append (*buffer_tail, object);
	if (buffer == NULL)
		*buffer = *buffer_tail;
	else
		*buffer_tail = (*buffer_tail)->next;
}


static void
delete_same_parent (HTMLEngine *e,
		    HTMLObject *start_object,
		    gboolean destroy_start,
		    GList **buffer,
		    GList **buffer_last)
{
	HTMLObject *parent;
	HTMLObject *p, *pnext;

	parent = start_object->parent;

	if (destroy_start)
		p = start_object;
	else
		p = html_object_next_not_slave (start_object);

	while (p != NULL && p != e->cursor->object) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (p->parent), p);
		append_to_buffer (buffer, buffer_last, p);
		html_object_destroy (p);

		p = pnext;
	}

	html_object_relayout (parent, e, e->cursor->object);
}

/* This destroys object from the cursor backwards to the specified
   `start_object'.  */
static void
delete_different_parent (HTMLEngine *e,
			 HTMLObject *start_object,
			 gboolean destroy_start,
			 GList **buffer,
			 GList **buffer_last)
{
	HTMLObject *p, *pnext, *pprev;
	HTMLObject *start_parent;
	HTMLObject *end_parent;

	start_parent = start_object->parent;
	end_parent = e->cursor->object->parent;

	if (destroy_start)
		p = start_object;
	else
		p = html_object_next_not_slave (start_object);

	/* First destroy the elements in the `start_object's paragraph.  */

	while (p != NULL) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (start_parent), p);
		append_to_buffer (buffer, buffer_last, p);
		html_object_destroy (p);

		p = pnext;
	}

	/* Then move all the (remaining) objects from the `start_parent's to the cursor's
           paragraph.  This will merge the two paragraphs.  */

	for (p = HTML_CLUE (start_parent)->tail; p != NULL; p = pprev) {
		pprev = p->prev;

		html_clue_remove (HTML_CLUE (start_parent), p);

		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE)
			html_object_destroy (p);
		else
			html_clue_prepend (HTML_CLUE (end_parent), p);
	}

	/* Finally destroy all the paragraphs from the one after `start_object's, until
	   the cursor's paragraph.  Of course, we don't destroy the cursor's paragraph.  */

	p = start_parent;
	while (1) {
		pnext = p->next;

		if (p == end_parent)
			break;

		if (p->parent != NULL)
			html_clue_remove (HTML_CLUE (p->parent), p);

		append_to_buffer (buffer, buffer_last, html_object_dup (p));

		g_assert (HTML_OBJECT_TYPE (p) == HTML_TYPE_CLUEFLOW);
		html_object_destroy (p);

		p = pnext;
	}

	html_object_relayout (end_parent->parent, e, end_parent);
}

static guint
merge_text_at_cursor (HTMLEngine *e)
{
	HTMLObject *curr, *prev;
	HTMLObject *p;
	gchar *curr_string, *prev_string;
	gchar *new_string;
	guint retval;
	gint offset;

	curr = e->cursor->object;
	offset = e->cursor->offset;

	prev = html_object_prev_not_slave (curr);
	if (prev == NULL)
		return offset;

	if (! html_object_is_text (curr) || ! html_object_is_text (prev))
		return offset;
	if (HTML_OBJECT_TYPE (curr) != HTML_OBJECT_TYPE (prev))
		return offset;
	if (! gdk_color_equal (& HTML_TEXT (curr)->color, & HTML_TEXT (prev)->color))
		return offset;
	if (HTML_TEXT (curr)->font_style != HTML_TEXT (prev)->font_style)
		return offset;

	/* The items can be merged: remove the slaves in between.  */

	p = prev->next;
	while (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE) {
		HTMLObject *pnext;

		pnext = p->next;
		html_clue_remove (HTML_CLUE (p->parent), p);
		html_object_destroy (p);
		p = pnext;
	}

	curr_string = HTML_TEXT (curr)->text;
	prev_string = HTML_TEXT (prev)->text;

	new_string = g_strconcat (prev_string, curr_string, NULL);

	g_free (HTML_TEXT (curr)->text);
	HTML_TEXT (curr)->text = new_string;
	HTML_TEXT (curr)->text_len += HTML_TEXT (prev)->text_len;

	retval = HTML_TEXT (prev)->text_len;

	html_clue_remove (HTML_CLUE (prev->parent), prev);
	html_object_destroy (prev);

	html_object_calc_size (curr->parent, e->painter);
	if (curr->parent->parent != NULL)
		html_object_relayout (curr->parent->parent, e, curr);

	return retval;
}


/**
 * html_engine_delete:
 * @e: 
 * @amount:
 * @do_undo: Whether we want to save undo information for this operation
 * 
 * Delete @count characters forward, starting at the current cursor position.
 **/
void
html_engine_delete (HTMLEngine *e,
		    guint count,
		    gboolean do_undo,
		    gboolean backwards)
{
	HTMLObject *orig_object;
	HTMLObject *prev;
	HTMLObject *curr;
	guint orig_offset;
	guint prev_offset;
	gboolean destroy_orig;
	gint saved_position;
	GList *save_buffer;
	GList *save_buffer_last;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (e->cursor->object->parent == NULL || e->cursor->object->parent == NULL)
		return;

	if (backwards)
		count = html_engine_move_cursor (e, HTML_ENGINE_CURSOR_LEFT, count);

	if (count == 0)
		return;

	save_buffer = NULL;
	save_buffer_last = NULL;

	html_engine_hide_cursor (e);

	saved_position = e->cursor->position;

	if (html_object_is_text (e->cursor->object)
	    && e->cursor->offset == HTML_TEXT (e->cursor->object)->text_len) {
		HTMLObject *next;

		next = html_object_next_not_slave (e->cursor->object);

		if (next != NULL) {
			e->cursor->object = next;
			e->cursor->offset = 0;
		}
	}

	orig_object = e->cursor->object;
	orig_offset = e->cursor->offset;

	if (! html_object_is_text (orig_object)) {
		if (orig_offset)
			destroy_orig = FALSE;
		else {
			destroy_orig = TRUE;
			count++;
		}
	} else {
		append_to_buffer (&save_buffer, &save_buffer_last,
				  HTML_OBJECT (html_text_extract_text (HTML_TEXT (orig_object),
								       e->cursor->offset,
								       count)));

		count -= html_text_remove_text (HTML_TEXT (orig_object), e,
						e->cursor->offset, count);

		/* If the text object has become empty, then itself needs to be
		   destroyed unless it's the only one on the line (because we don't
		   want any clueflows to be empty) or the next element is a vspace
		   (because a vspace alone in a clueflow does not give us an empty
		   line).  */

		if (HTML_TEXT (orig_object)->text[0] == '\0') {
			HTMLObject *next;

			next = html_object_next_not_slave (orig_object);
			if (next != NULL && HTML_OBJECT_TYPE (next) != HTML_TYPE_VSPACE) {
				count++;
				destroy_orig = TRUE;
			} else {
				destroy_orig = FALSE;
			}
		} else {
			destroy_orig = FALSE;
		}
	}

	if (count == 0)
		goto end;

	/* Look for the end point.  We want to delete `count'
           characters/elements from the current position.  While moving
           forward, we must check that: (1) all the elements are children of
           HTMLClueFlow and (2) the parent HTMLClueFlow is always child of the
           same clue.  */

	while (count > 0) {
		prev = e->cursor->object;
		prev_offset = e->cursor->offset;

		if (! html_cursor_forward (e->cursor, e)) {
			/* Cannot delete more.  */
			goto merge;
		}

		curr = e->cursor->object;

		if (curr->parent == NULL
		    || curr->parent->parent != prev->parent->parent
		    || HTML_OBJECT_TYPE (curr->parent) != HTML_TYPE_CLUEFLOW) {
			e->cursor->object = prev;
			e->cursor->offset = prev_offset;
			break;
		}

		/* The rule is special, as it is always alone in a line.  */

		if (HTML_OBJECT_TYPE (curr) != HTML_TYPE_RULE)
			count--;
	}

	if (curr->parent == orig_object->parent)
		delete_same_parent (e, orig_object, destroy_orig, &save_buffer, &save_buffer_last);
	else
		delete_different_parent (e, orig_object, destroy_orig, &save_buffer, &save_buffer_last);

	if (destroy_orig)
		html_cursor_backward (e->cursor, e);
	else {
		if (e->cursor->offset >= 1 && html_object_is_text (e->cursor->object)) {
			append_to_buffer (&save_buffer, &save_buffer_last,
					  HTML_OBJECT (html_text_extract_text (HTML_TEXT (e->cursor->object),
									       0, e->cursor->offset)));
			html_text_remove_text (HTML_TEXT (e->cursor->object), e,
					       0, e->cursor->offset);
			e->cursor->offset = 0;
		}
	}

 merge:
	e->cursor->offset = merge_text_at_cursor (e);

 end:
	e->cursor->position = saved_position;

	if (do_undo)
		setup_undo (e, create_action_data (save_buffer, backwards));

	html_cursor_normalize (e->cursor);
	html_engine_show_cursor (e);
}
