/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

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

#include "htmlgdkpainter.h"


static HTMLPainterClass *parent_class = NULL;


/* FIXME: This allocates the pixel values in `color_set' directly.  This is
   broken.  Instead, we should be copying the RGB values and allocating the
   pixel values somewhere else.  The reason why don't do this yet in
   `HTMLColorSet' is that we do not want to have any allocation in HTMLColorSet
   to make it independent of the painter device in the future.  */
static void
allocate_color_set (HTMLGdkPainter *gdk_painter)
{
	GdkColormap *colormap;
	HTMLPainter *painter;

	painter = HTML_PAINTER (gdk_painter);
	colormap = gdk_window_get_colormap (gdk_painter->window);

	gdk_colormap_alloc_color (colormap, & painter->color_set->background_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->foreground_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->link_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->highlight_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, & painter->color_set->highlight_foreground_color, TRUE, TRUE);
}


/* GtkObject methods.  */

static void
finalize (GtkObject *object)
{
	HTMLGdkPainter *painter;

	painter = HTML_GDK_PAINTER (object);

	if (painter->gc != NULL)
		gdk_gc_destroy (painter->gc);

	html_gdk_font_manager_destroy (painter->font_manager);

	if (painter->pixmap != NULL)
		gdk_pixmap_unref (painter->pixmap);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
set_color_set (HTMLPainter *painter,
	       HTMLColorSet *color_set)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	(* HTML_PAINTER_CLASS (parent_class)->set_color_set) (painter, color_set);

	if (gdk_painter->window != NULL && color_set != NULL)
		allocate_color_set (gdk_painter);
}

static void
alloc_color (HTMLPainter *painter,
	     GdkColor *color)
{
	HTMLGdkPainter *gdk_painter;
	GdkColormap *colormap;

	gdk_painter = HTML_GDK_PAINTER (painter);
	g_return_if_fail (gdk_painter->window != NULL);

	colormap = gdk_window_get_colormap (gdk_painter->window);

	gdk_colormap_alloc_color (colormap, color, FALSE, TRUE);
}

static void
free_color (HTMLPainter *painter,
	    GdkColor *color)
{
	HTMLGdkPainter *gdk_painter;
	GdkColormap *colormap;

	gdk_painter = HTML_GDK_PAINTER (painter);

	g_return_if_fail (gdk_painter->window != NULL);
	g_return_if_fail (gdk_painter->gc != NULL);

	colormap = gdk_window_get_colormap (gdk_painter->window);
	gdk_colormap_free_colors (colormap, color, 1);
}


static void
begin (HTMLPainter *painter, int x1, int y1, int x2, int y2)
{
	HTMLGdkPainter *gdk_painter;
	GdkVisual *visual;

	gdk_painter = HTML_GDK_PAINTER (painter);
	visual = gdk_window_get_visual (gdk_painter->window);

	if (gdk_painter->double_buffer){
		const int width = x2 - x1 + 1;
		const int height = y2 - y1 + 1;

		g_assert (gdk_painter->pixmap == NULL);
		
		gdk_painter->pixmap = gdk_pixmap_new (gdk_painter->pixmap, width, height, visual->depth);
		gdk_painter->x1 = x1;
		gdk_painter->y1 = y1;
		gdk_painter->x2 = x2;
		gdk_painter->y2 = y2;

		if (gdk_painter->set_background){
			gdk_gc_set_background (gdk_painter->gc, &gdk_painter->background);
			gdk_painter->set_background = FALSE;
		}

		gdk_gc_set_foreground (gdk_painter->gc, &gdk_painter->background);
		gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc,
				    TRUE, 0, 0, width, height);
	} else {
		gdk_painter->pixmap = gdk_painter->window;
		gdk_painter->x1 = 0;
		gdk_painter->y1 = 0;
		gdk_painter->x2 = 0;
		gdk_painter->y2 = 0;
	}
}

