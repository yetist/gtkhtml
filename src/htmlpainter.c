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

#include <config.h>
#include <string.h> /* strcmp */
#include <stdlib.h>
#include "gtkhtml-compat.h"

#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlentity.h"
#include "htmlpainter.h"


/* Convenience macro to extract the HTMLPainterClass from a GTK+ object.  */
#define HP_CLASS(obj)					\
	HTML_PAINTER_CLASS (G_OBJECT_GET_CLASS (obj))

#define HTML_ALLOCA_MAX 2048

/* Our parent class.  */
static GObjectClass *parent_class = NULL;

static gint calc_translated_text_bytes (const gchar *text, gint len, gint line_offset, gint *translated_len, gboolean tabs);
static gint translate_text_special_chars (const gchar *text, gchar *translated, gint len, gint line_offset, gboolean tabs);


/* GObject methods.  */

static void
finalize (GObject *object)
{
	HTMLPainter *painter;

	painter = HTML_PAINTER (object);
	html_font_manager_finalize (&painter->font_manager);

	html_colorset_destroy (painter->color_set);

	/* FIXME ownership of the color set?  */

	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


#define DEFINE_UNIMPLEMENTED(method)						\
	static gint								\
	method##_unimplemented (GObject *obj)					\
	{									\
		g_warning ("Class `%s' does not implement `" #method "'\n",	\
			   g_type_name (G_TYPE_FROM_INSTANCE (obj)));		\
		return 0;							\
	}

DEFINE_UNIMPLEMENTED (begin)
DEFINE_UNIMPLEMENTED (end)

DEFINE_UNIMPLEMENTED (alloc_font)
DEFINE_UNIMPLEMENTED (  ref_font)
DEFINE_UNIMPLEMENTED (unref_font)

DEFINE_UNIMPLEMENTED (alloc_color)
DEFINE_UNIMPLEMENTED (free_color)

DEFINE_UNIMPLEMENTED (calc_text_size)
DEFINE_UNIMPLEMENTED (calc_text_size_bytes)

DEFINE_UNIMPLEMENTED (set_pen)
DEFINE_UNIMPLEMENTED (get_black)
DEFINE_UNIMPLEMENTED (draw_line)
DEFINE_UNIMPLEMENTED (draw_rect)
DEFINE_UNIMPLEMENTED (draw_text)
DEFINE_UNIMPLEMENTED (draw_spell_error)
DEFINE_UNIMPLEMENTED (fill_rect)
DEFINE_UNIMPLEMENTED (draw_pixmap)
DEFINE_UNIMPLEMENTED (draw_ellipse)
DEFINE_UNIMPLEMENTED (clear)
DEFINE_UNIMPLEMENTED (set_background_color)
DEFINE_UNIMPLEMENTED (draw_shade_line)
DEFINE_UNIMPLEMENTED (draw_panel)

DEFINE_UNIMPLEMENTED (set_clip_rectangle)
DEFINE_UNIMPLEMENTED (draw_background)
DEFINE_UNIMPLEMENTED (draw_embedded)

DEFINE_UNIMPLEMENTED (get_pixel_size)
DEFINE_UNIMPLEMENTED (get_page_width)
DEFINE_UNIMPLEMENTED (get_page_height)


static void
html_painter_init (GObject *object, HTMLPainterClass *real_klass)
{
	HTMLPainter *painter;

	painter = HTML_PAINTER (object);
	painter->color_set = html_colorset_new (NULL);

	html_font_manager_init (&painter->font_manager, painter);
	painter->font_style = GTK_HTML_FONT_STYLE_DEFAULT;
	painter->font_face = NULL;
}

static void
html_painter_class_init (GObjectClass *object_class)
{
	HTMLPainterClass *class;

	class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;
	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	class->begin = (gpointer) begin_unimplemented;
	class->end = (gpointer) end_unimplemented;

	class->alloc_font = (gpointer) alloc_font_unimplemented;
	class->ref_font   = (gpointer)   ref_font_unimplemented;
	class->unref_font = (gpointer) unref_font_unimplemented;

	class->alloc_color = (gpointer) alloc_color_unimplemented;
	class->free_color = (gpointer) free_color_unimplemented;

	class->calc_text_size = (gpointer) calc_text_size_unimplemented;
	class->calc_text_size_bytes = (gpointer) calc_text_size_bytes_unimplemented;

	class->set_pen = (gpointer) set_pen_unimplemented;
	class->get_black = (gpointer) get_black_unimplemented;
	class->draw_line = (gpointer) draw_line_unimplemented;
	class->draw_rect = (gpointer) draw_rect_unimplemented;
	class->draw_text = (gpointer) draw_text_unimplemented;
	class->draw_spell_error = (gpointer) draw_spell_error_unimplemented;
	class->fill_rect = (gpointer) fill_rect_unimplemented;
	class->draw_pixmap = (gpointer) draw_pixmap_unimplemented;
	class->draw_ellipse = (gpointer) draw_ellipse_unimplemented;
	class->clear = (gpointer) clear_unimplemented;
	class->set_background_color = (gpointer) set_background_color_unimplemented;
	class->draw_shade_line = (gpointer) draw_shade_line_unimplemented;
	class->draw_panel = (gpointer) draw_panel_unimplemented;

	class->set_clip_rectangle = (gpointer) set_clip_rectangle_unimplemented;
	class->draw_background = (gpointer) draw_background_unimplemented;
	class->draw_embedded = (gpointer) draw_embedded_unimplemented;

	class->get_pixel_size = (gpointer) get_pixel_size_unimplemented;

	class->get_page_width  = (gpointer) get_page_width_unimplemented;
	class->get_page_height = (gpointer) get_page_height_unimplemented;
}

GType
html_painter_get_type (void)
{
	static GType html_painter_type = 0;

	if (html_painter_type == 0) {
		static const GTypeInfo html_painter_info = {
			sizeof (HTMLPainterClass),
			NULL,
			NULL,
			(GClassInitFunc) html_painter_class_init,
			NULL,
			NULL,
			sizeof (HTMLPainter),
			1,
			(GInstanceInitFunc) html_painter_init,
		};
		html_painter_type = g_type_register_static (G_TYPE_OBJECT, "HTMLPainter", &html_painter_info, 0);
	}

	return html_painter_type;
}

HTMLPainter *
html_painter_new (void)
{
	return g_object_new (HTML_TYPE_PAINTER, NULL);
}


/* Functions to begin/end a painting process.  */

void
html_painter_begin (HTMLPainter *painter,
		    int x1, int y1, int x2, int y2)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->begin) (painter, x1, y1, x2, y2);
}

