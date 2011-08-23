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
#include "gtkhtml-compat.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <glib/gi18n-lib.h>

#include "htmlentity.h"
#include "htmlgdkpainter.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlembedded.h"
#include "htmlengine.h"
#include "htmltextslave.h"
#include "htmlsettings.h"
#include "gtkhtml-embedded.h"
#include "gtkhtml.h"

static HTMLPainterClass *parent_class = NULL;

static void set_clip_rectangle (HTMLPainter *painter, gint x, gint y, gint width, gint height);

/* GObject methods.  */

static void
finalize (GObject *object)
{
	HTMLGdkPainter *painter;

	painter = HTML_GDK_PAINTER (object);

	if (painter->cr != NULL) {
		cairo_destroy (painter->cr);
		painter->cr = NULL;
	}

	if (painter->surface != NULL) {
		cairo_surface_destroy (painter->surface);
		painter->surface = NULL;
	}

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
alloc_color (HTMLPainter *painter,
             GdkColor *color)
{
}

static void
free_color (HTMLPainter *painter,
            GdkColor *color)
{
}

static void
_cairo_draw_line (cairo_t *cr,
                  gint x1,
                  gint y1,
                  gint x2,
                  gint y2)
{
	cairo_save (cr);

	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_width (cr, 1);

	cairo_move_to (cr, x1 + 0.5, y1 + 0.5);
	cairo_line_to (cr, x2 + 0.5, y2 + 0.5);
	cairo_stroke (cr);

	cairo_restore (cr);
}

static void
_cairo_draw_rectangle (cairo_t *cr,
                       gboolean filled,
                       gint x,
                       gint y,
                       gint width,
                       gint height)
{
  if (filled)
    {
      cairo_rectangle (cr, x, y, width, height);
      cairo_fill (cr);
    }
  else
    {
      cairo_rectangle (cr, x + 0.5, y + 0.5, width, height);
      cairo_stroke (cr);
    }
}

static void
_cairo_draw_ellipse (cairo_t *cr,
                     gboolean filled,
                     gint x,
                     gint y,
                     gint width,
                     gint height)
{
	cairo_save (cr);

	cairo_translate (cr, x + width / 2.0, y + height / 2.0);
	cairo_scale (cr, width / 2.0, height / 2.0);
	cairo_arc (cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);

	if (filled) {
		cairo_fill (cr);
	} else {
		cairo_stroke (cr);
	}

	cairo_restore (cr);
}

static void
_cairo_draw_glyphs (cairo_t *cr,
                    PangoFont *font,
                    gint x,
                    gint y,
                    PangoGlyphString *glyphs)
{
	cairo_save (cr);
	cairo_move_to (cr, x, y);
	pango_cairo_show_glyph_string (cr, font, glyphs);
	cairo_restore (cr);
}


static void
begin (HTMLPainter *painter,
       gint x1,
       gint y1,
       gint x2,
       gint y2)
{
	HTMLGdkPainter *gdk_painter;

	/* printf ("painter begin %d,%d %d,%d\n", x1, y1, x2, y2); */

	gdk_painter = HTML_GDK_PAINTER (painter);
	g_return_if_fail (gdk_painter->window != NULL);

	/* FIXME: Ideally it should be NULL before coming here. */
	if (gdk_painter->cr)
		cairo_destroy (gdk_painter->cr);
	if (gdk_painter->surface)
		cairo_surface_destroy (gdk_painter->surface);

	if (gdk_painter->double_buffer) {
		const gint width = x2 - x1 + 1;
		const gint height = y2 - y1 + 1;

		gdk_painter->surface = gdk_window_create_similar_surface (gdk_painter->window,
									  CAIRO_CONTENT_COLOR,
									  MAX (width, 1),
									  MAX (height, 1));
		gdk_painter->x1 = x1;
		gdk_painter->y1 = y1;
		gdk_painter->x2 = x2;
		gdk_painter->y2 = y2;

		if (gdk_painter->set_background) {
			gdk_painter->set_background = FALSE;
		}

		gdk_painter->cr = cairo_create (gdk_painter->surface);
		gdk_cairo_set_source_color (gdk_painter->cr, &gdk_painter->background);
		_cairo_draw_rectangle (gdk_painter->cr,
				       TRUE, 0, 0, width, height);
	} else {
		gdk_painter->cr = gdk_cairo_create (gdk_painter->window);
		gdk_painter->surface = NULL;
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
	cairo_t *cr;

	/* printf ("painter end\n"); */

	gdk_painter = HTML_GDK_PAINTER (painter);

	cairo_destroy (gdk_painter->cr);
	gdk_painter->cr = NULL;

	if (!gdk_painter->double_buffer)
		return;

	cr = gdk_cairo_create (gdk_painter->window);
	cairo_set_source_surface (cr, gdk_painter->surface,
				  gdk_painter->x1,
				  gdk_painter->y1);
	cairo_rectangle (cr,
			 gdk_painter->x1,
			 gdk_painter->y1,
			 gdk_painter->x2 - gdk_painter->x1,
			 gdk_painter->y2 - gdk_painter->y1);
	cairo_fill (cr);
	cairo_destroy (cr);

	cairo_surface_destroy (gdk_painter->surface);
	gdk_painter->surface = NULL;
}

static void
clear (HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (!gdk_painter->double_buffer) {
		gdk_cairo_set_source_color (gdk_painter->cr, &gdk_painter->background);
		cairo_paint (gdk_painter->cr);
	} else {
		if (gdk_painter->surface != NULL) {
			gdk_cairo_set_source_color (gdk_painter->cr, &gdk_painter->background);
			cairo_paint (gdk_painter->cr);
		} else {
			gdk_painter->do_clear = TRUE;
		}
	}
}


static void
set_clip_rectangle (HTMLPainter *painter,
                    gint x,
                    gint y,
                    gint width,
                    gint height)
{
	HTMLGdkPainter *gdk_painter;
	GdkRectangle rect;

	gdk_painter = HTML_GDK_PAINTER (painter);

	cairo_reset_clip (gdk_painter->cr);

	if (width == 0 || height == 0)
		return;

	rect.x = CLAMP (x - gdk_painter->x1, 0, gdk_painter->x2 - gdk_painter->x1);
	rect.y = CLAMP (y - gdk_painter->y1, 0, gdk_painter->y2 - gdk_painter->y1);
	rect.width = CLAMP (width, 0, gdk_painter->x2 - gdk_painter->x1 - rect.x);
	rect.height = CLAMP (height, 0, gdk_painter->y2 - gdk_painter->y1 - rect.y);

	gdk_cairo_rectangle (gdk_painter->cr, &rect);
	cairo_clip (gdk_painter->cr);
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
	gdk_cairo_set_source_color (gdk_painter->cr, (GdkColor *) color);
}

static const GdkColor *
get_black (const HTMLPainter *painter)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);
	return &gdk_painter->black;
}


