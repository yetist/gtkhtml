/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

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

#include "htmlinterval.h"

HTMLInterval *
html_interval_new (HTMLObject *from, HTMLObject *to, guint from_offset, guint to_offset)
{
	HTMLInterval *i = g_new (HTMLInterval, 1);

	i->from = from;
	i->to   = to;
	i->from_offset = from_offset;
	i->to_offset   = to_offset;

	return i;
}

HTMLInterval *
html_interval_new_from_cursor (HTMLCursor *begin, HTMLCursor *end)
{
	return html_interval_new (begin->object, end->object, begin->offset, end->offset);
}

void
html_interval_destroy (HTMLInterval *i)
{
	g_free (i);
}

guint
html_interval_get_length (HTMLInterval *i, HTMLObject *obj)
{
	if (obj != i->from && obj != i->to)
		return html_object_get_length (obj);	
	if (obj == i->from) {
		if (obj == i->to)
			return i->to_offset - i->from_offset;
		else
			return html_object_get_length (obj) - i->from_offset;
	} else
		return i->to_offset;
}

guint
html_interval_get_start (HTMLInterval *i, HTMLObject *obj)
{
	return (obj != i->from) ? 0 : i->from_offset;
}

void
html_interval_select (HTMLInterval *i, HTMLEngine *e)
{
	HTMLObject *obj;
	guint offset, len;

	obj = i->from;
	while (obj) {
		offset = html_interval_get_start  (i, obj);
		len    = html_interval_get_length (i, obj);
		if (len) {
			e->active_selection = TRUE;
			html_object_select_range (obj, e, offset, len, TRUE);
		}
		if (obj == i->to)
			break;
		obj = html_object_next_not_slave (obj);
	}
	html_engine_set_active_selection (e, e->active_selection, GDK_CURRENT_TIME);
}
