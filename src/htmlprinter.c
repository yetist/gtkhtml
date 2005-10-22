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
#include <gnome.h>
#include <ctype.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-pango.h>
#include <libgnomeprint/gnome-print-paper.h>

#include "htmlembedded.h"
#include "gtkhtml-embedded.h"
#include "htmlfontmanager.h"
#include "htmlprinter.h"
#include "htmltext.h"

/* #define PRINTER_DEBUG */


static HTMLPainterClass *parent_class = NULL;


/* The size of a pixel in the printed output, in points.  */
#define PIXEL_SIZE .5

static void
insure_config (HTMLPrinter *p)
{
	if (!p->config)
		p->config = p->master ? gnome_print_job_get_config (p->master) : gnome_print_config_default ();
}

static gdouble
printer_get_page_height (HTMLPrinter *printer)
{
	gdouble width, height = 0.0;

	insure_config (printer);
	gnome_print_config_get_page_size (printer->config, &width, &height);

	return height;
}

static gdouble
printer_get_page_width (HTMLPrinter *printer)
{
	gdouble width = 0.0, height;

	insure_config (printer);
	gnome_print_config_get_page_size (printer->config, &width, &height);

	return width;
}

/* .5" in PS units */
#define TEMP_MARGIN 72*.5

static gdouble
get_lmargin (HTMLPrinter *printer)
{
	/*gdouble lmargin = 0.0;

	insure_config (printer);
	gnome_print_config_get_double (printer->config, GNOME_PRINT_KEY_PAGE_MARGIN_LEFT, &lmargin);

	printf ("lmargin %f\n", lmargin);
	return lmargin;*/

	return TEMP_MARGIN;
}

static gdouble
get_rmargin (HTMLPrinter *printer)
{
	/*gdouble rmargin = 0.0;

	insure_config (printer);
	gnome_print_config_get_double (printer->config, GNOME_PRINT_KEY_PAGE_MARGIN_RIGHT, &rmargin);

	return rmargin;*/

	return TEMP_MARGIN;
}

static gdouble
get_tmargin (HTMLPrinter *printer)
{
	/* gdouble tmargin = 0.0;

	insure_config (printer);
	gnome_print_config_get_double (printer->config, GNOME_PRINT_KEY_PAGE_MARGIN_TOP, &tmargin);

	return tmargin; */

	return TEMP_MARGIN;
}

static gdouble
get_bmargin (HTMLPrinter *printer)
{
	/* gdouble bmargin = 0.0;

	insure_config (printer);
	gnome_print_config_get_double (printer->config, GNOME_PRINT_KEY_PAGE_MARGIN_BOTTOM, &bmargin);

	return bmargin; */

	return TEMP_MARGIN;
}

gdouble
html_printer_scale_to_gnome_print (HTMLPrinter *printer, gint x)
{
	return SCALE_ENGINE_TO_GNOME_PRINT (x);
}

void
html_printer_coordinates_to_gnome_print (HTMLPrinter *printer,
					 gint engine_x, gint engine_y,
					 gdouble *print_x_return, gdouble *print_y_return)
{
	gdouble print_x, print_y;

	print_x = SCALE_ENGINE_TO_GNOME_PRINT (engine_x);
	print_y = SCALE_ENGINE_TO_GNOME_PRINT (engine_y);

	print_x = print_x + get_lmargin (printer);
	print_y = (printer_get_page_height (printer) - print_y) - get_tmargin (printer);

	*print_x_return = print_x;
	*print_y_return = print_y;
}

#if 0
static void
gnome_print_coordinates_to_engine (HTMLPrinter *printer,
				   gdouble print_x, gdouble print_y,
				   gint *engine_x_return, gint *engine_y_return)
{
	print_x -= get_lmargin (printer);
	print_y -= get_bmargin (printer);

	*engine_x_return = SCALE_ENGINE_TO_GNOME_PRINT (print_x);
	*engine_y_return = SCALE_GNOME_PRINT_TO_ENGINE (get_page_height (printer) - print_y);
}
#endif


/* GtkObject methods.  */

static void
finalize (GObject *object)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (object);

	if (printer->config != NULL) {
		/* FIXME g_object_unref (printer->config); */
		printer->config = NULL;
	}
	if (printer->context != NULL) {
		gnome_print_context_close (printer->context);
		g_object_unref (printer->context);
		printer->context = NULL;
	}

	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