/* HTMLPainter drawing functions.  */

static void
draw_line (HTMLPainter *painter,
           gint x1,
           gint y1,
           gint x2,
           gint y2)
 {
        HTMLGdkPainter *gdk_painter;

        gdk_painter = HTML_GDK_PAINTER (painter);

        x1 -= gdk_painter->x1;
        y1 -= gdk_painter->y1;
        x2 -= gdk_painter->x1;
        y2 -= gdk_painter->y1;

        _cairo_draw_line (gdk_painter->cr,
           x1,
           y1,
           x2,
           y2);
}

static void
draw_ellipse (HTMLPainter *painter,
              gint x,
              gint y,
              gint width,
              gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	_cairo_draw_ellipse (gdk_painter->cr, TRUE,
			     x, y, width, height);
}

static void
draw_rect (HTMLPainter *painter,
           gint x,
           gint y,
           gint width,
           gint height)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	_cairo_draw_rectangle (gdk_painter->cr, FALSE,
			       x - gdk_painter->x1, y - gdk_painter->y1,
			       width, height);
}

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
	HTMLGdkPainter *gdk_painter;
	GdkColor *col1 = NULL, *col2 = NULL;
	GdkColor dark, light;

	#define INC 0x8000
	#define DARK(c)  dark.c = MAX (((gint) bg->c) - INC, 0)
	#define LIGHT(c) light.c = MIN (((gint) bg->c) + INC, 0xffff)

	DARK (red);
	DARK (green);
	DARK (blue);
	LIGHT (red);
	LIGHT (green);
	LIGHT (blue);

	gdk_painter = HTML_GDK_PAINTER (painter);

	switch (style) {
	case HTML_BORDER_SOLID:
		col1 = bg;
		col2 = bg;
		break;
	case HTML_BORDER_OUTSET:
		col1 = &light;
		col2 = &dark;
		break;
	default:
	case HTML_BORDER_INSET:
		col1 = &dark;
		col2 = &light;
		break;
	}

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	cairo_save (gdk_painter->cr);

	while (bordersize > 0) {
		if (col2) {
			gdk_cairo_set_source_color (gdk_painter->cr, col2);
		}

		_cairo_draw_line (gdk_painter->cr,
				  x + width - 1, y, x + width - 1, y + height - 1);
		_cairo_draw_line (gdk_painter->cr,
				  x + 1, y + height - 1, x + width - 1, y + height - 1);
		if (col1) {
			gdk_cairo_set_source_color (gdk_painter->cr, col1);
		}

		_cairo_draw_line (gdk_painter->cr,
				  x, y, x + width - 2, y);
		_cairo_draw_line (gdk_painter->cr,
				  x, y, x, y + height - 1);
		bordersize--;
		x++;
		y++;
		width-=2;
		height-=2;
	}

	cairo_restore (gdk_painter->cr);

	free_color (painter, &dark);
	free_color (painter, &light);
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
	gint pw;
	gint ph;
	gint tile_width, tile_height;
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

	tile_x += paint.x - expose.x;
	tile_y += paint.y - expose.y;

	if (!color && !pixbuf)
		return;

	if (color && !pixbuf) {
		gdk_cairo_set_source_color (gdk_painter->cr, color);
		_cairo_draw_rectangle (gdk_painter->cr,
				       TRUE, paint.x - clip.x, paint.y - clip.y,
				       paint.width, paint.height);

	}

	if (!pixbuf)
		return;

	pw = gdk_pixbuf_get_width (pixbuf);
	ph = gdk_pixbuf_get_height (pixbuf);

	/* optimize out some special cases */
	if (pw == 1 && ph == 1) {
		GdkColor pixcol;
		guchar *p;

		p = gdk_pixbuf_get_pixels (pixbuf);

		if (!(gdk_pixbuf_get_has_alpha (pixbuf) && (p[3] < 0x80))) {
			pixcol.red = p[0] * 0xff;
			pixcol.green = p[1] * 0xff;
			pixcol.blue = p[2] * 0xff;

			color = &pixcol;
		}

		if (color) {
			gdk_cairo_set_source_color (gdk_painter->cr, color);
			_cairo_draw_rectangle (gdk_painter->cr,
					       TRUE, paint.x - clip.x, paint.y - clip.y,
					       paint.width, paint.height);
		}

		return;
	}

	tile_width = (tile_x % pw) + paint.width;
	tile_height = (tile_y % ph) + paint.height;

	/* do tiling */
	if (tile_width > pw || tile_height > ph) {
		gint dw, dh;

		dw = MIN (pw, tile_width);
		dh = MIN (ph, tile_height);

		if (color) {
			gdk_cairo_set_source_color (gdk_painter->cr, color);
			_cairo_draw_rectangle (gdk_painter->cr,
					       TRUE, 0, 0,
					       dw, dh);
		}

		gdk_cairo_set_source_pixbuf (gdk_painter->cr, pixbuf,
					     paint.x - (tile_x % pw) - clip.x,
					     paint.y - (tile_y % ph) - clip.y);
		cairo_pattern_set_extend (cairo_get_source (gdk_painter->cr), CAIRO_EXTEND_REPEAT);

		cairo_rectangle (gdk_painter->cr, paint.x - clip.x, paint.y - clip.y,
				 paint.width, paint.height);
		cairo_fill (gdk_painter->cr);
	} else {
		if (color && gdk_pixbuf_get_has_alpha (pixbuf)) {
			gdk_cairo_set_source_color (gdk_painter->cr, color);
			_cairo_draw_rectangle (gdk_painter->cr,
					       TRUE,
					       paint.x - clip.x, paint.y - clip.y,
					       paint.width, paint.height);
		}

		gdk_cairo_set_source_pixbuf (gdk_painter->cr,
					     pixbuf,
					     tile_x % pw, tile_y % ph);
		cairo_rectangle (gdk_painter->cr,
				 paint.x - clip.x, paint.y - clip.y,
				 paint.width, paint.height);
		cairo_fill (gdk_painter->cr);
	}
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
	GdkRectangle clip, image, paint;
	HTMLGdkPainter *gdk_painter;
	GdkPixbuf *tmp_pixbuf;
	guint n_channels;
	gint orig_width;
	gint orig_height;
	gint bilinear;

	gdk_painter = HTML_GDK_PAINTER (painter);

	orig_width = gdk_pixbuf_get_width (pixbuf);
	orig_height = gdk_pixbuf_get_height (pixbuf);

	if (scale_width < 0)
		scale_width = orig_width;
	if (scale_height < 0)
		scale_height = orig_height;

	image.x = x;
	image.y = y;
	image.width  = scale_width;
	image.height = scale_height;

	clip.x = gdk_painter->x1;
	clip.width = gdk_painter->x2 - gdk_painter->x1;
	clip.y = gdk_painter->y1;
	clip.height = gdk_painter->y2 - gdk_painter->y1;

	if (!gdk_rectangle_intersect (&clip, &image, &paint))
	    return;

	if (scale_width == orig_width && scale_height == orig_height && color == NULL) {
		gdk_cairo_set_source_pixbuf (gdk_painter->cr, pixbuf,
					     image.x - clip.x,
					     image.y - clip.y);
		cairo_rectangle (gdk_painter->cr,
				 image.x - clip.x, image.y - clip.y,
				 image.width, image.height);
		cairo_fill (gdk_painter->cr);
		return;
	}

	tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				     gdk_pixbuf_get_has_alpha (pixbuf),
				     gdk_pixbuf_get_bits_per_sample (pixbuf),
				     image.width, image.height);

	gdk_pixbuf_fill (tmp_pixbuf, 0xff000000);

	if (tmp_pixbuf == NULL)
		return;

	/*
	 * FIXME this is a hack to work around a gdk-pixbuf bug
	 * it could be removed when
	 * http://bugzilla.ximian.com/show_bug.cgi?id=12968
	 * is fixed.
	 */
	bilinear = !((scale_width == 1) && (scale_height == 1));

	gdk_pixbuf_composite (pixbuf, tmp_pixbuf,
			      0,
			      0,
			      image.width, image.height,
			      (double) 0.,
			      (double) 0.,
			      (gdouble) scale_width/ (gdouble) orig_width,
			      (gdouble) scale_height/ (gdouble) orig_height,
			      bilinear ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST,
			      255);

	if (color != NULL) {
		guchar *q;
		guint i, j;

		n_channels = gdk_pixbuf_get_n_channels (tmp_pixbuf);
		q = gdk_pixbuf_get_pixels (tmp_pixbuf);
		for (i = 0; i < image.height; i++) {
			guchar *p = q;

			for (j = 0; j < image.width; j++) {
				gint r, g, b, a;

				if (n_channels > 3)
					a = p[3];
				else
					a = 0xff;

				r = (a * p[0] + color->red) >> 9;
				g = (a * p[1] + color->green) >> 9;
				b = (a * p[2] + color->blue) >> 9;

				p[0] = r;
				p[1] = g;
				p[2] = b;

				if (n_channels > 3)
					p[3] = (a + 127) / 2;

				p += n_channels;
			}

			q += gdk_pixbuf_get_rowstride (tmp_pixbuf);
		}
	}

	gdk_cairo_set_source_pixbuf (gdk_painter->cr, tmp_pixbuf,
				     image.x - clip.x,
				     image.y - clip.y);
	cairo_rectangle (gdk_painter->cr,
			 image.x - clip.x, image.y - clip.y,
			 image.width, image.height);
	cairo_fill (gdk_painter->cr);
	g_object_unref (tmp_pixbuf);
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

	_cairo_draw_rectangle (gdk_painter->cr,
			       TRUE, x - gdk_painter->x1, y - gdk_painter->y1,
			       width, height);
}

