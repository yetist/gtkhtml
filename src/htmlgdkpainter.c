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

#include <config.h>
#include "gtkhtml-compat.h"

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtksignal.h>

#include <libgnome/gnome-i18n.h>

#include "htmlentity.h"
#include "htmlgdkpainter.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlembedded.h"
#include "htmlengine.h"
#include "gtkhtml-embedded.h"

static HTMLPainterClass *parent_class = NULL;

/* GObject methods.  */

static void
finalize (GObject *object)
{
	HTMLGdkPainter *painter;

	painter = HTML_GDK_PAINTER (object);

	if (painter->gc != NULL) {
		g_object_unref (painter->gc);
		painter->gc = NULL;
	}

	if (painter->pixmap != NULL) {
		g_object_unref (painter->pixmap);
		painter->pixmap = NULL;
	}

	if (painter->pc) {
		g_object_unref (painter->pc);
		painter->pc = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

inline static GList *
text_itemize_and_prepare_glyphs (PangoContext *context, PangoFontDescription *desc, const gchar *text, gint bytes, PangoGlyphString **glyphs)
{
	PangoAttrList *attrs;
	GList *items;

	pango_context_set_font_description (context, desc);
	attrs = pango_attr_list_new ();
	items = pango_itemize (context, text, 0, bytes, attrs, NULL);
	pango_attr_list_unref (attrs);

	if (items && items->data) {
		PangoItem *item = (PangoItem *) items->data;

		*glyphs = pango_glyph_string_new ();
		pango_shape (text, bytes, &item->analysis, *glyphs);
	} else
		*glyphs = NULL;

	return items;
}

inline static void
items_destroy (GList *items)
{
	GList *l;

	for (l = items; l; l = l->next)
		pango_item_free ((PangoItem *) l->data);
	g_list_free (items);
}

static gint
text_width (PangoContext *context, PangoFontDescription *desc, const gchar *text, gint bytes)
{
	GList *items;
	PangoGlyphString *glyphs;
	gint width = 0;

	items = text_itemize_and_prepare_glyphs (context, desc, text, bytes, &glyphs);

	if (items && glyphs) {
		int i;
		for (i=0; i < glyphs->num_glyphs; i ++)
			width += glyphs->glyphs [i].geometry.width;
	}
	if (glyphs)
		pango_glyph_string_free (glyphs);
	if (items)
		items_destroy (items);
	/* printf ("text_width %d\n", PANGO_PIXELS (width)); */
	return PANGO_PIXELS (width);
}

static void
text_size (PangoContext *context, PangoFontDescription *desc, const gchar *text, gint bytes, PangoRectangle *log_rect)
{
	GList *items;
	PangoGlyphString *glyphs;

	items = text_itemize_and_prepare_glyphs (context, desc, text, bytes, &glyphs);
	if (items && items->data && glyphs)
		pango_glyph_string_extents (glyphs, ((PangoItem *) items->data)->analysis.font, NULL, log_rect);
	else
		log_rect->x = log_rect->y = log_rect->width = log_rect->height = 0;

	if (glyphs)
		pango_glyph_string_free (glyphs);
	if (items)
		items_destroy (items);
}

static HTMLFont *
alloc_font (HTMLPainter *painter, gchar *face, gdouble size, gboolean points, GtkHTMLFontStyle style)
{
	PangoFontDescription *desc;

	desc = pango_font_description_new ();
	pango_font_description_set_family (desc, face ? face : (style & GTK_HTML_FONT_STYLE_FIXED ? "Monospace" : "Sans"));
	pango_font_description_set_size (desc, size * PANGO_SCALE);
	pango_font_description_set_style (desc, style & GTK_HTML_FONT_STYLE_ITALIC ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_weight (desc, style & GTK_HTML_FONT_STYLE_BOLD ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);

	return html_font_new (desc,
			      text_width (HTML_GDK_PAINTER (painter)->pc, desc, " ", 1),
			      text_width (HTML_GDK_PAINTER (painter)->pc, desc, "\xc2\xa0", 2),
			      text_width (HTML_GDK_PAINTER (painter)->pc, desc, "\t", 1));
}

static void
ref_font (HTMLPainter *painter, HTMLFont *font)
{
}

static void
unref_font (HTMLPainter *painter, HTMLFont *font)
{
	/* FIX2 stop leaking font description */
}


static void
alloc_color (HTMLPainter *painter,
	     GdkColor *color)
{
	HTMLGdkPainter *gdk_painter;
	GdkColormap *colormap;

	gdk_painter = HTML_GDK_PAINTER (painter);
	g_return_if_fail (gdk_painter->window != NULL);

	colormap = gdk_drawable_get_colormap (gdk_painter->window);
	gdk_rgb_find_color (colormap, color);
}

static void
free_color (HTMLPainter *painter,
	    GdkColor *color)
{
}


static void
begin (HTMLPainter *painter, int x1, int y1, int x2, int y2)
{
	HTMLGdkPainter *gdk_painter;
	GdkVisual *visual;

	/* printf ("painter begin %d,%d %d,%d\n", x1, y1, x2, y2); */

	gdk_painter = HTML_GDK_PAINTER (painter);
	g_return_if_fail (gdk_painter->window != NULL);
	visual = gdk_drawable_get_visual (gdk_painter->window);
	g_return_if_fail (visual != NULL);

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

	/* printf ("painter end\n"); */

	gdk_painter = HTML_GDK_PAINTER (painter);
	
	if (! gdk_painter->double_buffer)
		return;

	gdk_draw_drawable (gdk_painter->window, gdk_painter->gc,
			   gdk_painter->pixmap,
			   0, 0,
			   gdk_painter->x1, gdk_painter->y1,
			   gdk_painter->x2 - gdk_painter->x1,
			   gdk_painter->y2 - gdk_painter->y1);

	g_object_unref (gdk_painter->pixmap);
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
	    GdkColor *bg,
	    gint x, gint y,
	    gint width, gint height,
	    GtkHTMLEtchStyle inset,
	    gint bordersize)
{
	HTMLGdkPainter *gdk_painter;
	GdkColor *col1 = NULL, *col2 = NULL;
	GdkColor dark, light;

	#define INC 0x8000
	#define DARK(c)  dark.c = MAX (((gint) bg->c) - INC, 0)
	#define LIGHT(c) light.c = MIN (((gint) bg->c) + INC, 0xffff)

	DARK(red);
	DARK(green);
	DARK(blue);
	LIGHT(red);
	LIGHT(green);
	LIGHT(blue);

	alloc_color (painter, &dark);
	alloc_color (painter, &light);

	gdk_painter = HTML_GDK_PAINTER (painter);

	switch (inset) {
	case GTK_HTML_ETCH_NONE:
		/* use the current pen color */
		col1 = NULL;
		col2 = NULL;
		break;
	case GTK_HTML_ETCH_OUT:
		col1 = &light;
		col2 = &dark;
		break;
	default:
	case GTK_HTML_ETCH_IN:
		col1 = &dark;
		col2 = &light;
		break;
	}
	
	x -= gdk_painter->x1;
	y -= gdk_painter->y1;
	
	while (bordersize > 0) {
		if (col2) {
			gdk_gc_set_foreground (gdk_painter->gc, col2);
		}

		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x + width - 1, y, x + width - 1, y + height - 1);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x + 1, y + height - 1, x + width - 1, y + height - 1);
		if (col1) {
			gdk_gc_set_foreground (gdk_painter->gc, col1);
		}

		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x, y, x + width - 2, y);
		gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc,
			       x, y, x, y + height - 1);
		bordersize--;
		x++;
		y++;
		width-=2;
		height-=2;
	}

	free_color (painter, &dark);
	free_color (painter, &light);
}

