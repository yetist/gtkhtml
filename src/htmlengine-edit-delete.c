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
#include "htmltext.h"

#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-cursor.h"


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

	html_object_calc_size (curr->parent, e->painter);
	if (curr->parent->parent != NULL)
		html_object_relayout (curr->parent->parent, e, curr);

	return retval;
}


/**
 * html_engine_delete:
 * @e: 
 * @count: 
 * 
 * Delete @count characters forward, starting at the current cursor position.
 **/
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
	
	if (e->cursor->object->parent == NULL || e->cursor->object->parent == NULL)
		return;

	html_engine_hide_cursor (e);

	if (html_object_is_text (e->cursor->object)
	    && e->cursor->offset == HTML_TEXT (e->cursor->object)->text_len) {
		HTMLObject *next;

		next = e->cursor->object->next;
		while (next != NULL && HTML_OBJECT_TYPE (next) == HTML_TYPE_TEXTSLAVE)
			next = next->next;

		if (next != NULL) {
			e->cursor->object = next;
			e->cursor->offset = 0;
		}
	}

	orig_object = e->cursor->object;
	orig_offset = e->cursor->offset;

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
		goto end;

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

 end:

	html_cursor_normalize (e->cursor);
	html_engine_show_cursor (e);
}