static gint
draw_spell_error (HTMLPainter *painter,
                  gint x,
                  gint y,
                  gint width)
{
	HTMLGdkPainter *gdk_painter;
	const double dashes[] = { 2, 2 };
	gint ndash = G_N_ELEMENTS (dashes);

	gdk_painter = HTML_GDK_PAINTER (painter);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	cairo_save (gdk_painter->cr);

	cairo_set_dash (gdk_painter->cr, dashes, ndash, 2);
	_cairo_draw_line (gdk_painter->cr, x, y, x + width, y);
	cairo_set_dash (gdk_painter->cr, dashes, ndash, 0);
	_cairo_draw_line (gdk_painter->cr, x, y + 1, x + width, y + 1);

	cairo_restore (gdk_painter->cr);

	return width;
}

static void
draw_embedded (HTMLPainter *p,
               HTMLEmbedded *o,
               gint x,
               gint y)
{
	HTMLGdkPainter *gdk_painter = HTML_GDK_PAINTER (p);
	GtkWidget *embedded_widget;

	embedded_widget = html_embedded_get_widget (o);
	if (embedded_widget && GTK_IS_HTML_EMBEDDED (embedded_widget)) {
		g_signal_emit_by_name (embedded_widget,
				       "draw_gdk", 0,
				       gdk_painter->cr,
				       x, y);
	}
}

