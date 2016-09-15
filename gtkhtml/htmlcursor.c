/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright 1999, 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

/* This file is a bit of a hack.  To make things work in a really nice way, we
 * should have some extra methods in the various subclasses to implement cursor
 * movement.  But for now, I think this is a reasonable way to get things to
 * work.  */

#include <config.h>
#include <stdlib.h>
#include <glib.h>

#include "gtkhtml-private.h"
#include "htmlclue.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmltext.h"
#include "htmltextslave.h"
#include "htmltype.h"

#include "htmlcursor.h"

static gboolean move_right (HTMLCursor *cursor, HTMLEngine *e);
static gboolean move_left (HTMLCursor *cursor, HTMLEngine *e);

#define _HTML_CURSOR_DEBUG

#ifdef _HTML_CURSOR_DEBUG
static gint gtk_html_cursor_debug_flag = -1;

static void
debug_location (const HTMLCursor *cursor)
{
	HTMLObject *object;

	if (gtk_html_cursor_debug_flag == -1) {
		if (getenv ("GTK_HTML_DEBUG_CURSOR") != NULL)
			gtk_html_cursor_debug_flag = 1;
		else
			gtk_html_cursor_debug_flag = 0;
	}

	if (!gtk_html_cursor_debug_flag)
		return;

	object = cursor->object;
	if (object == NULL) {
		g_print ("Cursor has no position.\n");
		return;
	}

	g_print ("Cursor in %s (%p), offset %d, position %d\n",
		 html_type_name (HTML_OBJECT_TYPE (object)),
		 (gpointer) object, cursor->offset, cursor->position);

	if (cursor->position < 0) {
		g_error ("error! cursor->position < 0");
	}
}
#else
#define debug_location(cursor)
#endif


static void
normalize (HTMLObject **object,
           guint *offset)
{
	if (*offset == 0 && (*object)->prev != NULL) {
		*object = html_object_prev_not_slave (*object);
		*offset = html_object_get_length (*object);
	}
}



inline void
html_cursor_init (HTMLCursor *cursor,
                  HTMLObject *o,
                  guint offset)
{
	cursor->object = o;
	cursor->offset = offset;

	cursor->target_x = 0;
	cursor->have_target_x = FALSE;

	cursor->position = 0;
}

HTMLCursor *
html_cursor_new (void)
{
	HTMLCursor *new_cursor;

	new_cursor = g_new (HTMLCursor, 1);
	html_cursor_init (new_cursor, NULL, 0);

	return new_cursor;
}

void
html_cursor_destroy (HTMLCursor *cursor)
{
	g_return_if_fail (cursor != NULL);

	g_free (cursor);
}

/**
 * html_cursor_copy:
 * @dest: A cursor object to copy into
 * @src: A cursor object to copy from
 *
 * Copy @src into @dest.  @dest does not need to be an initialized cursor, so
 * for example declaring a cursor as a local variable and then calling
 * html_cursor_copy() to initialize it from another cursor's position works.
 **/
void
html_cursor_copy (HTMLCursor *dest,
                  const HTMLCursor *src)
{
	g_return_if_fail (dest != NULL);
	g_return_if_fail (src != NULL);

	dest->object = src->object;
	dest->offset = src->offset;
	dest->target_x = src->target_x;
	dest->have_target_x = src->have_target_x;
	dest->position = src->position;
}

HTMLCursor *
html_cursor_dup (const HTMLCursor *cursor)
{
	HTMLCursor *new;

	new = html_cursor_new ();
	html_cursor_copy (new, cursor);

	return new;
}

void
html_cursor_normalize (HTMLCursor *cursor)
{
	g_return_if_fail (cursor != NULL);

	normalize (&cursor->object, &cursor->offset);
}

void
html_cursor_home (HTMLCursor *cursor,
                  HTMLEngine *engine)
{
	HTMLObject *obj;

	g_return_if_fail (cursor != NULL);
	g_return_if_fail (engine != NULL);

	gtk_html_im_reset (engine->widget);

	if (engine->clue == NULL) {
		cursor->object = NULL;
		cursor->offset = 0;
		return;
	}

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	obj = engine->clue;
	while (!html_object_accepts_cursor (obj)) {
		HTMLObject *head = html_object_head (obj);
		if (head)
			obj = head;
		else
			break;
	}

	cursor->object = obj;
	cursor->offset = 0;

	if (!html_object_accepts_cursor (obj))
		html_cursor_forward (cursor, engine);

	cursor->position = 0;

	debug_location (cursor);
}