void
html_painter_end (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->end) (painter);
}


/* Color control.  */
void
html_painter_alloc_color (HTMLPainter *painter,
			  GdkColor *color)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (color != NULL);

	(* HP_CLASS (painter)->alloc_color) (painter, color);
}

void
html_painter_free_color (HTMLPainter *painter,
			 GdkColor *color)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (color != NULL);

	(* HP_CLASS (painter)->free_color) (painter, color);
}


/* Font handling.  */

void
html_painter_set_font_style (HTMLPainter *painter,
			     GtkHTMLFontStyle font_style)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (font_style != GTK_HTML_FONT_STYLE_DEFAULT);

	painter->font_style = font_style;
}

GtkHTMLFontStyle
html_painter_get_font_style (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, GTK_HTML_FONT_STYLE_DEFAULT);
	g_return_val_if_fail (HTML_IS_PAINTER (painter), GTK_HTML_FONT_STYLE_DEFAULT);

	return painter->font_style;
}

void
html_painter_set_font_face (HTMLPainter *painter,
			    HTMLFontFace *face)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	if (!painter->font_face || !face || strcmp (painter->font_face, face)) {
		g_free (painter->font_face);
		painter->font_face = g_strdup (face);
	}
}

gpointer
html_painter_get_font (HTMLPainter *painter, HTMLFontFace *face, GtkHTMLFontStyle style)
{
	HTMLFont *font;

	font = html_font_manager_get_font (&painter->font_manager, face, style);
	return font ? font->data : NULL;
}

