/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

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

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "htmlpainter.h"
#include "htmlfont.h"
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

void
html_painter_draw_line (HTMLPainter *painter,
			gint x1, gint y1,
			gint x2, gint y2)
{
	x1 -= painter->x1;
	y1 -= painter->y1;
	x2 -= painter->x1;
	y2 -= painter->y1;
	gdk_draw_line (painter->pixmap, painter->gc, x1, y1, x2, y2);
}

void
html_painter_draw_ellipse (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	
	gdk_draw_arc (painter->pixmap, painter->gc, TRUE,
		      x - painter->x1, y - painter->y1, width, height, 0, 360 * 64);
}

void
html_painter_draw_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_rectangle (painter->pixmap, painter->gc, FALSE, x - painter->x1, y - painter->y1, width, height);
}

void
html_painter_draw_panel (HTMLPainter *painter, gint x, gint y, gint width, gint height, gboolean inset, gint bordersize)
{
	GdkColor *col1, *col2;

	if (inset) {
		col1 = &painter->dark;
		col2 = &painter->light;
	}
	else {
		col1 = &painter->light;
		col2 = &painter->dark;
	}

	x -= painter->x1;
	y -= painter->y1;
	
	while (bordersize > 0) {
		gdk_gc_set_foreground (painter->gc, col1);
		gdk_draw_line (painter->pixmap, painter->gc,
			       x, y, x + width - 2, y);
		gdk_draw_line (painter->pixmap, painter->gc,
			       x, y, x, y + height - 1);
		gdk_gc_set_foreground (painter->gc, col2);
		gdk_draw_line (painter->pixmap, painter->gc,
			       x + width - 1, y, x + width - 1, y + height - 1);
		gdk_draw_line (painter->pixmap, painter->gc,
			       x + 1, y + height - 1, x + width - 1, y + height - 1);
		bordersize--;
		x++;
		y++;
		width-=2;
		height-=2;
	}
}

void
html_painter_draw_background_pixmap (HTMLPainter *painter, gint x, gint y, 
				     GdkPixbuf *pixbuf, gint pix_width, gint pix_height)
{
	gdk_pixbuf_render_to_drawable (pixbuf, painter->pixmap,
				       painter->gc,
				       0, 0, x - painter->x1, y - painter->y1, 
				       pix_width ? pix_width : pixbuf->art_pixbuf->width,
				       pix_height ? pix_height : pixbuf->art_pixbuf->height,
				       GDK_RGB_DITHER_NORMAL,
				       x, y);
}

void
html_painter_draw_pixmap (HTMLPainter *painter, gint x, gint y, GdkPixbuf *pixbuf, gint clipx, gint clipy, gint clipwidth, gint clipheight)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (pixbuf != NULL);

	x -= painter->x1;
	y -= painter->y1;

	/*
	 * the idea here is good, but we want to avoid to use the GC
	 * stuff for clipping, to avoid sending to the X server all
	 * this information
	 */
	if (clipwidth && clipheight)
		html_painter_set_clip_rectangle (painter, clipx, clipy, clipwidth, clipheight);

	gdk_pixbuf_render_to_drawable_alpha (pixbuf, painter->pixmap,
					     0, 0,
					     x, y, /* dest x/y in pixmap*/
					     pixbuf->art_pixbuf->width,
					     pixbuf->art_pixbuf->height,
					     GDK_PIXBUF_ALPHA_BILEVEL,
					     128, GDK_RGB_DITHER_NORMAL,
					     x, y);
	if (clipwidth && clipheight)
		gdk_gc_set_clip_rectangle (painter->gc, NULL);
}

void
html_painter_fill_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	gdk_draw_rectangle (painter->pixmap, painter->gc, TRUE,
			    x - painter->x1, y - painter->y1, width, height);
}

void
html_painter_set_background_color (HTMLPainter *painter, GdkColor *color)
{
	g_return_if_fail (painter != NULL);

	return;

	if (color){
		if (!painter->pixmap) {
			painter->background = *color;
			painter->set_background = TRUE;
		}
	} 
}

void
html_painter_set_pen (HTMLPainter *painter, GdkColor *color)
{
	g_return_if_fail (painter != NULL);

	gdk_gc_set_foreground (painter->gc, color);
}

void
html_painter_draw_text (HTMLPainter *painter, gint x, gint y, gchar *text, gint len)
{
	g_return_if_fail (painter != NULL);
	
	if (len == -1)
		len = strlen (text);

	x -= painter->x1;
	y -= painter->y1;
	
	gdk_draw_text (painter->pixmap,
		       painter->font->gdk_font,
		       painter->gc, x, y, text, len);
	
	if (painter->font->underline) {
		gdk_draw_line (painter->pixmap, painter->gc, 
			       x, 
			       y + 1, 
			       x + html_font_calc_width (painter->font,
							 text, len),
			       y + 1);
	}

}

