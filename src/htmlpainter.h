/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#include "htmlfont.h"

typedef struct _HTMLPainter HTMLPainter;

/*
 * To drive the painting process
 */
HTMLPainter *html_painter_new          (void);
void         html_painter_destroy      (HTMLPainter *painter);
void         html_painter_begin     (HTMLPainter *painter, int x1, int y1, int x2, int y2);
void         html_painter_end       (HTMLPainter *painter);
void         html_painter_realize   (HTMLPainter *painter, GdkWindow *window);
void         html_painter_unrealize (HTMLPainter *painter);

/*
 * The actual paint operations
 */
void         html_painter_set_pen      (HTMLPainter *painter, GdkColor *color);
void         html_painter_draw_rect    (HTMLPainter *painter, gint x, gint y, gint width, gint height);
void         html_painter_draw_text    (HTMLPainter *painter, gint x, gint y, gchar *text, gint len);
void         html_painter_set_font     (HTMLPainter *p, HTMLFont *f);
void         html_painter_fill_rect    (HTMLPainter *painter, gint x, gint y, gint width, gint height);
void         html_painter_draw_pixmap  (HTMLPainter *painter, gint x, gint y, GdkPixbuf *pixbuf);
void         html_painter_draw_ellipse (HTMLPainter *painter, gint x, gint y, gint width, gint height);
void         html_painter_clear        (HTMLPainter *painter);

void         html_painter_set_background_color (HTMLPainter *painter, GdkColor *color);
void         html_painter_draw_shade_line (HTMLPainter *p, gint x, gint y, gint width);

HTMLFont    *html_painter_get_font (HTMLPainter *p);

/*
 * For debugging
 */
GdkWindow   *html_painter_get_window (HTMLPainter *painter);

#endif /* _HTMLPAINTER_H_ */



