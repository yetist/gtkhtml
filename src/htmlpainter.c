/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999, 2000 Helix Code, Inc.
   Copyright (C) 2000, 2001, 2002, 2003 Ximian, Inc.

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
#include "htmltext.h"
#include "htmltextslave.h"
#include "htmlpainter.h"


/* Convenience macro to extract the HTMLPainterClass from a GTK+ object.  */
#define HP_CLASS(obj)					\
	HTML_PAINTER_CLASS (G_OBJECT_GET_CLASS (obj))

/* Our parent class.  */
static GObjectClass *parent_class = NULL;


/* GObject methods.  */

static void
finalize (GObject *object)
{
	HTMLPainter *painter;

	painter = HTML_PAINTER (object);
	html_font_manager_finalize (&painter->font_manager);

	g_free (painter->font_face);

	if (painter->pango_context)
		g_object_unref (painter->pango_context);

	/* FIXME ownership of the color set?  */

	(* G_OBJECT_CLASS (parent_class)->finalize) (object);

	if (painter->widget) {
		g_object_unref (painter->widget);
		painter->widget = NULL;
	}
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

DEFINE_UNIMPLEMENTED (alloc_color)
DEFINE_UNIMPLEMENTED (free_color)

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
	html_font_manager_init (&painter->font_manager, painter);
	painter->font_style = GTK_HTML_FONT_STYLE_DEFAULT;
	painter->font_face = NULL;
	painter->widget = NULL;
}

static void
html_painter_real_set_widget (HTMLPainter *painter, GtkWidget *widget)
{
	if (painter->widget)
		g_object_unref (painter->widget);
	painter->widget = widget;
	g_object_ref (widget);
}

static gint
text_width (HTMLPainter *painter, PangoFontDescription *desc, const gchar *text, gint bytes)
{
	HTMLTextPangoInfo *pi;
	GList *glyphs;
	gint width = 0;

	pi = html_painter_text_itemize_and_prepare_glyphs (HTML_PAINTER (painter), desc, text, bytes, &glyphs, NULL);

	if (pi && glyphs) {
		GList *list;
		int i;
		for (list = glyphs, i = 0; list; list = list->next->next, i++) {
			PangoGlyphString *str = (PangoGlyphString *) list->data;
			for (i=0; i < str->num_glyphs; i ++)
				width += str->glyphs [i].geometry.width;
		}
	}
	if (glyphs)
		html_painter_glyphs_destroy (glyphs);
	if (pi)
		html_text_pango_info_destroy (pi);
	/* printf ("text_width %d\n", PANGO_PIXELS (width)); */
	return html_painter_pango_to_engine (painter, width);
}

static void
text_size (HTMLPainter *painter, PangoFontDescription *desc, const gchar *text, gint bytes,
	   HTMLTextPangoInfo *pi, PangoAttrList *attrs, GList *glyphs, gint start_byte_offset,
	   gint *width_out, gint *ascent_out, gint *descent_out)
{
	gboolean temp_pi = FALSE;
	gint ascent = 0;
	gint descent = 0;
	gint width = 0;
	
	if (!pi) {
		pi = html_painter_text_itemize_and_prepare_glyphs (painter, desc, text, bytes, &glyphs, attrs);
		temp_pi = TRUE;
	}

	if (pi && pi->n && glyphs) {
		GList *gl;
		PangoRectangle log_rect;
		PangoItem *item;
		PangoGlyphString *str;
		const gchar *c_text = text;
		gint c_bytes, ii;

		c_bytes = 0;
		for (gl = glyphs; gl && c_bytes < bytes; gl = gl->next) {
			str = (PangoGlyphString *) gl->data;
			gl = gl->next;
			ii = GPOINTER_TO_INT (gl->data);
			item = pi->entries [ii].item;
			pango_glyph_string_extents (str, item->analysis.font, NULL, &log_rect);
			width += log_rect.width;

			if (ascent_out || descent_out) {
				PangoFontMetrics *pfm;

				pfm = pango_font_get_metrics (item->analysis.font, item->analysis.language);
				ascent = MAX (ascent, pango_font_metrics_get_ascent (pfm));
				descent = MAX (descent, pango_font_metrics_get_descent (pfm));
				pango_font_metrics_unref (pfm);
			}
	
			c_text = g_utf8_offset_to_pointer (c_text, str->num_glyphs);
			if (*text == '\t')
				c_text ++;
			c_bytes = c_text - text;
		}
	}

	if (width_out)
		*width_out = html_painter_pango_to_engine (painter, width);
	if (ascent_out)
		*ascent_out = html_painter_pango_to_engine (painter, ascent);
	if (descent_out)
		*descent_out = html_painter_pango_to_engine (painter, descent);

	if (temp_pi) {
		if (glyphs)
			html_painter_glyphs_destroy (glyphs);
		if (pi)
			html_text_pango_info_destroy (pi);
	}
}

