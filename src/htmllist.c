/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#include "htmllist.h"

gboolean
html_list_stack_is_empty (HTMLListStack *ls)
{
	g_return_val_if_fail (ls != NULL, TRUE);
	
	return (g_list_length (ls->list) == 0);
}

gint
html_list_stack_count (HTMLListStack *ls)
{
	g_return_val_if_fail (ls != NULL, 0);
	
	return g_list_length (ls->list);
}

void
html_list_stack_push (HTMLListStack *ls, HTMLList *l)
{
	g_return_if_fail (ls != NULL);
	g_return_if_fail (l != NULL);
	
	ls->list = g_list_prepend (ls->list, (gpointer) l);
}

void
html_list_stack_clear (HTMLListStack *ls)
{
	GList *stack;
	
	g_return_if_fail (ls != NULL);

	for (stack = ls->list; stack; stack = stack->next){
		HTMLList *list = stack->data;


		html_list_destroy (list);
	}
	g_list_free (ls->list);
	ls->list = NULL;
}

HTMLList *
html_list_stack_top (HTMLListStack *ls)
{
	HTMLList *list;

	g_return_val_if_fail (ls != NULL, NULL);
	
	list = (HTMLList *)(g_list_first (ls->list))->data;

	return list;
}

HTMLList *
html_list_stack_pop (HTMLListStack *ls)
{
	HTMLList *list;

	g_return_val_if_fail (ls != NULL, NULL);
	
	list = html_list_stack_top (ls);

	ls->list = g_list_remove (ls->list, list);

	return list;
}

HTMLListStack *
html_list_stack_new (void)
{
	HTMLListStack *ls;

	ls = g_new0 (HTMLListStack, 1);

	return ls;
}

void
html_list_stack_destroy (HTMLListStack *list_stack)
{
	g_return_if_fail (list_stack != NULL);

	html_list_stack_clear (list_stack);
	g_free (list_stack);
}

HTMLList *
html_list_new (ListType t, ListNumType nt)
{
	HTMLList *list;
	
	list = g_new0 (HTMLList, 1);
	list->type = t;
	list->numType = nt;
	list->itemNumber = 1;

	return list;
}

void
html_list_destroy (HTMLList *list)
{
	g_return_if_fail (list != NULL);
	
	g_free (list);
}