begin (HTMLPainter *painter,
       int x1, int y1,
       int x2, int y2)
{
	HTMLPrinter *printer;
	GnomePrintContext *pc;
	gdouble printer_x1, printer_y1;
	gdouble printer_x2, printer_y2;
#ifdef PRINTER_DEBUG
	gdouble dash [2];
#endif
	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer);
	pc      = printer->context;
	g_return_if_fail (pc);

	gnome_print_beginpage (pc, "page");
	gnome_print_gsave (pc);

	html_printer_coordinates_to_gnome_print (printer, x1, y1, &printer_x1, &printer_y1);
	printer_x2 = printer_x1 + SCALE_ENGINE_TO_GNOME_PRINT (x2);
	printer_y2 = printer_y1 - SCALE_ENGINE_TO_GNOME_PRINT (y2);

	gnome_print_newpath (pc);

	gnome_print_moveto (pc, printer_x1, printer_y1);
	gnome_print_lineto (pc, printer_x1, printer_y2);
	gnome_print_lineto (pc, printer_x2, printer_y2);
	gnome_print_lineto (pc, printer_x2, printer_y1);
	gnome_print_lineto (pc, printer_x1, printer_y1);
	gnome_print_closepath (pc);

#ifdef PRINTER_DEBUG
	gnome_print_gsave (pc);
	dash [0] = 10.0;
	dash [1] = 10.0;
	gnome_print_setrgbcolor (pc, .5, .5, .5);
	gnome_print_setlinewidth (pc, .3);
	gnome_print_setdash (pc, 2, dash, .0);
	gnome_print_stroke (pc);
	gnome_print_grestore (pc);
#endif
	gnome_print_clip (pc);
}

static void
end (HTMLPainter *painter)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	gnome_print_grestore (printer->context);
	gnome_print_showpage (printer->context);
}

static void
clear (HTMLPainter *painter)
{
}


static void
alloc_color (HTMLPainter *painter, GdkColor *color)
{
}

static void
free_color (HTMLPainter *painter, GdkColor *color)
{
}


static void
set_pen (HTMLPainter *painter,
	 const GdkColor *color)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	gnome_print_setrgbcolor (printer->context,
				 color->red / 65535.0, color->green / 65535.0, color->blue / 65535.0);
}

static const GdkColor *
get_black (const HTMLPainter *painter)
{
	static GdkColor black = { 0, 0, 0, 0 };

	return &black;
}

static void
prepare_rectangle (HTMLPainter *painter, gint _x, gint _y, gint w, gint h)
{
	HTMLPrinter *printer = HTML_PRINTER (painter);
	GnomePrintContext *context = printer->context;
	gdouble x;
	gdouble y;
	gdouble width;
	gdouble height;

	width = SCALE_ENGINE_TO_GNOME_PRINT (w);
	height = SCALE_ENGINE_TO_GNOME_PRINT (h);

	html_printer_coordinates_to_gnome_print (HTML_PRINTER (painter), _x, _y, &x, &y);

	gnome_print_newpath (context);
	gnome_print_moveto  (context, x, y);
	gnome_print_lineto  (context, x + width, y);
	gnome_print_lineto  (context, x + width, y - height);
	gnome_print_lineto  (context, x, y - height);
	gnome_print_lineto  (context, x, y);
	gnome_print_closepath (context);
}

static void
do_rectangle (HTMLPainter *painter, gint x, gint y, gint w, gint h, gint lw)
{
	HTMLPrinter *printer = HTML_PRINTER (painter);
	GnomePrintContext *context = printer->context;

	gnome_print_setlinewidth (context, SCALE_ENGINE_TO_GNOME_PRINT (lw) * PIXEL_SIZE);
	prepare_rectangle (painter, x, y, w, h);
	gnome_print_stroke (context);
}

static void
set_clip_rectangle (HTMLPainter *painter,
		    gint x, gint y,
		    gint width, gint height)
{
	prepare_rectangle (painter, x, y, width, height);
	gnome_print_clip (HTML_PRINTER (painter)->context);
}

/* HTMLPainter drawing functions.  */

