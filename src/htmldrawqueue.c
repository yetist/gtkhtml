/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1999, 2000 Helix Code, Inc.
   
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

#include "htmldrawqueue.h"


HTMLDrawQueue *
html_draw_queue_new (HTMLEngine *engine)
{
	HTMLDrawQueue *new;

	g_return_val_if_fail (engine != NULL, NULL);

	new = g_new (HTMLDrawQueue, 1);

	new->engine = engine;
	new->elems = NULL;
	new->last = NULL;

	return new;
}

void
html_draw_queue_destroy (HTMLDrawQueue *queue)
{
	GList *p;

	g_return_if_fail (queue != NULL);

	for (p = queue->elems; p != NULL; p = p->next) {
		HTMLObject *obj;

		obj = p->data;
		obj->redraw_pending = FALSE;
	}

	g_list_free (queue->elems);

	g_free (queue);
}

void
html_draw_queue_add (HTMLDrawQueue *queue, HTMLObject *object)
{
	g_return_if_fail (queue != NULL);
	g_return_if_fail (object != NULL);

	if (object->redraw_pending)
		return;

	object->redraw_pending = TRUE;

	queue->last = g_list_append (queue->last, object);
	if (queue->elems == NULL) {
		gtk_signal_emit_by_name (GTK_OBJECT (queue->engine), "draw_pending");
		queue->elems = queue->last;
	} else {
		queue->last = queue->last->next;
	}
}


static void
draw_obj (HTMLDrawQueue *queue,
	  HTMLObject *obj)
{
	HTMLEngine *e;
	HTMLObject *o;
	gint x1, y1, x2, y2;
	gint tx, ty;

	e = queue->engine;

	tx = e->leftBorder - e->x_offset;
	ty = e->topBorder - e->y_offset;

	/* First calculate the translation offset for drawing the object.  */

	for (o = obj->parent; o != NULL; o = o->parent) {
		tx += o->x;
		ty += o->y - o->ascent;
	}

	/* Then prepare for drawing.  We will only update this object, so we
           only allocate enough size for it.  */

	x1 = obj->x + tx;
	y1 = obj->y - obj->ascent + ty;
	x2 = obj->x + obj->width + tx;
	y2 = obj->y + obj->descent + ty;

	if (x2 < 0 || y2 < 0 || x1 > e->width || y1 > e->height)
		return;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > e->width)
		x2 = e->width;
	if (y2 > e->height)
		y2 = e->height;

	html_painter_begin (e->painter, x1, y1, x2, y2);

	/* FIXME we are duplicating code from HTMLEngine here.  Instead, there
           should be a function in HTMLEngine to paint stuff.  */

	html_painter_set_pen (e->painter, &e->bgColor);
	html_painter_fill_rect (e->painter, x1, y1, x2, y2);

	/* Draw the actual object.  */

	html_object_draw (obj,
			  e->painter, 
			  obj->x, obj->y - obj->ascent,
			  obj->width, obj->ascent + obj->descent,
			  tx, ty);
#if 0
	html_painter_draw_line (e->painter, x1, y1, x2, y2);
	html_painter_draw_line (e->painter, x2, y1, x1, y2);
#endif
	/* Done.  */

	html_painter_end (e->painter);

	if (e->editable)
		html_engine_draw_cursor_in_area (e, x1, y1, x2 - x1, y2 - y1);
}

void
html_draw_queue_flush (HTMLDrawQueue *queue)
{
	GList *p;

	for (p = queue->elems; p != NULL; p = p->next) {
		HTMLObject *obj;

		obj = p->data;

		if (obj->free_pending) {
			g_free (obj);
		} else if (obj->redraw_pending) {
			draw_obj (queue, obj);
			obj->redraw_pending = FALSE;
		}
	}

	g_list_free (queue->elems);

	queue->elems = NULL;
	queue->last = NULL;
}