static void
set_gdk_color_from_pango_color (GdkColor *gdkc,
                                PangoColor *pc)
{
	gdkc->red = pc->red;
	gdkc->green = pc->green;
	gdkc->blue = pc->blue;
}

static void
set_item_gc (HTMLPainter *p,
             HTMLPangoProperties *properties,
             GdkColor **fg_color,
             GdkColor **bg_color)
{
	if (properties->fg_color) {
		*fg_color = g_new0 (GdkColor, 1);
		set_gdk_color_from_pango_color (*fg_color, properties->fg_color);
	} else
		*fg_color = NULL;

	if (properties->bg_color) {
		*bg_color = g_new0 (GdkColor, 1);
		set_gdk_color_from_pango_color (*bg_color, properties->bg_color);
	} else {
		*bg_color = NULL;
	}
}

static gint
draw_lines (PangoGlyphString *str,
            gint x,
            gint y,
            cairo_t *cr,
            PangoItem *item,
            HTMLPangoProperties *properties)
{
	PangoRectangle log_rect;
	gint width, dsc, asc;

	pango_glyph_string_extents (str, item->analysis.font, NULL, &log_rect);

	width = log_rect.width;
	dsc = PANGO_PIXELS (PANGO_DESCENT (log_rect));
	asc = PANGO_PIXELS (PANGO_ASCENT (log_rect));

	if (properties->underline)
		_cairo_draw_line (cr, x, y + dsc - 2, x + PANGO_PIXELS (width), y + dsc - 2);

	if (properties->strikethrough)
		_cairo_draw_line (cr, x, y - asc + (asc + dsc) / 2, x + PANGO_PIXELS (width), y - asc + (asc + dsc) / 2);

	return width;
}