static void
draw_line (HTMLPainter *painter,
	   gint x1, gint y1,
	   gint x2, gint y2)
{
	HTMLPrinter *printer;
	double printer_x1, printer_y1;
	double printer_x2, printer_y2;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	html_printer_coordinates_to_gnome_print (printer, x1, y1, &printer_x1, &printer_y1);
	html_printer_coordinates_to_gnome_print (printer, x2, y2, &printer_x2, &printer_y2);

	gnome_print_setlinewidth (printer->context, PIXEL_SIZE);

	gnome_print_newpath (printer->context);
	gnome_print_moveto (printer->context, printer_x1, printer_y1);
	gnome_print_lineto (printer->context, printer_x2, printer_y2);

	gnome_print_stroke (printer->context);
}

static void
draw_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
	do_rectangle (painter, x, y, width, height, 1);
}

static void
draw_border (HTMLPainter *painter,
	     GdkColor *bg,
	     gint _x, gint _y,
	     gint w, gint h,
	     HTMLBorderStyle style,
	     gint bordersize)
{
	HTMLPrinter *printer = HTML_PRINTER (painter);
	GnomePrintContext *pc = printer->context;
	GdkColor *col1 = NULL, *col2 = NULL;
	GdkColor dark, light;
	gdouble x;
	gdouble y;
	gdouble width, bs;
	gdouble height;

	#define INC 0x8000
	#define DARK(c)  dark.c = MAX (((gint) bg->c) - INC, 0)
	#define LIGHT(c) light.c = MIN (((gint) bg->c) + INC, 0xffff)

	DARK(red);
	DARK(green);
	DARK(blue);
	LIGHT(red);
	LIGHT(green);
	LIGHT(blue);

	switch (style) {
	case HTML_BORDER_SOLID:
		/* do not draw peripheral border *
		col1 = bg;
		col2 = bg;
		break; */
		return;
	case HTML_BORDER_OUTSET:
		col1 = &light;
		col2 = &dark;
		break;
	default:
	case HTML_BORDER_INSET:
		col1 = &dark;
		col2 = &light;
		break;
	}

	width  = SCALE_ENGINE_TO_GNOME_PRINT (w);
	height = SCALE_ENGINE_TO_GNOME_PRINT (h);
	bs     = SCALE_ENGINE_TO_GNOME_PRINT (bordersize);

	html_printer_coordinates_to_gnome_print (HTML_PRINTER (painter), _x, _y, &x, &y);

	if (col2)
		gnome_print_setrgbcolor (pc, col1->red / 65535.0, col1->green / 65535.0, col1->blue / 65535.0);

	gnome_print_newpath (pc);
	gnome_print_moveto  (pc, x, y);
	gnome_print_lineto  (pc, x + width, y);
	gnome_print_lineto  (pc, x + width - bs, y - bs);
	gnome_print_lineto  (pc, x + bs, y - bs );
	gnome_print_lineto  (pc, x + bs, y - height + bs);
	gnome_print_lineto  (pc, x, y - height);
	gnome_print_closepath (pc);
	gnome_print_fill    (pc);

	if (col1)
		gnome_print_setrgbcolor (pc, col2->red / 65535.0, col2->green / 65535.0, col2->blue / 65535.0);

	gnome_print_newpath (pc);
	gnome_print_moveto  (pc, x, y - height);
	gnome_print_lineto  (pc, x + width, y - height);
	gnome_print_lineto  (pc, x + width, y);
	gnome_print_lineto  (pc, x + width - bs, y - bs);
	gnome_print_lineto  (pc, x + width - bs, y - height + bs);
	gnome_print_lineto  (pc, x + bs, y - height + bs);
	gnome_print_closepath (pc);
	gnome_print_fill    (pc);
}

static void
draw_background (HTMLPainter *painter,
		 GdkColor *color,
		 GdkPixbuf *pixbuf,
		 gint ix, gint iy, 
		 gint pix_width, gint pix_height,
		 gint tile_x, gint tile_y)
{
	GnomePrintContext *pc;
	HTMLPrinter *printer;
	gdouble x, y, width, height;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer);
	pc = printer->context;
	g_return_if_fail (printer->context);

	width = SCALE_ENGINE_TO_GNOME_PRINT  (pix_width);
	height = SCALE_ENGINE_TO_GNOME_PRINT (pix_height);
	html_printer_coordinates_to_gnome_print (printer, ix, iy, &x, &y);

	if (color) {
		gnome_print_setrgbcolor (pc, color->red / 65535.0, color->green / 65535.0, color->blue / 65535.0);

		gnome_print_newpath (pc);
		gnome_print_moveto (pc, x, y);
		gnome_print_lineto (pc, x + width, y);
		gnome_print_lineto (pc, x + width, y - height);
		gnome_print_lineto (pc, x, y - height);
		gnome_print_lineto (pc, x, y);
		gnome_print_closepath (pc);

		gnome_print_fill (pc);
	}
}

