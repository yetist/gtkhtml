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
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _HTML_INTERVAL_H_
#define _HTML_INTERVAL_H_

typedef struct _HTMLInterval HTMLInterval;

#include "htmlobject.h"
#include "htmlengine.h"

struct _HTMLInterval {
	HTMLObject *from;
	HTMLObject *to;
	guint       from_offset;
	guint       to_offset;
};

HTMLInterval *               html_interval_new             (HTMLObject *from,
							    HTMLObject *to,
							    guint from_offset,
							    guint to_offset);
HTMLInterval *               html_interval_new_from_cursor (HTMLCursor *begin,
							    HTMLCursor *end);

void                         html_interval_destroy         (HTMLInterval *i);
guint                        html_interval_get_length      (HTMLInterval *i,
							    HTMLObject *obj);
guint                        html_interval_get_start       (HTMLInterval *i,
							    HTMLObject *obj);
void                         html_interval_select          (HTMLInterval *i,
							    HTMLEngine *e);
#endif