static gint
draw_glyphs (HTMLPainter *painter,
             gint x,
             gint y,
             PangoItem *item,
             PangoGlyphString *glyphs,
             GdkColor *fg,
             GdkColor *bg)
{
	HTMLGdkPainter *gdk_painter;
	guint i;
	HTMLPangoProperties properties;
	GdkColor *fg_text_color;
	GdkColor *bg_text_color;
	gint cw = 0;

	gdk_painter = HTML_GDK_PAINTER (painter);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	html_pango_get_item_properties (item, &properties);

	set_item_gc (painter, &properties, &fg_text_color, &bg_text_color);

	if (bg_text_color || bg) {
		PangoRectangle log_rect;

		cairo_save (gdk_painter->cr);

		if (bg)
			gdk_cairo_set_source_color (gdk_painter->cr, bg);
		else
			gdk_cairo_set_source_color (gdk_painter->cr, bg_text_color);

		pango_glyph_string_extents (glyphs, item->analysis.font, NULL, &log_rect);
		_cairo_draw_rectangle (gdk_painter->cr, TRUE, x, y - PANGO_PIXELS (PANGO_ASCENT (log_rect)),
				       PANGO_PIXELS (log_rect.width), PANGO_PIXELS (log_rect.height));
		cairo_restore (gdk_painter->cr);
	}

	if (fg_text_color || fg) {
		cairo_save (gdk_painter->cr);

		if (fg)
			gdk_cairo_set_source_color (gdk_painter->cr, fg);
		else
			gdk_cairo_set_source_color (gdk_painter->cr, fg_text_color);
	}

	_cairo_draw_glyphs (gdk_painter->cr,
			    item->analysis.font, x, y, glyphs);
	if (properties.strikethrough || properties.underline)
		cw = draw_lines (glyphs, x, y, gdk_painter->cr, item, &properties);
	else
		for (i = 0; i < glyphs->num_glyphs; i++)
			cw += glyphs->glyphs[i].geometry.width;

	if (fg_text_color || fg)
		cairo_restore (gdk_painter->cr);

	if (fg_text_color)
		g_free (fg_text_color);

	if (bg_text_color)
		g_free (bg_text_color);

	return cw;
}

