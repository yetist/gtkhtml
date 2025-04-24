/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 * Copyright (C) 1997 Torben Weis (weis@kde.org)
 * Copyright (C) 1999, 2000 Helix Code, Inc.
 * Copyright 2025 Xiaotian Wu <yetist@gmail.com>
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

#ifndef _HTMLPAINTER_H_
#define _HTMLPAINTER_H_

#include <gtk/gtk.h>

#include "gtkhtml-enums.h"
#include "htmltypes.h"
#include "htmlfontmanager.h"

G_BEGIN_DECLS

#define HTML_TYPE_PAINTER                 (html_painter_get_type ())
#define HTML_PAINTER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_TYPE_PAINTER, HTMLPainter))
#define HTML_PAINTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_PAINTER, HTMLPainterClass))
#define HTML_IS_PAINTER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HTML_TYPE_PAINTER))
#define HTML_IS_PAINTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_TYPE_PAINTER))
#define HTML_PAINTER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), HTML_TYPE_PAINTER, HTMLPainterClass))


struct _HTMLPainter {
	GObject base;

	GtkWidget          *widget;
	HTMLFontManager     font_manager;
	HTMLFontFace       *font_face;
	GtkHTMLFontStyle    font_style;
	PangoContext       *pango_context;

	gdouble  engine_to_pango; /* Scale factor for engine coordinates => Pango coordinates */
	gboolean focus;

	gint clip_x, clip_y, clip_width, clip_height;
};

struct _HTMLPainterClass {
	GObjectClass   base;

	void (* set_widget)       (HTMLPainter *painter, GtkWidget *widget);
	void (* begin)            (HTMLPainter *painter, gint x1, gint y1, gint x2, gint y2);
	void (* end)              (HTMLPainter *painter);

        HTMLFont * (* alloc_font) (HTMLPainter *p, gchar *face_name, gdouble size, gboolean points, GtkHTMLFontStyle  style);
	void       (* ref_font)   (HTMLPainter *p, HTMLFont *font);
	void       (* unref_font) (HTMLPainter *p, HTMLFont *font);

	void (* alloc_color)      (HTMLPainter *painter, GdkColor *color);
	void (* free_color)       (HTMLPainter *painter, GdkColor *color);

