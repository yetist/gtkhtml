/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright 1999, Helix Code, Inc.

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

/* This file is a bit of a hack.  To make things work in a really nice way, we
   should have some extra methods in the various subclasses to implement cursor
   movement.  But for now, I think this is a reasonable way to get things to
   work.  */

#include <glib.h>

#include "htmlclue.h"
#include "htmltext.h"
#include "htmltextslave.h"
#include "htmltype.h"

#include "htmlcursor.h"


static void
debug_location (const HTMLCursor *cursor)
{
	HTMLObject *object;

	object = cursor->object;
	if (object == NULL) {
		g_print ("Cursor has no position.\n");
		return;
	}

	g_print ("Cursor in %s (%p), offset %d\n",
		 html_type_name (HTML_OBJECT_TYPE (object)),
		 object,
		 cursor->offset);
}


HTMLCursor *
html_cursor_new (void)
{
	HTMLCursor *new;

	new = g_new (HTMLCursor, 1);
	new->object = NULL;
	new->offset = 0;

	new->target_x = 0;
	new->have_target_x = FALSE;

	return new;
}

void
html_cursor_set_position (HTMLCursor *cursor,
			  HTMLObject *object,
			  guint offset)
{
	g_return_if_fail (cursor != NULL);

	cursor->object = object;
	cursor->offset = offset;
}

void
html_cursor_destroy (HTMLCursor *cursor)
{
	g_return_if_fail (cursor != NULL);

	g_free (cursor);
}

gboolean
html_cursor_equal (HTMLCursor *a, HTMLCursor *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return a->object == b->object && a->offset == b->offset;
}


/* This is a gross hack as we don't have a `is_a()' function in the object
   system.  */

static gboolean
is_clue (HTMLObject *object)
{
	HTMLType type;

	type = HTML_OBJECT_TYPE (object);

	return (type == HTML_TYPE_CLUE || type == HTML_TYPE_CLUEV
		|| type == HTML_TYPE_CLUEH || type == HTML_TYPE_CLUEFLOW);
}

static HTMLObject *
next (HTMLObject *object)
{
	while (object->next == NULL) {
		if (object->parent == NULL) {
			object = NULL;
			break;
		}
		object = object->parent;
	}

	if (object == NULL)
		return NULL;
	else
		return object->next;
}


void
html_cursor_home (HTMLCursor *cursor,
		  HTMLEngine *engine)
{
	g_return_if_fail (cursor != NULL);
	g_return_if_fail (engine != NULL);

	if (engine->clue == NULL) {
		cursor->object = NULL;
		cursor->offset = 0;
		return;
	}

	cursor->object = engine->clue;
	cursor->offset = 0;

	html_cursor_forward (cursor, engine);

	debug_location (cursor);
}


static gboolean
forward (HTMLCursor *cursor,
	 HTMLEngine *engine)
{
	HTMLObject *obj;
	guint offset;

	obj = cursor->object;
	if (obj == NULL) {
		/* FIXME this is probably not the way it should be.  */
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}		

	offset = cursor->offset;
	obj = cursor->object;

	if (html_object_is_text (obj)) {
		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_TEXT:
		case HTML_TYPE_LINKTEXT:
		case HTML_TYPE_TEXTMASTER:
		case HTML_TYPE_LINKTEXTMASTER:
		{
			const gchar *text;

			text = HTML_TEXT (obj)->text;
			if (text[offset + 1] != 0) {
				offset++;
				goto end;
			}
			break;
		}

		case HTML_TYPE_TEXTSLAVE:
			/* Do nothing: go to the next element.  */
			break;

		default:
			g_assert_not_reached ();
		}
		
		obj = next (obj);
		offset = 0;

		while (obj != NULL && html_object_is_text (obj)) {
			if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_TEXTMASTER
			    && HTML_OBJECT_TYPE (obj) != HTML_TYPE_LINKTEXTMASTER) {
				obj = next (obj);
			} else {
				goto end;
			}
		}
	} else if (! is_clue (obj)) {
		/* Objects that are not a clue or text are always skipped, as
                   they are a unique non-splittable element.  */
		obj = next (obj);
	}

	/* No more text.  Traverse the tree in top-bottom, left-right order
           until a text element is found.  */

	while (obj != NULL) {
		if (is_clue (obj)) {
			obj = HTML_CLUE (obj)->head;
			continue;
		}

		if (html_object_accepts_cursor (obj))
			break;

		obj = next (obj);
		offset = 0;
	}

	if (obj == NULL)
		return FALSE;

 end:
	cursor->object = obj;
	cursor->offset = offset;

	return TRUE;
}

gboolean
html_cursor_forward (HTMLCursor *cursor,
		     HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	cursor->have_target_x = FALSE;
	retval = forward (cursor, engine);

	debug_location (cursor);

	return retval;
}


