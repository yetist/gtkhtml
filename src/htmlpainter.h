#ifndef _HTMLPAINTER_H_
#define _HTMLPAINTER_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "htmlfont.h"

struct _HTMLPainter {
	GdkWindow *window; /* GdkWindow to draw on */
	GdkGC *gc; /* The current GC used */
	HTMLFont *font; /* The current font */
};

typedef struct _HTMLPainter HTMLPainter;

HTMLPainter *html_painter_new (void);
void         html_painter_set_pen (HTMLPainter *painter, GdkColor *color);
void         html_painter_draw_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height);
void         html_painter_draw_text (HTMLPainter *painter, gint x, gint y, gchar *text, gint len);
void         html_painter_set_font (HTMLPainter *p, HTMLFont *f);
void         html_painter_fill_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height);
void         html_painter_draw_pixmap (HTMLPainter *painter, gint x, gint y, GdkPixbuf *pixbuf);
void         html_painter_set_background_color (HTMLPainter *painter, GdkColor *color);
void         html_painter_draw_shade_line (HTMLPainter *p, gint x, gint y, gint width);
void         html_painter_draw_ellipse (HTMLPainter *painter, gint x, gint y, gint width, gint height);

#endif /* _HTMLPAINTER_H_ */


