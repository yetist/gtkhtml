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

#ifndef _HTMLPAINTER_H_
#define _HTMLPAINTER_H_

typedef struct _HTMLPainter HTMLPainter;

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "gtkhtmlfontstyle.h"
#include "htmlfontmanager.h"
#include "htmlcolorset.h"


#define HTML_TYPE_PAINTER                 (html_painter_get_type ())
#define HTML_PAINTER(obj)                 (GTK_CHECK_CAST ((obj), HTML_TYPE_PAINTER, HTMLPainter))
#define HTML_PAINTER_CLASS(klass)         (GTK_CHECK_CLASS_CAST ((klass), HTML_TYPE_PAINTER, HTMLPainterClass))
#define HTML_IS_PAINTER(obj)              (GTK_CHECK_TYPE ((obj), HTML_TYPE_PAINTER))
#define HTML_IS_PAINTER_CLASS(klass)      (GTK_CHECK_CLASS_TYPE ((klass), HTML_TYPE_PAINTER))


typedef enum {
	GTK_HTML_ETCH_IN,
	GTK_HTML_ETCH_OUT,
	GTK_HTML_ETCH_NONE
} GtkHTMLEtchStyle; 


struct _HTMLPainter {
	GtkObject base;

	HTMLFontManager     font_manager;
	HTMLColorSet       *color_set;
	HTMLFontFace       *font_face;
	GtkHTMLFontStyle    font_style;
};

struct _HTMLPainterClass {
	GtkObjectClass   base;

	void (* begin) (HTMLPainter *painter, int x1, int y1, int x2, int y2);
	void (* end) (HTMLPainter *painter);

	HTMLFontManagerAllocFont  alloc_font;
	HTMLFontManagerFreeFont    free_font;

	void (* alloc_color) (HTMLPainter *painter, GdkColor *color);
	void (* free_color) (HTMLPainter *painter, GdkColor *color);

	guint (* calc_ascent) (HTMLPainter *p, GtkHTMLFontStyle f, HTMLFontFace *face);
	guint (* calc_descent) (HTMLPainter *p, GtkHTMLFontStyle f, HTMLFontFace *face);
	guint (* calc_text_width) (HTMLPainter *p, const gchar *text, guint len,
				   GtkHTMLFontStyle font_style, HTMLFontFace *face);

	void (* set_pen) (HTMLPainter *painter, const GdkColor *color);
	const GdkColor * (* get_black) (const HTMLPainter *painter);
	void (* draw_line) (HTMLPainter *painter, gint x1, gint y1, gint x2, gint y2);
	void (* draw_rect) (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* draw_text) (HTMLPainter *painter, gint x, gint y, const gchar *text, gint len);
#ifdef GTKHTML_HAVE_PSPELL
	void (* draw_spell_error) (HTMLPainter *painter, gint x, gint y, const gchar *text, guint off, gint len);
#endif
	void (* fill_rect) (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* draw_pixmap) (HTMLPainter *painter, GdkPixbuf *pixbuf, gint x, gint y,
			      gint scale_width, gint scale_height, const GdkColor *color);
	void (* draw_ellipse) (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* clear) (HTMLPainter *painter);
	void (* set_background_color) (HTMLPainter *painter, const GdkColor *color);
	void (* draw_shade_line) (HTMLPainter *p, gint x, gint y, gint width);
	void (* draw_panel) (HTMLPainter *painter,
			     gint x, gint y, gint width, gint height,
			     GtkHTMLEtchStyle inset, gint bordersize);

	void (* set_clip_rectangle) (HTMLPainter *painter, gint x, gint y, gint width, gint height);
	void (* draw_background) (HTMLPainter *painter, GdkColor *color, GdkPixbuf *pixbuf,
				  gint x, gint y, gint width, gint height, gint tile_x, gint tile_y);
	
	guint (* get_pixel_size) (HTMLPainter *painter);
};
typedef struct _HTMLPainterClass HTMLPainterClass;


/* Creation.  */
GtkType      html_painter_get_type   (void);
HTMLPainter *html_painter_new        (void);

/* Functions to drive the painting process.  */
void         html_painter_begin      (HTMLPainter *painter,
				      int          x1,
				      int          y1,
				      int          x2,
				      int          y2);