void
html_painter_draw_shade_line (HTMLPainter *p, gint x, gint y, gint width)
{
	g_return_if_fail (p != NULL);
	
	x -= p->x1;
	y -= p->y1;
	
	gdk_gc_set_foreground (p->gc, &p->dark);
	gdk_draw_line (p->pixmap, p->gc, x, y, x+width, y);
	gdk_gc_set_foreground (p->gc, &p->light);
	gdk_draw_line (p->pixmap, p->gc, x, y + 1, x+width, y + 1);
}

void
html_painter_set_font (HTMLPainter *p, HTMLFont *f)
{
	g_return_if_fail (p != NULL);

	if (p->font != NULL)
		html_font_destroy (p->font);

	p->font = html_font_dup (f); /* FIXME refcount would be better. */
}

HTMLPainter *
html_painter_new (void)
{
	HTMLPainter *painter;

	painter = g_new0 (HTMLPainter, 1);

	if (getenv ("DISABLE_DB") != NULL)
		painter->double_buffer = FALSE;
	else
		painter->double_buffer = TRUE;
	
	return painter;
}

void
html_painter_destroy (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);

	g_free (painter);
}

void
html_painter_begin (HTMLPainter *painter, int x1, int y1, int x2, int y2)
{
	GdkVisual *visual;

	g_return_if_fail (painter != NULL);

	visual = gdk_window_get_visual (painter->window);
	if (painter->double_buffer){
		const int width = x2 - x1 + 1;
		const int height = y2 - y1 + 1;

		g_assert (painter->pixmap == NULL);
		
		painter->pixmap = gdk_pixmap_new (painter->pixmap, width, height, visual->depth);
		painter->x1 = x1;
		painter->y1 = y1;
		painter->x2 = x2;
		painter->y2 = y2;

		if (painter->set_background){
			gdk_gc_set_background (painter->gc, &painter->background);
			painter->set_background = FALSE;
		}

		gdk_gc_set_foreground (painter->gc, &painter->background);
		gdk_draw_rectangle (
			painter->pixmap, painter->gc, TRUE,
			0, 0, width, height);
	} else {
		painter->pixmap = painter->window;
		painter->x1 = 0;
		painter->y1 = 0;
		painter->x2 = 0;
		painter->y2 = 0;
	}
}

void
html_painter_end (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	
	if (painter->double_buffer){
		gdk_draw_pixmap (
			painter->window, painter->gc, painter->pixmap,
			0, 0,
			painter->x1, painter->y1,
			painter->x2 - painter->x1,
			painter->y2 - painter->y1);
		gdk_pixmap_unref (painter->pixmap);
	}
	painter->pixmap = NULL;
}

void 
html_painter_realize (HTMLPainter *painter, GdkWindow *window)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (window != NULL);
	
	/* Create our painter dc */
	painter->gc = gdk_gc_new (window);

	/* Set our painter window */
	painter->window = window;

	/* Allocate dark/light colors */
	painter->light.red = 49151;
	painter->light.green = 49151;
	painter->light.blue = 49151;
	gdk_colormap_alloc_color (gdk_window_get_colormap (window), &painter->light, TRUE, TRUE);

	painter->dark.red = 32767;
	painter->dark.green = 32767;
	painter->dark.blue = 32767;
	gdk_colormap_alloc_color (gdk_window_get_colormap (window), &painter->dark, TRUE, TRUE);

	painter->black.red = 0;
	painter->black.green = 0;
	painter->black.blue = 0;
	gdk_colormap_alloc_color (gdk_window_get_colormap (window), &painter->black, TRUE, TRUE);
}

void
html_painter_unrealize (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	
	gdk_gc_unref (painter->gc);
	painter->gc = NULL;
}

void
html_painter_clear (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);

	if (painter->double_buffer){
		if (painter->pixmap)
			gdk_window_clear (painter->pixmap);
		else
			painter->do_clear = TRUE;
	} else
		gdk_window_clear (painter->window);
}

GdkWindow *
html_painter_get_window (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);

	return painter->window;
}

HTMLFont *
html_painter_get_font (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);

	return painter->font;
}

void
html_painter_set_clip_rectangle (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	GdkRectangle rect;

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;
	
	gdk_gc_set_clip_rectangle (painter->gc, (width && height) ? &rect : NULL);
}

/* This will draw a cursor for the specified position, with the specified
   ascent/descent.  */
void
html_painter_draw_cursor (HTMLPainter *painter, gint x, gint y, gint ascent, gint descent)
{
	html_painter_set_pen (painter, &painter->black);
	html_painter_draw_line (painter, x, y - ascent, x, y + descent - 1);
}