static gboolean
backward (HTMLCursor *cursor,
	  HTMLEngine *engine)
{
	HTMLObject *obj;
	guint offset;

	obj = cursor->object;
	if (obj == NULL) {
		/* FIXME this is probably not the way it should be.  */
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}		

	offset = cursor->offset;
	obj = cursor->object;

	if (html_object_is_text (obj)) {
		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_TEXT:
		case HTML_TYPE_LINKTEXT:
		case HTML_TYPE_TEXTMASTER:
		case HTML_TYPE_LINKTEXTMASTER:
			if (offset > 0) {
				offset--;
				goto end;
			}
			break;

		case HTML_TYPE_TEXTSLAVE:
			/* Do nothing: go to the previous element, as this is
			   not a suitable place for the cursor.  */
			break;

		default:
			g_assert_not_reached ();
		}
	}

	while (obj != NULL) {
		while (obj != NULL && obj->prev == NULL)
			obj = obj->parent;

		if (obj == NULL)
			break;

		obj = obj->prev;
		if (is_clue (obj)) {
			while (is_clue (obj) && HTML_CLUE (obj)->head != NULL) {
				obj = HTML_CLUE (obj)->head;
				while (obj->next != NULL)
					obj = obj->next;
			}
		}

		if (html_object_is_text (obj)) {
			switch (HTML_OBJECT_TYPE (obj)) {
			case HTML_TYPE_TEXT:
			case HTML_TYPE_LINKTEXT:
			case HTML_TYPE_TEXTMASTER:
			case HTML_TYPE_LINKTEXTMASTER:
				offset = strlen (HTML_TEXT (obj)->text);
				if (offset > 0)
					offset--;
				else
					g_warning ("Zero-length text element.");
				goto end;

			case HTML_TYPE_TEXTSLAVE:
				/* Do nothing: go to the previous element, as
				   this is not a suitable place for the
				   cursor.  */
				break;

			default:
				g_assert_not_reached ();
			}
		} else if (html_object_accepts_cursor (obj)) {
			break;
		}
	}

 end:
	if (obj != NULL) {
		cursor->object = obj;
		cursor->offset = offset;
		return TRUE;
	}

	return FALSE;
}

gboolean
html_cursor_backward (HTMLCursor *cursor,
		      HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (cursor != NULL, FALSE);
	g_return_val_if_fail (engine != NULL, FALSE);

	cursor->have_target_x = FALSE;
	retval = backward (cursor, engine);

	debug_location (cursor);

	return retval;
}


gboolean
html_cursor_up (HTMLCursor *cursor,
		HTMLEngine *engine)
{
	HTMLCursor orig_cursor;
	HTMLCursor prev_cursor;
	gint prev_x, prev_y;
	gint x, y;
	gint target_x;
	gint orig_y;
	gboolean new_line;

	if (cursor->object == NULL) {
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}

	orig_cursor = *cursor;

	if (html_object_is_text (cursor->object))
		html_text_calc_char_position (HTML_TEXT (cursor->object),
					      cursor->offset, &x, &y);
	else
		html_object_calc_abs_position (cursor->object, &x, &y);

	if (! cursor->have_target_x) {
		cursor->target_x = x;
		cursor->have_target_x = TRUE;
	}

	target_x = cursor->target_x;

	orig_y = y;

	new_line = FALSE;

	while (1) {
		prev_cursor = *cursor;
		prev_x = x;
		prev_y = y;

		if (! backward (cursor, engine))
			return FALSE;

		if (html_object_is_text (cursor->object))
			html_text_calc_char_position (HTML_TEXT (cursor->object),
						      cursor->offset, &x, &y);
		else
			html_object_calc_abs_position (cursor->object, &x, &y);

		if (html_cursor_equal (&prev_cursor, cursor)) {
			*cursor = orig_cursor;
			return FALSE;
		}

		if (prev_y != y) {
			if (new_line) {
				*cursor = prev_cursor;
				return FALSE;
			}

			new_line = TRUE;
		}

		if (new_line && x <= target_x) {
			if (! cursor->have_target_x) {
				cursor->have_target_x = TRUE;
				cursor->target_x = target_x;
			}

			/* Choose the character which is the nearest to the
                           target X.  */
			if (prev_y == y && target_x - x >= prev_x - target_x) {
				cursor->object = prev_cursor.object;
				cursor->offset = prev_cursor.offset;
			}

			debug_location (cursor);
			return TRUE;
		}
	}
}


gboolean
html_cursor_down (HTMLCursor *cursor,
		  HTMLEngine *engine)

{
	HTMLCursor orig_cursor;
	HTMLCursor prev_cursor;
	gint prev_x, prev_y;
	gint x, y;
	gint target_x;
	gint orig_y;
	gboolean new_line;

	if (cursor->object == NULL) {
		g_warning ("The cursor is in a NULL position: going home.");
		html_cursor_home (cursor, engine);
		return TRUE;
	}

	if (html_object_is_text (cursor->object))
		html_text_calc_char_position (HTML_TEXT (cursor->object),
					      cursor->offset, &x, &y);
	else
		html_object_calc_abs_position (cursor->object, &x, &y);


	if (! cursor->have_target_x) {
		cursor->target_x = x;
		cursor->have_target_x = TRUE;
	}

	target_x = cursor->target_x;

	orig_y = y;

	new_line = FALSE;

	while (1) {
		prev_cursor = *cursor;
		prev_x = x;
		prev_y = y;

		if (! forward (cursor, engine))
			return FALSE;

		if (html_object_is_text (cursor->object))
			html_text_calc_char_position (HTML_TEXT (cursor->object),
						      cursor->offset, &x, &y);
		else
			html_object_calc_abs_position (cursor->object, &x, &y);


		if (html_cursor_equal (&prev_cursor, cursor)) {
			*cursor = orig_cursor;
			return FALSE;
		}

		if (prev_y != y) {
			if (new_line) {
				*cursor = prev_cursor;
				return FALSE;
			}

			new_line = TRUE;
		}

		if (new_line && x >= target_x) {
			if (! cursor->have_target_x) {
				cursor->have_target_x = TRUE;
				cursor->target_x = target_x;
			}

			/* Choose the character which is the nearest to the
                           target X.  */
			if (prev_y == y && x - target_x >= target_x - prev_x) {
				cursor->object = prev_cursor.object;
				cursor->offset = prev_cursor.offset;
			}

			debug_location (cursor);
			return TRUE;
		}
	}
}
