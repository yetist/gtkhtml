/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "htmlundo.h"


static void
destroy_action_list (GList *lp)
{
	GList *p;

	for (p = lp; p != NULL; p = p->next)
		html_undo_action_destroy ((HTMLUndoAction *) p->data);
}


HTMLUndo *
html_undo_new (void)
{
	HTMLUndo *new;

	new = g_new (HTMLUndo, 1);

	new->undo_stack = NULL;
	new->undo_stack_size = 0;

	new->redo_stack = NULL;
	new->redo_stack_size = 0;

	return new;
}

void
html_undo_destroy  (HTMLUndo *undo)
{
	g_return_if_fail (undo != NULL);

	destroy_action_list (undo->undo_stack);
	destroy_action_list (undo->redo_stack);

	g_free (undo);
}


static void
do_action (HTMLUndo *undo,
	   HTMLEngine *engine,
	   GList **action_list)
{
	HTMLUndoAction *action;
	HTMLCursor *cursor;
	GList *first;

	cursor = engine->cursor;

	/* First of all, restore the cursor position.  */
	html_cursor_goto_zero (cursor, engine);

	action = (HTMLUndoAction *) (*action_list)->data;

	(* action->function) (engine, action->closure);

	html_cursor_set_relative (engine->cursor, action->relative_cursor_position);

	html_undo_action_destroy (action);

	first = *action_list;
	*action_list = (*action_list)->next;

	if (*action_list == NULL)
		g_warning ("No more actions in list!");

	g_list_remove_link (first, first);
	g_list_free (first);
}

void
html_undo_do_undo (HTMLUndo *undo,
		   HTMLEngine *engine)
{
	g_return_if_fail (undo != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (undo->undo_stack_size > 0);

	do_action (undo, engine, &undo->undo_stack);

	undo->undo_stack_size--;
}

void
html_undo_do_redo (HTMLUndo *undo,
		   HTMLEngine *engine)
{
	g_return_if_fail (undo != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (undo->redo_stack_size > 0);

	do_action (undo, engine, &undo->redo_stack);

	undo->redo_stack_size--;
}


void
html_undo_discard_redo (HTMLUndo *undo)
{
	g_return_if_fail (undo != NULL);

	if (undo->redo_stack == NULL)
		return;

	destroy_action_list (undo->redo_stack);
	undo->redo_stack_size = 0;
}


void
html_undo_add_undo_action  (HTMLUndo *undo,
			    HTMLUndoAction *action)
{
	g_return_if_fail (undo != NULL);
	g_return_if_fail (action != NULL);

	html_undo_discard_redo (undo);

	if (undo->undo_stack_size >= HTML_UNDO_LIMIT) {
		HTMLUndoAction *last_action;
		GList *last;

		last = g_list_last (undo->undo_stack);
		last_action = (HTMLUndoAction *) last->data;

		undo->undo_stack = g_list_remove_link (undo->undo_stack, last);
		g_list_free (last);

		html_undo_action_destroy (last_action);
	}

	undo->undo_stack = g_list_prepend (undo->undo_stack, action);
	undo->undo_stack_size++;
}

void
html_undo_add_redo_action  (HTMLUndo *undo,
			    HTMLUndoAction *action)
{
	g_return_if_fail (undo != NULL);
	g_return_if_fail (action != NULL);

	undo->redo_stack = g_list_prepend (undo->redo_stack, action);
	undo->redo_stack_size++;
}