static void
draw_background (HTMLPainter *painter,
		 GdkColor *color,
		 GdkPixbuf *pixbuf,
		 gint x, gint y, 
		 gint width, gint height,
		 gint tile_x, gint tile_y)
{
	HTMLGdkPainter *gdk_painter;
	gint pw;
	gint ph;
	gint tile_width, tile_height;
	gint w, h;
	GdkRectangle expose, paint, clip;
	GdkPixbufAlphaMode alpha_mode;

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
		gdk_gc_set_foreground (gdk_painter->gc, color);
		gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc,
				    TRUE, paint.x - clip.x, paint.y - clip.y,
				    paint.width, paint.height);	
		
	}

	if (!pixbuf)
		return;

	alpha_mode = gdk_painter->alpha ? GDK_PIXBUF_ALPHA_FULL : GDK_PIXBUF_ALPHA_BILEVEL;

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
			
			html_painter_alloc_color (painter, &pixcol);
			color = &pixcol;
		}

		if (color) {
			gdk_gc_set_foreground (gdk_painter->gc, color);
			gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc,
					    TRUE, paint.x - clip.x, paint.y - clip.y,
					    paint.width, paint.height);
		}	
		
		return;
	}

	tile_width = (tile_x % pw) + paint.width;
	tile_height = (tile_y % ph) + paint.height;

	/* do tiling */
	if (tile_width > pw || tile_height > ph) {
		GdkPixmap *pixmap = NULL;
		gint cw, ch, cx, cy;
		gint dw, dh;
		GdkGC *gc;
		GdkBitmap *bitmap = NULL;
		
		dw = MIN (pw, tile_width);
		dh = MIN (ph, tile_height);

		gc = gdk_gc_new (gdk_painter->window);

  		if (color || !gdk_pixbuf_get_has_alpha (pixbuf)) {
			pixmap = gdk_pixmap_new (gdk_painter->window, dw, dh, -1);		
			
			if (color) {
				gdk_gc_set_foreground (gc, color);
				gdk_draw_rectangle (pixmap, gc,
						    TRUE, 0, 0,
						    dw, dh);
			}	

			gdk_pixbuf_render_to_drawable_alpha (pixbuf, pixmap,
							     0, 0,
							     0, 0, 
							     dw, dh,
							     alpha_mode,
							     128,
							     GDK_RGB_DITHER_NORMAL,
							     paint.x, paint.y);
	
			gdk_gc_set_tile (gc, pixmap);
			gdk_gc_set_fill (gc, GDK_TILED);
			gdk_gc_set_ts_origin (gc, 
					      paint.x - (tile_x % pw) - clip.x,  
					      paint.y - (tile_y % ph) - clip.y);

			gdk_draw_rectangle (gdk_painter->pixmap, gc, TRUE,
					    paint.x - clip.x, paint.y - clip.y, 
					    paint.width, paint.height);
			
			g_object_unref (pixmap);			
			g_object_unref (gc);			
		} else if (alpha_mode == GDK_PIXBUF_ALPHA_FULL) {
			int incr_x = 0;
			int incr_y = 0;

			cy = paint.y;
			ch = paint.height;
			h = tile_y % ph;
			while (ch > 0) {
				incr_y = dh - h;

				cx = paint.x;
				cw = paint.width;
				w = tile_x % pw;
				while (cw > 0) {
					incr_x = dw - w;

					gdk_pixbuf_render_to_drawable_alpha (pixbuf, gdk_painter->pixmap,
									     w, h, 
									     cx - clip.x, cy - clip.y,
									     (cw >= incr_x) ? incr_x : cw,
									     (ch >= incr_y) ? incr_y : ch,
									     alpha_mode,
									     128,
									     GDK_RGB_DITHER_NORMAL,
									     cx, cy);

					cw -= incr_x;
					cx += incr_x;
					w = 0;
				}
				ch -= incr_y;
				cy += incr_y;
				h = 0;
			}

			g_object_unref (gc);			
		} else {
			int incr_x = 0;
			int incr_y = 0;

			/* Right now we only support GDK_PIXBUF_ALPHA_BILEVEL, so we
			 * unconditionally create the clipping mask.
			 */
			bitmap = gdk_pixmap_new (NULL, dw, dh, 1);
			gdk_pixbuf_render_threshold_alpha (pixbuf, bitmap,
							   0, 0,
							   0, 0,
							   dw, dh,
							   128);
			gdk_gc_set_clip_mask (gc, bitmap);
			
			pixmap = gdk_pixmap_new (gdk_painter->window, dw, dh, -1);		
			gdk_pixbuf_render_to_drawable (pixbuf, pixmap, gc,
						       0, 0,
						       0, 0, 
						       dw, dh,
						       GDK_RGB_DITHER_NORMAL,
						       paint.x, paint.y);
			
			cy = paint.y;
			ch = paint.height;
			h = tile_y % ph;
			while (ch > 0) {
				incr_y = dh - h;

				cx = paint.x;
				cw = paint.width;
				w = tile_x % pw;
				while (cw > 0) {
					incr_x = dw - w;

					gdk_gc_set_clip_origin (gc, 
								cx - w - clip.x,
								cy - h - clip.y);
					
					gdk_draw_drawable (gdk_painter->pixmap, gc, pixmap,
							   w, h, cx - clip.x, cy - clip.y,
							   (cw >= incr_x) ? incr_x : cw,
							   (ch >= incr_y) ? incr_y : ch);

					cw -= incr_x;
					cx += incr_x;
					w = 0;
				}
				ch -= incr_y;
				cy += incr_y;
				h = 0;
			}
			g_object_unref (pixmap);			
			g_object_unref (bitmap);
			g_object_unref (gc);
		}
	} else {
		if (color && gdk_pixbuf_get_has_alpha (pixbuf)) {
			gdk_gc_set_foreground (gdk_painter->gc, color);
			gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc, TRUE,
					    paint.x - clip.x, paint.y - clip.y,
					    paint.width, paint.height);	
		}
		
		gdk_pixbuf_render_to_drawable_alpha (pixbuf, gdk_painter->pixmap,
						     tile_x % pw, tile_y % ph,
						     paint.x - clip.x, paint.y - clip.y, 
						     paint.width, paint.height,
						     alpha_mode,
						     128,
						     GDK_RGB_DITHER_NORMAL,
						     paint.x, paint.y);
	}
}

