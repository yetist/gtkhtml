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


static void
print_page (HTMLPainter *painter,
	    HTMLEngine *engine,
	    gint start_y,
	    gint page_width, gint page_height)
{
	gint end_y;

	end_y = start_y + page_height;

	html_painter_begin (painter, 0, 0, page_width, end_y - start_y);

	html_object_draw (engine->clue, painter,
			  0, start_y,
			  page_width, end_y - start_y,
			  0, -start_y);

	html_painter_end (painter);
}

static void
print_all_pages (HTMLPainter *printer,
		 HTMLEngine *engine)
{
	gint new_split_offset, split_offset;
	gint page_width, page_height;
	gint document_height;

	page_height = html_printer_get_page_height (HTML_PRINTER (printer));
	page_width = html_printer_get_page_width (HTML_PRINTER (printer));

	split_offset = 0;

	document_height = html_engine_get_doc_height (engine);

	do {
		new_split_offset = html_object_check_page_split (engine->clue,
								 split_offset + page_height);

		if (new_split_offset <= split_offset)
			new_split_offset = split_offset + page_height;

		print_page (printer, engine, split_offset, page_width, new_split_offset- split_offset);

		split_offset = new_split_offset;
	}  while (split_offset < document_height);
}


void
html_engine_print (HTMLEngine *engine,
		   GnomePrintContext *print_context)
{
	HTMLPainter *printer;
	HTMLPainter *old_painter;
	guint old_width, max_width, min_width;

	g_return_if_fail (engine->clue != NULL);

	old_width   = engine->width;
	old_painter = engine->painter;
	printer     = html_printer_new (print_context);

	max_width = engine->width = html_printer_get_page_width (HTML_PRINTER (printer));
	html_engine_set_painter (engine, printer, max_width);

	print_all_pages (HTML_PAINTER (printer), engine);


	engine->width = old_width;
	html_engine_set_painter (engine, old_painter, old_width);
	gtk_object_unref (GTK_OBJECT (printer));
}
