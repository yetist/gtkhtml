/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) 1999 Helix Code, Inc.

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


#define HC_CLASS(x) (HTML_CLUE_CLASS (HTML_OBJECT (x)->klass))

HTMLClueClass html_clue_class;


/* HTMLObject methods.  */

/* FIXME TODO */
static void
destroy (HTMLObject *o)
{
#if 0
	while (HTML_CLUE (o)->head) {
		HTML_CLUE (o)->curr = HTML_CLUE (o)->head->next;
		html_object_destroy (HTML_CLUE (o)->head);
		HTML_CLUE (o)->head = HTML_CLUE (o)->curr;
	}
#endif

	HTML_OBJECT_CLASS (&html_object_class)->destroy (o);
}

static void
draw (HTMLObject *o, HTMLPainter *p,
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

#if 0
	/* FIXME FIXME!  Temporary hack.  :-)  */
	/* Draw a rect around the clue */
	if (HTML_OBJECT_TYPE (o) == HTML_TYPE_CLUEV) {
		html_painter_set_pen (p, &red);
	} else if (HTML_OBJECT_TYPE (o) == HTML_TYPE_CLUEFLOW) {
		html_painter_set_pen (p, &green);
		html_painter_draw_rect (p, tx, ty, o->width, o->ascent + o->descent);
	} else if (HTML_OBJECT_TYPE (o) == HTML_TYPE_TABLECELL) {
		html_painter_set_pen (p, &blue);
	}
#endif

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		if (!(obj->flags & HTML_OBJECT_FLAG_ALIGNED)) {
			html_object_draw (obj, p,
					  x - o->x, y - (o->y - o->ascent),
					  width, height,
					  tx, ty);
		}
	}
}

static void
set_max_ascent (HTMLObject *o, gint a)
{
	HTMLClue *clue = HTML_CLUE (o);
	HTMLObject *obj;

	if (clue->valign == HTML_VALIGN_CENTER) {
		for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
			obj->y = obj->y + ((a - o->ascent) / 2);
		}
	}

	else if (clue->valign == HTML_VALIGN_BOTTOM) {
		for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
			obj->y = obj->y + a - o->ascent;
		}
	}

	o->ascent = a;
	
}

static void
set_max_descent (HTMLObject *o, gint d)
{
	HTMLClue *clue = HTML_CLUE (o);
	HTMLObject *obj;
	
	if (clue->valign == HTML_VALIGN_CENTER) {
		for (obj = clue->head; obj != 0; obj = obj->next) {
			obj->y = obj->y + ((d - o->descent) / 2);
		}
	}
	else if (clue->valign == HTML_VALIGN_BOTTOM) {
		for (obj = clue->head; obj != 0; obj = obj->next) {
			obj->y = obj->y + d - o->descent;
		}
	}
	
	o->descent = d;

}

static void
reset (HTMLObject *clue)
{
	HTMLObject *obj;

	for (obj = HTML_CLUE (clue)->head; obj != 0; obj = obj->next)
		html_object_reset (obj);

	HTML_CLUE (clue)->curr = 0;
}

static void
calc_size (HTMLObject *o,
	   HTMLObject *parent)
{
	/* If we have already called calc_size for the children, then just
	   continue from the last object done in previous call. */
	if (!HTML_CLUE (o)->curr) {
		o->ascent = 0;
		HTML_CLUE (o)->curr = HTML_CLUE (o)->head;
	}

	while (HTML_CLUE (o)->curr != 0) {
		html_object_calc_size (HTML_CLUE (o)->curr, o);
		HTML_CLUE (o)->curr = HTML_CLUE (o)->curr->next;
	}

	/* Remember the last object so that we can start from here next time
	   we are called */
	HTML_CLUE (o)->curr = HTML_CLUE (o)->tail;
}

static gint
calc_preferred_width (HTMLObject *o)
{
	gint prefWidth = 0;
	HTMLObject *obj;
	
	if (o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH)
		return o->width;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		gint w;

		w = html_object_calc_preferred_width (obj);
		if (w > prefWidth)
			prefWidth = w;
	}

	return prefWidth;
}

static void
calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	HTMLObject *obj;

	gint lx = x + o->x;
	gint ly = y + o->y - o->ascent;

	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next)
		html_object_calc_absolute_pos (obj, lx, ly);
}

static gint
calc_min_width (HTMLObject *o)
{
	HTMLObject *obj;
	gint minWidth = 0;
	
	for (obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next) {
		gint w;

		w = html_object_calc_min_width (obj);
		if (w > minWidth)
			minWidth = w;
	}
	
	if ((o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH)) {
		if (o->width > minWidth)
			minWidth = o->width;
	}
	
	return minWidth;
}

static HTMLObject *
mouse_event (HTMLObject *self, gint _x, gint _y,
	     gint button, gint state)
{
	HTMLClue *clue;
	HTMLObject *obj;
	HTMLObject *obj2;

	clue = HTML_CLUE (self);

	if ( _x < self->x
	     || _x > self->x + self->width
	     || _y > self->y + self->descent
	     || _y < self->y - self->ascent)
		return 0;

	for ( obj = clue->head; obj != NULL; obj = obj->next ) {
		if ((obj2 = html_object_mouse_event
			( obj, _x - self->x, _y - (self->y - self->ascent),
			  button, state )) != 0 )
			return obj2;
	}

	return 0;
}