static gboolean
forward (HTMLCursor *cursor,
         HTMLEngine *engine,
         gboolean exact_position)
{
	gboolean retval;
	gboolean (*forward_func) (HTMLObject *self, HTMLCursor *cursor, HTMLEngine *engine);

	retval = TRUE;
	if (exact_position)
		forward_func = html_object_cursor_forward_one;
	else
		forward_func = html_object_cursor_forward;

	if (!forward_func (cursor->object, cursor, engine)) {
		HTMLObject *next;

		next = html_object_next_cursor (cursor->object, (gint *) &cursor->offset);
		if (next) {
			if (!html_object_is_container (next))
				cursor->offset = (next->parent == cursor->object->parent) ? 1 : 0;
			cursor->object = next;
			cursor->position++;
		} else
			retval = FALSE;
	}
	return retval;
}

static gboolean
html_cursor_real_forward (HTMLCursor *cursor,
                          HTMLEngine *engine,
                          gboolean exact_position)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	cursor->have_target_x = FALSE;
	retval = forward (cursor, engine, exact_position);

	debug_location (cursor);

	return retval;
}

gboolean
html_cursor_forward (HTMLCursor *cursor,
                     HTMLEngine *engine)
{
	return html_cursor_real_forward (cursor, engine, FALSE);
}

gboolean
html_cursor_forward_one (HTMLCursor *cursor,
                         HTMLEngine *engine)
{
	return html_cursor_real_forward (cursor, engine, TRUE);
}

static gboolean
backward (HTMLCursor *cursor,
          HTMLEngine *engine,
          gboolean exact_position)
{
	gboolean retval;
	gboolean (*backward_func) (HTMLObject *self, HTMLCursor *cursor, HTMLEngine *engine);

	retval = TRUE;
	if (exact_position)
		backward_func = html_object_cursor_backward_one;
	else
		backward_func = html_object_cursor_backward;
	if (!backward_func (cursor->object, cursor, engine)) {
		HTMLObject *prev;

		prev = html_object_prev_cursor (cursor->object, (gint *) &cursor->offset);
		if (prev) {
			if (!html_object_is_container (prev))
				cursor->offset = html_object_get_length (prev);
			cursor->object = prev;
			cursor->position--;
		} else
			retval = FALSE;
	}
	return retval;
}

static gboolean
html_cursor_real_backward (HTMLCursor *cursor,
                           HTMLEngine *engine,
                           gboolean exact_position)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	cursor->have_target_x = FALSE;
	retval = backward (cursor, engine, exact_position);

	debug_location (cursor);

	return retval;
}

gboolean
html_cursor_backward (HTMLCursor *cursor,
                      HTMLEngine *engine)
{
	return html_cursor_real_backward (cursor, engine, FALSE);
}

gboolean
html_cursor_backward_one (HTMLCursor *cursor,
                          HTMLEngine *engine)
{
	return html_cursor_real_backward (cursor, engine, TRUE);
}


