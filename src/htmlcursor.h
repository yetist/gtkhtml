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

   Author: Ettore Perazzoli <ettore@helixcode.com>
*/

#ifndef HTMLCURSOR_H
#define HTMLCURSOR_H

#include <glib.h>

#include "htmlobject.h"
#include "htmlengine.h"


struct _HTMLCursor {
	HTMLObject *object;
	guint offset;

	gint target_x;
	gboolean have_target_x : 1;
};


HTMLCursor *html_cursor_new (void);
void html_cursor_set_position (HTMLCursor *cursor, HTMLObject *object,
			       guint offset);
void html_cursor_destroy (HTMLCursor *cursor);

void html_cursor_home (HTMLCursor *cursor, HTMLEngine *engine);
gboolean html_cursor_forward (HTMLCursor *cursor, HTMLEngine *engine);
gboolean html_cursor_backward (HTMLCursor *cursor, HTMLEngine *engine);
gboolean html_cursor_up (HTMLCursor *cursor, HTMLEngine *engine);
gboolean html_cursor_down (HTMLCursor *cursor, HTMLEngine *engine);

gboolean html_cursor_equal (HTMLCursor *a, HTMLCursor *b);

#endif