void
html_painter_calc_text_size (HTMLPainter *painter,
			     const gchar *text,
			     guint len, GList *items, PangoGlyphString *glyphs, gint *line_offset,
			     GtkHTMLFontStyle font_style,
			     HTMLFontFace *face,
			     gint *width, gint *asc, gint *dsc)
{
	gchar *translated;
	gint translated_len;
	gchar *tmp = NULL;
	gint tmp_len;
	
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (text != NULL);
	g_return_if_fail (font_style != GTK_HTML_FONT_STYLE_DEFAULT);

	tmp_len = calc_translated_text_bytes (text, len, *line_offset, &translated_len, *line_offset != -1) + 1;

	if (tmp_len > HTML_ALLOCA_MAX)
		tmp = translated = g_malloc (tmp_len);
	else 
		translated = alloca (tmp_len);

	*line_offset = translate_text_special_chars (text, translated, len, *line_offset, *line_offset != -1);

	(* HP_CLASS (painter)->calc_text_size) (painter, translated, translated_len, items, glyphs, font_style, face, width, asc, dsc);
	
	g_free (tmp);
}

static gint
correct_width (const gchar *text, guint bytes_len, gint *lo, HTMLFont *font)
{
	gint delta = 0;
	gunichar uc;
	const gchar *s, *end = text + bytes_len;
	gint skip, line_offset = *lo;
	gboolean tabs = *lo != -1;

	if (!tabs) {
		if (font->space_width == font->nbsp_width) {
			if (font->space_width == font->tab_width) {
				return 0;
			} else {
				while (text < end) {
					if (*text == '\t')
						delta += font->space_width - font->tab_width;
					text ++;
				}

				return delta;
			}
		}
	}

	s = text;
	while (s < end && (uc = g_utf8_get_char (s))) {
		switch (uc) {
		case ENTITY_NBSP:
			line_offset ++;
			delta += font->space_width - font->nbsp_width;
			break;
		case '\t':
			if (tabs) {
				skip = 8 - (line_offset % 8);
				line_offset += skip;
				delta += skip * font->space_width - font->tab_width;
			} else {
				delta += font->space_width - font->tab_width;
				line_offset ++;
			}
			break;
		default:
			line_offset ++;
		}
		s = g_utf8_next_char (s);
	}

	if (tabs)
		*lo = line_offset;

	return delta;
}

void
html_painter_calc_text_size_bytes (HTMLPainter *painter,
				    const gchar *text,
				    guint bytes_len, GList *items, PangoGlyphString *glyphs, gint *line_offset,
				    HTMLFont *font, GtkHTMLFontStyle style,
				    gint *width, gint *asc, gint *dsc)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (text != NULL);
	g_return_if_fail (style != GTK_HTML_FONT_STYLE_DEFAULT);

	(* HP_CLASS (painter)->calc_text_size_bytes) (painter, text, bytes_len, items, glyphs, font, style, width, asc, dsc);
	*width += correct_width (text, bytes_len, line_offset, font);
}

/* The actual paint operations.  */

void
html_painter_set_pen (HTMLPainter *painter,
		      const GdkColor *color)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (color != NULL);

	(* HP_CLASS (painter)->set_pen) (painter, color);
}

void
html_painter_draw_line (HTMLPainter *painter,
			gint x1, gint y1,
			gint x2, gint y2)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_line) (painter, x1, y1, x2, y2);
}

void
html_painter_draw_rect (HTMLPainter *painter,
			gint x, gint y,
			gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_rect) (painter, x, y, width, height);
}