static void
end (HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	
	if (! gdk_painter->double_buffer)
		return;

	gdk_draw_pixmap (gdk_painter->window, gdk_painter->gc,
			 gdk_painter->pixmap,
			 0, 0,
			 gdk_painter->x1, gdk_painter->y1,
			 gdk_painter->x2 - gdk_painter->x1,
			 gdk_painter->y2 - gdk_painter->y1);

	gdk_pixmap_unref (gdk_painter->pixmap);
	gdk_painter->pixmap = NULL;
}

static void
clear (HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (! gdk_painter->double_buffer){
		gdk_window_clear (gdk_painter->window);
	} else {
		if (gdk_painter->pixmap != NULL)
			gdk_window_clear (gdk_painter->pixmap);
		else
			gdk_painter->do_clear = TRUE;
	}
}


static void
set_clip_rectangle (HTMLPainter *painter,
		    gint x, gint y,
		    gint width, gint height)
{
	HTMLGdkPainter *gdk_painter;
	GdkRectangle rect;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (width == 0 || height == 0) {
		gdk_gc_set_clip_rectangle (gdk_painter->gc, NULL);
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;
	
	gdk_gc_set_clip_rectangle (gdk_painter->gc, &rect);
}

static void
set_background_color (HTMLPainter *painter,
		      const GdkColor *color)
{
	g_warning ("HTMLGdkPainter::set_background_color() needs to be implemented.");
}

static void
set_pen (HTMLPainter *painter,
	 const GdkColor *color)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	/* GdkColor API not const-safe!  */
	gdk_gc_set_foreground (gdk_painter->gc, (GdkColor *) color);
}

static const GdkColor *
get_black (const HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	return &gdk_painter->black;
}

static void
set_font_style (HTMLPainter *painter,
		HTMLFontStyle style)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	gdk_painter->font_style = style;
}

static HTMLFontStyle
get_font_style (HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	return gdk_painter->font_style;
}


/* HTMLPainter drawing functions.  */

static void
draw_line (HTMLPainter *painter,
	   gint x1, gint y1,
	   gint x2, gint y2)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	x1 -= gdk_painter->x1;
	y1 -= gdk_painter->y1;
	x2 -= gdk_painter->x1;
	y2 -= gdk_painter->y1;

	gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, x1, y1, x2, y2);
}

static void
draw_ellipse (HTMLPainter *painter,
	      gint x, gint y,
	      gint width, gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_arc (gdk_painter->pixmap, gdk_painter->gc, TRUE,
		      x - gdk_painter->x1, y - gdk_painter->y1,
		      width, height,
		      0, 360 * 64);
}

static void
draw_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc, FALSE,
			    x - gdk_painter->x1, y - gdk_painter->y1,
			    width, height);
}

static void
draw_panel (HTMLPainter *painter,
	    gint x, gint y,
	    gint width, gint height,
	    gboolean inset,
	    gint bordersize)
{
	HTMLGdkPainter *gdk_painter;
	GdkColor *col1, *col2;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (inset) {
		col1 = &gdk_painter->dark;
		col2 = &gdk_painter->light;
	}
	else {
		col1 = &gdk_painter->light;
		col2 = &gdk_painter->dark;
	}

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;
	
	while (bordersize > 0) {
		gdk_gc_set_foreground (gdk_painter->gc, col1);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x, y, x + width - 2, y);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x, y, x, y + height - 1);
		gdk_gc_set_foreground (gdk_painter->gc, col2);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x + width - 1, y, x + width - 1, y + height - 1);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x + 1, y + height - 1, x + width - 1, y + height - 1);
		bordersize--;
		x++;
		y++;
		width-=2;
		height-=2;
	}
}

static void
draw_background_pixmap (HTMLPainter *painter,
			gint x, gint y, 
			GdkPixbuf *pixbuf,
			gint pix_width, gint pix_height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_pixbuf_render_to_drawable (pixbuf, gdk_painter->pixmap,
				       gdk_painter->gc,
				       0, 0, x - gdk_painter->x1, y - gdk_painter->y1, 
				       pix_width ? pix_width : pixbuf->art_pixbuf->width,
				       pix_height ? pix_height : pixbuf->art_pixbuf->height,
				       GDK_RGB_DITHER_NORMAL,
				       x, y);
}

