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
#include <string.h>
#include <stdlib.h>
#include <unicode.h>
#include <gdk/gdkx.h>
#include <libart_lgpl/art_rect.h>
#include <gal/widgets/e-font.h>

#include "htmlentity.h"
#include "htmlgdkpainter.h"
#include "htmlplainpainter.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"

static HTMLGdkPainterClass *parent_class = NULL;

static EFontStyle me_style (GtkHTMLFontStyle style);

static void
draw_panel (HTMLPainter *painter,
	    GdkColor *bg,
	    gint x, gint y,
	    gint width, gint height,
	    GtkHTMLEtchStyle inset,
	    gint bordersize)
{
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
	ArtIRect expose, paint, clip;

	gdk_painter = HTML_GDK_PAINTER (painter);

	expose.x0 = x;
	expose.y0 = y;
	expose.x1 = x + width;
	expose.y1 = y + height;

	clip.x0 = gdk_painter->x1;
	clip.x1 = gdk_painter->x2;
	clip.y0 = gdk_painter->y1;
	clip.y1 = gdk_painter->y2;
	clip.x0 = gdk_painter->x1;

	art_irect_intersect (&paint, &clip, &expose);
	if (art_irect_empty (&paint))
	    return;

	width = paint.x1 - paint.x0;
	height = paint.y1 - paint.y0;	
	
	tile_x += paint.x0 - x;
	tile_y += paint.y0 - y;
	
	x = paint.x0;
	y = paint.y0;

	if (!color && !pixbuf)
		return;

	if (color) {
		gdk_gc_set_foreground (gdk_painter->gc, color);
		gdk_draw_rectangle (gdk_painter->pixmap, gdk_painter->gc,
				    TRUE, x - gdk_painter->x1, y - gdk_painter->y1,
				    width, height);	
		
	}

	return;
}

static void
draw_pixmap (HTMLPainter *painter,
	     GdkPixbuf *pixbuf,
	     gint x, gint y,
	     gint scale_width, gint scale_height,
	     const GdkColor *color)
{
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

static EFontStyle
me_style (GtkHTMLFontStyle style)
{
	EFontStyle rv = E_FONT_PLAIN;
	return rv;
}

static gchar *
font_name_substitute_attr (const gchar *name, gint nth, gchar *val)
{
	const gchar *s;
	gchar *s1, *s2, *rv;

	for (s = name; nth; nth--) {
		s = strchr (s, '-');
		g_assert (s);
		s ++;
	}
	s1 = g_strndup (name, s - name);
	s2 = strchr (s, '-');
	g_assert (s);

	rv = g_strconcat (s1, val, s2, NULL);
	g_free (s1);

	return rv;
}

static HTMLFont *
alloc_fixed_font (HTMLPainter *painter, gchar *face, gdouble size, GtkHTMLFontStyle style)
{
	EFont *font;
        gchar *name, *fixed_size;

	fixed_size = g_strdup_printf ("%d", painter->font_manager.fix_size);
	name = font_name_substitute_attr (painter->font_manager.fixed.face, 7, fixed_size);
	font = e_font_from_gdk_name (name);
	g_free (fixed_size);
	g_free (name);
	
	if (!font) 
		return NULL;
	
	return html_font_new (font, e_font_utf8_text_width (font, 
							    me_style (style),
							    " ", 1));
}



static void
draw_shade_line (HTMLPainter *painter,
		 gint x, gint y,
		 gint width)
{
}

static void
init (GtkObject *object)
{
}

static void
draw_text (HTMLPainter *painter,
	   gint x, gint y,
	   const gchar *text,
	   gint len)
{
	HTMLGdkPainter *gdk_painter;
	EFont *e_font;

	gdk_painter = HTML_GDK_PAINTER (painter);

	if (len == -1)
		len = unicode_strlen (text, -1);

	x -= gdk_painter->x1;
	y -= gdk_painter->y1;

	e_font = html_painter_get_font (painter, painter->font_face,
					painter->font_style);

	e_font_draw_utf8_text (gdk_painter->pixmap, e_font, 
			       me_style (painter->font_style), gdk_painter->gc,
			       x, y, text, 
			       unicode_offset_to_index (text, len));

	if (painter->font_style & (GTK_HTML_FONT_STYLE_UNDERLINE
				   | GTK_HTML_FONT_STYLE_STRIKEOUT)) {
		guint width;

		width = e_font_utf8_text_width (e_font, 
						me_style (painter->font_style),
						text, unicode_offset_to_index (text, len));
		/*
		if (painter->font_style & GTK_HTML_FONT_STYLE_UNDERLINE)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y + 1, 
				       x + width, y + 1);

		if (painter->font_style & GTK_HTML_FONT_STYLE_STRIKEOUT)
			gdk_draw_line (gdk_painter->pixmap, gdk_painter->gc, 
				       x, y - e_font_ascent (e_font) / 2, 
				       x + width, y - e_font_ascent (e_font) / 2);
		*/
	}
}

static guint
calc_text_width (HTMLPainter *painter,
		 const gchar *text,
		 guint len,
		 GtkHTMLFontStyle style,
		 HTMLFontFace *face)
{
	HTMLGdkPainter *gdk_painter;
	EFont *e_font;
	gint width;


	gdk_painter = HTML_GDK_PAINTER (painter);
	e_font = html_painter_get_font (painter, face, style);

	width = e_font_utf8_text_width (e_font, me_style (style),
					text, unicode_offset_to_index (text, len));

	return width;
}

static void
draw_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
}

static void
class_init (GtkObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	//object_class->finalize = finalize;

	painter_class->alloc_font = alloc_fixed_font;
	//painter_class->calc_text_width = calc_text_width;
	//painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_text = draw_text;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	//	painter_class->draw_ellipse = draw_ellipse;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_panel = draw_panel;
	painter_class->draw_background = draw_background;
	//painter_class->get_pixel_size = get_pixel_size;

	parent_class = gtk_type_class (html_gdk_painter_get_type ());
}

GtkType
html_plain_painter_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"HTMLPlainPainter",
			sizeof (HTMLPlainPainter),
			sizeof (HTMLPlainPainterClass),
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (HTML_TYPE_GDK_PAINTER, &info);
	}

	return type;
}


HTMLPainter *
html_plain_painter_new (gboolean double_buffer)
{
	HTMLPlainPainter *new;

	new = gtk_type_new (html_plain_painter_get_type ());

	HTML_GDK_PAINTER (new)->double_buffer = double_buffer;

	return HTML_PAINTER (new);
}
