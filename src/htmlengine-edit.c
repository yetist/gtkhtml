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
				      e,
				      e->cursor->offset,
				      1);
}

void
html_engine_move_cursor (HTMLEngine *e,
			 HTMLEngineCursorMovement movement,
			 guint count)
{
	guint i;

	g_return_if_fail (e != NULL);

	if (count == 0)
		return;

	queue_draw_for_cursor (e);

	switch (movement) {
	case HTML_ENGINE_CURSOR_RIGHT:
		for (i = 0; i < count; i++)
			html_cursor_forward (e->cursor, e);
		break;
	case HTML_ENGINE_CURSOR_LEFT:
		for (i = 0; i < count; i++)
			html_cursor_backward (e->cursor, e);
		break;
	case HTML_ENGINE_CURSOR_UP:
		for (i = 0; i < count; i++)
			html_cursor_up (e->cursor, e);
		break;
	case HTML_ENGINE_CURSOR_DOWN:
		for (i = 0; i < count; i++)
			html_cursor_down (e->cursor, e);
		break;
	default:
		g_warning ("Unsupported movement %d\n", (gint) movement);
		return;
	}

	queue_draw_for_cursor (e);
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
