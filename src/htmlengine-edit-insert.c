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
#include "htmltextmaster.h"

#include "htmlengine-edit.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-movement.h"

#include "htmlengine-edit-insert.h"


/* Paragraph insertion.  */

/* FIXME this sucks.  */
static HTMLObject *
get_flow (HTMLObject *object)
{
	HTMLObject *p;

	for (p = object->parent; p != NULL; p = p->parent) {
		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_CLUEFLOW)
			break;
	}

	return p;
}

void
html_engine_insert_para (HTMLEngine *engine,
			 gboolean vspace)
{
	HTMLObject *flow;
	HTMLObject *next_flow;
	HTMLObject *current;
	guint offset;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->cursor != NULL);
	g_return_if_fail (engine->cursor->object != NULL);

	current = engine->cursor->object;
	offset = engine->cursor->offset;

	flow = get_flow (current);
	if (flow == NULL) {
		g_warning ("%s: Object %p (%s) is not contained in an HTMLClueFlow\n",
			   __FUNCTION__, current, html_type_name (HTML_OBJECT_TYPE (current)));
		return;
	}

	html_engine_hide_cursor (engine);

	/* Remove text slaves, if any.  */

	if (html_object_is_text (current) && current->next != NULL) {
		HTMLObject *p;
		HTMLObject *pnext;

		for (p = current->next;
		     p != NULL && HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE;
		     p = pnext) {
			pnext = p->next;
			html_clue_remove (HTML_CLUE (p->parent), p);
			html_object_destroy (p);
		}
	}

	if (offset > 0) {
		if (current->next != NULL) {
			next_flow = HTML_OBJECT (html_clueflow_split (HTML_CLUEFLOW (flow),
								      current->next));
		} else {
			/* FIXME we need a `html_clueflow_like_another_one()'.  */
			next_flow = html_clueflow_new (HTML_CLUEFLOW (flow)->style,
						       HTML_CLUEFLOW (flow)->level);
		}
	} else {
		next_flow = HTML_OBJECT (html_clueflow_split (HTML_CLUEFLOW (flow), current));
		if (current->prev == NULL) {
			if (html_object_is_text (current)) {
				HTMLObject *elem;

				elem = html_text_master_new ("",
							     HTML_TEXT (current)->font_style,
							     &HTML_TEXT (current)->color);
				html_clue_append (HTML_CLUE (flow), elem);
			}
		}
	}

	html_clue_append_after (HTML_CLUE (flow->parent), next_flow, flow);

	if (offset > 0 && html_object_is_text (current)) {
		HTMLObject *text_next;

		text_next = HTML_OBJECT (html_text_split (HTML_TEXT (current), offset));

		html_clue_prepend (HTML_CLUE (next_flow), text_next);

		/* FIXME relative offset?  */

		engine->cursor->object = text_next;
		engine->cursor->offset = 0;
		engine->cursor->have_target_x = FALSE;
	}

	if (flow->parent == NULL) {
		html_object_relayout (flow, engine, NULL);
		html_engine_queue_draw (engine, flow);
	} else {
		html_object_calc_size (next_flow, engine->painter);
		html_object_relayout (flow->parent, engine, flow);
		html_engine_queue_draw (engine, flow->parent);
	}

	engine->cursor->position++;

	html_engine_show_cursor (engine);
}



static void
undo_insert_closure_destroy (gpointer closure)
{
	/* Nothing to do here, as the closure is actually an int.  */
}

static void
undo_insert (HTMLEngine *engine,
	     gpointer closure)
{
	guint count;

	count = GPOINTER_TO_INT (closure);

	/* FIXME we must make sure this delete operation does not save Undo
           information.  */
	html_engine_delete (engine, count);

	/* FIXME TODO redo */
}


