/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
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


#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include "htmlpainter.h"


/* FIXME: This allocates the pixel values in `color_set' directly.  This is
   broken.  Instead, we should be copying the RGB values and allocating the
   pixel values somewhere else.  The reason why don't do this yet in
   `HTMLColorSet' is that we do not want to have any allocation in HTMLColorSet
   to make it independent of the painter device in the future.  */
static void
allocate_color_set (HTMLPainter *painter)
{
	GdkColormap *colormap;

	colormap = gdk_window_get_colormap (painter->window);

	gdk_colormap_alloc_color (colormap, & painter->color_set->background_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->foreground_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->link_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->highlight_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->highlight_foreground_color, TRUE, TRUE);
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

	painter->font_manager = html_font_manager_new ();

	painter->color_set = NULL;
	
	return painter;
}

void
html_painter_destroy (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);

	html_font_manager_destroy (painter->font_manager);
	g_free (painter);
}

void 
html_painter_realize (HTMLPainter *painter, GdkWindow *window)
{
	GdkColormap *colormap;

	g_return_if_fail (painter != NULL);
	g_return_if_fail (window != NULL);
	
	/* Create our painter dc */
	painter->gc = gdk_gc_new (window);

	/* Set our painter window */
	painter->window = window;

	colormap = gdk_window_get_colormap (window);

	/* Allocate dark/light colors */
	painter->light.red = 0xffff;
	painter->light.green = 0xffff;
	painter->light.blue = 0xffff;
	gdk_colormap_alloc_color (colormap, &painter->light, TRUE, TRUE);

	painter->dark.red = 0x7fff;
	painter->dark.green = 0x7fff;
	painter->dark.blue = 0x7fff;
	gdk_colormap_alloc_color (colormap, &painter->dark, TRUE, TRUE);

	painter->black.red = 0x0000;
	painter->black.green = 0x0000;
	painter->black.blue = 0x0000;
	gdk_colormap_alloc_color (colormap, &painter->black, TRUE, TRUE);

	if (painter->color_set != NULL)
		allocate_color_set (painter);
}

void
html_painter_unrealize (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	
	gdk_gc_unref (painter->gc);
	painter->gc = NULL;
}


void
html_painter_set_color_set (HTMLPainter *painter,
			    HTMLColorSet *color_set)
{
	g_return_if_fail (painter != NULL);

	painter->color_set = color_set;

	if (painter->window != NULL && color_set != NULL)
		allocate_color_set (painter);
}


void
html_painter_alloc_color (HTMLPainter *painter,
			  GdkColor *color)
{
	GdkWindow *window;
	GdkColormap *colormap;

	g_return_if_fail (painter != NULL);
	g_return_if_fail (color != NULL);
	g_return_if_fail (painter->window != NULL);

	window = html_painter_get_window (painter);
	colormap = gdk_window_get_colormap (window);

	gdk_colormap_alloc_color (colormap, color, FALSE, TRUE);
}

void
html_painter_free_color (HTMLPainter *painter,
			 GdkColor *color)
{
	GdkWindow *window;
	GdkColormap *colormap;

	g_return_if_fail (painter != NULL);
	g_return_if_fail (color != NULL);
	g_return_if_fail (painter->window != NULL);
	g_return_if_fail (painter->gc != NULL);

	window = html_painter_get_window (painter);
	colormap = gdk_window_get_colormap (window);

	gdk_colormap_free_colors (colormap, color, 1);
}

const GdkColor *
html_painter_get_black (const HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);

	return &painter->black;
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
html_painter_set_pen (HTMLPainter *painter, const GdkColor *color)
{
	g_return_if_fail (painter != NULL);

	/* GdkColor API not const-safe!  */
	gdk_gc_set_foreground (painter->gc, (GdkColor *) color);
}

void
html_painter_set_font_style (HTMLPainter *p,
			     HTMLFontStyle style)
{
	g_return_if_fail (p != NULL);
	g_return_if_fail (style != HTML_FONT_STYLE_DEFAULT);

	p->font_style = style;
}


HTMLFontStyle
html_painter_get_font_style (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, HTML_FONT_STYLE_DEFAULT);

	return painter->font_style;
}


/* Drawing functions.  */

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
html_painter_draw_ellipse (HTMLPainter *painter,
			   gint x, gint y,
			   gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	
	gdk_draw_arc (painter->pixmap, painter->gc, TRUE,
		      x - painter->x1, y - painter->y1,
		      width, height,
		      0, 360 * 64);
}

void
html_painter_draw_rect (HTMLPainter *painter,
			gint x, gint y,
			gint width, gint height)
{
	gdk_draw_rectangle (painter->pixmap, painter->gc, FALSE,
			    x - painter->x1, y - painter->y1,
			    width, height);
}