static void
draw_shade_line (HTMLPainter *painter,
                 gint x,
                 gint y,
                 gint width)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (painter);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	cairo_save (gdk_painter->cr);

	gdk_cairo_set_source_color (gdk_painter->cr, &gdk_painter->dark);
	_cairo_draw_line (gdk_painter->cr, x, y, x + width, y);
	gdk_cairo_set_source_color (gdk_painter->cr, &gdk_painter->light);
	_cairo_draw_line (gdk_painter->cr, x, y + 1, x + width, y + 1);

	cairo_restore (gdk_painter->cr);
}

static guint
get_pixel_size (HTMLPainter *painter)
{
	return 1;
}

static guint
get_page_width (HTMLPainter *painter,
                HTMLEngine *e)
{
	return html_engine_get_view_width (e) + html_engine_get_left_border (e) + html_engine_get_right_border (e);
}

static guint
get_page_height (HTMLPainter *painter,
                 HTMLEngine *e)
{
	return html_engine_get_view_height (e) + html_engine_get_top_border (e) + html_engine_get_bottom_border (e);
}

static void
init_color (GdkColor *color,
            gushort red,
            gushort green,
            gushort blue)
{
	color->pixel = 0;
	color->red = red;
	color->green = green;
	color->blue = blue;
}

static void
html_gdk_painter_init (GObject *object)
{
	HTMLPainter *painter;
	HTMLGdkPainter *gdk_painter;

	painter = HTML_PAINTER (object);
	gdk_painter = HTML_GDK_PAINTER (object);

	painter->engine_to_pango = PANGO_SCALE;

	gdk_painter->window = NULL;

	gdk_painter->cr = NULL;

	gdk_painter->double_buffer = TRUE;
	gdk_painter->surface = NULL;
	gdk_painter->x1 = gdk_painter->y1 = 0;
	gdk_painter->x2 = gdk_painter->y2 = 0;
	gdk_painter->set_background = FALSE;
	gdk_painter->do_clear = FALSE;

	init_color (& gdk_painter->background, 0xffff, 0xffff, 0xffff);
	init_color (& gdk_painter->dark, 0, 0, 0);
	init_color (& gdk_painter->light, 0, 0, 0);
}