static HTMLObject*
check_point (HTMLObject *o, gint _x, gint _y )
{
	HTMLObject *obj;
	HTMLObject *obj2;

	if ( _x < o->x || _x > o->x + o->width
	     || _y > o->y + o->descent || _y < o->y - o->ascent)
		return 0L;

	for ( obj = HTML_CLUE (o)->head; obj != 0; obj = obj->next ) {
		obj2 = html_object_check_point (obj, _x - o->x,
						_y - (o->y - o->ascent) );
		if (obj2 != NULL)
			return obj2;
	}

	return 0;
}


/* HTMLClue methods.  */

static gint
get_left_margin (HTMLClue *o, gint y)
{
	return 0;
}

static gint
get_right_margin (HTMLClue *o, gint y)
{
	return HTML_OBJECT (o)->max_width;
}

static gint
get_left_clear (HTMLClue *o, gint y)
{
	return y;
}

static gint
get_right_clear (HTMLClue *o, gint y)
{
	return y;
}

static void
find_free_area (HTMLClue *clue, gint y,
		gint width, gint height,
		gint indent, gint *y_pos,
		gint *lmargin, gint *rmargin)
{
	*y_pos = y;
	*lmargin = 0;
	*rmargin = HTML_OBJECT (clue)->max_width;
}

static void
append_right_aligned (HTMLClue *clue, HTMLClue *aclue)
{
	/* This needs to be implemented in the subclasses.  */
	g_warning ("`%s' does not implement `append_right_aligned()'.",
		   html_type_name (HTML_OBJECT_TYPE (clue)));
}

static gboolean
appended (HTMLClue *clue, HTMLClue *aclue)
{
	return FALSE;
}


void
html_clue_type_init (void)
{
	html_clue_class_init (&html_clue_class, HTML_TYPE_CLUE);
}

void
html_clue_class_init (HTMLClueClass *klass,
		      HTMLType type)
{
	HTMLObjectClass *object_class;

	g_return_if_fail (klass != NULL);

	object_class = HTML_OBJECT_CLASS (klass);
	html_object_class_init (object_class, type);
	
	/* HTMLObject functions */
	object_class->destroy = destroy;
	object_class->draw = draw;
	object_class->set_max_ascent = set_max_ascent;
	object_class->set_max_descent = set_max_descent;
	object_class->destroy = destroy;
	object_class->reset = reset;
	object_class->calc_size = calc_size;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_absolute_pos = calc_absolute_pos;
	object_class->mouse_event = mouse_event;
	object_class->check_point = check_point;

	/* HTMLClue methods.  */
	klass->get_left_margin = get_left_margin;
	klass->get_right_margin = get_right_margin;
	klass->get_left_clear = get_left_clear;
	klass->get_right_clear = get_right_clear;
	klass->find_free_area = find_free_area;
	klass->append_right_aligned = append_right_aligned;
	klass->appended = appended;
}

void
html_clue_init (HTMLClue *clue,
		HTMLClueClass *klass)
{
	HTMLObject *object;

	object = HTML_OBJECT (clue);
	html_object_init (object, HTML_OBJECT_CLASS (klass));

	clue->head = NULL;
	clue->tail = NULL;
	clue->curr = NULL;

	clue->valign = HTML_VALIGN_TOP;
	clue->halign = HTML_HALIGN_LEFT;
}


gint
html_clue_get_left_margin (HTMLClue *clue, gint y)
{
	return (* HC_CLASS (clue)->get_left_margin) (clue, y);
}

gint
html_clue_get_right_margin (HTMLClue *clue, gint y)
{
	return (* HC_CLASS (clue)->get_right_margin) (clue, y);
}

gint
html_clue_get_left_clear (HTMLClue *clue, gint y)
{
	return (* HC_CLASS (clue)->get_left_clear) (clue, y);
}

gint
html_clue_get_right_clear (HTMLClue *clue, gint y)
{
	return (* HC_CLASS (clue)->get_right_clear) (clue, y);
}

void
html_clue_find_free_area (HTMLClue *clue, gint y,
			  gint width, gint height, gint indent, gint *y_pos,
			  gint *lmargin, gint *rmargin)
{
	(* HC_CLASS (clue)->find_free_area) (clue, y, width, height,
					     indent, y_pos,
					     lmargin, rmargin);
}

void
html_clue_append_right_aligned (HTMLClue *clue, HTMLClue *aclue)
{
	(* HC_CLASS (clue)->append_right_aligned) (clue, aclue);
}

void
html_clue_append_left_aligned (HTMLClue *clue, HTMLClue *aclue)
{
	(* HC_CLASS (clue)->append_left_aligned) (clue, aclue);
}

gboolean
html_clue_appended (HTMLClue *clue, HTMLClue *aclue)
{
	return (* HC_CLASS (clue)->appended) (clue, aclue);
}


/* Utility function.  */

void
html_clue_append (HTMLClue *clue, HTMLObject *o)
{
	if (! clue->head) {
		clue->head = clue->tail = o;
	} else {
		clue->tail->next = o;
		clue->tail = o;
	}
}