static void
print_pixbuf (GnomePrintContext *pc, GdkPixbuf *pixbuf)
{
	if (!pixbuf || (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB))
		return;
	
	if (gdk_pixbuf_get_has_alpha (pixbuf)) {
		gnome_print_rgbaimage (pc, 
				       gdk_pixbuf_get_pixels (pixbuf),
				       gdk_pixbuf_get_width (pixbuf),
				       gdk_pixbuf_get_height (pixbuf),
				       gdk_pixbuf_get_rowstride (pixbuf));
	} else {
		gnome_print_rgbimage (pc, 
				      gdk_pixbuf_get_pixels (pixbuf),
				      gdk_pixbuf_get_width (pixbuf),
				      gdk_pixbuf_get_height (pixbuf),
				      gdk_pixbuf_get_rowstride (pixbuf));
	}
}

static void
draw_pixmap (HTMLPainter *painter, GdkPixbuf *pixbuf, gint x, gint y, gint scale_width, gint scale_height, const GdkColor *color)
{
	HTMLPrinter *printer;
	gint width, height;
	double print_x, print_y;
	double print_scale_width, print_scale_height;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	html_printer_coordinates_to_gnome_print (printer, x, y, &print_x, &print_y);

	print_scale_width  = SCALE_ENGINE_TO_GNOME_PRINT (scale_width);
	print_scale_height = SCALE_ENGINE_TO_GNOME_PRINT (scale_height);

	gnome_print_gsave (printer->context);
	gnome_print_translate (printer->context, print_x, print_y - print_scale_height);
	gnome_print_scale (printer->context, print_scale_width, print_scale_height);
	print_pixbuf (printer->context, pixbuf);
	gnome_print_grestore (printer->context);
}

static void
fill_rect (HTMLPainter *painter, gint x, gint y, gint width, gint height)
{
	HTMLPrinter *printer;
	double printer_x, printer_y;
	double printer_width, printer_height;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	printer_width = SCALE_ENGINE_TO_GNOME_PRINT (width);
	printer_height = SCALE_ENGINE_TO_GNOME_PRINT (height);

	html_printer_coordinates_to_gnome_print (printer, x, y, &printer_x, &printer_y);

	gnome_print_newpath (printer->context);
	gnome_print_moveto (printer->context, printer_x, printer_y);
	gnome_print_lineto (printer->context, printer_x + printer_width, printer_y);
	gnome_print_lineto (printer->context, printer_x + printer_width, printer_y - printer_height);
	gnome_print_lineto (printer->context, printer_x, printer_y - printer_height);
	gnome_print_lineto (printer->context, printer_x, printer_y);
	gnome_print_closepath (printer->context);

	gnome_print_fill (printer->context);
}