static gint
calc_translated_text_bytes (const gchar *text, gint len, gint line_offset, gint *translated_len, gboolean tabs)
{
	gunichar uc;
	const gchar *s;
	gint delta, skip, current_len;

	current_len = 0;
	delta = 0;
	*translated_len = 0;
	s = text;
	while (s && (uc = g_utf8_get_char (s)) && current_len < len) {
		switch (uc) {
		case ENTITY_NBSP:
			delta --;
			(*translated_len) ++;
			line_offset ++;
			break;
		case '\t':
			if (tabs) {
				skip = 8 - (line_offset % 8);
				delta += skip - 1;
				line_offset += skip;
				(*translated_len) += skip;
			} else {
				(*translated_len) ++;
				line_offset ++;
			}
			break;
		default:
			(*translated_len) ++;
			line_offset ++;
		}
		current_len ++;
		s = g_utf8_next_char (s);
	}

	return s - text + delta;
}

static inline void
put_last (const gchar *s, const gchar **ls, gchar **translated)
{
	if (*ls)
		for (; *ls < s; (*ls) ++, (*translated) ++)
			**translated = **ls;
}

static gint
translate_text_special_chars (const gchar *text, gchar *translated, gint len, gint line_offset, gboolean tabs)
{
	gunichar uc;
	const gchar *s, *ls;
	gint skip, current_len;

	current_len = 0;
	s = text;
	ls = NULL;
	while (s && (uc = g_utf8_get_char (s)) && current_len < len) {
		put_last (s, &ls, &translated);
		switch (uc) {
		case ENTITY_NBSP:
			*translated = ' ';
			translated ++;
			line_offset ++;
			ls = NULL;
			break;
		case '\t':
			if (tabs) {
				skip = 8 - (line_offset % 8);
				line_offset += skip;
				for (; skip; skip --, translated ++)
					*translated = ' ';
			} else {
				*translated = ' ';
				translated ++;
				line_offset ++;
			}
			ls = NULL;
			break;
		default:
			ls = s;
			line_offset ++;
		}
		current_len ++;
		s = g_utf8_next_char (s);
	}
	put_last (s, &ls, &translated);
	*translated = 0;

	return line_offset;
}

gchar *
html_painter_translate_text (const gchar *text, gint len, gint *line_offset, gint *bytes)
{
	gchar *new_text;
	gint translated_len;

	*bytes = calc_translated_text_bytes (text, len, *line_offset, &translated_len, *line_offset != -1);
	new_text = g_malloc (*bytes + 1);
	*line_offset = translate_text_special_chars (text, new_text, len, *line_offset, *line_offset != -1);

	return new_text;
}

gint
html_painter_draw_text (HTMLPainter *painter,
			gint x, gint y,
			const gchar *text, gint len, GList *items, PangoGlyphString *glyphs, gint line_offset)
{
	gchar *translated;
	gchar *tmp = NULL;
	gint translated_len;
	gint tmp_len;

	g_return_val_if_fail (painter != NULL, line_offset);
	g_return_val_if_fail (HTML_IS_PAINTER (painter), line_offset);

	tmp_len = calc_translated_text_bytes (text, len, line_offset, &translated_len, line_offset != -1) + 1;
	
	if (tmp_len > HTML_ALLOCA_MAX)
		tmp = translated = g_malloc (tmp_len);
	else 
		translated = alloca (tmp_len);

	line_offset = translate_text_special_chars (text, translated, len, line_offset, line_offset != -1);

	(* HP_CLASS (painter)->draw_text) (painter, x, y, translated, translated_len, items, glyphs);

	g_free (tmp);

	return line_offset;
}

void
html_painter_fill_rect (HTMLPainter *painter,
			gint x, gint y,
			gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->fill_rect) (painter, x, y, width, height);
}

