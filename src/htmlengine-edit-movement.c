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

#include "htmlengine-edit-movement.h"


guint
html_engine_move_cursor (HTMLEngine *e,
			 HTMLEngineCursorMovement movement,
			 guint count)
{
	gboolean (* movement_func) (HTMLCursor *, HTMLEngine *);
	guint c;

	g_return_val_if_fail (e != NULL, 0);
	g_return_val_if_fail (HTML_IS_ENGINE (e), 0);

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
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (object != NULL);

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

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	obj = html_engine_get_object_at (e, x, y, &offset, TRUE);
	if (obj == NULL)
		return;

	html_engine_jump_to_object (e, obj, offset);
}


gboolean
html_engine_end_of_line (HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);

	html_engine_draw_cursor (engine);

	retval = html_cursor_end_of_line (engine->cursor, engine);

	html_engine_draw_cursor (engine);

	return retval;
}

gboolean
html_engine_beginning_of_line (HTMLEngine *engine)
{
	gboolean retval;

	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);

	html_engine_draw_cursor (engine);

	retval = html_cursor_beginning_of_line (engine->cursor, engine);

	html_engine_draw_cursor (engine);

	return retval;
}
