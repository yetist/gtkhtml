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


#include <glib.h>

#include "htmlobject.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmltext.h"
#include "htmltextslave.h"

#include "htmlengine-edit.h"


guint
html_engine_move_cursor (HTMLEngine *e,
			 HTMLEngineCursorMovement movement,
			 guint count)
{
	gboolean (* movement_func) (HTMLCursor *, HTMLEngine *);
	guint c;

	g_return_val_if_fail (e != NULL, 0);

	if (count == 0)
		return 0;

	switch (movement) {
	case HTML_ENGINE_CURSOR_RIGHT:
		movement_func = html_cursor_forward;
		break;

	case HTML_ENGINE_CURSOR_LEFT:
		movement_func = html_cursor_backward;
		break;

	case HTML_ENGINE_CURSOR_UP:
		movement_func = html_cursor_up;
		break;

	case HTML_ENGINE_CURSOR_DOWN:
		movement_func = html_cursor_down;
		break;

	default:
		g_warning ("Unsupported movement %d\n", (gint) movement);
		return 0;
	}

	html_engine_draw_cursor (e);

	for (c = 0; c < count; c++) {
		if (! (* movement_func) (e->cursor, e))
			break;
	}

	html_engine_draw_cursor (e);

	return c;
}

/**
 * html_engine_jump_to_object:
 * @e: An HTMLEngine object
 * @object: Object to move the cursor to
 * @offset: Cursor offset within @object
 * 
 * Move the cursor to object @object, at the specified @offset.
 * 
 * Return value: 
 **/
void
html_engine_jump_to_object (HTMLEngine *e,
			    HTMLObject *object,
			    guint offset)
{
	/* Delete the cursor in the original position.  */
	html_engine_draw_cursor (e);

	/* Jump to the specified position.  FIXME: Beep if it fails?  */
	html_cursor_jump_to (e->cursor, e, object, offset);

	/* Draw the cursor in the new position.  */
	html_engine_draw_cursor (e);
}

/**
 * html_engine_jump_at:
 * @e: An HTMLEngine object
 * @x: X coordinate
 * @y: Y coordinate
 * 
 * Make the cursor jump at the specified @x, @y pointer position.
 **/
void
html_engine_jump_at (HTMLEngine *e,
		     gint x, gint y)
{
	HTMLObject *obj;
	guint offset;

	obj = html_engine_get_object_at (e, x, y, &offset, TRUE);
	if (obj == NULL)
		return;

	html_engine_jump_to_object (e, obj, offset);
}


static void
delete_same_parent (HTMLEngine *e,
		    HTMLObject *start_object,
		    gboolean destroy_start)
{
	HTMLObject *parent;
	HTMLObject *p, *pnext;

	parent = start_object->parent;

	if (destroy_start) {
		p = start_object;
	} else {
		p = start_object->next;
		while (p != NULL && HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE)
			p = p->next;
	}

	while (p != e->cursor->object) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (p->parent), p);
		html_object_destroy (p);

		p = pnext;
	}

	html_object_relayout (parent, e, start_object);
}

/* This destroys object from the cursor backwards to the specified
   `start_object'.  */
static void
delete_different_parent (HTMLEngine *e,
			 HTMLObject *start_object,
			 gboolean destroy_start)
{
	HTMLObject *p, *pnext;
	HTMLObject *start_parent;
	HTMLObject *end_parent;

	start_parent = start_object->parent;
	end_parent = e->cursor->object->parent;

	if (destroy_start) {
		p = start_object;
	} else {
		p = start_object->next;
		while (p != NULL && HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE)
			p = p->next;
	}

	while (p != NULL) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (start_parent), p);
		html_object_destroy (p);

		p = pnext;
	}

	for (p = e->cursor->object; p != NULL; p = pnext) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (end_parent), p);

		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE)
			html_object_destroy (p);
		else
			html_clue_append (HTML_CLUE (start_parent), p);
	}

	p = start_parent->next;
	while (1) {
		pnext = p->next;

		if (p->parent != NULL)
			html_clue_remove (HTML_CLUE (p->parent), p);
		html_object_destroy (p);
		if (p == end_parent)
			break;

		p = pnext;
	}

	html_object_relayout (start_parent->parent, e, start_parent);
}

