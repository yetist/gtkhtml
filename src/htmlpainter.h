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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "htmlfontmanager.h"
#include "htmlcolorset.h"

typedef struct _HTMLPainter HTMLPainter;

struct _HTMLPainter {
	GdkWindow *window; /* GdkWindow to draw on */

	HTMLColorSet *color_set;

	/* For the double-buffering system.  */
	gboolean double_buffer;
	GdkPixmap *pixmap;
	gint x1, y1, x2, y2;
	GdkColor background;
	gboolean set_background;
	gboolean do_clear;

	/* The current GC used.  */
	GdkGC *gc;

	/* Font handling.  */
	HTMLFontManager *font_manager;
	HTMLFontStyle font_style;

	/* Colors used for shading.  */
	GdkColor dark;
	GdkColor light;
	GdkColor black;
};


/* To drive the painting process.  */
HTMLPainter *html_painter_new        (void);
void         html_painter_destroy    (HTMLPainter *painter);
void         html_painter_begin      (HTMLPainter *painter,
				      int          x1,
				      int          y1,
				      int          x2,
				      int          y2);
void         html_painter_end        (HTMLPainter *painter);
void         html_painter_realize    (HTMLPainter *painter,
				      GdkWindow   *window);
void         html_painter_unrealize  (HTMLPainter *painter);

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
void           html_painter_set_font_style   (HTMLPainter   *p,
					      HTMLFontStyle  f);
HTMLFontStyle  html_painter_get_font_style   (HTMLPainter   *p);
guint          html_painter_calc_ascent      (HTMLPainter   *p,
					      HTMLFontStyle  f);
guint          html_painter_calc_descent     (HTMLPainter   *p,
					      HTMLFontStyle  f);
guint          html_painter_calc_text_width  (HTMLPainter   *p,
					      const gchar   *text,
					      guint          len,
					      HTMLFontStyle  font_style);

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
					  gint            x,
					  gint            y,
					  GdkPixbuf      *pixbuf,
					  gint            clipx,
					  gint            clipy,
					  gint            clipwidth,
					  gint            clipheight);
void  html_painter_draw_ellipse          (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);
void  html_painter_clear                 (HTMLPainter    *painter);
void  html_painter_set_background_color  (HTMLPainter    *painter,
					  GdkColor       *color);
void  html_painter_draw_shade_line       (HTMLPainter    *p,
					  gint            x,
					  gint            y,
					  gint            width);
void  html_painter_draw_panel            (HTMLPainter    *painter,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height,
					  gboolean        inset,
					  gint            bordersize);

/* Passing 0 for width/height means remove clip rectangle */
void  html_painter_set_clip_rectangle  (HTMLPainter *painter,
					gint         x,
					gint         y,
					gint         width,
					gint         height);

/* Passing 0 for pix_width / pix_height makes it use the image width */
void  html_painter_draw_background_pixmap  (HTMLPainter *painter,
					    gint         x,
					    gint         y,
					    GdkPixbuf   *pixbuf,
					    gint         pix_width,
					    gint         pix_height);

/* For debugging.  */
GdkWindow *html_painter_get_window  (HTMLPainter *painter);

#endif /* _HTMLPAINTER_H_ */