static GdkPixbuf *
create_temporary_pixbuf (GdkPixbuf *src,
			 gint clip_width, gint clip_height)
{
	GdkPixbuf *pixbuf;
	gboolean has_alpha;
	guint bits_per_sample;

	has_alpha = gdk_pixbuf_get_has_alpha (src);
	bits_per_sample = gdk_pixbuf_get_bits_per_sample (src);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, bits_per_sample, clip_width, clip_height);
	gdk_pixbuf_fill (pixbuf, 0xff000000);
	
	return pixbuf;
}

static void
draw_pixmap (HTMLPainter *painter,
	     GdkPixbuf *pixbuf,
	     gint x, gint y,
	     gint scale_width, gint scale_height,
	     const GdkColor *color)
{
	GdkRectangle clip, image, paint;
	HTMLGdkPainter *gdk_painter;
	GdkPixbufAlphaMode alpha_mode;
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

	alpha_mode = gdk_painter->alpha ? GDK_PIXBUF_ALPHA_FULL : GDK_PIXBUF_ALPHA_BILEVEL;

	if (scale_width == orig_width && scale_height == orig_height && color == NULL) {
		gdk_pixbuf_render_to_drawable_alpha (pixbuf, gdk_painter->pixmap,
						     paint.x - image.x,
						     paint.y - image.y,
						     paint.x - clip.x,
						     paint.y - clip.y,
						     paint.width,
						     paint.height,
						     alpha_mode,
						     128,
						     GDK_RGB_DITHER_NORMAL,
						     paint.x, paint.y);
		return;
	}

       	tmp_pixbuf = create_temporary_pixbuf (pixbuf,
					      paint.width,
					      paint.height);

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
			      paint.width, paint.height,
			      (double)-(paint.x - image.x), 
			      (double)-(paint.y - image.y),
			      (gdouble) scale_width/ (gdouble) orig_width,
			      (gdouble) scale_height/ (gdouble) orig_height,
			      bilinear ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST,
			      255);

	if (color != NULL) {
		guchar *p, *q;
		guint i, j;

		n_channels = gdk_pixbuf_get_n_channels (tmp_pixbuf);
		p = q = gdk_pixbuf_get_pixels (tmp_pixbuf);
		for (i = 0; i < paint.height; i++) {
			p = q;

			for (j = 0; j < paint.width; j++) {
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

	gdk_pixbuf_render_to_drawable_alpha (tmp_pixbuf, gdk_painter->pixmap,
					     0,
					     0,
					     paint.x - clip.x,
					     paint.y - clip.y,
					     paint.width,
					     paint.height,
					     alpha_mode,
					     128,
					     GDK_RGB_DITHER_NORMAL,
					     paint.x, paint.y);
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
			    TRUE, x - gdk_painter->x1, y - gdk_painter->y1,
			    width, height);
}

static gint
draw_spell_error (HTMLPainter *painter, gint x, gint y, const gchar *text, gint len)
{
	PangoFontDescription *desc;
	HTMLGdkPainter *gdk_painter;
	GdkGCValues values;
	guint width;
	gchar dash [2];

	gdk_painter = HTML_GDK_PAINTER (painter);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;


	desc  = html_painter_get_font  (painter, painter->font_face, painter->font_style);
	width = text_width (gdk_painter->pc, desc, text, g_utf8_offset_to_pointer (text, len) - text);

	gdk_gc_get_values (gdk_painter->gc, &values);
	gdk_gc_set_fill (gdk_painter->gc, GDK_OPAQUE_STIPPLED);
	dash [0] = 2;
	dash [1] = 2;
	gdk_gc_set_line_attributes (gdk_painter->gc, 1, GDK_LINE_ON_OFF_DASH, values.cap_style, values.join_style);
	gdk_gc_set_dashes (gdk_painter->gc, 2, dash, 2);
	gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, x, y, x + width, y);
	gdk_gc_set_dashes (gdk_painter->gc, 0, dash, 2);
	gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, x, y + 1, x + width, y + 1);
	gdk_gc_set_line_attributes (gdk_painter->gc, values.line_width,
				    values.line_style, values.cap_style, values.join_style);

	return width;
}

