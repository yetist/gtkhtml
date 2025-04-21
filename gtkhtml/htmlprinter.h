/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright (C) 2000 Helix Code, Inc.
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

#ifndef _HTMLPRINTER_H
#define _HTMLPRINTER_H

#include <gtk/gtk.h>
#include "htmlpainter.h"

G_BEGIN_DECLS

#define HTML_TYPE_PRINTER                 (html_printer_get_type ())
#define HTML_PRINTER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), HTML_TYPE_PRINTER, HTMLPrinter))
#define HTML_PRINTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_PRINTER, HTMLPrinterClass))
#define HTML_IS_PRINTER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HTML_TYPE_PRINTER))
#define HTML_IS_PRINTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_TYPE_PRINTER))

/* The following macros are used to convert between the HTMLEngine coordinate
 * system (which uses integers) to the GnomePrint one (which uses doubles). */

#define SCALE_ENGINE_TO_GNOME_PRINT(x) ((printer->scale * (x)) / 1024.0)
#define SCALE_GNOME_PRINT_TO_ENGINE(x) ((gint) (((x) * 1024.0 / printer->scale) + 0.5))

struct _HTMLPrinter {
	HTMLPainter base;

	GtkPrintContext *context;
	gdouble scale;
};

struct _HTMLPrinterClass {
	HTMLPainterClass base;
};

GType      html_printer_get_type                    (void);
HTMLPainter *html_printer_new                         (GtkWidget         *widget,
						       GtkPrintContext *context);
guint        html_printer_get_page_width              (HTMLPrinter       *printer);
guint        html_printer_get_page_height             (HTMLPrinter       *printer);
gdouble      html_printer_scale_to_gnome_print        (HTMLPrinter       *printer,
						       gint               x);
void         html_printer_set_scale                   (HTMLPrinter       *printer,
						       gdouble            scale);

G_END_DECLS

#endif /* _HTMLPRINTER_H */
