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

#include <glib.h>

#include "htmltext.h"
#include "htmltextslave.h"
#include "htmltype.h"

#include "htmlcursor.h"


HTMLCursor *
html_cursor_new (void)
{
	HTMLCursor *new;

	new = g_new (HTMLCursor, 1);
	new->object = NULL;
	new->offset = 0;

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


/* This is a gross hack as we don't have a `is_a()' function in the object system.  */

static gboolean
is_clue (HTMLObject *object)
{
	HTMLType type;

	type = HTML_OBJECT_TYPE (object);

	return (type == HTML_TYPE_CLUE || type == HTML_TYPE_CLUEV
		|| type == HTML_TYPE_CLUEH || type == HTML_TYPE_CLUEFLOW);
}

static gboolean
is_text (HTMLObject *object)
{
	HTMLType type;

	type = HTML_OBJECT_TYPE (object);

	return (type == HTML_TYPE_TEXTMASTER || type == HTML_TYPE_TEXT
		|| type == HTML_TYPE_LINKTEXTMASTER || type == HTML_TYPE_LINKTEXT);
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
}

void
html_cursor_forward (HTMLCursor *cursor,
		     HTMLEngine *engine)
{
	HTMLObject *obj;

	obj = cursor->object;

	if (is_text (obj)) {
		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_TEXT:
		case HTML_TYPE_LINKTEXT:
		{
			const gchar *text;

			text = HTML_TEXT (obj)->text;
			if (text[cursor->offset] != 0) {
				cursor->offset++;
				return;
			}
			break;
		}

		case HTML_TYPE_TEXTMASTER:
		case HTML_TYPE_LINKTEXTMASTER:
			/* Do nothing: go to the next element, which is the slave.  */
			break;

		case HTML_TYPE_TEXTSLAVE:
		{
			HTMLTextSlave *slave;

			slave = HTML_TEXT_SLAVE (obj);
			if (slave->posLen > 0 && cursor->offset < slave->posLen - 1) {
				cursor->offset++;
				return;
			} else {
				if ((slave->posLen == 0 || cursor->offset == slave->posLen - 1)
				    && obj->next == NULL) {
					cursor->offset++;
					return;
				}
			}
		}

		default:
			g_assert_not_reached ();
		}
		
		obj = next (obj);
		cursor->offset = 0;

		while (is_text (obj)) {
			if (HTML_OBJECT_TYPE (obj) == HTML_TYPE_TEXTMASTER
			    || HTML_OBJECT_TYPE (obj) == HTML_TYPE_LINKTEXTMASTER) {
				obj = next (obj);
			} else {
				return;
			}
		}
	}

	/* No more text.  Traverse the tree in top-bottom, left-right order until a
           text element is found.  */

	while (obj != NULL) {
		if (is_clue (obj)) {
			obj = HTML_CLUE (obj)->head;
			continue;
		}

		if (is_text (obj))
			break;

		obj = next (obj);
		cursor->offset = 0;
	}

	cursor->object = obj;
	cursor->offset = 0;
}