static GdkPixbuf *
create_temporary_pixbuf (GdkPixbuf *src,
			 gint clip_x, gint clip_y,
			 gint clip_width, gint clip_height)
{
	GdkPixbuf *pixbuf;
	gint src_width, src_height;
	gboolean has_alpha;
	guint n_channels;
	guint bits_per_sample;

	src_width = gdk_pixbuf_get_width (src);
	src_height = gdk_pixbuf_get_height (src);

	if (clip_width < 0)
		clip_width = src_width;
	if (clip_height < 0)
		clip_height = src_height;

	if (clip_x < 0 || clip_x >= src_width)
		return NULL;
	if (clip_y < 0 || clip_y >= src_height)
		return NULL;

	if (clip_width == 0 || clip_height == 0)
		return NULL;

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	n_channels = gdk_pixbuf_get_n_channels (src);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample (src);

	pixbuf = gdk_pixbuf_new (ART_PIX_RGB, has_alpha, bits_per_sample, clip_width, clip_height);

	return pixbuf;
}

static void
draw_pixmap (HTMLPainter *painter,
	     GdkPixbuf *pixbuf,
	     gint x, gint y,
	     gint scale_width, gint scale_height,
	     const GdkColor *color)
{
	HTMLGdkPainter *gdk_painter;
	GdkPixbuf *tmp_pixbuf;
	gint clip_x, clip_y;
	gint clip_width, clip_height;
	gint orig_width, orig_height;
	guint n_channels;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (x >= gdk_painter->x2 || x + scale_width < gdk_painter->x1
	    || y >= gdk_painter->y2 || y + scale_height < gdk_painter->y1)
		return;

	if (x >= gdk_painter->x1) {
		clip_x = 0;
		clip_width = scale_width;
		x -= gdk_painter->x1;
	} else {
		clip_x = gdk_painter->x1 - x;
		clip_width = scale_width - (gdk_painter->x1 - x);
		x = 0;
	}

	if (y >= gdk_painter->y1) {
		clip_y = 0;
		clip_height = scale_height;
		y -= gdk_painter->y1;
	} else {
		clip_y = gdk_painter->y1 - y;
		clip_height = scale_height - (gdk_painter->y1 - y);
		y = 0;
	}

	if (x + clip_width > gdk_painter->x2 - gdk_painter->x1)
		clip_width = gdk_painter->x2 - gdk_painter->x1 - x;

	if (y + clip_height > gdk_painter->y2 - gdk_painter->y1)
		clip_height = gdk_painter->y2 - gdk_painter->y1 - y;

	orig_width = gdk_pixbuf_get_width (pixbuf);
	orig_height = gdk_pixbuf_get_height (pixbuf);

	if (scale_width < 0)
		scale_width = orig_width;
	if (scale_height < 0)
		scale_height = orig_height;

	if (scale_width == orig_width && scale_height == orig_height && color == NULL) {
		gdk_pixbuf_render_to_drawable_alpha (pixbuf, gdk_painter->pixmap,
						     clip_x, clip_y,
						     x, y,
						     clip_width, clip_height,
						     GDK_PIXBUF_ALPHA_BILEVEL,
						     128,
						     GDK_RGB_DITHER_NORMAL,
						     0, 0);
		return;
	}

	tmp_pixbuf = create_temporary_pixbuf (pixbuf, clip_x, clip_y, clip_width, clip_height);
	if (tmp_pixbuf == NULL)
		return;

	gdk_pixbuf_scale (pixbuf, tmp_pixbuf,
			  0, 0,
			  clip_width, clip_height,
			  -clip_x, -clip_y,
			  (double) (scale_width + 1) / (double) orig_width,
			  (double) (scale_height + 1) / (double) orig_height,
			  ART_FILTER_BILINEAR);

	if (color != NULL) {
		guchar *p, *q;
		guint i, j;
		
		n_channels = gdk_pixbuf_get_n_channels (tmp_pixbuf);
		p = q = gdk_pixbuf_get_pixels (tmp_pixbuf);
		for (i = 0; i < clip_height; i++) {
			p = q;

			for (j = 0; j < clip_width; j++) {
				gint r, g, b, a;

				if (n_channels > 3)
					a = p[3];
				else
					a = 0xff;

				r = ((a * p[0] + color->red) >> 8) / 2 ;
				g = ((a * p[1] + color->green) >> 8) / 2 ;
				b = ((a * p[2] + color->blue) >> 8) / 2 ;

				p[0] = r;
				p[1] = g;
				p[2] = b;

				if (n_channels > 3)
					p[3] = 0xff;

				p += n_channels;
			}

			q += gdk_pixbuf_get_rowstride (tmp_pixbuf);
		}
	}

	gdk_pixbuf_render_to_drawable_alpha (tmp_pixbuf, gdk_painter->pixmap,
					     0, 0,
					     x, y,
					     clip_width, clip_height,
					     GDK_PIXBUF_ALPHA_BILEVEL,
					     128,
					     GDK_RGB_DITHER_NORMAL,
					     0, 0);

	gdk_pixbuf_unref (tmp_pixbuf);
}

