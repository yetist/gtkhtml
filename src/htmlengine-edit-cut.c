/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

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

#include "htmltextmaster.h"

#include "htmlengine-edit-copy.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-movement.h"

#include "htmlengine-edit-cut.h"


/* FIXME this is a dirty hack.  */
static gboolean
cursor_at_end_of_region (HTMLEngine *engine)
{
	HTMLCursor *cursor;
	HTMLObject *obj;

	cursor = engine->cursor;
	obj = cursor->object;

	if (html_object_is_text (obj)) {
		HTMLTextMaster *text_master;

		text_master = HTML_TEXT_MASTER (obj);
		if (text_master->select_start < cursor->offset)
			return TRUE;
		else
			return FALSE;
	}

	if (obj->selected) {
		if (cursor->offset == 0)
			return FALSE;
		else
			return TRUE;
	} else {
		if (cursor->offset == 0)
			return TRUE;
		else
			return FALSE;
	}
}


/* WARNING: this assumes that the cursor is always at either the
   beginning or the end of the selected region.  */
void
html_engine_cut (HTMLEngine *engine)
{
	guint elems_copied;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->active_selection);

	g_warning ("Cut!");

	html_engine_freeze (engine);

	elems_copied = html_engine_copy (engine);

	if (elems_copied > 0) {
		if (cursor_at_end_of_region (engine))
			html_engine_move_cursor (engine,
						 HTML_ENGINE_CURSOR_LEFT,
						 elems_copied);

		html_engine_delete (engine, elems_copied);
	}

	html_engine_unselect_all (engine, FALSE);
	html_engine_thaw (engine);
}
