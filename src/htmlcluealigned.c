/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
#include "htmlcluealigned.h"

#define ALIGN_BORDER 0

static void html_cluealigned_calc_size (HTMLObject *o, HTMLObject *parent);
static void html_cluealigned_set_max_width (HTMLObject *o, gint max_width);

HTMLObject *
html_cluealigned_new (HTMLClue *parent, gint x, gint y, gint max_width, gint percent)
{
	HTMLClueAligned *aclue = g_new0 (HTMLClueAligned, 1);
	HTMLClue *clue = HTML_CLUE (aclue);
	HTMLObject *object = HTML_OBJECT (aclue);
	html_clue_init (clue, ClueAligned);

	/* HTMLObject functions */
	object->calc_size = html_cluealigned_calc_size;
	object->draw = html_clue_draw;
	object->set_max_width = html_cluealigned_set_max_width;
	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = percent;
	
	clue->valign = Bottom;
	clue->halign = Left;
	clue->head = clue->tail = clue->curr = 0;

	if (percent > 0) {
		object->width = max_width * percent / 100;
		object->flags &= ~FixedWidth;
	}
	else if (percent < 0) {
		object->width = max_width;
		object->flags |= ~FixedWidth;
	}
	else 
		object->width = max_width;

	aclue->prnt = parent;
	aclue->nextAligned = 0;
	object->flags |= Aligned;

	return object;
}

static void
html_cluealigned_calc_size (HTMLObject *o, HTMLObject *parent)
{
	HTMLObject *obj;

	html_clue_calc_size (o, parent);

	o->width = 0;
	o->ascent = ALIGN_BORDER;
	o->descent = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		if (obj->width > o->width)
			o->width = obj->width;
		o->ascent += obj->ascent + obj->descent;
		obj->x = ALIGN_BORDER;
		obj->y = o->ascent - obj->descent;
	}
	
	o->ascent += ALIGN_BORDER;
	o->width += (ALIGN_BORDER * 2);
}

static void
html_cluealigned_set_max_width (HTMLObject *o, gint max_width)
{
	HTMLObject *obj;

	o->max_width = max_width;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj)
		obj->set_max_width (obj, max_width);
}
