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
#include "htmlclue.h"
#include "htmlcluev.h"

static void html_clue_destroy (HTMLObject *o);
static void html_clue_set_max_descent (HTMLObject *o, gint d);
static gint html_clue_get_left_margin (HTMLClue *o, gint y);
static gint html_clue_get_right_margin (HTMLClue *o, gint y);
static gint html_clue_calc_min_width (HTMLObject *o);
static void html_clue_calc_absolute_pos (HTMLObject *o, gint x, gint y);
static gint html_clue_appended (HTMLClue *clue, HTMLClue *aclue);

void
html_clue_init (HTMLClue *clue, objectType ObjectType)
{
	HTMLObject *object = HTML_OBJECT (clue);
	html_object_init (object, ObjectType);
	
	/* HTMLObject functions */
	object->draw = html_clue_draw;
	object->set_max_ascent = html_clue_set_max_ascent;
	object->set_max_descent = html_clue_set_max_descent;
	object->destroy = html_clue_destroy;
	object->reset = html_clue_reset;
	object->calc_preferred_width = html_clue_calc_preferred_width;
	object->calc_min_width = html_clue_calc_min_width;
	object->calc_absolute_pos = html_clue_calc_absolute_pos;

	/* HTMLClue functions */
	clue->get_left_margin = html_clue_get_left_margin;
	clue->get_right_margin = html_clue_get_right_margin;
	clue->appended = html_clue_appended;
}

static void
html_clue_destroy (HTMLObject *o)
{
	while (HTML_CLUE (o)->head) {
		HTML_CLUE (o)->curr = HTML_CLUE (o)->head->nextObj;
		HTML_OBJECT (HTML_CLUE (o)->head)->destroy (HTML_CLUE (o)->head);
		HTML_CLUE (o)->head = HTML_CLUE (o)->curr;
	}

	g_free (o);
}

void
html_clue_draw (HTMLObject *o, HTMLPainter *p,
		gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLObject *obj;
	static GdkColor red = {0}, green = {0}, blue = {0};
	
	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	tx += o->x;
	ty += o->y - o->ascent;
	
	if (!red.pixel) {
		gdk_color_parse ("red", &red);
		gdk_colormap_alloc_color (gdk_window_get_colormap (html_painter_get_window (p)),
					  &red, FALSE, TRUE);
		gdk_color_parse ("green", &green);
		gdk_colormap_alloc_color (gdk_window_get_colormap (html_painter_get_window (p)),
					  &green, FALSE, TRUE);
		gdk_color_parse ("blue", &blue);
		gdk_colormap_alloc_color (gdk_window_get_colormap (html_painter_get_window (p)),
					  &blue, FALSE, TRUE);
	}

	/* Draw a rect around the clue */
	if (o->ObjectType == ClueV) {
		html_painter_set_pen (p, &red);
	}
	else if (o->ObjectType == ClueFlow) {
		html_painter_set_pen (p, &green);
		html_painter_draw_rect (p, tx, ty, o->width, o->ascent + o->descent);
	}
	else if (o->ObjectType == TableCell) {
		html_painter_set_pen (p, &blue);
	}

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		if (!(obj->flags & Aligned)) {
			if (obj->draw)
				obj->draw (obj, p, x - o->x, y - (o->y - o->ascent),
					   width, height, tx, ty);
		}     	}

}



static gint
html_clue_get_left_margin (HTMLClue *o, gint y)
{
	return 0;
}

static gint
html_clue_get_right_margin (HTMLClue *o, gint y)
{
	return HTML_OBJECT (o)->max_width;
}

void
html_clue_set_max_ascent (HTMLObject *o, gint a)
{
	HTMLClue *clue = HTML_CLUE (o);
	HTMLObject *obj;

	if (clue->valign == VCenter) {
		for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
			obj->y = obj->y + ((a - o->ascent) / 2);
		}
	}

	else if (clue->valign == Bottom) {
		for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
			obj->y = obj->y + a - o->ascent;
		}
	}

	o->ascent = a;
	
}

static void
html_clue_set_max_descent (HTMLObject *o, gint d)
{
	HTMLClue *clue = HTML_CLUE (o);
	HTMLObject *obj;
	
	if (clue->valign == VCenter) {
		for (obj = clue->head; obj != 0; obj = obj->nextObj) {
			obj->y = obj->y + ((d - o->descent) / 2);
		}
	}
	else if (clue->valign == Bottom) {
		for (obj = clue->head; obj != 0; obj = obj->nextObj) {
			obj->y = obj->y + d - o->descent;
		}
	}
	
	o->descent = d;

}

gint
html_clue_get_left_clear (HTMLObject *clue, gint y)
{
	switch (clue->ObjectType) {
	default:
		g_print ("get_left_clear: Unknown object type: %d\n", clue->ObjectType);
		return y;
	}
}

gint
html_clue_get_right_clear (HTMLObject *clue, gint y)
{
	switch (clue->ObjectType) {
	default:
		g_print ("get_right_clear: Unknown object type: %d\n", clue->ObjectType);
		return y;
	}
}

void
html_clue_reset (HTMLObject *clue)
{
	HTMLObject *obj;

	for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->nextObj)
		obj->reset (obj);

	HTML_CLUE (clue)->curr = 0;
}

void
html_clue_set_next (HTMLObject *clue, HTMLObject *o)
{
	clue->nextObj = o;
}

void
html_clue_append (HTMLObject *clue, HTMLObject *o)
{

	if (!HTML_CLUE (clue)->head) {
		HTML_CLUE (clue)->head = HTML_CLUE (clue)->tail = o;
	}
	else {
		html_clue_set_next (HTML_CLUE (clue)->tail, o);
		HTML_CLUE (clue)->tail = o;
	}
}

void
html_clue_calc_size (HTMLObject *o, HTMLObject *parent)
{
	/* If we have already called calc_size for the children, then just
	   continue from the last object done in previous call. */
	if (!HTML_CLUE (o)->curr) {
		o->ascent = 0;
		HTML_CLUE (o)->curr = HTML_CLUE (o)->head;
	}

	while (HTML_CLUE (o)->curr != 0) {
		HTML_CLUE (o)->curr->calc_size (HTML_CLUE (o)->curr, o);
		HTML_CLUE (o)->curr = HTML_CLUE (o)->curr->nextObj;
	}

	/* Remember the last object so that we can start from here next time
	   we are called */
	HTML_CLUE (o)->curr = HTML_CLUE (o)->tail;
}

gint
html_clue_calc_preferred_width (HTMLObject *o)
{
	gint prefWidth = 0;
	HTMLObject *obj;
	
	if (o->flags & FixedWidth)
		return o->width;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		gint w = obj->calc_preferred_width (obj);
		if (w > prefWidth)
			prefWidth = w;
	}

	return prefWidth;
}

static void
html_clue_calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	HTMLObject *obj;

	gint lx = x + o->x;
	gint ly = y + o->y - o->ascent;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj)
		obj->calc_absolute_pos (obj, lx, ly);
}

static gint
html_clue_calc_min_width (HTMLObject *o) {
	HTMLObject *obj;
	gint minWidth = 0;
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->nextObj) {
		gint w = obj->calc_min_width (obj);
		if (w > minWidth)
			minWidth = w;
	}
	
	if ((o->flags & FixedWidth)) {
		if (o->width > minWidth)
			minWidth = o->width;
	}
	
	return minWidth;
}

static gboolean
html_clue_appended (HTMLClue *clue, HTMLClue *aclue)
{
	return FALSE;
}
