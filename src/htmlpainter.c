#include <glib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "htmlpainter.h"
#include "htmlfont.h"

void
html_painter_draw_ellipse (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_arc (painter->window, painter->gc, TRUE,
		      x, y, width, height, 0, 360 * 64);
}

void
html_painter_draw_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_rectangle (painter->window, painter->gc, FALSE, x, y, width, height);
}

void
html_painter_draw_pixmap (HTMLPainter *painter, gint x, gint y, GdkPixBuf *pixbuf)
{
	/* FIXME: Support alpha */
	gdk_draw_rgb_image (painter->window,
			    painter->gc,
			    x, y,
			    pixbuf->art_pixbuf->width,
			    pixbuf->art_pixbuf->height,
			    GDK_RGB_DITHER_NORMAL,
			    pixbuf->art_pixbuf->pixels,
			    pixbuf->art_pixbuf->rowstride);
}

void
html_painter_fill_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	gdk_draw_rectangle (painter->window, painter->gc, TRUE, x, y, width, height);
}

void
html_painter_set_background_color (HTMLPainter *painter, GdkColor *color)
{
	if (color)
		gdk_window_set_background (painter->window, color);
}

void
html_painter_set_pen (HTMLPainter *painter, GdkColor *color)
{
	gdk_gc_set_foreground (painter->gc, color);
}

void
html_painter_draw_text (HTMLPainter *painter, gint x, gint y, gchar *text, gint len)
{
	if (len == -1)
		len = strlen (text);

	gdk_draw_text (painter->window, painter->font->gdk_font, painter->gc, x, y, text, len);
	
	if (painter->font->underline) {
		gdk_draw_line (painter->window, painter->gc, 
			       x, 
			       y + 1, 
			       x + html_font_calc_width (painter->font,
							 text, len), 
			       y + 1);
	}

}

void
html_painter_draw_shade_line (HTMLPainter *p, gint x, gint y, gint width)
{
	static GdkColor* dark = NULL;
	static GdkColor* light = NULL;

	if (!dark) {
		dark = g_new0 (GdkColor, 1);
		dark->red = 32767;
		dark->green = 32767;
		dark->blue = 32767;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->window), dark, TRUE, TRUE);
	}
	
	if (!light) {
		light = g_new0 (GdkColor, 1);
		light->red = 49151;
		light->green = 49151;
		light->blue = 49151;
		gdk_colormap_alloc_color (gdk_window_get_colormap (p->window), light, TRUE, TRUE);
	}
	gdk_gc_set_foreground (p->gc, dark);
	gdk_draw_line (p->window, p->gc, x, y, x+width, y);
	gdk_gc_set_foreground (p->gc, light);
	gdk_draw_line (p->window, p->gc, x, y + 1, x+width, y + 1);
}

void
html_painter_set_font (HTMLPainter *p, HTMLFont *f)
{
	p->font = f;
}

HTMLPainter *
html_painter_new (void)
{
	HTMLPainter *painter;

	painter = g_new0 (HTMLPainter, 1);

	return painter;
}