static void
draw_lines (HTMLPrinter *printer, double x, double y, double width, PangoAnalysis *analysis, HTMLPangoProperties *properties)
{
	PangoFontMetrics *metrics;
	
	if (!properties->underline && !properties->strikethrough)
		return;
	
	metrics = pango_font_get_metrics (analysis->font, analysis->language);

	gnome_print_setlinecap (printer->context, GDK_CAP_BUTT);

	if (properties->underline) {
#ifdef PANGO_1_5_OR_HIGHER
		double thickness = SCALE_PANGO_TO_GNOME_PRINT (pango_font_metrics_get_underline_thickness (metrics));
		double position = SCALE_PANGO_TO_GNOME_PRINT (pango_font_metrics_get_underline_position (metrics));
		double ly = y + position - thickness / 2;
#else
		double thickness = 1.0;
		double position = -1.0;
		double ly = y + position - thickness / 2;
#endif

		gnome_print_newpath (printer->context);
		gnome_print_moveto (printer->context, x, ly);
		gnome_print_lineto (printer->context, x + width, ly);
		gnome_print_setlinewidth (printer->context, thickness);
		gnome_print_stroke (printer->context);
	}
		
	if (properties->strikethrough) {
#ifdef PANGO_1_5_OR_HIGHER
		double thickness = SCALE_PANGO_TO_GNOME_PRINT (pango_font_metrics_get_strikethrough_thickness (metrics));
		double position = SCALE_PANGO_TO_GNOME_PRINT (pango_font_metrics_get_strikethrough_position (metrics));
		double ly = y + position - thickness / 2;
#else
		double thickness = 1.0;
		double position = SCALE_PANGO_TO_GNOME_PRINT (pango_font_metrics_get_ascent (metrics)/3);
		double ly = y + position - thickness / 2;
#endif
		gnome_print_newpath (printer->context);
		gnome_print_moveto (printer->context, x, ly);
		gnome_print_lineto (printer->context, x + width, ly);
		gnome_print_setlinewidth (printer->context, thickness);
		gnome_print_stroke (printer->context);
	}
}

static gint
draw_glyphs (HTMLPainter *painter, gint x, gint y, PangoItem *item, PangoGlyphString *glyphs, GdkColor *fg, GdkColor *bg)
{
	HTMLPrinter *printer;
	gdouble print_x, print_y;
	PangoRectangle log_rect;
	HTMLPangoProperties properties;

	printer = HTML_PRINTER (painter);

	html_printer_coordinates_to_gnome_print (printer, x, y, &print_x, &print_y);

	gnome_print_gsave (printer->context);

	html_pango_get_item_properties (item, &properties);
	
	pango_glyph_string_extents (glyphs, item->analysis.font, NULL, &log_rect);
	
	if (properties.bg_color) {
		gnome_print_setrgbcolor (printer->context,
					 properties.bg_color->red / 65535.0,
					 properties.bg_color->green / 65535.0,
					 properties.bg_color->blue / 65535.0);
		gnome_print_rect_filled (printer->context,
					 print_x,
					 print_y - SCALE_PANGO_TO_GNOME_PRINT (log_rect.y + log_rect.height),
					 SCALE_PANGO_TO_GNOME_PRINT (log_rect.width),
					 SCALE_PANGO_TO_GNOME_PRINT (log_rect.height));
	}
	
	if (properties.fg_color) {
		gnome_print_setrgbcolor (printer->context,
					 properties.fg_color->red / 65535.0,
					 properties.fg_color->green / 65535.0,
					 properties.fg_color->blue / 65535.0);
	} else {
		gnome_print_setrgbcolor (printer->context, 0., 0., 0.);
	}
	
	gnome_print_moveto (printer->context, print_x, print_y);
	gnome_print_pango_glyph_string (printer->context, 
					item->analysis.font,
					glyphs);
	draw_lines (printer, print_x, print_y,
		    SCALE_PANGO_TO_GNOME_PRINT (log_rect.width),
		    &item->analysis, &properties);
	
	gnome_print_grestore (printer->context);
	
	
	return log_rect.width;
}

static void
draw_embedded (HTMLPainter *p, HTMLEmbedded *o, gint x, gint y) 
{
	gdouble print_x, print_y;	
	HTMLPrinter *printer = HTML_PRINTER(p);
	GtkWidget *embedded_widget;

	html_printer_coordinates_to_gnome_print (printer, x, y, &print_x, &print_y);
	gnome_print_gsave(printer->context); 

	gnome_print_translate(printer->context, 
			      print_x, print_y - o->height * PIXEL_SIZE);
 
	embedded_widget = html_embedded_get_widget(o);
	if (embedded_widget && GTK_IS_HTML_EMBEDDED (embedded_widget)) {
		g_signal_emit_by_name(GTK_OBJECT (embedded_widget), 0,
				      "draw_print", 
				      printer->context);
	}

	gnome_print_grestore(printer->context); 
}

static void
draw_shade_line (HTMLPainter *painter,
		 gint x, gint y,
		 gint width)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->context != NULL);

	/* FIXME */
}

static guint
get_pixel_size (HTMLPainter *painter)
{
	HTMLPrinter *printer = HTML_PRINTER (painter);

	return SCALE_GNOME_PRINT_TO_ENGINE (PIXEL_SIZE);
}

