/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik (rodo@helixcode.com)

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

#include "utils.h"

GtkWidget *
color_table_new (GtkSignalFunc f, gpointer data)
{
	GtkWidget *table, *button;
	GtkStyle *style;
	guint val, i, j, base_val;

	table = gtk_table_new (8, 8, TRUE);
	for (val=0, i=0; i<8; i++)
		for (j=0; j<8; j++, val++) {

			button = gtk_button_new ();
			gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_HALF);
			gtk_widget_set_usize (button, 16, 16);
			style = gtk_style_copy (button->style);

#define VAL (base_val << 14) | 0x1555*base_val
			base_val = (val & 12) >> 2;
			style->bg [GTK_STATE_NORMAL].red   = VAL;
			base_val = (((val & 16) >> 2) | (val & 2)) >> 1;
			style->bg [GTK_STATE_NORMAL].green = VAL;
			base_val = ((val & 32) >> 4) | (val & 1);
			style->bg [GTK_STATE_NORMAL].blue  = VAL;
			style->bg [GTK_STATE_ACTIVE] = style->bg [GTK_STATE_NORMAL];
			style->bg [GTK_STATE_PRELIGHT] = style->bg [GTK_STATE_NORMAL];
			style->bg [GTK_STATE_SELECTED] = style->bg [GTK_STATE_NORMAL];
			style->bg [GTK_STATE_INSENSITIVE] = style->bg [GTK_STATE_NORMAL];

			/* printf ("rgb: %4x %4x %4x\n",
				style->bg [GTK_STATE_NORMAL].red,
				style->bg [GTK_STATE_NORMAL].green,
				style->bg [GTK_STATE_NORMAL].blue); */

			gtk_signal_connect (GTK_OBJECT (button), "clicked", f, data);
			gtk_widget_set_style (button, style);
			gtk_table_attach_defaults (GTK_TABLE (table), button, i, i+1, j, j+1);
		}

	return table;
}
