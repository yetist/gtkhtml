/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

#include <config.h>
#include <gtk/gtkmain.h>
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlobject.h"


#define BLINK_TIMEOUT 500


void
html_engine_hide_cursor  (HTMLEngine *engine)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	if (engine->editable && engine->cursor_hide_count == 0)
		html_engine_draw_cursor_in_area (engine, 0, 0, -1, -1);

	engine->cursor_hide_count++;
}

void
html_engine_show_cursor  (HTMLEngine *engine)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));

	if (engine->cursor_hide_count > 0) {
		engine->cursor_hide_count--;
		if (engine->editable && engine->cursor_hide_count == 0)
			html_engine_draw_cursor_in_area (engine, 0, 0, -1, -1);
	}
}

static inline void
move_rect (HTMLEngine *engine, gint *x1, gint *y1, gint *x2, gint *y2)
{
	*x1 = *x1 + engine->leftBorder - engine->x_offset;
	*y1 = *y1 + engine->topBorder - engine->y_offset;
	*x2 = *x2 + engine->leftBorder - engine->x_offset;
	*y2 = *y2 + engine->topBorder - engine->y_offset;
}

static gboolean
clip_rect (HTMLEngine *engine, gint x, gint y, gint width, gint height, gint *x1, gint *y1, gint *x2, gint *y2)
{
	if (*x1 >= x + width || *y1 >= y + height || *x2 < x || *y2 < y)
		return FALSE;

	if (*x2 >= x + width)
		*x2 = x + width - 1;
	if (*y2 >= y + height)
		*y2 = y + height - 1;

	if (*x1 < x)
		*x1 = x;
	if (*y1 < y)
		*y1 = y;

	return TRUE;
}

static void
draw_cursor_rectangle (HTMLEngine *e, gint *x1, gint *y1, gint *x2, gint *y2)
{
	GdkGC *gc;
	GdkColor color;
	gint8 dashes [2] = { 3, 3 };
	static gint offset = 0;

	move_rect (e, x1, y1, x2, y2);

	gc = gdk_gc_new (e->window);
	gdk_color_black (gdk_window_get_colormap (e->window), &color);
	gdk_gc_set_foreground (gc, &color);
	color.red = color.green = 0xffff;
	color.blue = 0;
	gdk_color_alloc (gdk_window_get_colormap (e->window), &color);
	gdk_gc_set_background (gc, &color);
	gdk_gc_set_line_attributes (gc, 1, GDK_LINE_DOUBLE_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	gdk_gc_set_dashes (gc, offset, dashes, 2);
	gdk_draw_rectangle (e->window, gc, 0, *x1, *y1, *x2 - *x1, *y2 - *y1);
	gdk_gc_unref (gc);
	offset++;
	offset %= 6;
}

void
html_engine_draw_cell_cursor (HTMLEngine *e)
{
	HTMLTableCell *cell;

	cell = html_engine_get_table_cell (e);

	if (cell) {
		gint x1, y1, x2, y2;

		if (cell != e->cursor_cell) {
			/* printf ("clear cell cursor 1 %p\n", e->cursor_cell); */
			if (e->cursor_cell)
				html_engine_queue_draw (e, HTML_OBJECT (e->cursor_cell));
			/* printf ("set cursor cell %p\n", cell); */
			e->cursor_cell = cell;
		}

		html_object_calc_abs_position (HTML_OBJECT (cell), &x1, &y2);
		x2 = x1 + HTML_OBJECT (cell)->width - 1;
		y2 -= 2;
		y1 = y2 - (HTML_OBJECT (cell)->ascent + HTML_OBJECT (cell)->descent - 1) + 1;

		/* printf ("draw cell cursor %p\n", cell); */
		draw_cursor_rectangle (e, &x1, &y1, &x2, &y2);
	} else
		if (e->cursor_cell) {
			/* printf ("clear cell cursor 2\n"); */
			html_engine_queue_draw (e, HTML_OBJECT (e->cursor_cell));
			e->cursor_cell = NULL;
		}
}

void
html_engine_draw_cursor_in_area (HTMLEngine *engine,
				 gint x, gint y,
				 gint width, gint height)
{
	HTMLObject *obj;
	guint offset;
	gint x1, y1, x2, y2;

	g_assert (engine->editable);

	html_engine_draw_cell_cursor (engine);

	if (engine->cursor_hide_count > 0 || ! engine->editable || engine->thaw_idle_id)
		return;

	obj = engine->cursor->object;
	if (obj == NULL)
		return;

	offset = engine->cursor->offset;

	if (width < 0 || height < 0) {
		width = engine->width;
		height = engine->height;
		x = 0;
		y = 0;
	}

	html_object_get_cursor (obj, engine->painter, offset, &x1, &y1, &x2, &y2);
	move_rect (engine, &x1, &y1, &x2, &y2);
	if (clip_rect (engine, x, y, width, height, &x1, &y1, &x2, &y2))
		gdk_draw_line (engine->window, engine->invert_gc, x1, y1, x2, y2);
}


/* Blinking cursor implementation.  */

static gint
blink_timeout_cb (gpointer data)
{
	HTMLEngine *engine;

	g_return_val_if_fail (HTML_IS_ENGINE (data), FALSE);

	engine = HTML_ENGINE (data);

	engine->blinking_status = ! engine->blinking_status;

	if (engine->blinking_status)
		html_engine_show_cursor (engine);
	else
		html_engine_hide_cursor (engine);

	return TRUE;
}

void
html_engine_setup_blinking_cursor (HTMLEngine *engine)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->blinking_timer_id == 0);

	html_engine_show_cursor (engine);
	engine->blinking_status = FALSE;

	engine->blinking_timer_id = gtk_timeout_add (BLINK_TIMEOUT, blink_timeout_cb, engine);
}

void
html_engine_stop_blinking_cursor (HTMLEngine *engine)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->blinking_timer_id != 0);

	if (engine->blinking_status) {
		html_engine_hide_cursor (engine);
		engine->blinking_status = FALSE;
	}

	gtk_timeout_remove (engine->blinking_timer_id);
	engine->blinking_timer_id = 0;
}

void
html_engine_reset_blinking_cursor (HTMLEngine *engine)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->blinking_timer_id != 0);

	if (engine->blinking_status)
		return;

	html_engine_show_cursor (engine);
	engine->blinking_status = TRUE;
	gtk_timeout_remove (engine->blinking_timer_id);
	engine->blinking_timer_id = gtk_timeout_add (BLINK_TIMEOUT, blink_timeout_cb, engine);
}