static guint
get_page_width (HTMLPainter *painter, HTMLEngine *e)
{
	return html_printer_get_page_width (HTML_PRINTER (painter));
}

static guint
get_page_height (HTMLPainter *painter, HTMLEngine *e)
{
	return html_printer_get_page_height (HTML_PRINTER (painter));
}

static void
html_printer_init (GObject *object)
{
	HTMLPrinter *printer;

	printer                = HTML_PRINTER (object);
	printer->context       = NULL;
	printer->config        = NULL;

	html_printer_set_scale (printer, 1.0);
}

static void
html_printer_class_init (GObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	parent_class = g_type_class_ref (HTML_TYPE_PAINTER);

	object_class->finalize = finalize;

	painter_class->begin = begin;
	painter_class->end = end;
	painter_class->alloc_color = alloc_color;
	painter_class->free_color = free_color;
	painter_class->set_pen = set_pen;
	painter_class->get_black = get_black;
	painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_border = draw_border;
	painter_class->draw_glyphs = draw_glyphs;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->clear = clear;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_background = draw_background;
	painter_class->get_pixel_size = get_pixel_size;
	painter_class->set_clip_rectangle = set_clip_rectangle;
	painter_class->draw_embedded = draw_embedded;
	painter_class->get_page_width = get_page_width;
	painter_class->get_page_height = get_page_height;
}

GType
html_printer_get_type (void)
{
	static GType html_printer_type = 0;

	if (html_printer_type == 0) {
		static const GTypeInfo html_printer_info = {
			sizeof (HTMLPrinterClass),
			NULL,
			NULL,
			(GClassInitFunc) html_printer_class_init,
			NULL,
			NULL,
			sizeof (HTMLPrinter),
			1,
			(GInstanceInitFunc) html_printer_init,
		};
		html_printer_type = g_type_register_static (HTML_TYPE_PAINTER, "HTMLPrinter", &html_printer_info, 0);
	}

	return html_printer_type;
}

HTMLPainter *
html_printer_new (GtkWidget *widget, GnomePrintContext *context, GnomePrintJob *master)
{
	PangoFontMap *font_map;
	HTMLPrinter *new;
	HTMLPainter *painter;

	new = g_object_new (HTML_TYPE_PRINTER, NULL);
	painter = HTML_PAINTER (new);

	g_object_ref (context);
	html_painter_set_widget (HTML_PAINTER (new), widget);
	new->context = context;
	new->master = master;

	font_map = gnome_print_pango_get_default_font_map ();
	painter->pango_context = gnome_print_pango_create_context (font_map);
	
	return HTML_PAINTER (new);
}


guint
html_printer_get_page_width (HTMLPrinter *printer)
{
	double printer_width;
	guint engine_width;

	g_return_val_if_fail (printer != NULL, 0);
	g_return_val_if_fail (HTML_IS_PRINTER (printer), 0);

	printer_width = printer_get_page_width (printer) - get_lmargin (printer) - get_rmargin (printer);
	engine_width = SCALE_GNOME_PRINT_TO_ENGINE (printer_width);

	return engine_width;
}

guint
html_printer_get_page_height (HTMLPrinter *printer)
{
	double printer_height;
	guint engine_height;

	g_return_val_if_fail (printer != NULL, 0);
	g_return_val_if_fail (HTML_IS_PRINTER (printer), 0);

	printer_height = printer_get_page_height (printer) - get_tmargin (printer) - get_bmargin (printer);
	engine_height = SCALE_GNOME_PRINT_TO_ENGINE (printer_height);

	return engine_height;
}

/**
 * html_printer_set_scale:
 * @printer: a #HTMLPrinter
 * @scale: new scale factor
 * 
 * Sets the scale factor for printer output. Output coordinates
 * will be multiplied by @scale.
 **/
void
html_printer_set_scale (HTMLPrinter *printer, gdouble scale)
{
	HTMLPainter *painter;

	g_return_if_fail (HTML_IS_PRINTER (printer));
	g_return_if_fail (scale >= 0);

	painter = HTML_PAINTER (printer);
	
	printer->scale = scale;
	painter->engine_to_pango = scale * PANGO_SCALE / 1024.;
}
