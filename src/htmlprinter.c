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

#include <gnome.h>
#include <libgnomeprint/gnome-print.h>

#include "htmlprinter.h"


static HTMLPainterClass *parent_class = NULL;


/* The size of a pixel in the printed output, in points.  */
#define PIXEL_SIZE 1.0

/* The following macros are used to convert between the HTMLEngine coordinate
   system (which uses integers) to the GnomePrint one (which uses doubles). */

#define SCALE_ENGINE_TO_GNOME_PRINT(x) (x / 1024.0)
#define SCALE_GNOME_PRINT_TO_ENGINE(x) ((gint) (x * 1024.0 + 0.5))

/* Hm, this might need fixing.  */
#define SPACING_FACTOR 1.2


/* This is just a quick hack until GnomePrintContext is fixed to hold the paper
   size.  */

static const GnomePaper *paper = NULL;

static void
insure_paper (void)
{
	if (paper != NULL)
		return;

	paper = gnome_paper_with_name ("A4");
	g_assert (paper != NULL);
}

static gdouble
get_page_height (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_psheight (paper);
}

static gdouble
get_page_width (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_pswidth (paper);
}

static gdouble
get_lmargin (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_lmargin (paper);
}

static gdouble
get_rmargin (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_rmargin (paper);
}

static gdouble
get_tmargin (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_tmargin (paper);
}

#if 0
static gdouble
get_bmargin (HTMLPrinter *printer)
{
	insure_paper ();
	return gnome_paper_bmargin (paper);
}
#endif


static void
engine_coordinates_to_gnome_print (HTMLPrinter *printer,
				   gint engine_x, gint engine_y,
				   gdouble *print_x_return, gdouble *print_y_return)
{
	gdouble print_x, print_y;

	print_x = SCALE_ENGINE_TO_GNOME_PRINT (engine_x);
	print_y = SCALE_ENGINE_TO_GNOME_PRINT (engine_y);

	print_x = print_x + get_lmargin (printer);
	print_y = (get_page_height (printer) - print_y) - get_tmargin (printer);

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
finalize (GtkObject *object)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (object);

	if (printer->print_context != NULL) {
		gnome_print_showpage (printer->print_context);
		gnome_print_grestore (printer->print_context);
		gnome_print_context_close (printer->print_context);
		gtk_object_unref (GTK_OBJECT (printer->print_context));
	}

	html_print_font_manager_destroy (printer->font_manager);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
begin (HTMLPainter *painter,
       int x1, int y1,
       int x2, int y2)
{
}

static void
end (HTMLPainter *painter)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	gnome_print_showpage (printer->print_context);
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
	g_return_if_fail (printer->print_context != NULL);

	gnome_print_setrgbcolor (printer->print_context,
				 color->red / 65535.0, color->green / 65535.0, color->blue / 65535.0);
}

static const GdkColor *
get_black (const HTMLPainter *painter)
{
	static GdkColor black = { 0, 0, 0, 0 };

	return &black;
}

static void
set_font_style (HTMLPainter *painter,
		HTMLFontStyle font_style)
{
	HTMLPrinter *printer;
	GnomeFont *font;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	font = html_print_font_manager_get_font (printer->font_manager, font_style);
	gnome_print_setfont (printer->print_context, font);

	printer->font_style = font_style;
}

static HTMLFontStyle
get_font_style (HTMLPainter *painter)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);

	return printer->font_style;
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
	g_return_if_fail (printer->print_context != NULL);

	engine_coordinates_to_gnome_print (printer, x1, y1, &printer_x1, &printer_y1);
	engine_coordinates_to_gnome_print (printer, x2, y2, &printer_x2, &printer_y2);

	gnome_print_moveto (printer->print_context, printer_x1, printer_y1);
	gnome_print_lineto (printer->print_context, printer_x2, printer_y2);

	gnome_print_stroke (printer->print_context);
}

static void
draw_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
	HTMLPrinter *printer;
	double printer_x, printer_y;
	double printer_width, printer_height;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	printer_width = SCALE_ENGINE_TO_GNOME_PRINT (width);
	printer_height = SCALE_ENGINE_TO_GNOME_PRINT (height);

	engine_coordinates_to_gnome_print (printer, x, y, &printer_x, &printer_y);

	gnome_print_moveto (printer->print_context, printer_x, printer_y);
	gnome_print_lineto (printer->print_context, printer_x + printer_width, printer_y);
	gnome_print_lineto (printer->print_context, printer_x + printer_width, printer_y - printer_height);
	gnome_print_lineto (printer->print_context, printer_x, printer_y - printer_height);
	gnome_print_lineto (printer->print_context, printer_x, printer_y);

	gnome_print_stroke (printer->print_context);
}