gboolean
html_cursor_up (HTMLCursor *cursor,
                HTMLEngine *engine)
{
	HTMLCursor orig_cursor;
	HTMLCursor prev_cursor;
	HTMLDirection dir;
	gint prev_x, prev_y;
	gint x, y;
	gint target_x;
	gboolean new_line;

	gtk_html_im_reset (engine->widget);

	if (cursor->object == NULL) {
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	if (cursor->object->parent)
		dir = html_object_get_direction (cursor->object->parent);
	else
		dir = HTML_DIRECTION_LTR;

	html_cursor_copy (&orig_cursor, cursor);

	html_object_get_cursor_base (cursor->object,
				     engine->painter, cursor->offset,
				     &x, &y);

	if (!cursor->have_target_x) {
		cursor->target_x = x;
		cursor->have_target_x = TRUE;
	}

	target_x = cursor->target_x;

	new_line = FALSE;

	while (1) {
		html_cursor_copy (&prev_cursor, cursor);

		prev_x = x;
		prev_y = y;

		if (!backward (cursor, engine, FALSE))
			return FALSE;

		html_object_get_cursor_base (cursor->object,
					     engine->painter, cursor->offset,
					     &x, &y);

		if (html_cursor_equal (&prev_cursor, cursor)) {
			html_cursor_copy (cursor, &orig_cursor);
			return FALSE;
		}

		if (y + cursor->object->descent - 1 < prev_y - prev_cursor.object->ascent) {
			if (new_line) {
				html_cursor_copy (cursor, &prev_cursor);
				return TRUE;
			}

			new_line = TRUE;
			if (cursor->object->parent)
				dir = html_object_get_direction (cursor->object->parent);
			else
				dir = HTML_DIRECTION_LTR;
		}

		if (dir == HTML_DIRECTION_RTL) {
			if (new_line && x >= target_x) {
				if (!cursor->have_target_x) {
					cursor->have_target_x = TRUE;
					cursor->target_x = target_x;
				}

				/* Choose the character which is the nearest to the
				 * target X.  */
				if (prev_y == y && x - target_x >= target_x - prev_x) {
					cursor->object = prev_cursor.object;
					cursor->offset = prev_cursor.offset;
					cursor->position = prev_cursor.position;
				}

				debug_location (cursor);
				return TRUE;
			}
		} else {
			if (new_line && x <= target_x) {
				if (!cursor->have_target_x) {
					cursor->have_target_x = TRUE;
					cursor->target_x = target_x;
				}

				/* Choose the character which is the nearest to the
				 * target X.  */
				if (prev_y == y && target_x - x >= prev_x - target_x) {
					cursor->object = prev_cursor.object;
					cursor->offset = prev_cursor.offset;
					cursor->position = prev_cursor.position;
				}

				debug_location (cursor);
				return TRUE;
			}
		}
	}
}


gboolean
html_cursor_down (HTMLCursor *cursor,
                  HTMLEngine *engine)
{
	HTMLCursor orig_cursor;
	HTMLCursor prev_cursor;
	HTMLDirection dir;
	gint prev_x, prev_y;
	gint x, y;
	gint target_x;
	gboolean new_line;

	gtk_html_im_reset (engine->widget);

	if (cursor->object == NULL) {
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	if (cursor->object->parent)
		dir = html_object_get_direction (cursor->object->parent);
	else
		dir = HTML_DIRECTION_LTR;

	html_cursor_copy (&orig_cursor, cursor);

	html_object_get_cursor_base (cursor->object,
				     engine->painter, cursor->offset,
				     &x, &y);

	if (!cursor->have_target_x) {
		cursor->target_x = x;
		cursor->have_target_x = TRUE;
	}

	target_x = cursor->target_x;

	new_line = FALSE;

	while (1) {
		prev_cursor = *cursor;
		prev_x = x;
		prev_y = y;

		if (dir == HTML_DIRECTION_RTL) {
			if (!move_left (cursor, engine))
				return FALSE;
		} else {
			if (!move_right (cursor, engine))
				return FALSE;
		}

		html_object_get_cursor_base (cursor->object,
					     engine->painter, cursor->offset,
					     &x, &y);

		if (html_cursor_equal (&prev_cursor, cursor)) {
			html_cursor_copy (cursor, &orig_cursor);
			return FALSE;
		}

		if (y - cursor->object->ascent > prev_y + prev_cursor.object->descent - 1) {
			if (new_line) {
				html_cursor_copy (cursor, &prev_cursor);
				return TRUE;
			}

			new_line = TRUE;
		}
		if (cursor->object->parent)
			dir = html_object_get_direction (cursor->object->parent);
		else
			dir = HTML_DIRECTION_LTR;

		if (dir == HTML_DIRECTION_RTL) {
			if (new_line && x <= target_x) {
				if (!cursor->have_target_x) {
					cursor->have_target_x = TRUE;
					cursor->target_x = target_x;
				}

				/* Choose the character which is the nearest to the
				 * target X.  */
				if (prev_y == y && target_x - x >= prev_x - target_x) {
					cursor->object = prev_cursor.object;
					cursor->offset = prev_cursor.offset;
					cursor->position = prev_cursor.position;
				}

				debug_location (cursor);
				return TRUE;
			}
		} else {
			if (new_line && x >= target_x) {
				if (!cursor->have_target_x) {
					cursor->have_target_x = TRUE;
					cursor->target_x = target_x;
				}

				/* Choose the character which is the nearest to the
				 * target X.  */
				if (prev_y == y && x - target_x >= target_x - prev_x) {
					cursor->object = prev_cursor.object;
					cursor->offset = prev_cursor.offset;
					cursor->position = prev_cursor.position;
				}

				debug_location (cursor);
				return TRUE;
			}
		}
	}
}


static gboolean
html_cursor_real_jump_to (HTMLCursor *cursor,
                          HTMLEngine *engine,
                          HTMLObject *object,
                          guint offset,
                          gboolean exact_position)
{
	HTMLCursor original;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (object != NULL, FALSE);

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	html_cursor_normalize (cursor);
	normalize (&object, &offset);

	if (cursor->object == object && cursor->offset == offset)
		return TRUE;

	html_cursor_copy (&original, cursor);

	while (forward (cursor, engine, exact_position)) {
		if (cursor->object == object && cursor->offset == offset)
			return TRUE;
	}

	html_cursor_copy (cursor, &original);

	while (backward (cursor, engine, exact_position)) {
		if (cursor->object == object && cursor->offset == offset)
			return TRUE;
	}

	return FALSE;
}

/**
 * html_cursor_jump_to:
 * @cursor:
 * @object:
 * @offset:
 *
 * Move the cursor to the specified @offset in the specified @object.
 * Where exactly move to, depends on the is_cursor_position in PangoLogAttr say.
 * This is useful for such as Indic languages that relies on that feature.
 *
 * Return value: %TRUE if successful, %FALSE if failed.
 **/
gboolean
html_cursor_jump_to (HTMLCursor *cursor,
                     HTMLEngine *engine,
                     HTMLObject *object,
                     guint offset)
{
	return html_cursor_real_jump_to (cursor, engine, object, offset, FALSE);
}

/**
 * html_cursor_exactly_jump_to:
 * @cursor:
 * @object:
 * @offset:
 *
 * Move the cursor to near the specified @offset in the specified @object.
 *
 * Return value: %TRUE if successful, %FALSE if failed.
 **/
gboolean
html_cursor_exactly_jump_to (HTMLCursor *cursor,
                             HTMLEngine *engine,
                             HTMLObject *object,
                             guint offset)
{
	return html_cursor_real_jump_to (cursor, engine, object, offset, TRUE);
}


/* Complex cursor movement commands.  */

void
html_cursor_beginning_of_document (HTMLCursor *cursor,
                                   HTMLEngine *engine)
{
	g_return_if_fail (cursor != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	while (backward (cursor, engine, FALSE))
		;
}

void
html_cursor_end_of_document (HTMLCursor *cursor,
                             HTMLEngine *engine)
{
	g_return_if_fail (cursor != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	while (forward (cursor, engine, FALSE))
		;
}

gint
html_cursor_get_position (HTMLCursor *cursor)
{
	g_return_val_if_fail (cursor != NULL, 0);

	return cursor->position;
}

static void
html_cursor_real_jump_to_position (HTMLCursor *cursor,
                                   HTMLEngine *engine,
                                   gint position,
                                   gboolean exact_position)
{
	g_return_if_fail (cursor != NULL);
	g_return_if_fail (position >= 0);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	if (cursor->position < position) {
		while (cursor->position < position) {
			if (!forward (cursor, engine, exact_position))
				break;
		}
	} else if (cursor->position > position) {
		while (cursor->position > position) {
			if (!backward (cursor, engine, exact_position))
				break;
		}
	}
	gtk_html_im_reset (engine->widget);
}

void
html_cursor_jump_to_position (HTMLCursor *cursor,
                              HTMLEngine *engine,
                              gint position)
{
	html_cursor_real_jump_to_position (cursor, engine, position, FALSE);
}

void
html_cursor_exactly_jump_to_position (HTMLCursor *cursor,
                                      HTMLEngine *engine,
                                      gint position)
{
	html_cursor_real_jump_to_position (cursor, engine, position, TRUE);
}

static void
html_cursor_real_jump_to_position_no_spell (HTMLCursor *cursor,
                                            HTMLEngine *engine,
                                            gint position,
                                            gboolean exact_position)
{
	gboolean need_spell_check;

	need_spell_check = engine->need_spell_check;
	engine->need_spell_check = FALSE;
	html_cursor_real_jump_to_position (cursor, engine, position, exact_position);
	engine->need_spell_check = need_spell_check;
}

void
html_cursor_jump_to_position_no_spell (HTMLCursor *cursor,
                                       HTMLEngine *engine,
                                       gint position)
{
	html_cursor_real_jump_to_position_no_spell (cursor, engine, position, FALSE);
}

void
html_cursor_exactly_jump_to_position_no_spell (HTMLCursor *cursor,
                                               HTMLEngine *engine,
                                               gint position)
{
	html_cursor_real_jump_to_position_no_spell (cursor, engine, position, TRUE);
}


/* Comparison.  */

gboolean
html_cursor_equal (const HTMLCursor *a,
                   const HTMLCursor *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return a->object == b->object && a->offset == b->offset;
}

gboolean
html_cursor_precedes (const HTMLCursor *a,
                      const HTMLCursor *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return a->position < b->position;
}

gboolean
html_cursor_follows (const HTMLCursor *a,
                     const HTMLCursor *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return a->position > b->position;
}


gunichar
html_cursor_get_current_char (const HTMLCursor *cursor)
{
	HTMLObject *next;

	g_return_val_if_fail (cursor != NULL, 0);

	if (!html_object_is_text (cursor->object)) {
		if (cursor->offset < html_object_get_length (cursor->object))
			return 0;

		next = html_object_next_not_slave (cursor->object);
		if (next != NULL && html_object_is_text (next))
			return html_text_get_char (HTML_TEXT (next), 0);

		return 0;
	}

	if (cursor->offset < HTML_TEXT (cursor->object)->text_len)
		return html_text_get_char (HTML_TEXT (cursor->object), cursor->offset);

	next = html_object_next_not_slave (cursor->object);
	if (next == NULL || !html_object_is_text (next))
		return 0;

	return html_text_get_char (HTML_TEXT (next), 0);
}

gunichar
html_cursor_get_prev_char (const HTMLCursor *cursor)
{
	HTMLObject *prev;

	g_return_val_if_fail (cursor != NULL, 0);

	if (cursor->offset)
		return (html_object_is_text (cursor->object))
			? html_text_get_char (HTML_TEXT (cursor->object), cursor->offset - 1)
			: 0;
	prev = html_object_prev_not_slave (cursor->object);
	return (prev && html_object_is_text (prev))
		? html_text_get_char (HTML_TEXT (prev), HTML_TEXT (prev)->text_len - 1)
		: 0;
}

gboolean
html_cursor_beginning_of_paragraph (HTMLCursor *cursor,
                                    HTMLEngine *engine)
{
	HTMLCursor copy;
	HTMLObject *flow;
	gboolean rv = FALSE;
	gint level, new_level;

	gtk_html_im_reset (engine->widget);

	level = html_object_get_parent_level (cursor->object);
	flow  = cursor->object->parent;

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	while (1) {
		if (!cursor->offset) {
			html_cursor_copy (&copy, cursor);
			if (backward (cursor, engine, FALSE)) {
				new_level = html_object_get_parent_level (cursor->object);
				if (new_level < level
				    || (new_level == level && flow != cursor->object->parent)) {
					html_cursor_copy (cursor, &copy);
					break;
				}
			} else
				break;
		}
			else
				if (!backward (cursor, engine, FALSE))
					break;
		rv = TRUE;
	}

	return rv;
}

gboolean
html_cursor_end_of_paragraph (HTMLCursor *cursor,
                              HTMLEngine *engine)
{
	HTMLCursor copy;
	HTMLObject *flow;
	gboolean rv = FALSE;
	gint level, new_level;

	gtk_html_im_reset (engine->widget);

	level = html_object_get_parent_level (cursor->object);
	flow  = cursor->object->parent;

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	while (1) {
		if (cursor->offset == html_object_get_length (cursor->object)) {
			html_cursor_copy (&copy, cursor);
			if (forward (cursor, engine, FALSE)) {
				new_level = html_object_get_parent_level (cursor->object);
				if (new_level < level
				    || (new_level == level && flow != cursor->object->parent)) {
					html_cursor_copy (cursor, &copy);
					break;
				}
			} else
				break;
		}
			else
				if (!forward (cursor, engine, FALSE))
					break;
		rv = TRUE;
	}

	return rv;
}

gboolean
html_cursor_forward_n (HTMLCursor *cursor,
                       HTMLEngine *e,
                       guint n)
{
	gboolean rv = FALSE;

	while (n && html_cursor_forward (cursor, e)) {
		n--;
		rv = TRUE;
	}

	return rv;
}

gboolean
html_cursor_backward_n (HTMLCursor *cursor,
                        HTMLEngine *e,
                        guint n)
{
	gboolean rv = FALSE;

	while (n && html_cursor_backward (cursor, e)) {
		n--;
		rv = TRUE;
	}

	return rv;
}

HTMLObject *
html_cursor_child_of (HTMLCursor *cursor,
                      HTMLObject *parent)
{
	HTMLObject *child = cursor->object;

	while (child) {
		if (child->parent == parent)
			return child;
		child = child->parent;
	}

	return NULL;
}

static gboolean
move_to_next_object (HTMLCursor *cursor,
                     HTMLEngine *e)
{
	HTMLObject *next;

	next = html_object_next_cursor (cursor->object, (gint *) &cursor->offset);
	if (next && next->parent) {
		cursor->object = next;
		cursor->position++;

		if (!html_object_is_container (next)) {
			if (html_object_get_direction (next->parent) == HTML_DIRECTION_RTL) {
				cursor->offset = html_object_get_right_edge_offset (next, e->painter, 0);
			} else {
				cursor->offset = html_object_get_left_edge_offset (next, e->painter, 0);
			}
			cursor->position += cursor->offset;
		}

		return TRUE;
	} else
		return FALSE;
}

static gboolean
move_to_prev_object (HTMLCursor *cursor,
                     HTMLEngine *e)
{
	HTMLObject *prev;

	prev = html_object_prev_cursor (cursor->object, (gint *) &cursor->offset);
	if (prev && prev->parent) {
		cursor->object = prev;
		cursor->position--;

		if (!html_object_is_container (prev)) {
			if (html_object_get_direction (prev->parent) == HTML_DIRECTION_RTL) {
				cursor->offset = html_object_get_left_edge_offset (prev, e->painter, html_object_get_length (prev));
			} else {
				cursor->offset = html_object_get_right_edge_offset (prev, e->painter, html_object_get_length (prev));
			}
			cursor->position -= cursor->offset - html_object_get_length (prev);
		}

		return TRUE;
	} else
		return FALSE;
}

static gboolean
move_left (HTMLCursor *cursor,
           HTMLEngine *e)
{
	if (!html_object_cursor_left (cursor->object, e->painter, cursor)) {
		if (cursor->object->parent) {
			if (html_object_get_direction (cursor->object->parent) == HTML_DIRECTION_RTL)
				return move_to_next_object (cursor, e);
			else
				return move_to_prev_object (cursor, e);
		}
	}

	return TRUE;
}

gboolean
html_cursor_left (HTMLCursor *cursor,
                  HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	cursor->have_target_x = FALSE;
	retval = move_left (cursor, engine);

	debug_location (cursor);

	return retval;
}

static gboolean
left_in_flow (HTMLCursor *cursor,
              HTMLEngine *e)
{
	gboolean retval;

	if (cursor->offset != html_object_get_left_edge_offset (cursor->object, e->painter, cursor->offset) && html_object_is_container (cursor->object)) {
		HTMLObject *obj;

		obj = cursor->object;
		while ((retval = move_left (cursor, e)) && cursor->object != obj)
			;
	} else {
		if (cursor->offset > 1 || !cursor->object->prev)
			retval = html_object_cursor_left (cursor->object, e->painter, cursor);
		else if (cursor->object->prev)
			retval = move_left (cursor, e);
		else
			retval = FALSE;
	}

	debug_location (cursor);

	return retval;
}

static gboolean
html_cursor_left_edge_of_line (HTMLCursor *cursor,
                               HTMLEngine *engine)
{
	HTMLCursor prev_cursor;
	gint x, y, prev_y;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);

	gtk_html_im_reset (engine->widget);

	cursor->have_target_x = FALSE;

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	html_cursor_copy (&prev_cursor, cursor);
	html_object_get_cursor_base (cursor->object, engine->painter, cursor->offset,
				     &x, &prev_y);

	while (1) {
		if (!left_in_flow (cursor, engine))
			return TRUE;

		html_object_get_cursor_base (cursor->object, engine->painter, cursor->offset,
					     &x, &y);

		if (y + cursor->object->descent - 1 < prev_y - prev_cursor.object->ascent) {
			html_cursor_copy (cursor, &prev_cursor);
			return TRUE;
		}

		prev_y = y;
		html_cursor_copy (&prev_cursor, cursor);
	}
}

static gboolean
move_right (HTMLCursor *cursor,
            HTMLEngine *e)
{
	gboolean retval;

	retval = TRUE;
	if (!html_object_cursor_right (cursor->object, e->painter, cursor)) {
		gboolean rv;
		HTMLObject *orig = cursor->object;

		if (cursor->object->parent &&
			html_object_get_direction (cursor->object->parent) == HTML_DIRECTION_RTL)
			rv = move_to_prev_object (cursor, e);
		else
			rv = move_to_next_object (cursor, e);

		if (rv && !html_object_is_container (cursor->object) && cursor->object->parent == orig->parent) {
			if (html_object_get_direction (cursor->object) == HTML_DIRECTION_RTL)
				cursor->offset--;
			else
				cursor->offset++;
		}

		return rv;
	}
	return retval;
}

gboolean
html_cursor_right (HTMLCursor *cursor,
                   HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	gtk_html_im_reset (engine->widget);

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	cursor->have_target_x = FALSE;
	retval = move_right (cursor, engine);

	debug_location (cursor);

	return retval;
}

static gboolean
right_in_flow (HTMLCursor *cursor,
               HTMLEngine *e)
{
	gboolean retval;

	if (cursor->offset != html_object_get_right_edge_offset (cursor->object, e->painter, cursor->offset)) {
		if (html_object_is_container (cursor->object)) {
			HTMLObject *obj;

			obj = cursor->object;
			while ((retval = move_right (cursor, e)) && cursor->object != obj)
				;
		} else
			retval = html_object_cursor_right (cursor->object, e->painter, cursor);
	} else {
		if (html_object_next_not_slave (cursor->object))
			retval = move_right (cursor, e);
		else
			retval = FALSE;
	}

	debug_location (cursor);

	return retval;
}

static gboolean
html_cursor_right_edge_of_line (HTMLCursor *cursor,
                                HTMLEngine *engine)
{
	HTMLCursor prev_cursor;
	gint x, y, prev_y;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);

	gtk_html_im_reset (engine->widget);

	cursor->have_target_x = FALSE;

	if (engine->need_spell_check)
		html_engine_spell_check_range (engine, engine->cursor, engine->cursor);

	html_cursor_copy (&prev_cursor, cursor);
	html_object_get_cursor_base (cursor->object, engine->painter, cursor->offset,
				     &x, &prev_y);

	while (1) {
		if (!right_in_flow (cursor, engine))
			return TRUE;

		html_object_get_cursor_base (cursor->object, engine->painter, cursor->offset,
					     &x, &y);

		if (y - cursor->object->ascent > prev_y + prev_cursor.object->descent - 1) {
			html_cursor_copy (cursor, &prev_cursor);
			return TRUE;
		}
		prev_y = y;
		html_cursor_copy (&prev_cursor, cursor);
	}
}

gboolean
html_cursor_beginning_of_line (HTMLCursor *cursor,
                               HTMLEngine *engine)
{
	if (html_object_get_direction (cursor->object) == HTML_DIRECTION_RTL)
		return html_cursor_right_edge_of_line (cursor, engine);
	else
		return html_cursor_left_edge_of_line (cursor, engine);
}

gboolean
html_cursor_end_of_line (HTMLCursor *cursor,
                         HTMLEngine *engine)
{
	if (html_object_get_direction (cursor->object) == HTML_DIRECTION_RTL)
		return html_cursor_left_edge_of_line (cursor, engine);
	else
		return html_cursor_right_edge_of_line (cursor, engine);
}
