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
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "htmlpainter.h"
#include "htmlfont.h"

struct _HTMLPainter {
	GdkWindow *window; /* GdkWindow to draw on */

	/*
	 * For the double-buffering system
	 */
	gboolean   double_buffer;
	GdkPixmap *pixmap;
	int        x1, y1, x2, y2;
	GdkColor   background;
	gboolean   set_background;
	gboolean  do_clear;
	
	GdkGC *gc;      /* The current GC used */
	HTMLFont *font; /* The current font */
};

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
html_painter_draw_pixmap (HTMLPainter *painter, gint x, gint y, GdkPixbuf *pixbuf)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (pixbuf != NULL);
	
	if (pixbuf->art_pixbuf->has_alpha) {
		gdk_draw_rgb_32_image (painter->pixmap,
				       painter->gc,
				       x - painter->x1,
				       y - painter->y1,
				       pixbuf->art_pixbuf->width,
				       pixbuf->art_pixbuf->height,
				       GDK_RGB_DITHER_NORMAL,
				       pixbuf->art_pixbuf->pixels,
				       pixbuf->art_pixbuf->rowstride);
	}
	else {
		gdk_draw_rgb_image (painter->pixmap,
				    painter->gc,
				    x, y,
				    pixbuf->art_pixbuf->width,
				    pixbuf->art_pixbuf->height,
				    GDK_RGB_DITHER_NORMAL,
				    pixbuf->art_pixbuf->pixels,
				    pixbuf->art_pixbuf->rowstride);
	}
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

	if (color){
		printf ("COLOR: %d %d %d\n", color->red, color->green, color->blue);
		if (painter->pixmap)
			gdk_window_set_background (painter->pixmap, color);
		else {
			painter->background = *color;
			painter->set_background = TRUE;
		}
	} else
		printf ("COLOR: none\n");
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

	gdk_draw_text (painter->pixmap, painter->font->gdk_font, painter->gc, x, y, text, len);
	
	if (painter->font->underline) {
		gdk_draw_line (painter->pixmap, painter->gc, 
			       x - painter->x1, 
			       y + 1 - painter->y1, 
			       x + html_font_calc_width (painter->font,
							 text, len) - painter->x1,
			       y + 1 - painter->y1);
	}

}

void
html_painter_draw_shade_line (HTMLPainter *p, gint x, gint y, gint width)
{
	static GdkColor* dark = NULL;
	static GdkColor* light = NULL;

	g_return_if_fail (p != NULL);
	
	if (!dark) {
		dark = g_new0 (GdkColor, 1);
		dark->red = 32767;
		dark->green = 32767;
		dark->blue = 32767;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->pixmap), dark, TRUE, TRUE);
	}
	
	if (!light) {
		light = g_new0 (GdkColor, 1);
		light->red = 49151;
		light->green = 49151;
		light->blue = 49151;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->pixmap), light, TRUE, TRUE);
	}
	x -= p->x1;
	y -= p->y1;
	
	gdk_gc_set_foreground (p->gc, dark);
	gdk_draw_line (p->pixmap, p->gc, x, y, x+width, y);
	gdk_gc_set_foreground (p->gc, light);
	gdk_draw_line (p->pixmap, p->gc, x, y + 1, x+width, y + 1);
}

void
html_painter_set_font (HTMLPainter *p, HTMLFont *f)
{
	g_return_if_fail (p != NULL);
	
	p->font = f;
}

HTMLPainter *
html_painter_new (void)
{
	HTMLPainter *painter;

	painter = g_new0 (HTMLPainter, 1);

	if (getenv ("TESTDB") != NULL){
		painter->double_buffer = TRUE;
	}
	
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

		printf ("PAINTING: %d %d %d %d\n", x1, y1, x2, y2);
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
			x1, y1, width, height);
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
}

void
html_painter_unrealize (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	
	gdk_gc_unref (painter->gc);
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
