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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _HTMLENGINE_PRINT_H
#define _HTMLENGINE_PRINT_H

#include <gtk/gtk.h>
#include "htmlengine.h"

void	html_engine_print		(HTMLEngine *engine,
					 GtkPrintContext *context,
					 gdouble header_height,
					 gdouble footer_height,
					 GtkHTMLPrintCallback header_print,
					 GtkHTMLPrintCallback footer_print,
					 gpointer user_data);
gint	html_engine_print_get_pages_num	(HTMLEngine *engine,
					 GtkPrintContext *context,
					 gdouble header_height,
					 gdouble footer_height);
void	html_engine_print_set_min_split_index	(HTMLEngine *engine,
						 gdouble index);

GtkPrintOperationResult
html_engine_print_operation_run	   (HTMLEngine *engine,
				    GtkPrintOperation *operation,
				    GtkPrintOperationAction action,
				    GtkWindow *parent,
				    GtkHTMLPrintCalcHeight calc_header_height,
				    GtkHTMLPrintCalcHeight calc_footer_height,
				    GtkHTMLPrintDrawFunc draw_header,
				    GtkHTMLPrintDrawFunc draw_footer,
				    gpointer user_data,
				    GError **error);

#endif
