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

void
html_painter_draw_ellipse (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_arc (painter->window, painter->gc, TRUE,
		      x, y, width, height, 0, 360 * 64);
}

void
html_painter_draw_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_rectangle (painter->window, painter->gc, FALSE, x, y, width, height);
}

void
html_painter_draw_pixmap (HTMLPainter *painter, gint x, gint y, GdkPixbuf *pixbuf)
{
	if (pixbuf->art_pixbuf->has_alpha) {
		gdk_draw_rgb_32_image (painter->window,
				       painter->gc,
				       x, y,
				       pixbuf->art_pixbuf->width,
				       pixbuf->art_pixbuf->height,
				       GDK_RGB_DITHER_NORMAL,
				       pixbuf->art_pixbuf->pixels,
				       pixbuf->art_pixbuf->rowstride);
	}
	else {
		gdk_draw_rgb_image (painter->window,
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
	gdk_draw_rectangle (painter->window, painter->gc, TRUE, x, y, width, height);
}

void
html_painter_set_background_color (HTMLPainter *painter, GdkColor *color)
{
	if (color)
		gdk_window_set_background (painter->window, color);
}

void
html_painter_set_pen (HTMLPainter *painter, GdkColor *color)
{
	gdk_gc_set_foreground (painter->gc, color);
}

void
html_painter_draw_text (HTMLPainter *painter, gint x, gint y, gchar *text, gint len)
{
	if (len == -1)
		len = strlen (text);

	gdk_draw_text (painter->window, painter->font->gdk_font, painter->gc, x, y, text, len);
	
	if (painter->font->underline) {
		gdk_draw_line (painter->window, painter->gc, 
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
	static GdkColor* dark = NULL;
	static GdkColor* light = NULL;

	if (!dark) {
		dark = g_new0 (GdkColor, 1);
		dark->red = 32767;
		dark->green = 32767;
		dark->blue = 32767;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->window), dark, TRUE, TRUE);
	}
	
	if (!light) {
		light = g_new0 (GdkColor, 1);
		light->red = 49151;
		light->green = 49151;
		light->blue = 49151;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->window), light, TRUE, TRUE);
	}
	gdk_gc_set_foreground (p->gc, dark);
	gdk_draw_line (p->window, p->gc, x, y, x+width, y);
	gdk_gc_set_foreground (p->gc, light);
	gdk_draw_line (p->window, p->gc, x, y + 1, x+width, y + 1);
}

void
html_painter_set_font (HTMLPainter *p, HTMLFont *f)
{
	p->font = f;
}

HTMLPainter *
html_painter_new (void)
{
	HTMLPainter *painter;

	painter = g_new0 (HTMLPainter, 1);

	return painter;
}
