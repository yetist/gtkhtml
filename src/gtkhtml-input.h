/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright 1999, 2000 Helix Code, Inc.
    Authors:             Radek Doulik (rodo@helixcode.com)
    
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

#ifndef _GTK_HTML_INPUT_LINE_
#define _GTK_HTML_INPUT_LINE_

#include "gtkhtml.h"

GtkHTMLInputLine *      gtk_html_input_line_new           (GtkHTML *html);
void                    gtk_html_input_line_destroy       (GtkHTMLInputLine *il);
void                    gtk_html_input_line_activate      (GtkHTMLInputLine *il,
							   const gchar *label,
							   GtkSignalFunc changed,
							   GtkSignalFunc activate,
							   GtkSignalFunc key_press,
							   GtkSignalFunc finish,
							   gpointer data);
void                    gtk_html_input_line_deactivate    (GtkHTMLInputLine *il);

#endif