#if 0
static void
draw_panel (HTMLPainter *painter,
	    gint x, gint y,
	    gint width, gint height,
	    gboolean inset,
	    gint bordersize)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);
}
#endif

static void
draw_background_pixmap (HTMLPainter *painter,
			gint x, gint y, 
			GdkPixbuf *pixbuf,
			gint pix_width, gint pix_height)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	/* FIXME */
}

static void
draw_pixmap (HTMLPainter *painter,
	     GdkPixbuf *pixbuf,
	     gint x, gint y,
	     gint scale_width, gint scale_height,
	     const GdkColor *color)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	/* FIXME */
}

static void
fill_rect (HTMLPainter *painter,
	   gint x, gint y,
	   gint width, gint height)
{
	HTMLPrinter *printer;
	double printer_x, printer_y;
	double printer_width, printer_height;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	printer_width = SCALE_ENGINE_TO_GNOME_PRINT (width);
	printer_height = SCALE_ENGINE_TO_GNOME_PRINT (height);

	engine_coordinates_to_gnome_print (printer, x, y, &printer_x, &printer_y);

	gnome_print_moveto (printer->print_context, printer_x, printer_y);
	gnome_print_lineto (printer->print_context, printer_x + printer_width, printer_y);
	gnome_print_lineto (printer->print_context, printer_x + printer_width, printer_y - printer_height);
	gnome_print_lineto (printer->print_context, printer_x, printer_y - printer_height);
	gnome_print_lineto (printer->print_context, printer_x, printer_y);

	gnome_print_fill (printer->print_context);
}

static void
draw_text (HTMLPainter *painter,
	   gint x, gint y,
	   const gchar *text,
	   gint len)
{
	HTMLPrinter *printer;
	HTMLFontStyle font_style;
	gdouble print_x, print_y;
	gchar *text_tmp;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	engine_coordinates_to_gnome_print (printer, x, y, &print_x, &print_y);
	gnome_print_moveto (printer->print_context, print_x, print_y);

	/* Oh boy, this sucks so much.  The GnomePrint API could be improved to
           avoid this.  */
	text_tmp = alloca (len + 1);
	memcpy (text_tmp, text, len);
	text_tmp[len] = '\0';

	gnome_print_show (printer->print_context, text_tmp);

	font_style = printer->font_style;
	if (font_style & (HTML_FONT_STYLE_UNDERLINE | HTML_FONT_STYLE_STRIKEOUT)) {
		GnomeFont *font;
		double text_width;
		double ascender, descender;
		double y;

		gnome_print_gsave (printer->print_context);

		/* FIXME: We need something in GnomeFont to do this right.  */
		gnome_print_setlinewidth (printer->print_context, 1.0);
		gnome_print_setlinecap (printer->print_context, GDK_CAP_BUTT);

		font = html_print_font_manager_get_font (printer->font_manager, font_style);

		text_width = gnome_font_get_width_string_n (font, text, len);
		if (font_style & HTML_FONT_STYLE_UNDERLINE) {
			descender = gnome_font_get_descender (font);
			y = print_y - descender / 2.0;
			gnome_print_moveto (printer->print_context, print_x, y);
			gnome_print_lineto (printer->print_context, print_x + text_width, y);
			gnome_print_stroke (printer->print_context);
		}

		if (font_style & HTML_FONT_STYLE_STRIKEOUT) {
			ascender = gnome_font_get_ascender (font);
			y = print_y + ascender / 2.0;
			gnome_print_moveto (printer->print_context, print_x, y);
			gnome_print_lineto (printer->print_context, print_x + text_width, y);
			gnome_print_stroke (printer->print_context);
		}

		gnome_print_grestore (printer->print_context);
	}
}

static void
draw_shade_line (HTMLPainter *painter,
		 gint x, gint y,
		 gint width)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (painter);
	g_return_if_fail (printer->print_context != NULL);

	/* FIXME */
}