void         html_painter_end        (HTMLPainter *painter);

/* Color control.  */
void  html_painter_alloc_color  (HTMLPainter *painter,
				 GdkColor    *color);
void  html_painter_free_color   (HTMLPainter *painter,
				 GdkColor    *color);

/* Color set handling.  */
void            html_painter_set_color_set                           (HTMLPainter       *painter,
								      HTMLColorSet      *color_set);
const GdkColor *html_painter_get_default_background_color            (HTMLPainter       *painter);
const GdkColor *html_painter_get_default_foreground_color            (HTMLPainter       *painter);
const GdkColor *html_painter_get_default_link_color                  (HTMLPainter       *painter);
const GdkColor *html_painter_get_default_highlight_color             (HTMLPainter       *painter);
const GdkColor *html_painter_get_default_highlight_foreground_color  (HTMLPainter       *painter);
const GdkColor *html_painter_get_black                               (const HTMLPainter *painter);

/* Font handling.  */
HTMLFontFace     *html_painter_find_font_face   (HTMLPainter *p,
						 const gchar *families);
void              html_painter_set_font_style   (HTMLPainter      *p,
						 GtkHTMLFontStyle  f);
GtkHTMLFontStyle  html_painter_get_font_style   (HTMLPainter      *p);
void              html_painter_set_font_face    (HTMLPainter      *p,
						 HTMLFontFace     *f);
gpointer          html_painter_get_font         (HTMLPainter *painter,
						 HTMLFontFace *face,
						 GtkHTMLFontStyle style);
guint             html_painter_calc_ascent      (HTMLPainter      *p,
						 GtkHTMLFontStyle  f,
						 HTMLFontFace     *face);
guint             html_painter_calc_descent     (HTMLPainter      *p,
						 GtkHTMLFontStyle  f,
						 HTMLFontFace     *face);
guint             html_painter_calc_text_width  (HTMLPainter      *p,
						 const gchar      *text,
						 guint             len,
						 GtkHTMLFontStyle  font_style,
						 HTMLFontFace     *face);

/* The actual paint operations.  */
void  html_painter_set_pen               (HTMLPainter    *painter,
					  const GdkColor *color);
void  html_painter_draw_line             (HTMLPainter    *painter,
					  gint            x1,
					  gint            y1,
					  gint            x2,
					  gint            y2);
void  html_painter_draw_rect             (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);
void  html_painter_draw_text             (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  const gchar    *text,
					  gint            len);
void  html_painter_fill_rect             (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);
void  html_painter_draw_pixmap           (HTMLPainter    *painter,
					  GdkPixbuf      *pixbuf,
					  gint            x,
					  gint            y,
					  gint            scale_width,
					  gint            scale_height,
					  const GdkColor *color);
void  html_painter_draw_ellipse          (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);
void  html_painter_clear                 (HTMLPainter    *painter);
void  html_painter_set_background_color  (HTMLPainter    *painter,
					  const GdkColor *color);
void  html_painter_draw_shade_line       (HTMLPainter    *p,
					  gint            x,
					  gint            y,
					  gint            width);
void  html_painter_draw_panel            (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height,
					  GtkHTMLEtchStyle inset,
					  gint            bordersize);

/* Passing 0 for width/height means remove clip rectangle */
void  html_painter_set_clip_rectangle  (HTMLPainter *painter,
					gint         x,
					gint         y,
					gint         width,
					gint         height);

/* Passing 0 for pix_width / pix_height makes it use the image width */
void  html_painter_draw_background  (HTMLPainter *painter,
				     GdkColor *color,
				     GdkPixbuf   *pixbuf,
				     gint         x,
				     gint         y,
				     gint         width,
				     gint         height,
				     gint         tile_x,
				     gint         tile_y);

guint  html_painter_get_pixel_size  (HTMLPainter *painter);

#ifdef GTKHTML_HAVE_PSPELL
void   html_painter_draw_spell_error (HTMLPainter *painter,
				      gint x, gint y,
				      const gchar *text,
				      guint off, gint len);
#endif

#endif /* _HTMLPAINTER_H_ */