static void
draw_embedded (HTMLPainter * p, HTMLEmbedded *o, gint x, gint y) 
{
	HTMLGdkPainter *gdk_painter = HTML_GDK_PAINTER(p);
	GtkWidget *embedded_widget;

	embedded_widget = html_embedded_get_widget (o);
	if (embedded_widget && GTK_IS_HTML_EMBEDDED (embedded_widget)) {
		g_signal_emit_by_name (embedded_widget,
				       "draw_gdk", 0,
				       gdk_painter->pixmap, 
				       gdk_painter->gc, 
				       x, y);
	}
}

static void
draw_text (HTMLPainter *painter, gint x, gint y, const gchar *text, gint len)
{
	HTMLGdkPainter *gdk_painter;
	PangoFontDescription *desc;
	PangoGlyphString *glyphs;
	GList *items;
	gint blen;

	if (len == -1)
		len = g_utf8_strlen (text, -1);

	gdk_painter = HTML_GDK_PAINTER (painter);
	desc = html_painter_get_font (painter, painter->font_face, painter->font_style);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	blen = g_utf8_offset_to_pointer (text, len) - text;
	items = text_itemize_and_prepare_glyphs (gdk_painter->pc, desc, text, blen, &glyphs);
	if (items && items->data)
		gdk_draw_glyphs (gdk_painter->pixmap, gdk_painter->gc, ((PangoItem *) items->data)->analysis.font, x, y, glyphs);

	if (items && items->data && glyphs && painter->font_style & (GTK_HTML_FONT_STYLE_UNDERLINE | GTK_HTML_FONT_STYLE_STRIKEOUT)) {
		PangoRectangle log_rect;
		gint width, dsc, asc;

		pango_glyph_string_extents (glyphs, ((PangoItem *) items->data)->analysis.font, NULL, &log_rect);

		width = PANGO_PIXELS (log_rect.width);
		dsc = PANGO_PIXELS (PANGO_DESCENT (log_rect));
		asc = PANGO_PIXELS (PANGO_ASCENT (log_rect));

		if (painter->font_style & GTK_HTML_FONT_STYLE_UNDERLINE)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y + dsc - 2, 
				       x + width, y + dsc - 2);

		if (painter->font_style & GTK_HTML_FONT_STYLE_STRIKEOUT)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y - asc + (asc + dsc)/2, 
				       x + width, y - asc + (asc + dsc)/2);
	}

	if (glyphs)
		pango_glyph_string_free (glyphs);
	if (items)
		items_destroy (items);
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

