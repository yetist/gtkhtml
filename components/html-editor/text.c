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

#include "text.h"

struct _GtkHTMLEditTextProperties {
	GtkWidget *style_option;
	GtkWidget *sel_size;
};
typedef struct _GtkHTMLEditTextProperties GtkHTMLEditTextProperties;

static void
menu_activate (GtkWidget *mi, GtkHTMLEditTextProperties *data)
{
}

GtkWidget *
text_properties (GtkHTMLControlData *cd)
{
	GtkHTMLEditTextProperties *data = g_new (GtkHTMLEditTextProperties, 1);
	GtkWidget *vbox, *hbox, *frame, *table, *cpicker, *table1, *button, *check, *menu, *menuitem;
	GtkStyle *style;
	gint i, j;
	guint val, add_val;

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);

	frame = gtk_frame_new (_("Style"));
	vbox = gtk_vbox_new (FALSE, 2);

#define ADD_CHECK(x) \
	check = gtk_check_button_new_with_label (x); \
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);

	ADD_CHECK (_("Bold"));
	ADD_CHECK (_("Italic"));
	ADD_CHECK (_("Underline"));
	ADD_CHECK (_("Strikeout"));

	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);

	frame = gtk_frame_new (_("Size"));
	menu = gtk_menu_new ();

#undef ADD_ITEM
#define ADD_ITEM(n) \
	menuitem = gtk_menu_item_new_with_label (_(n)); \
        gtk_menu_append (GTK_MENU (menu), menuitem); \
        gtk_widget_show (menuitem); \
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (menu_activate), data);

	ADD_ITEM("-2");
	ADD_ITEM("-1");
	ADD_ITEM("+0");
	ADD_ITEM("+1");
	ADD_ITEM("+2");
	ADD_ITEM("+3");
	ADD_ITEM("+4");

	data->sel_size = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->sel_size), menu);
	gtk_container_add (GTK_CONTAINER (frame), data->sel_size);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 1, 2);

	/* color selection */
	frame = gtk_frame_new (_("Color"));
	cpicker = gnome_color_picker_new ();
	vbox = gtk_vbox_new (FALSE, 2);
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);
	gtk_box_pack_start (GTK_BOX (hbox), cpicker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("text color")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	table1 = gtk_table_new (8, 8, TRUE);
	for (val=0, i=0; i<8; i++)
		for (j=0; j<8; j++, val++) {

			button = gtk_button_new ();
			gtk_widget_set_usize (button, 24, 16);
			style = gtk_style_copy (button->style);
			add_val = (val * 0x3fff / 0x3f);

			style->bg [GTK_STATE_NORMAL].red   = ((val & 12) << 12) | add_val;
			style->bg [GTK_STATE_NORMAL].green = ((((val & 16) >> 2) | (val & 2))<< 13) | add_val;
			style->bg [GTK_STATE_NORMAL].blue  = ((((val & 32) >> 4) | (val & 1))<< 14) | add_val;
			style->bg [GTK_STATE_ACTIVE] = style->bg [GTK_STATE_NORMAL];
			style->bg [GTK_STATE_PRELIGHT] = style->bg [GTK_STATE_NORMAL];

			gtk_widget_set_style (button, style);
			gtk_table_attach_defaults (GTK_TABLE (table1), button, i, i+1, j, j+1);
		}

	gtk_box_pack_start (GTK_BOX (vbox), table1, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 2);

	return table;
}

void
text_apply_cb (GtkHTMLControlData *cd)
{
}
