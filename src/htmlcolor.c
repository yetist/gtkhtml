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
#include <glib.h>
#include <gdk/gdk.h>
#include "htmlcolor.h"

HTMLColorStack *
html_color_stack_new (void)
{
	HTMLColorStack *cs;

	cs = g_new0 (HTMLColorStack, 1);
	
	return cs;
}

void
html_color_stack_destroy (HTMLColorStack *cs)
{
	g_return_if_fail (cs != NULL);

	html_color_stack_clear (cs);
	g_free (cs);
}

void
html_color_stack_push (HTMLColorStack *cs, GdkColor *c)
{
	g_return_if_fail (cs != NULL);
	g_return_if_fail (c != NULL);
	
	cs->list = g_list_prepend (cs->list, (gpointer) c);
}

void
html_color_stack_clear (HTMLColorStack *cs)
{
	GList *stack;
	
	g_return_if_fail (cs != NULL);

	for (stack = cs->list; stack; stack = stack->next){
		GdkColor *c = stack->data;
		
		gdk_color_free (c);
	}
	
	g_list_free (cs->list);
	cs->list = NULL;
}

GdkColor *
html_color_stack_top (HTMLColorStack *cs) 
{
	GdkColor *c;

	c = (GdkColor *)(g_list_first (cs->list))->data;

	return c;
}

GdkColor *
html_color_stack_pop (HTMLColorStack *cs)
{
	GdkColor *c;

	c = html_color_stack_top (cs);

	cs->list = g_list_remove (cs->list, c);

	return c;
}