	void (* set_pen)          (HTMLPainter *painter, const GdkColor *color);
	const GdkColor * (* get_black) (const HTMLPainter *painter);
	void (* draw_line)        (HTMLPainter *painter, gint x1, gint y1, gint x2, gint y2);
	void (* draw_rect)        (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	gint (* draw_glyphs)      (HTMLPainter *painter, gint x, gint y, PangoItem *item, PangoGlyphString *glyphs, GdkColor *fg, GdkColor *bg);
	gint (* draw_spell_error) (HTMLPainter *painter, gint x, gint y, gint width);
	void (* fill_rect)        (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* draw_pixmap)      (HTMLPainter *painter, GdkPixbuf *pixbuf,
				   gint x, gint y,
				   gint scale_width,
				   gint scale_height,
				   const GdkColor *color);
	void (* draw_ellipse)     (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* clear)            (HTMLPainter *painter);
	void (* set_background_color) (HTMLPainter *painter, const GdkColor *color);
	void (* draw_shade_line)  (HTMLPainter *p, gint x, gint y, gint width);
	void (* draw_border)      (HTMLPainter *painter, GdkColor *bg,
				   gint x, gint y, gint width, gint height,
				   HTMLBorderStyle style, gint bordersize);

	void (* set_clip_rectangle) (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* draw_background)    (HTMLPainter *painter, GdkColor *color, GdkPixbuf *pixbuf,
				     gint x, gint y, gint width, gint height, gint tile_x, gint tile_y);
	guint (* get_pixel_size)    (HTMLPainter *painter);
	void (* draw_embedded)      (HTMLPainter *painter, HTMLEmbedded *element, gint x, gint y);

	guint (*get_page_width)     (HTMLPainter *painter, HTMLEngine *e);
	guint (*get_page_height)    (HTMLPainter *painter, HTMLEngine *e);
};

struct _HTMLPangoProperties {
	gboolean        underline;
	gboolean        strikethrough;
	PangoColor     *fg_color;
	PangoColor     *bg_color;
};


/* Creation.  */
GType             html_painter_get_type                                (void);

void              html_painter_set_widget                              (HTMLPainter       *painter,
									GtkWidget         *widget);

/* Functions to drive the painting process.  */
void              html_painter_begin                                   (HTMLPainter       *painter,
									gint                x1,
									gint                y1,
									gint                x2,
									gint                y2);
void              html_painter_end                                     (HTMLPainter       *painter);

/* Color control.  */
void              html_painter_alloc_color                             (HTMLPainter       *painter,
									GdkColor          *color);
void              html_painter_free_color                              (HTMLPainter       *painter,
									GdkColor          *color);

/* Color set handling.  */
const GdkColor   *html_painter_get_default_background_color            (HTMLPainter       *painter);
const GdkColor   *html_painter_get_default_foreground_color            (HTMLPainter       *painter);
const GdkColor   *html_painter_get_default_link_color                  (HTMLPainter       *painter);
const GdkColor   *html_painter_get_default_highlight_color             (HTMLPainter       *painter);
const GdkColor   *html_painter_get_default_highlight_foreground_color  (HTMLPainter       *painter);
const GdkColor   *html_painter_get_black                               (const HTMLPainter *painter);

/* Font handling.  */
HTMLFontFace     *html_painter_find_font_face                          (HTMLPainter       *p,
									const gchar       *families);
void              html_painter_set_font_style                          (HTMLPainter       *p,
									GtkHTMLFontStyle   f);
GtkHTMLFontStyle  html_painter_get_font_style                          (HTMLPainter       *p);
void              html_painter_set_font_face                           (HTMLPainter       *p,
									HTMLFontFace      *f);
gpointer          html_painter_get_font                                (HTMLPainter       *painter,
									HTMLFontFace      *face,
									GtkHTMLFontStyle   style);
void              html_painter_calc_entries_size                       (HTMLPainter       *p,
									const gchar       *text,
									guint              len,
									HTMLTextPangoInfo *pi,
									GList             *glyphs,
									gint              *line_offset,
									gint              *width,
									gint              *asc,
									gint              *dsc);
void              html_painter_calc_text_size                          (HTMLPainter       *p,
									const gchar       *text,
									guint              len,
									gint              *width,
									gint              *asc,
									gint              *dsc);

/* The actual paint operations.  */
void              html_painter_set_pen                                 (HTMLPainter       *painter,
									const GdkColor    *color);
void              html_painter_draw_line                               (HTMLPainter       *painter,
									gint               x1,
									gint               y1,
									gint               x2,
									gint               y2);
void              html_painter_draw_rect                               (HTMLPainter       *painter,
									gint               x,
									gint               y,
									gint               width,
									gint               height);
void              html_painter_draw_text                               (HTMLPainter       *painter,
									gint               x,
									gint               y,
									const gchar       *text,
									gint               len);
gint               html_painter_draw_glyphs                             (HTMLPainter       *painter,
									gint                x,
									gint                y,
									PangoItem         *item,
									PangoGlyphString  *glyphs,
									GdkColor          *fg,
									GdkColor          *bg);
void              html_painter_draw_entries                            (HTMLPainter       *painter,
									gint               x,
									gint               y,
									const gchar       *text,
									gint               len,
									HTMLTextPangoInfo *pi,
									GList             *glyphs,
									gint               line_offset);
void              html_painter_fill_rect                               (HTMLPainter       *painter,
									gint               x,
									gint               y,
									gint               width,
									gint               height);
void              html_painter_draw_pixmap                             (HTMLPainter       *painter,
									GdkPixbuf         *pixbuf,
									gint               x,
									gint               y,
									gint               scale_width,
									gint               scale_height,
									const GdkColor    *color);
void              html_painter_draw_ellipse                            (HTMLPainter       *painter,
									gint               x,
									gint               y,
									gint               width,
									gint               height);
void              html_painter_clear                                   (HTMLPainter       *painter);
void              html_painter_set_background_color                    (HTMLPainter       *painter,
									const GdkColor    *color);
void              html_painter_draw_shade_line                         (HTMLPainter       *p,
									gint               x,
									gint               y,
									gint               width);
void              html_painter_draw_border                             (HTMLPainter       *painter,
									GdkColor          *bg,
									gint               x,
									gint               y,
									gint               width,
									gint               height,
									HTMLBorderStyle    style,
									gint               bordersize);

/* Passing 0 for width/height means remove clip rectangle */
void              html_painter_set_clip_rectangle                      (HTMLPainter       *painter,
									gint               x,
									gint               y,
									gint               width,
									gint               height);
void              html_painter_get_clip_rectangle                      (HTMLPainter       *painter,
									gint              *x,
									gint              *y,
									gint              *width,
									gint              *height);

/* Passing 0 for pix_width / pix_height makes it use the image width */
void              html_painter_draw_background                         (HTMLPainter       *painter,
									GdkColor          *color,
									GdkPixbuf         *pixbuf,
									gint               x,
									gint               y,
									gint               width,
									gint               height,
									gint               tile_x,
									gint               tile_y);
guint             html_painter_get_pixel_size                          (HTMLPainter       *painter);
gint              html_painter_draw_spell_error                        (HTMLPainter       *painter,
									gint                x,
									gint                y,
									gint                width);
HTMLFont         *html_painter_alloc_font                              (HTMLPainter       *painter,
									gchar             *face_name,
									gdouble            size,
									gboolean           points,
									GtkHTMLFontStyle   style);
void              html_painter_ref_font                                (HTMLPainter       *painter,
									HTMLFont          *font);
void              html_painter_unref_font                              (HTMLPainter       *painter,
									HTMLFont          *font);
guint             html_painter_get_space_width                         (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face);
guint             html_painter_get_space_asc                           (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face);
guint             html_painter_get_space_dsc                           (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face);
guint             html_painter_get_e_width                             (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face);
guint             html_painter_get_block_indent_width                  (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face);
guint             html_painter_get_block_cite_width                    (HTMLPainter       *painter,
									GtkHTMLFontStyle   font_style,
									HTMLFontFace      *face,
									HTMLDirection      dir);
void              html_painter_draw_embedded                           (HTMLPainter       *painter,
									HTMLEmbedded      *element,
									gint               x,
									gint               y);
guint             html_painter_get_page_width                          (HTMLPainter       *painter,
									HTMLEngine        *e);
guint             html_painter_get_page_height                         (HTMLPainter       *painter,
									HTMLEngine        *e);
gint              html_painter_pango_to_engine                         (HTMLPainter       *painter,
									gint               pango_units);
gint              html_painter_engine_to_pango                         (HTMLPainter       *painter,
									gint               engine_units);
void              html_painter_set_focus                               (HTMLPainter       *painter,
									gboolean           focus);
void              html_replace_tabs                                    (const gchar       *text,
									gchar             *translated,
									guint              bytes);

HTMLTextPangoInfo *html_painter_text_itemize_and_prepare_glyphs  (HTMLPainter           *painter,
								  PangoFontDescription  *desc,
								  const gchar           *text,
								  gint                   bytes,
								  GList                **glyphs,
								  PangoAttrList         *attrs);
void               html_painter_glyphs_destroy                   (GList                 *glyphs);

/* Retrieves the properties we care about for a single run in an
 * easily accessible structure rather than a list of attributes
 */
void html_pango_get_item_properties (PangoItem *item, HTMLPangoProperties *properties);

#define HTML_BLOCK_INDENT   "        "
#define HTML_BLOCK_CITE_LTR ">"
#define HTML_BLOCK_CITE_RTL "<"
#define HTML_ALLOCA_MAX     2048

G_END_DECLS
#endif /* _HTMLPAINTER_H_ */