void
html_painter_draw_panel (HTMLPainter *painter,
			 gint x, gint y,
			 gint width, gint height,
			 gboolean inset,
			 gint bordersize)
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
html_painter_draw_pixmap (HTMLPainter *painter,
			  gint x, gint y,
			  GdkPixbuf *pixbuf,
			  gint clipx, gint clipy,
			  gint clipwidth, gint clipheight)
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
html_painter_fill_rect (HTMLPainter *painter,
			gint x, gint y,
			gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	gdk_draw_rectangle (painter->pixmap, painter->gc, TRUE,
			    x - painter->x1, y - painter->y1, width, height);
}

void
html_painter_draw_text (HTMLPainter *painter,
			gint x, gint y,
			const gchar *text,
			gint len)
{
	GdkFont *gdk_font;

	g_return_if_fail (painter != NULL);
	
	if (len == -1)
		len = strlen (text);

	x -= painter->x1;
	y -= painter->y1;

	gdk_font = html_font_manager_get_gdk_font (painter->font_manager,
						   painter->font_style);

	gdk_draw_text (painter->pixmap,
		       gdk_font,
		       painter->gc,
		       x, y,
		       text, len);

	if (painter->font_style & (HTML_FONT_STYLE_UNDERLINE
				   | HTML_FONT_STYLE_STRIKEOUT)) {
		guint width;

		width = gdk_text_width (gdk_font, text, len);

		if (painter->font_style & HTML_FONT_STYLE_UNDERLINE)
			gdk_draw_line (painter->pixmap, painter->gc, 
				       x, y + 1, 
				       x + width, y + 1);

		if (painter->font_style & HTML_FONT_STYLE_STRIKEOUT)
			gdk_draw_line (painter->pixmap, painter->gc, 
				       x, y - gdk_font->ascent / 2, 
				       x + width, y - gdk_font->ascent / 2);
	}

	gdk_font_unref (gdk_font);
}

void
html_painter_draw_shade_line (HTMLPainter *p,
			      gint x, gint y,
			      gint width)
{
	g_return_if_fail (p != NULL);
	
	x -= p->x1;
	y -= p->y1;
	
	gdk_gc_set_foreground (p->gc, &p->dark);
	gdk_draw_line (p->pixmap, p->gc, x, y, x+width, y);
	gdk_gc_set_foreground (p->gc, &p->light);
	gdk_draw_line (p->pixmap, p->gc, x, y + 1, x+width, y + 1);
}


GdkWindow *
html_painter_get_window (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);

	return painter->window;
}


guint
html_painter_calc_ascent (HTMLPainter *painter,
			  HTMLFontStyle style)
{
	GdkFont *gdk_font;
	gint ascent;

	g_return_val_if_fail (painter != NULL, 0);

	gdk_font = html_font_manager_get_gdk_font (painter->font_manager, style);
	if (gdk_font == NULL)
		return 0;

	ascent = gdk_font->ascent;
	gdk_font_unref (gdk_font);

	return ascent;
}

guint
html_painter_calc_descent (HTMLPainter *painter,
			   HTMLFontStyle style)
{
	GdkFont *gdk_font;
	gint descent;

	g_return_val_if_fail (painter != NULL, 0);

	gdk_font = html_font_manager_get_gdk_font (painter->font_manager, style);
	if (gdk_font == NULL)
		return 0;

	descent = gdk_font->descent;
	gdk_font_unref (gdk_font);

	return descent;
}

guint
html_painter_calc_text_width (HTMLPainter *painter,
			      const gchar *text,
			      guint len,
			      HTMLFontStyle style)
{
	GdkFont *gdk_font;
	gint width;

	g_return_val_if_fail (painter != NULL, 0);
	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (style != HTML_FONT_STYLE_DEFAULT, 0);

	gdk_font = html_font_manager_get_gdk_font (painter->font_manager, style);
	width = gdk_text_width (gdk_font, text, len);
	gdk_font_unref (gdk_font);

	return width;
}


const GdkColor *
html_painter_get_default_background_color (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);
	g_return_val_if_fail (painter->window != NULL, NULL);
	g_return_val_if_fail (painter->color_set != NULL, NULL);

	return &painter->color_set->background_color;
}

const GdkColor *
html_painter_get_default_foreground_color (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);
	g_return_val_if_fail (painter->window != NULL, NULL);
	g_return_val_if_fail (painter->color_set != NULL, NULL);

	return &painter->color_set->foreground_color;
}

const GdkColor *
html_painter_get_default_link_color (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);
	g_return_val_if_fail (painter->window != NULL, NULL);
	g_return_val_if_fail (painter->color_set != NULL, NULL);

	return &painter->color_set->link_color;
}

const GdkColor *
html_painter_get_default_highlight_color (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);
	g_return_val_if_fail (painter->window != NULL, NULL);
	g_return_val_if_fail (painter->color_set != NULL, NULL);

	return &painter->color_set->highlight_color;
}

const GdkColor *
html_painter_get_default_highlight_foreground_color (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, NULL);
	g_return_val_if_fail (painter->window != NULL, NULL);
	g_return_val_if_fail (painter->color_set != NULL, NULL);

	return &painter->color_set->highlight_foreground_color;
}
