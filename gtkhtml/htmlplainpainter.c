/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include "htmlentity.h"
#include "htmlgdkpainter.h"
#include "htmlplainpainter.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine.h"
#include "htmltext.h"

G_DEFINE_TYPE (HTMLPlainPainter, html_plain_painter, HTML_TYPE_GDK_PAINTER);
static void
draw_border (HTMLPainter *painter,
             GdkColor *bg,
             gint x,
             gint y,
             gint width,
             gint height,
             HTMLBorderStyle style,
             gint bordersize)
{
}

static void
draw_background (HTMLPainter *painter,
                 GdkColor *color,
                 GdkPixbuf *pixbuf,
                 gint x,
                 gint y,
                 gint width,
                 gint height,
                 gint tile_x,
                 gint tile_y)
{
	HTMLGdkPainter *gdk_painter;
	GdkRectangle expose, paint, clip;

	gdk_painter = HTML_GDK_PAINTER (painter);

	expose.x = x;
	expose.y = y;
	expose.width  = width;
	expose.height = height;

	clip.x = gdk_painter->x1;
	clip.width = gdk_painter->x2 - gdk_painter->x1;
	clip.y = gdk_painter->y1;
	clip.height = gdk_painter->y2 - gdk_painter->y1;

	if (!gdk_rectangle_intersect (&clip, &expose, &paint))
		return;

	if (!color && !pixbuf)
		return;

	if (color) {
		gdk_cairo_set_source_color (gdk_painter->cr, color);
		cairo_rectangle (gdk_painter->cr, paint.x - clip.x, paint.y - clip.y, paint.width, paint.height);
		cairo_fill (gdk_painter->cr);
	}

	return;
}

static void
draw_pixmap (HTMLPainter *painter,
             GdkPixbuf *pixbuf,
             gint x,
             gint y,
             gint scale_width,
             gint scale_height,
             const GdkColor *color)
{
}

static void
fill_rect (HTMLPainter *painter,
           gint x,
           gint y,
           gint width,
           gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	cairo_rectangle (gdk_painter->cr, x - gdk_painter->x1, y - gdk_painter->y1, width, height);
	cairo_fill (gdk_painter->cr);
}

static void
draw_shade_line (HTMLPainter *painter,
                 gint x,
                 gint y,
                 gint width)
{
}

static void
html_plain_painter_init (HTMLPlainPainter *painter)
{
}

static void
draw_rect (HTMLPainter *painter,
           gint x,
           gint y,
           gint width,
           gint height)
{
}

static guint
get_page_width (HTMLPainter *painter,
                HTMLEngine *e)
{
	return MIN (72 * MAX (html_painter_get_space_width (painter, GTK_HTML_FONT_STYLE_SIZE_3 | GTK_HTML_FONT_STYLE_FIXED, NULL),
			      html_painter_get_e_width (painter, GTK_HTML_FONT_STYLE_SIZE_3 | GTK_HTML_FONT_STYLE_FIXED, NULL)),
		    html_engine_get_view_width (e)) + (html_engine_get_left_border (e) + html_engine_get_right_border (e));
}

static guint
get_page_height (HTMLPainter *painter,
                 HTMLEngine *e)
{
	return html_engine_get_view_height (e) + (html_engine_get_top_border (e) + html_engine_get_bottom_border (e));
}

static void
html_plain_painter_class_init (HTMLPlainPainterClass *klass)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (klass);
	html_plain_painter_parent_class = g_type_class_peek_parent (klass);

	painter_class->draw_rect = draw_rect;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_border = draw_border;
	painter_class->draw_background = draw_background;
	painter_class->get_page_width = get_page_width;
	painter_class->get_page_height = get_page_height;
}

HTMLPainter *
html_plain_painter_new (GtkWidget *widget,
                        gboolean double_buffer)
{
	HTMLPlainPainter *new;

	new = g_object_new (HTML_TYPE_PLAIN_PAINTER, NULL);
	html_painter_set_widget (HTML_PAINTER (new), widget);
	HTML_GDK_PAINTER (new)->double_buffer = double_buffer;

	return HTML_PAINTER (new);
}