static void
calc_text_size (HTMLPainter *painter,
		const gchar *text,
		guint len,
		GtkHTMLFontStyle style,
		HTMLFontFace *face,
		gint *width, gint *asc, gint *dsc)
{
	HTMLFont *font;
	HTMLGdkPainter *gdk_painter;
	PangoRectangle logical_rect;

	gdk_painter = HTML_GDK_PAINTER (painter);
	font = html_font_manager_get_font (&painter->font_manager, face, style);

	text_size (gdk_painter->pc, (PangoFontDescription *) font->data, text, g_utf8_offset_to_pointer (text, len) - text, &logical_rect);

	*width = PANGO_PIXELS (logical_rect.width);
	*asc   = PANGO_PIXELS (PANGO_ASCENT (logical_rect));
	*dsc   = PANGO_PIXELS (PANGO_DESCENT (logical_rect));

	/* printf ("calc_text_size %d %d %d\n", *width, *asc, *dsc); */
}

static void
calc_text_size_bytes (HTMLPainter *painter, const gchar *text,
		      guint bytes_len, HTMLFont *font, GtkHTMLFontStyle style,
		      gint *width, gint *asc, gint *dsc)
{
	HTMLGdkPainter *gdk_painter;
	PangoRectangle logical_rect;

	gdk_painter = HTML_GDK_PAINTER (painter);

	text_size (gdk_painter->pc, (PangoFontDescription *) font->data, text, bytes_len, &logical_rect);

	*width = PANGO_PIXELS (logical_rect.width);
	*asc   = PANGO_PIXELS (PANGO_ASCENT (logical_rect));
	*dsc   = PANGO_PIXELS (PANGO_DESCENT (logical_rect));

	/* printf ("calc_text_size_bytes %d %d %d\n", *width, *asc, *dsc); */
}