static void
html_gdk_painter_real_set_widget (HTMLPainter *painter,
                                  GtkWidget *widget)
{
	parent_class->set_widget (painter, widget);

	if (painter->pango_context)
		g_object_unref (painter->pango_context);
	painter->pango_context = gtk_widget_get_pango_context (widget);
	g_object_ref (painter->pango_context);
}

static void
html_gdk_painter_class_init (GObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;
	parent_class = g_type_class_ref (HTML_TYPE_PAINTER);

	painter_class->set_widget = html_gdk_painter_real_set_widget;
	painter_class->begin = begin;
	painter_class->end = end;
	painter_class->alloc_color = alloc_color;
	painter_class->free_color = free_color;
	painter_class->set_pen = set_pen;
	painter_class->get_black = get_black;
	painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_glyphs = draw_glyphs;
	painter_class->draw_spell_error = draw_spell_error;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->draw_ellipse = draw_ellipse;
	painter_class->clear = clear;
	painter_class->set_background_color = set_background_color;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_border = draw_border;
	painter_class->set_clip_rectangle = set_clip_rectangle;
	painter_class->draw_background = draw_background;
	painter_class->get_pixel_size = get_pixel_size;
	painter_class->draw_embedded = draw_embedded;
	painter_class->get_page_width = get_page_width;
	painter_class->get_page_height = get_page_height;
}

GType
html_gdk_painter_get_type (void)
{
	static GType html_gdk_painter_type = 0;

	if (html_gdk_painter_type == 0) {
		static const GTypeInfo html_gdk_painter_info = {
			sizeof (HTMLGdkPainterClass),
			NULL,
			NULL,
			(GClassInitFunc) html_gdk_painter_class_init,
			NULL,
			NULL,
			sizeof (HTMLGdkPainter),
			1,
			(GInstanceInitFunc) html_gdk_painter_init,
		};
		html_gdk_painter_type = g_type_register_static (HTML_TYPE_PAINTER, "HTMLGdkPainter",
								&html_gdk_painter_info, 0);
	}

	return html_gdk_painter_type;
}

HTMLPainter *
html_gdk_painter_new (GtkWidget *widget,
                      gboolean double_buffer)
{
	HTMLGdkPainter *new;

	new = g_object_new (HTML_TYPE_GDK_PAINTER, NULL);

	new->double_buffer = double_buffer;
	html_painter_set_widget (HTML_PAINTER (new), widget);

	return HTML_PAINTER (new);
}

void
html_gdk_painter_realize (HTMLGdkPainter *gdk_painter,
                          GdkWindow *window)
{
	g_return_if_fail (gdk_painter != NULL);
	g_return_if_fail (window != NULL);

	gdk_painter->window = window;

	gdk_painter->light.red = 0xffff;
	gdk_painter->light.green = 0xffff;
	gdk_painter->light.blue = 0xffff;

	gdk_painter->dark.red = 0x7fff;
	gdk_painter->dark.green = 0x7fff;
	gdk_painter->dark.blue = 0x7fff;

	gdk_painter->black.red = 0x0000;
	gdk_painter->black.green = 0x0000;
	gdk_painter->black.blue = 0x0000;
}

void
html_gdk_painter_unrealize (HTMLGdkPainter *painter)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_GDK_PAINTER (painter));

	if (html_gdk_painter_realized (painter)) {
		painter->window = NULL;
	}
}

gboolean
html_gdk_painter_realized (HTMLGdkPainter *painter)
{
	g_return_val_if_fail (painter != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_GDK_PAINTER (painter), FALSE);

	if (painter->window == NULL)
		return FALSE;
	else
		return TRUE;
}