static guint
merge_text_at_cursor (HTMLEngine *e)
{
	HTMLObject *curr, *prev;
	HTMLObject *p;
	gchar *curr_string, *prev_string;
	gchar *new_string;
	guint retval;

	curr = e->cursor->object;
	prev = curr->prev;
	if (prev == NULL)
		return 0;

	while (HTML_OBJECT_TYPE (prev) == HTML_TYPE_TEXTSLAVE)
		prev = prev->prev;

	if (! html_object_is_text (curr) || ! html_object_is_text (prev))
		return 0;
	if (HTML_OBJECT_TYPE (curr) != HTML_OBJECT_TYPE (prev))
		return 0;
	if (! gdk_color_equal (& HTML_TEXT (curr)->color, & HTML_TEXT (prev)->color))
		return 0;
	if (HTML_TEXT (curr)->font_style != HTML_TEXT (prev)->font_style)
		return 0;

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

	html_object_relayout (curr->parent, e, curr);

	return retval;
}

void
html_engine_delete (HTMLEngine *e,
		    guint count)
{
	HTMLObject *orig_object;
	HTMLObject *prev;
	HTMLObject *curr;
	guint orig_offset;
	guint prev_offset;
	gboolean destroy_orig;

	html_engine_draw_cursor (e);

	orig_object = e->cursor->object;
	orig_offset = e->cursor->offset;

	if (orig_object->parent == NULL || orig_object->parent == NULL)
		return;

	if (! html_object_is_text (orig_object)) {
		destroy_orig = TRUE;
	} else {
		count -= html_text_remove_text (HTML_TEXT (orig_object), e,
						e->cursor->offset, count);

		/* If the text object has become empty, then itself needs to be
		   destroyed unless it's the only one on the line (because we don't
		   want any clueflows to be empty) or the next element is a vspace
		   (because a vspace alone in a clueflow does not give us an empty
		   line).  */

		if (HTML_TEXT (orig_object)->text[0] == '\0'
		    && orig_object->next != NULL
		    && HTML_OBJECT_TYPE (orig_object->next) != HTML_TYPE_VSPACE) {
			count++;
			destroy_orig = TRUE;
		} else {
			destroy_orig = FALSE;
		}
	}

	if (count == 0)
		return;

	/* Look for the end point.  We want to delete `count'
           characters/elements from the current position.  While moving
           forward, we must check that: (1) all the elements are children of
           HTMLClueFlow and (2) the parent HTMLClueFlow is always child of the
           same clue.  */

	while (count > 0) {
		prev = e->cursor->object;
		prev_offset = e->cursor->offset;

		html_cursor_forward (e->cursor, e);

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

	if (e->cursor->offset > 1 && html_object_is_text (e->cursor->object))
		html_text_remove_text (HTML_TEXT (e->cursor->object), e,
				       0, e->cursor->offset - 1);

	e->cursor->offset = 0;

	if (curr->parent == orig_object->parent)
		delete_same_parent (e, orig_object, destroy_orig);
	else
		delete_different_parent (e, orig_object, destroy_orig);

	e->cursor->offset += merge_text_at_cursor (e);

	html_engine_draw_cursor (e);
}


void
html_engine_undo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);

	g_warning ("*Undo!*");

	html_engine_freeze (e);

	undo = e->undo;
	html_undo_do_undo (undo, e);

	html_engine_thaw (e);
}

void
html_engine_redo (HTMLEngine *e)
{
	HTMLUndo *undo;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (e->undo != NULL);

	g_warning ("*Redo!*");

	html_engine_freeze (e);

	undo = e->undo;
	html_undo_do_redo (undo, e);

	html_engine_thaw (e);
}
