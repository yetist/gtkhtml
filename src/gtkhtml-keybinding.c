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
#include "htmlengine-edit-movement.h"

#include "gtkhtml-keybinding.h"


static void
scroll_by_amount (GtkHTML *html,
		  gint amount)
{
	GtkLayout *layout;
	GtkAdjustment *adj;
	gfloat new_value;

	layout = GTK_LAYOUT (html);
	adj = layout->vadjustment;

	new_value = adj->value + (gfloat) amount;
	if (new_value < adj->lower)
		new_value = adj->lower;
	else if (new_value > adj->upper)
		new_value = adj->upper;

	gtk_adjustment_set_value (adj, new_value);
}


/* The commands.  */

static void
forward (GtkHTML *html)
{
	html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_RIGHT, 1);
	html_engine_unselect_all (html->engine, TRUE);
}

static void
backward (GtkHTML *html)
{
	html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_LEFT, 1);
	html_engine_unselect_all (html->engine, TRUE);
}

static void
up (GtkHTML *html)
{
	html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_UP, 1);
	html_engine_unselect_all (html->engine, TRUE);
}

static void
down (GtkHTML *html)
{
	html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_DOWN, 1);
	html_engine_unselect_all (html->engine, TRUE);
}

static void
delete (GtkHTML *html)
{
	html_engine_unselect_all (html->engine, TRUE);
	html_engine_delete (html->engine, 1);
}

static void
insert_para (GtkHTML *html)
{
	html_engine_unselect_all (html->engine, TRUE);
	html_engine_insert_para (html->engine, TRUE);
}

static void
beginning_of_line (GtkHTML *html)
{
	html_engine_unselect_all (html->engine, TRUE);
	html_engine_beginning_of_line (html->engine);
}

static void
end_of_line (GtkHTML *html)
{
	html_engine_unselect_all (html->engine, TRUE);
	html_engine_end_of_line (html->engine);
}

static void
page_up (GtkHTML *html)
{
	gint amount;

	amount = html_engine_scroll_up (html->engine, GTK_WIDGET (html)->allocation.height);

	if (amount > 0)
		scroll_by_amount (html, - amount);
}

static void
page_down (GtkHTML *html)
{
	gint amount;

	amount = html_engine_scroll_down (html->engine, GTK_WIDGET (html)->allocation.height);

	if (amount > 0)
		scroll_by_amount (html, amount);
}

static void
forward_word (GtkHTML *html)
{
	html_engine_forward_word (html->engine);
}

static void
backward_word (GtkHTML *html)
{
	html_engine_backward_word (html->engine);
}


/* CTRL keybindings.  */
static gint
handle_ctrl (GtkHTML *html,
	     GdkEventKey *event)
{
	gboolean retval;

	retval = TRUE;

	switch (event->keyval) {
	case 'a':
		beginning_of_line (html);
		break;
	case 'b':
		backward (html);
		break;
	case 'd':
		delete (html);
		break;
	case 'e':
		end_of_line (html);
		break;
	case 'f':
		forward (html);
		break;
	case 'n':
		down (html);
		break;
	case 'p':
		up (html);
		break;
	case 'm':
	case 'j':
		insert_para (html);
		break;
	case 'v':
		page_down (html);
		break;
	case GDK_Left:
		backward_word (html);
		break;
	case GDK_Right:
		forward_word (html);
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
	gboolean retval;

	retval = FALSE;

	switch (event->keyval) {
	case 'f':
		forward_word (html);
		break;
	case 'b':
		backward_word (html);
		break;
	case 'v':
		page_up (html);
		break;
	default:
		retval = FALSE;
	}

	return TRUE;
}


/* Keybindings that do not require a modifier.  */

static gint
handle_none (GtkHTML *html,
	     GdkEventKey *event)
{
	gboolean retval;

	retval = TRUE;

	switch (event->keyval) {
	case GDK_Home:
		beginning_of_line (html);
		break;
	case GDK_End:
		end_of_line (html);
		break;
	case GDK_Right:
		forward (html);
		break;
	case GDK_Left:
		backward (html);
		break;
	case GDK_Up:
		up (html);
		break;
	case GDK_Down:
		down (html);
		break;
	case GDK_Page_Up:
	case GDK_KP_Page_Up:
		page_up (html);
		break;
	case GDK_Page_Down:
	case GDK_KP_Page_Down:
		page_down (html);
		break;
	case GDK_Delete:
	case GDK_KP_Delete:
		delete (html);
		retval = TRUE;
		break;
	case GDK_Return:
		insert_para (html);
		break;
	case GDK_BackSpace:
		html_engine_unselect_all (html->engine, TRUE);
		if (html_engine_move_cursor (html->engine, HTML_ENGINE_CURSOR_LEFT, 1) == 1)
			html_engine_delete (html->engine, 1);
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