static guint
calc_ascent (HTMLPainter *painter,
	     HTMLFontStyle style)
{
	HTMLPrinter *printer;
	GnomeFont *font;
	double ascender;

	printer = HTML_PRINTER (painter);
	g_return_val_if_fail (printer->print_context != NULL, 0);

	font = html_print_font_manager_get_font (printer->font_manager, style);
	g_return_val_if_fail (font != NULL, 0);

	ascender = gnome_font_get_ascender (font) * SPACING_FACTOR;
	return SCALE_GNOME_PRINT_TO_ENGINE (ascender);
}

static guint
calc_descent (HTMLPainter *painter,
	      HTMLFontStyle style)
{
	HTMLPrinter *printer;
	GnomeFont *font;
	double descender;

	printer = HTML_PRINTER (painter);
	g_return_val_if_fail (printer->print_context != NULL, 0);

	font = html_print_font_manager_get_font (printer->font_manager, style);
	g_return_val_if_fail (font != NULL, 0);

	descender = gnome_font_get_descender (font) * SPACING_FACTOR;
	return SCALE_GNOME_PRINT_TO_ENGINE (descender);
}

static guint
calc_text_width (HTMLPainter *painter,
		 const gchar *text,
		 guint len,
		 HTMLFontStyle style)
{
	HTMLPrinter *printer;
	GnomeFont *font;
	double width;

	printer = HTML_PRINTER (painter);
	g_return_val_if_fail (printer->print_context != NULL, 0);

	font = html_print_font_manager_get_font (printer->font_manager, style);
	g_return_val_if_fail (font != NULL, 0);

	width = gnome_font_get_width_string_n (font, text, len);
	return SCALE_GNOME_PRINT_TO_ENGINE (width);
}

static guint
get_pixel_size (HTMLPainter *painter)
{
	return SCALE_GNOME_PRINT_TO_ENGINE (PIXEL_SIZE);
}


static void
init (GtkObject *object)
{
	HTMLPrinter *printer;

	printer = HTML_PRINTER (object);

	printer->print_context = NULL;

	printer->font_style = HTML_FONT_STYLE_DEFAULT;

	printer->font_manager = html_print_font_manager_new ();
}

static void
class_init (GtkObjectClass *object_class)
{
	HTMLPainterClass *painter_class;

	painter_class = HTML_PAINTER_CLASS (object_class);

	object_class->finalize = finalize;

	painter_class->begin = begin;
	painter_class->end = end;
	painter_class->alloc_color = alloc_color;
	painter_class->free_color = free_color;
	painter_class->set_font_style = set_font_style;
	painter_class->get_font_style = get_font_style;
	painter_class->calc_ascent = calc_ascent;
	painter_class->calc_descent = calc_descent;
	painter_class->calc_text_width = calc_text_width;
	painter_class->set_pen = set_pen;
	painter_class->get_black = get_black;
	painter_class->draw_line = draw_line;
	painter_class->draw_rect = draw_rect;
	painter_class->draw_text = draw_text;
	painter_class->fill_rect = fill_rect;
	painter_class->draw_pixmap = draw_pixmap;
	painter_class->clear = clear;
	painter_class->draw_shade_line = draw_shade_line;
	painter_class->draw_background_pixmap = draw_background_pixmap;
	painter_class->get_pixel_size = get_pixel_size;

	parent_class = gtk_type_class (html_painter_get_type ());
}

GtkType
html_printer_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"HTMLPrinter",
			sizeof (HTMLPrinter),
			sizeof (HTMLPrinterClass),
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		type = gtk_type_unique (HTML_TYPE_PAINTER, &info);
	}

	return type;
}


HTMLPainter *
html_printer_new (GnomePrintContext *print_context)
{
	HTMLPrinter *new;

	new = gtk_type_new (html_printer_get_type ());

	gtk_object_ref (GTK_OBJECT (print_context));
	new->print_context = print_context;
	gnome_print_gsave (new->print_context);

	return HTML_PAINTER (new);
}


guint
html_printer_get_page_width (HTMLPrinter *printer)
{
	double printer_width;
	guint engine_width;

	g_return_val_if_fail (printer != NULL, 0);
	g_return_val_if_fail (HTML_IS_PRINTER (printer), 0);

	printer_width = get_page_width (printer) - get_lmargin (printer) - get_rmargin (printer);
	engine_width = SCALE_GNOME_PRINT_TO_ENGINE (printer_width);

	return engine_width;
}
