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
	return (g_list_length (ls->list) == 0);
}

gint
html_list_stack_count (HTMLListStack *ls)
{
	return g_list_length (ls->list);
}

void
html_list_stack_push (HTMLListStack *ls, HTMLList *l)
{
	ls->list = g_list_prepend (ls->list, (gpointer) l);
}

void
html_list_stack_clear (HTMLListStack *ls)
{
	/* FIXME: Do something */
}

HTMLList *
html_list_stack_top (HTMLListStack *ls)
{
	HTMLList *list;

	list = (HTMLList *)(g_list_first (ls->list))->data;

	return list;
}

HTMLList *
html_list_stack_pop (HTMLListStack *ls)
{
	HTMLList *list;

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