static HTMLFont *
html_painter_real_alloc_font (HTMLPainter *painter, gchar *face, gdouble size, gboolean points, GtkHTMLFontStyle style)
{
	PangoFontDescription *desc = NULL;
	gint space_width, space_asc, space_dsc;

	if (face) {
		desc = pango_font_description_from_string (face);
		pango_font_description_set_size (desc, (gint) size);
	}

	if (!desc || !pango_font_description_get_family (desc)) {
		if (desc)
			pango_font_description_free (desc);

		desc = pango_font_description_copy (gtk_widget_get_style (painter->widget)->font_desc);
	}

	pango_font_description_set_size (desc, size);
	pango_font_description_set_style (desc, style & GTK_HTML_FONT_STYLE_ITALIC ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_weight (desc, style & GTK_HTML_FONT_STYLE_BOLD ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);

	text_size (painter, desc, " ", 1, NULL, NULL, NULL, 0, &space_width, &space_asc, &space_dsc);

	return html_font_new (desc,
			      space_width,
			      space_asc, space_dsc,
			      text_width (painter, desc, "\xc2\xa0", 2),
			      text_width (painter, desc, "\t", 1),
			      text_width (painter, desc, "e", 1),
			      text_width (painter, desc, HTML_BLOCK_INDENT, strlen (HTML_BLOCK_INDENT)),
			      text_width (painter, desc, HTML_BLOCK_CITE, strlen (HTML_BLOCK_CITE)));
}

static void
html_painter_real_ref_font (HTMLPainter *painter, HTMLFont *font)
{
}

static void
html_painter_real_unref_font (HTMLPainter *painter, HTMLFont *font)
{
	if (font->ref_count < 1) {
		pango_font_description_free (font->data);
		font->data = NULL;
	}
}

static void
html_painter_class_init (GObjectClass *object_class)
{
	HTMLPainterClass *class;

	class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;
	parent_class = g_type_class_ref (G_TYPE_OBJECT);

	class->set_widget = html_painter_real_set_widget;
	class->begin = (gpointer) begin_unimplemented;
	class->end = (gpointer) end_unimplemented;

	class->alloc_font = html_painter_real_alloc_font;
	class->ref_font   = html_painter_real_ref_font;
	class->unref_font = html_painter_real_unref_font;

	class->alloc_color = (gpointer) alloc_color_unimplemented;
	class->free_color = (gpointer) free_color_unimplemented;

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
			     guint len, HTMLTextPangoInfo *pi, PangoAttrList *attrs, GList *glyphs, gint start_byte_offset, gint *line_offset,
			     GtkHTMLFontStyle font_style,
			     HTMLFontFace *face,
			     gint *width, gint *asc, gint *dsc)
{
	HTMLFont *font;
	
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (text != NULL);

	font = html_font_manager_get_font (&painter->font_manager, face, font_style);

	text_size (painter, (PangoFontDescription *) font->data, text, g_utf8_offset_to_pointer (text, len) - text,
		   pi, attrs, glyphs, start_byte_offset, width, asc, dsc);
	/* g_print ("calc_text_size %s %d %d %d\n", text, *width, asc ? *asc : -1, dsc ? *dsc : -1); */

	if (line_offset) {
		gint tabs;
		*width += (html_text_text_line_length (text, line_offset, len, &tabs) - len + tabs)*html_painter_get_space_width (painter, font_style, face);
	}
}

void
html_painter_calc_text_size_bytes (HTMLPainter *painter,
				   const gchar *text,
				   guint bytes_len, HTMLTextPangoInfo *pi, PangoAttrList *attrs, GList *glyphs, gint start_byte_offset, gint *line_offset,
				   HTMLFont *font, GtkHTMLFontStyle style,
				   gint *width, gint *asc, gint *dsc)
{
	g_return_if_fail (painter != NULL);
	g_return_if_fail (HTML_IS_PAINTER (painter));
	g_return_if_fail (text != NULL);
	g_return_if_fail (style != GTK_HTML_FONT_STYLE_DEFAULT);

	text_size (painter, (PangoFontDescription *) font->data, text, bytes_len, pi, attrs, glyphs, start_byte_offset, width, asc, dsc);
	
	/* g_print ("calc_text_size_bytes %d %d %d\n", *width, asc ? *asc : -1, dsc ? *dsc : -1); */
	
	if (line_offset) {
		gint tabs, len = g_utf8_pointer_to_offset (text, text + bytes_len);
		*width += (html_text_text_line_length (text, line_offset, len, &tabs) - len + tabs)*font->space_width;
	}
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

void
html_replace_tabs (const gchar *text, gchar *translated, guint bytes)
{
	const gchar *t, *tab;
	gchar *tt;

	t = text;
	tt = translated;

	do {
		tab = memchr (t, (unsigned char) '\t', bytes - (t - text));
		if (tab) {
			strncpy (tt, t, tab - t);
			tt += tab - t;
			*tt = ' ';
			tt ++;
			t = tab + 1;
		} else
			strncpy (tt, t, bytes - (t - text));
	} while (tab);
}

static inline GList *
shift_items (GList *items, gint byte_offset)
{
	if (items) {
		PangoItem *item;

		while (items && (item = (PangoItem *) items->data) && item->offset + item->length <= byte_offset)
			items = items->next;
	}

	return items;
}

static inline GList *
shift_glyphs (GList *glyphs, gint len)
{
	if (glyphs) {
		PangoGlyphString *str;

		while (glyphs && (str = (PangoGlyphString *) glyphs->data) && len > 0) {
			len -= str->num_glyphs;
			glyphs = glyphs->next->next;
		}
	}

	return glyphs;
}

gint
html_painter_draw_text (HTMLPainter *painter, gint x, gint y,
			const gchar *text, gint len, HTMLTextPangoInfo *pi, PangoAttrList *attrs, GList *glyphs, gint start_byte_offset, gint line_offset)
{
	const gchar *tab, *c_text = text;
	gint bytes, byte_offset = 0;

	g_return_val_if_fail (painter != NULL, line_offset);
	g_return_val_if_fail (HTML_IS_PAINTER (painter), line_offset);

	bytes = g_utf8_offset_to_pointer (text, len) - text;
	while ((tab = memchr (c_text, (unsigned char) '\t', bytes))) {
		gint c_bytes = tab - c_text;
		gint c_len = g_utf8_pointer_to_offset (c_text, tab);
		
		if (c_bytes)
			x += (* HP_CLASS (painter)->draw_text) (painter, x, y, c_text, c_len, pi, NULL, glyphs, start_byte_offset + (c_text - text));
		if (line_offset == -1)
			x += html_painter_get_space_width (painter, painter->font_style, painter->font_face);
		else {
			line_offset += c_len;
			x += html_painter_get_space_width (painter, painter->font_style, painter->font_face)*(8 - (line_offset % 8));
			line_offset += 8 - (line_offset % 8);
		}
		c_text += c_bytes + 1;
		bytes -= c_bytes + 1;
		byte_offset += c_bytes + 1;
		glyphs = shift_glyphs (glyphs, c_len);
		len -= c_len + 1;
	}

	(* HP_CLASS (painter)->draw_text) (painter, x, y, c_text, len, pi, attrs, glyphs, start_byte_offset + (c_text - text));

	return line_offset + len;
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
			       gint len, HTMLTextPangoInfo *pi, GList *glyphs, gint start_byte_offset)
{
	return (* HP_CLASS (painter)->draw_spell_error) (painter, x, y, text, len, pi, glyphs, start_byte_offset);
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
html_painter_get_space_asc (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->space_asc;
}

guint
html_painter_get_space_dsc (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->space_dsc;
}

guint
html_painter_get_e_width (HTMLPainter *painter, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	return html_font_manager_get_font (&painter->font_manager, face, style)->e_width;
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

/**
 * html_painter_pango_to_engine:
 * @painter: a #HTMLPainter
 * @pango_units: distance in Pango units
 * 
 * Convert a distance in Pango units (used for character layout) to
 * a distance in engine coordinates. Note that the computation is
 * only correct for positive values of @pango_units
 * 
 * Return value: distance converted to engine coordinates.
 **/
gint
html_painter_pango_to_engine (HTMLPainter       *painter,
			      gint               pango_units)
{
	return (gint)(0.5 + pango_units / painter->engine_to_pango);
}

/**
 * html_painter_engine_to_pango:
 * @painter: a #HTMLPainter
 * @engine_coordiantes: distance in Pango units
 * 
 * Convert a distance in engine coordinates to a distance in Pango
 * units (used for character layout). Note that the computation is
 * only correct for positive values of @pango_units
 * 
 * Return value: distance converted to Pango units
 **/
gint
html_painter_engine_to_pango (HTMLPainter       *painter,
			      gint               engine_units)
{
	return (gint)(0.5 + engine_units * painter->engine_to_pango);
}

void
html_painter_set_focus (HTMLPainter *p, gboolean focus)
{
	p->focus = focus;
}

void
html_painter_set_widget (HTMLPainter *painter, GtkWidget *widget)
{
	return 	(* HP_CLASS (painter)->set_widget) (painter, widget);
}

HTMLTextPangoInfo *
html_painter_text_itemize_and_prepare_glyphs (HTMLPainter *painter, PangoFontDescription *desc, const gchar *text, gint bytes, GList **glyphs, PangoAttrList *attrs)
{
	PangoAttribute *attr;
	GList *items = NULL;
	gboolean empty_attrs = (attrs == NULL);
	HTMLTextPangoInfo *pi = NULL;

	/* printf ("itemize + glyphs\n"); */

	if (empty_attrs) {
		attrs = pango_attr_list_new ();
		attr = pango_attr_font_desc_new (desc);
		attr->start_index = 0;
		attr->end_index = bytes;
		pango_attr_list_insert (attrs, attr);
	}
	items = pango_itemize (painter->pango_context, text, 0, bytes, attrs, NULL);
	if (empty_attrs)
		pango_attr_list_unref (attrs);

	if (items && items->data) {
		PangoItem *item;
		GList *il;
		const gchar *end;
		gint i = 0;

		pi = html_text_pango_info_new (g_list_length (items));

		*glyphs = NULL;
		for (il = items; il; il = il->next) {
			item = (PangoItem *) il->data;
			pi->entries [i].item = item;
			end = g_utf8_offset_to_pointer (text, item->num_chars);
			*glyphs = html_get_glyphs_non_tab (*glyphs, item, i, text, end - text, item->num_chars);
			text = end;
			i ++;
		}
		*glyphs = g_list_reverse (*glyphs);
		g_list_free (items);
	} else
		*glyphs = NULL;

	return pi;
}

void
html_painter_glyphs_destroy (GList *glyphs)
{
	GList *l;

	for (l = glyphs; l; l = l->next->next)
		pango_glyph_string_free ((PangoGlyphString *) l->data);
	g_list_free (glyphs);
}

/**
 * html_pango_get_item_properties:
 * @item: a #PangoItem
 * @properties: a #HTMLPangoProperties structure
 * 
 * Converts the list of extra attributes from @item into a more convenient
 * structure form.
 **/
void
html_pango_get_item_properties (PangoItem *item, HTMLPangoProperties *properties)
{
	GSList *tmp_list = item->analysis.extra_attrs;

	properties->underline = FALSE;
	properties->strikethrough = FALSE;
	properties->fg_color = NULL;
	properties->bg_color = NULL;

	while (tmp_list) {
		PangoAttribute *attr = tmp_list->data;
	  
		switch (attr->klass->type) {
		case PANGO_ATTR_UNDERLINE:
			properties->underline = ((PangoAttrInt *)attr)->value != PANGO_UNDERLINE_NONE;
			break;
		  
		case PANGO_ATTR_STRIKETHROUGH:
			properties->strikethrough = ((PangoAttrInt *)attr)->value;
			break;
		  
		case PANGO_ATTR_FOREGROUND:
			properties->fg_color = &((PangoAttrColor *)attr)->color;
			break;
		  
		case PANGO_ATTR_BACKGROUND:
			properties->bg_color = &((PangoAttrColor *)attr)->color;
			break;
		
		default:
			break;
		}
		tmp_list = tmp_list->next;
	}
}