void
html_painter_draw_pixmap (HTMLPainter    *painter,
			  GdkPixbuf *pixbuf,
			  gint x, gint y,
			  gint scale_width, gint scale_height,
			  const GdkColor *color)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (pixbuf != NULL);

	(* HP_CLASS (painter)->draw_pixmap) (painter, pixbuf, x, y, scale_width, scale_height, color);
}

void
html_painter_draw_ellipse (HTMLPainter *painter,
			   gint x, gint y,
			   gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_ellipse) (painter, x, y, width, height);
}

void
html_painter_clear (HTMLPainter *painter)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->clear) (painter);
}

void
html_painter_set_background_color (HTMLPainter *painter,
				   const GdkColor *color)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (color != NULL);

	(* HP_CLASS (painter)->set_background_color) (painter, color);
}

void
html_painter_draw_shade_line (HTMLPainter *painter,
			      gint x, gint y,
			      gint width)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_shade_line) (painter, x, y, width);
}

void
html_painter_draw_panel (HTMLPainter *painter,
			 GdkColor *bg,
			 gint x, gint y,
			 gint width, gint height,
			 GtkHTMLEtchStyle inset,
			 gint bordersize)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_panel) (painter, bg, x, y, width, height, inset, bordersize);
}

void  
html_painter_draw_embedded (HTMLPainter *painter, HTMLEmbedded *element, gint x, gint y) 
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (element != NULL);
	
	(* HP_CLASS (painter)->draw_embedded) (painter, element, x, y);
}

/* Passing 0 for width/height means remove clip rectangle */
void
html_painter_set_clip_rectangle (HTMLPainter *painter,
				 gint x, gint y,
				 gint width, gint height)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->set_clip_rectangle) (painter, x, y, width, height);
}

/* Passing 0 for pix_width / pix_height makes it use the image width */
void
html_painter_draw_background (HTMLPainter *painter,
			      GdkColor *color,
			      GdkPixbuf *pixbuf,
			      gint x, gint y,
			      gint width, gint height,
			      gint tile_x, gint tile_y)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));

	(* HP_CLASS (painter)->draw_background) (painter, color, pixbuf, x, y, width, height, tile_x, tile_y);
}

guint
html_painter_get_pixel_size (HTMLPainter *painter)
{
	g_return_val_if_fail (painter != NULL, 0);
	g_return_val_if_fail (HTML_IS_PAINTER (painter), 0);
	
	return (* HP_CLASS (painter)->get_pixel_size) (painter);
}

gint
html_painter_draw_spell_error (HTMLPainter *painter,
			       gint x, gint y,
			       const gchar *text,
			       gint len, GList *items, PangoGlyphString *glyphs)
{
	return (* HP_CLASS (painter)->draw_spell_error) (painter, x, y, text, len, items, glyphs);
}

HTMLFont *
html_painter_alloc_font (HTMLPainter *painter, gchar *face_name, gdouble size, gboolean points, GtkHTMLFontStyle style)
{
	return (* HP_CLASS (painter)->alloc_font) (painter, face_name, size, points, style);
}

void
html_painter_ref_font (HTMLPainter *painter, HTMLFont *font)
{
	(* HP_CLASS (painter)->ref_font) (painter, font);
}

void
html_painter_unref_font (HTMLPainter *painter, HTMLFont *font)
{
	(* HP_CLASS (painter)->unref_font) (painter, font);
}

guint
html_painter_get_space_width (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->space_width;
}

guint
html_painter_get_block_indent_width (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->indent_width;
}

guint
html_painter_get_block_cite_width (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->cite_width;
}

guint
html_painter_get_page_width (HTMLPainter *painter, HTMLEngine *e)
{
	return 	(* HP_CLASS (painter)->get_page_width) (painter, e);
}

guint
html_painter_get_page_height (HTMLPainter *painter, HTMLEngine *e)
{
	return 	(* HP_CLASS (painter)->get_page_height) (painter, e);
}

void
html_painter_set_focus (HTMLPainter *p, gboolean focus)
{
	p->focus = focus;
}