static void
fill_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc,
			    TRUE,
			    x - gdk_painter->x1, y - gdk_painter->y1,
			    width, height);
}

static void
draw_text (HTMLPainter *painter,
	   gint x, gint y,
	   const gchar *text,
	   gint len)
{
	HTMLGdkPainter *gdk_painter;
	GdkFont *gdk_font;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (len == -1)
		len = strlen (text);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	gdk_font = html_gdk_font_manager_get_font (gdk_painter->font_manager,
						   gdk_painter->font_style);

	gdk_draw_text (gdk_painter->pixmap,
		       gdk_font,
		       gdk_painter->gc,
		       x, y,
		       text, len);

	if (gdk_painter->font_style & (HTML_FONT_STYLE_UNDERLINE
				   | HTML_FONT_STYLE_STRIKEOUT)) {
		guint width;

		width = gdk_text_width (gdk_font, text, len);

		if (gdk_painter->font_style & HTML_FONT_STYLE_UNDERLINE)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y + 1, 
				       x + width, y + 1);

		if (gdk_painter->font_style & HTML_FONT_STYLE_STRIKEOUT)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y - gdk_font->ascent / 2, 
				       x + width, y - gdk_font->ascent / 2);
	}

	gdk_font_unref (gdk_font);
}

static void
draw_shade_line (HTMLPainter *painter,
		 gint x, gint y,
		 gint width)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	
	x -= gdk_painter->x1;
	y -= gdk_painter->y1;
	
	gdk_gc_set_foreground (gdk_painter->gc, &gdk_painter->dark);
	gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, x, y, x+width, y);
	gdk_gc_set_foreground (gdk_painter->gc, &gdk_painter->light);
	gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, x, y + 1, x + width, y + 1);
}

static guint
calc_ascent (HTMLPainter *painter,
	     HTMLFontStyle style)
{
	HTMLGdkPainter *gdk_painter;
	GdkFont *gdk_font;
	gint ascent;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_font = html_gdk_font_manager_get_font (gdk_painter->font_manager, style);
	if (gdk_font == NULL)
		return 0;

	ascent = gdk_font->ascent;
	gdk_font_unref (gdk_font);

	return ascent;
}

static guint
calc_descent (HTMLPainter *painter,
	      HTMLFontStyle style)
{
	HTMLGdkPainter *gdk_painter;
	GdkFont *gdk_font;
	gint descent;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_font = html_gdk_font_manager_get_font (gdk_painter->font_manager, style);
	if (gdk_font == NULL)
		return 0;

	descent = gdk_font->descent;
	gdk_font_unref (gdk_font);

	return descent;
}

static guint
calc_text_width (HTMLPainter *painter,
		 const gchar *text,
		 guint len,
		 HTMLFontStyle style)
{
	HTMLGdkPainter *gdk_painter;
	GdkFont *gdk_font;
	gint width;

	gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_font = html_gdk_font_manager_get_font (gdk_painter->font_manager, style);
	width = gdk_text_width (gdk_font, text, len);
	gdk_font_unref (gdk_font);

	return width;
}