static guint
get_pixel_size (HTMLPainter *painter)
{
	return 1;
}

static guint
get_page_width (HTMLPainter *painter, HTMLEngine *e)
{
	return html_engine_get_view_width (e) + e->leftBorder + e->rightBorder;
}

static guint
get_page_height (HTMLPainter *painter, HTMLEngine *e)
{
	return html_engine_get_view_height (e) + e->topBorder + e->bottomBorder;
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
html_gdk_painter_init (GObject *object)
{
	HTMLGdkPainter *gdk_painter;

	gdk_painter = HTML_GDK_PAINTER (object);

	gdk_painter->window = NULL;

	gdk_painter->alpha = TRUE;
	gdk_painter->gc = NULL;

	gdk_painter->double_buffer = TRUE;
	gdk_painter->pixmap = NULL;
	gdk_painter->x1 = gdk_painter->y1 = 0;
	gdk_painter->x2 = gdk_painter->y2 = 0;
	gdk_painter->set_background = FALSE;
	gdk_painter->do_clear = FALSE;

	init_color (& gdk_painter->background, 0xffff, 0xffff, 0xffff);
	init_color (& gdk_painter->dark, 0, 0, 0);
	init_color (& gdk_painter->light, 0, 0, 0);
}

static void
html_gdk_painter_class_init (GObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;
	parent_class = g_type_class_ref (HTML_TYPE_PAINTER);

	painter_class->begin = begin;
	painter_class->end = end;
	painter_class->alloc_font = alloc_font;
	painter_class->ref_font   = ref_font;
	painter_class->unref_font = unref_font;
	painter_class->alloc_color = alloc_color;
	painter_class->free_color = free_color;
	painter_class->calc_text_size = calc_text_size;
	painter_class->calc_text_size_bytes = calc_text_size_bytes;
	painter_class->set_pen = set_pen;
	painter_class->get_black = get_black;
	painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_text = draw_text;
	painter_class->draw_spell_error = draw_spell_error;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->draw_ellipse = draw_ellipse;
	painter_class->clear = clear;
	painter_class->set_background_color = set_background_color;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_panel = draw_panel;
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
html_gdk_painter_new (GtkWidget *widget, gboolean double_buffer)
{
	HTMLGdkPainter *new;

	new = g_object_new (HTML_TYPE_GDK_PAINTER, NULL);

	new->double_buffer = double_buffer;
	new->pc = gtk_widget_get_pango_context (widget);
	g_object_ref (new->pc);

	return HTML_PAINTER (new);
}

void
html_gdk_painter_realize (HTMLGdkPainter *gdk_painter,
			  GdkWindow *window)
{
	g_return_if_fail (gdk_painter != NULL);
	g_return_if_fail (window != NULL);
	
	gdk_painter->gc = gdk_gc_new (window);
	gdk_painter->window = window;

	gdk_painter->light.red = 0xffff;
	gdk_painter->light.green = 0xffff;
	gdk_painter->light.blue = 0xffff;
	html_painter_alloc_color (HTML_PAINTER (gdk_painter), &gdk_painter->light);

	gdk_painter->dark.red = 0x7fff;
	gdk_painter->dark.green = 0x7fff;
	gdk_painter->dark.blue = 0x7fff;
	html_painter_alloc_color (HTML_PAINTER (gdk_painter), &gdk_painter->dark);

	gdk_painter->black.red = 0x0000;
	gdk_painter->black.green = 0x0000;
	gdk_painter->black.blue = 0x0000;
	html_painter_alloc_color (HTML_PAINTER (gdk_painter), &gdk_painter->black);
}

void
html_gdk_painter_unrealize (HTMLGdkPainter *painter)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_GDK_PAINTER (painter));

	if (html_gdk_painter_realized (painter)) {
		g_object_unref (painter->gc);
		painter->gc = NULL;

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
