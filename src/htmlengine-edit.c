/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1999 Helix Code, Inc.

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

#include <glib.h>

#include "htmlobject.h"
#include "htmlcursor.h"
#include "htmltext.h"
#include "htmltextslave.h"

#include "htmlengine-edit.h"


static void
queue_draw_for_cursor (HTMLEngine *e)
{
	HTMLObject *current;

	current = e->cursor->object;
	if (current == NULL)
		return;

	if (html_object_is_text (current))
		html_text_queue_draw (HTML_TEXT (current),
				      e, e->cursor->offset, 1);
	else
		html_engine_queue_draw (e, current);
}

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

	queue_draw_for_cursor (e);

	for (c = 0; c < count; c++) {
		if (! (* movement_func) (e->cursor, e))
			break;
	}

	queue_draw_for_cursor (e);

	return c;
}


/* FIXME This should actually do a lot more.  */
void
html_engine_insert (HTMLEngine *e,
		    const gchar *text,
		    guint len)
{
	HTMLObject *current_object;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));
	g_return_if_fail (text != NULL);

	if (len == 0)
		return;

	current_object = e->cursor->object;

	if (! html_object_is_text (current_object)) {
		g_warning ("Cannot insert text in object of type `%s'",
			   html_type_name (HTML_OBJECT_TYPE (current_object)));
		return;
	}

	html_text_insert_text (HTML_TEXT (current_object), e,
			       e->cursor->offset, text, len);
}

/* FIXME This should actually do a lot more.  */
void
html_engine_delete (HTMLEngine *e,
		    guint count)
{
	HTMLObject *current_object;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (count == 0)
		return;

	current_object = e->cursor->object;

	if (! html_object_is_text (current_object)) {
		g_warning ("Cannot remove text in object of type `%s'",
			   html_type_name (HTML_OBJECT_TYPE (current_object)));
		return;
	}

	html_text_remove_text (HTML_TEXT (current_object), e,
			       e->cursor->offset, count);
}