static guint
get_pixel_size (HTMLPainter *painter)
{
	return 1;
}


static void
init_color (GdkColor *color, gushort red, gushort green, gushort blue)
{
	color->pixel = 0;
	color->red = red;
	color->green = green;
	color->blue = blue;
}

static void
init (GtkObject *object)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (object);

	gdk_painter->window = NULL;

	gdk_painter->gc = NULL;

	gdk_painter->double_buffer = TRUE;
	gdk_painter->pixmap = NULL;
	gdk_painter->x1 = gdk_painter->y1 = 0;
	gdk_painter->x2 = gdk_painter->y2 = 0;
	gdk_painter->set_background = FALSE;
	gdk_painter->do_clear = FALSE;

	gdk_painter->font_manager = html_gdk_font_manager_new ();
	gdk_painter->font_style = HTML_FONT_STYLE_DEFAULT;

	init_color (& gdk_painter->background, 0xffff, 0xffff, 0xffff);
	init_color (& gdk_painter->dark, 0, 0, 0);
	init_color (& gdk_painter->light, 0, 0, 0);
}

static void
class_init (GtkObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;

	painter_class->begin = begin;
	painter_class->end = end;
	painter_class->alloc_color = alloc_color;
	painter_class->free_color = free_color;
	painter_class->set_font_style = set_font_style;
	painter_class->get_font_style = get_font_style;
	painter_class->calc_ascent = calc_ascent;
	painter_class->calc_descent = calc_descent;
	painter_class->calc_text_width = calc_text_width;
	painter_class->set_pen = set_pen;
	painter_class->get_black = get_black;
	painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_text = draw_text;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->draw_ellipse = draw_ellipse;
	painter_class->clear = clear;
	painter_class->set_background_color = set_background_color;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_panel = draw_panel;
	painter_class->set_clip_rectangle = set_clip_rectangle;
	painter_class->draw_background_pixmap = draw_background_pixmap;
	painter_class->set_color_set = set_color_set;
	painter_class->get_pixel_size = get_pixel_size;

	parent_class = gtk_type_class (html_painter_get_type ());
}

GtkType
html_gdk_painter_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"HTMLGdkPainter",
			sizeof (HTMLGdkPainter),
			sizeof (HTMLGdkPainterClass),
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (HTML_TYPE_PAINTER, &info);
	}

	return type;
}


HTMLPainter *
html_gdk_painter_new (gboolean double_buffer)
{
	HTMLGdkPainter *new;

	new = gtk_type_new (html_gdk_painter_get_type ());

	new->double_buffer = double_buffer;

	return HTML_PAINTER (new);
}

void
html_gdk_painter_realize (HTMLGdkPainter *gdk_painter,
			  GdkWindow *window)
{
	GdkColormap *colormap;

	g_return_if_fail (gdk_painter != NULL);
	g_return_if_fail (window != NULL);
	
	gdk_painter->gc = gdk_gc_new (window);
	gdk_painter->window = window;

	colormap = gdk_window_get_colormap (window);

	gdk_painter->light.red = 0xffff;
	gdk_painter->light.green = 0xffff;
	gdk_painter->light.blue = 0xffff;
	gdk_colormap_alloc_color (colormap, &gdk_painter->light, TRUE, TRUE);

	gdk_painter->dark.red = 0x7fff;
	gdk_painter->dark.green = 0x7fff;
	gdk_painter->dark.blue = 0x7fff;
	gdk_colormap_alloc_color (colormap, &gdk_painter->dark, TRUE, TRUE);

	gdk_painter->black.red = 0x0000;
	gdk_painter->black.green = 0x0000;
	gdk_painter->black.blue = 0x0000;
	gdk_colormap_alloc_color (colormap, &gdk_painter->black, TRUE, TRUE);

	if (HTML_PAINTER (gdk_painter)->color_set != NULL)
		allocate_color_set (gdk_painter);
}

void
html_gdk_painter_unrealize (HTMLGdkPainter *painter)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_GDK_PAINTER (painter));
	
	gdk_gc_unref (painter->gc);
	painter->gc = NULL;

	painter->window = NULL;
}