static guint
do_insert_different_style (HTMLEngine *e,
			   const gchar *text,
			   guint len)
{
	HTMLText *right_side;
	HTMLObject *new;
	HTMLObject *curr;
	guint offset;
	guint retval;

	curr = e->cursor->object;
	offset = e->cursor->offset;

	if (offset == 0) {
		HTMLObject *p;

		/* If we are at the beginning of the element, we might
                   have an element on the left with the same
                   properties.  Look for it.  */
		/* FIXME color.  */

		p = html_object_prev_not_slave (curr);

		if (p != NULL
		    && HTML_OBJECT_TYPE (p) == HTML_OBJECT_TYPE (curr)
		    && HTML_TEXT (p)->font_style == e->insertion_font_style) {
			e->cursor->object = p;
			e->cursor->offset = HTML_TEXT (p)->text_len;
			return html_text_insert_text (HTML_TEXT (p), e, e->cursor->offset, text, len);
		}
	}

	/* FIXME color.  */
	new = html_text_master_new ("", e->insertion_font_style, &(HTML_TEXT (curr)->color));
	retval = html_text_insert_text (HTML_TEXT (new), e, 0, text, len);
	if (retval == 0) {
		html_object_destroy (new);
		return 0;
	}

	if (offset > 0) {
		right_side = html_text_split (HTML_TEXT (curr), offset);
		if (right_side != NULL)
			html_clue_append_after (HTML_CLUE (curr->parent), HTML_OBJECT (right_side), curr);
		html_clue_append_after (HTML_CLUE (curr->parent), new, curr);
	} else if (curr->prev != NULL) {
		html_clue_append_after (HTML_CLUE (curr->parent), new, curr->prev);
	} else {
		html_clue_prepend (HTML_CLUE (curr->parent), new);
	}

	html_engine_queue_draw (e, new);

	html_object_relayout (curr->parent, e, curr);

	e->cursor->object = new;
	e->cursor->offset = 0;

	return retval;
}

static guint
do_insert_same_style (HTMLEngine *e,
		      const gchar *text,
		      guint len)
{
	HTMLObject *curr;
	guint offset;

	curr = e->cursor->object;
	offset = e->cursor->offset;

	return html_text_insert_text (HTML_TEXT (curr), e, offset, text, len);
}

static guint
do_insert_not_text (HTMLEngine *e,
		    const gchar *text,
		    guint len)
{
	GdkColor color = { 0, 0, 0, 0 };
	HTMLObject *curr;
	HTMLObject *new_text;

	curr = e->cursor->object;

	/* FIXME Color */
	new_text = html_text_master_new_with_len (text, len, e->insertion_font_style, &color);

	if (e->cursor->offset == 0) {
		if (curr->prev == NULL)
			html_clue_prepend (HTML_CLUE (curr->parent), new_text);
		else
			html_clue_append_after (HTML_CLUE (curr->parent), new_text, curr->prev);
	} else {
		html_clue_append_after (HTML_CLUE (curr->parent), new_text, curr);
	}

	html_engine_queue_draw (e, new_text);
	html_object_relayout (curr->parent, e, new_text);

	return HTML_TEXT (new_text)->text_len;
}

static guint
do_insert (HTMLEngine *e,
	   const gchar *text,
	   guint len)
{
	HTMLObject *curr;

	curr = e->cursor->object;

	/* Case #1: the cursor is on an element that is not text.  */
	if (! html_object_is_text (curr)) {
		if (e->cursor->offset == 0) {
			if (curr->prev == NULL || ! html_object_is_text (curr->prev))
				return do_insert_not_text (e, text, len);
			e->cursor->object = curr->prev;
			e->cursor->offset = HTML_TEXT (curr->prev)->text_len;
		} else {
			if (curr->next == NULL || ! html_object_is_text (curr->next))
				return do_insert_not_text (e, text, len);
			e->cursor->object = curr->next;
			e->cursor->offset = 0;
		}
	}

	/* Case #2: the cursor is on a text, element, but the style is
           different.  This means that we possibly have to split the
           element.  FIXME: Notice that we need something for color
           too.  */
	if (HTML_TEXT (curr)->font_style != e->insertion_font_style)
		return do_insert_different_style (e, text, len);

	/* Case #3: we can simply add the text to the current element.  */
	return do_insert_same_style (e, text, len);
}


/* FIXME This should actually do a lot more.  */
guint
html_engine_insert (HTMLEngine *e,
		    const gchar *text,
		    guint len)
{
	HTMLObject *current_object;
	HTMLUndoAction *undo_action;
	guint n;

	g_return_val_if_fail (e != NULL, 0);
	g_return_val_if_fail (HTML_IS_ENGINE (e), 0);
	g_return_val_if_fail (text != NULL, 0);

	if (len == 0)
		return 0;

	current_object = e->cursor->object;
	if (current_object == NULL)
		return 0;

	html_engine_hide_cursor (e);

	n = do_insert (e, text, len);

	/* FIXME i18n */
	undo_action = html_undo_action_new ("text insertion",
					    undo_insert,
					    undo_insert_closure_destroy,
					    GINT_TO_POINTER (len),
					    html_cursor_get_position (e->cursor));
	html_undo_add_undo_action (e->undo, undo_action);

	html_engine_move_cursor (e, HTML_ENGINE_CURSOR_RIGHT, n);

	html_engine_show_cursor (e);

	return n;
}

