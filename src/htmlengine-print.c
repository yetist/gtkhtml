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

#include "htmlprinter.h"
#include "htmlengine-print.h"


void
html_engine_print (HTMLEngine *e,
		   GnomePrintContext *print_context)
{
	HTMLPainter *printer;
	guint old_width, max_width, min_width;

	g_return_if_fail (e->clue != NULL);

	html_object_reset (e->clue);

	printer = html_printer_new (print_context);

	/* FIXME ugly hack, but this is the way it's supposed to work with the
           current ugly framework.  */

	old_width = e->width;

	max_width = e->width = html_printer_get_page_width (HTML_PRINTER (printer));
	e->clue->width = max_width;

	min_width = html_object_calc_min_width (e->clue, printer);
	if (min_width > max_width)
		max_width = min_width;

	html_object_set_max_width (e->clue, printer, max_width);
	html_object_calc_size (e->clue, printer);

	e->clue->x = 0;
	e->clue->y = e->clue->ascent;

	html_painter_begin (HTML_PAINTER (printer), 0, 0, G_MAXINT, G_MAXINT);

	html_object_draw (e->clue, HTML_PAINTER (printer), 0, 0, G_MAXINT, G_MAXINT, 0, 0);

	html_painter_end (HTML_PAINTER (printer));

	gtk_object_unref (GTK_OBJECT (printer));

	/* FIXME ugly hack pt. 2.  */
	e->width = old_width;
	html_object_reset (e->clue);
	html_engine_calc_size (e);
}
