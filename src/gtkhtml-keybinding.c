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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

/* This is all hardcoded, as we really don't care about a high degree
   of customization for now.  */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "htmlengine-edit.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-insert.h"

#include "gtkhtml-keybinding.h"


/* The commands.  */

static void
forward (HTMLEngine *engine)
{
	html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_RIGHT, 1);
	html_engine_unselect_all (engine, TRUE);
}

static void
backward (HTMLEngine *engine)
{
	html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_LEFT, 1);
	html_engine_unselect_all (engine, TRUE);
}

static void
up (HTMLEngine *engine)
{
	html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_UP, 1);
	html_engine_unselect_all (engine, TRUE);
}

static void
down (HTMLEngine *engine)
{
	html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_DOWN, 1);
	html_engine_unselect_all (engine, TRUE);
}

static void
delete (HTMLEngine *engine)
{
	html_engine_unselect_all (engine, TRUE);
	html_engine_delete (engine, 1);
}

static void
insert_para (HTMLEngine *engine)
{
	html_engine_unselect_all (engine, TRUE);
	html_engine_insert_para (engine, TRUE);
}


/* CTRL keybindings.  */
static gint
handle_ctrl (GtkHTML *html,
	     GdkEventKey *event)
{
	HTMLEngine *engine;
	gboolean retval;

	engine = html->engine;

	retval = TRUE;

	switch (event->keyval) {
	case 'b':
		backward (engine);
		break;
	case 'd':
		delete (engine);
		break;
	case 'f':
		forward (engine);
		break;
	case 'n':
		down (engine);
		break;
	case 'p':
		up (engine);
		break;
	case 'm':
	case 'j':
		insert_para (engine);
		break;
	default:
		retval = FALSE;
	}

	return retval;
}


/* ALT keybindings.  */

static gint
handle_alt (GtkHTML *html,
	    GdkEventKey *event)
{
	return FALSE;
}


/* Keybindings that do not require a modifier.  */

static gint
handle_none (GtkHTML *html,
	     GdkEventKey *event)
{
	HTMLEngine *engine;
	gboolean retval;

	engine = html->engine;

	retval = TRUE;

	switch (event->keyval) {
	case GDK_Right:
		forward (engine);
		break;
	case GDK_Left:
		backward (engine);
		break;
	case GDK_Up:
		up (engine);
		break;
	case GDK_Down:
		down (engine);
		break;
	case GDK_Delete:
	case GDK_KP_Delete:
		delete (engine);
		retval = TRUE;
		break;
	case GDK_Return:
		insert_para (engine);
		break;
	case GDK_BackSpace:
		html_engine_unselect_all (engine, TRUE);
		if (html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_LEFT, 1) == 1)
			html_engine_delete (engine, 1);
		retval = TRUE;
		break;

		/* FIXME these are temporary bindings.  */
	case GDK_F1:
		gtk_html_undo (html);
		break;
	case GDK_F2:
		gtk_html_redo (html);
		break;
	case GDK_F3:
		gtk_html_cut (html);
		break;
	case GDK_F4:
		gtk_html_copy (html);
		break;
	case GDK_F5:
		gtk_html_paste (html);
		break;

		/* The following cases are for keys that we don't want to map yet, but
		   have an annoying default behavior if not handled. */
	case GDK_Tab:
		retval = TRUE;
		break;

	default:
		retval = FALSE;
	}

	return retval;
}


/* (This is a private function.)  */

gint
gtk_html_handle_key_event (GtkHTML *html,
			   GdkEventKey *event,
			   gboolean *update_styles)
{
	*update_styles = TRUE;

	switch (event->state) {
	case GDK_CONTROL_MASK:
		return handle_ctrl (html, event);
	case GDK_MOD1_MASK:
		return handle_alt (html, event);
	default:
		if (event->state == 0) {
			if (handle_none (html, event))
				return TRUE;
		}

		if (event->length == 0)
			return FALSE;

		html_engine_insert (html->engine, event->string, event->length);
		*update_styles = FALSE;
		return TRUE;
	}
}
