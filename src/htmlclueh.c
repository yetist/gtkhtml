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

#include "htmlclueh.h"


HTMLClueHClass html_clueh_class;


static void
set_max_width (HTMLObject *o, gint w)
{
	HTMLObject *obj;

	o->max_width = w;

	/* First calculate width minus fixed width objects */
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		if (obj->percent <= 0) /* i.e. fixed width objects */
			w -= obj->width;
	}

	/* Now call set_max_width for variable objects */
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		if (obj->percent > 0)
			html_object_set_max_width (obj,
						   w - HTML_CLUEH (o)->indent);
	}
}

static void
calc_size (HTMLObject *clue, HTMLObject *parent)
{
	HTMLObject *obj;
	gint lmargin = 0;
	gint a = 0, d = 0;

	/* Make sure the children are properly sized */
	html_object_set_max_width (clue, clue->max_width);

	HTML_OBJECT_CLASS (&html_clue_class)->calc_size (clue, parent);

	if (parent)
		lmargin = html_clue_get_left_margin (HTML_CLUE (parent),
						     clue->y);

	clue->width = lmargin + HTML_CLUEH (clue)->indent;
	clue->descent = 0;
	clue->ascent = 0;

	for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->next) {
		html_object_fit_line (obj,
				      (obj == HTML_CLUE (clue)->head),
				      TRUE, -1);
		obj->x = clue->width;
		clue->width += obj->width;
		if (obj->ascent > a)
			a = obj->ascent;
		if (obj->descent > d)
			d = obj->descent;
	}

	clue->ascent = a + d;

	switch (HTML_CLUE (clue)->valign) {
	case HTML_VALIGN_TOP:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->next)
			obj->y = obj->ascent;
		break;

	case HTML_VALIGN_CENTER:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->next)
			obj->y = clue->ascent / 2;
		break;

	default:
		for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->next)
			obj->y = clue->ascent - d;
	}
}

static gint
calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint minWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next)
		minWidth += html_object_calc_min_width (obj);

	return minWidth + HTML_CLUEH (o)->indent;
}

static gint
calc_preferred_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint prefWidth = 0;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) 
		prefWidth += html_object_calc_preferred_width (obj);

	return prefWidth + HTML_CLUEH (o)->indent;
}


void
html_clueh_type_init (void)
{
	html_clueh_class_init (&html_clueh_class, HTML_TYPE_CLUEH);
}

void
html_clueh_class_init (HTMLClueHClass *klass,
		       HTMLType type)
{
	HTMLClueClass *clue_class;
	HTMLObjectClass *object_class;

	clue_class = HTML_CLUE_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_clue_class_init (clue_class, type);

	object_class->set_max_width = set_max_width;
	object_class->calc_size = calc_size;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
}

void
html_clueh_init (HTMLClueH *clueh, HTMLClueHClass *klass,
		 gint x, gint y, gint max_width)
{
	HTMLObject *object;
	HTMLClue *clue;

	clue = HTML_CLUE (clueh);
	object = HTML_OBJECT (clueh);

	html_clue_init (clue, HTML_CLUE_CLASS (klass));

	clue->valign = HTML_VALIGN_BOTTOM;
	clue->halign = HTML_HALIGN_LEFT;
	clue->head = clue->tail = clue->curr = 0;

	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = 100;
	object->width = object->max_width;
	object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
}

HTMLObject *
html_clueh_new (gint x, gint y, gint max_width)
{
	HTMLClueH *clueh;

	clueh = g_new0 (HTMLClueH, 1);
	html_clueh_init (clueh, &html_clueh_class, x, y, max_width);

	return HTML_OBJECT (clueh);
}
