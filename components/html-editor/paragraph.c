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

#include "paragraph.h"

struct _GtkHTMLEditParagraphProperties {
	GtkWidget *style_option;
};
typedef struct _GtkHTMLEditParagraphProperties GtkHTMLEditParagraphProperties;

static void
menu_activate (GtkWidget *mi, GtkHTMLEditParagraphProperties *data)
{
}

GtkWidget *
paragraph_properties (GtkHTMLControlData *cd)
{
	GtkHTMLEditParagraphProperties *data = g_new (GtkHTMLEditParagraphProperties, 1);
	GtkWidget *vbox, *hbox, *menu, *menuitem, *frame, *radio;
	GSList *group;

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	hbox = gtk_hbox_new (FALSE, 3);
	frame = gtk_frame_new (_("Style"));

	menu = gtk_menu_new ();

#undef ADD_ITEM
#define ADD_ITEM(n,s) \
	menuitem = gtk_menu_item_new_with_label (_(n)); \
        gtk_menu_append (GTK_MENU (menu), menuitem); \
        gtk_widget_show (menuitem); \
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (menu_activate), data); \
        gtk_object_set_data (GTK_OBJECT (menuitem), "style", GINT_TO_POINTER (s));

	ADD_ITEM ("Normal",       GTK_HTML_PARAGRAPH_STYLE_NORMAL);
	ADD_ITEM ("Header1",      GTK_HTML_PARAGRAPH_STYLE_H1);
	ADD_ITEM ("Header2",      GTK_HTML_PARAGRAPH_STYLE_H2);
	ADD_ITEM ("Header3",      GTK_HTML_PARAGRAPH_STYLE_H3);
	ADD_ITEM ("Header4",      GTK_HTML_PARAGRAPH_STYLE_H4);
	ADD_ITEM ("Header5",      GTK_HTML_PARAGRAPH_STYLE_H5);
	ADD_ITEM ("Header6",      GTK_HTML_PARAGRAPH_STYLE_H6);
	ADD_ITEM ("Address",      GTK_HTML_PARAGRAPH_STYLE_ADDRESS);
	ADD_ITEM ("Pre",          GTK_HTML_PARAGRAPH_STYLE_PRE);
	ADD_ITEM ("Item dot",     GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED);
	ADD_ITEM ("Item roman",   GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN);
	ADD_ITEM ("Item digit",   GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT);

	data->style_option = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->style_option), menu);

	gtk_box_pack_start (GTK_BOX (hbox), data->style_option, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	frame = gtk_frame_new (_("Align"));
	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_border_width (GTK_CONTAINER (hbox), 3);

#define ADD_RADIO(x) \
	radio = gtk_radio_button_new_with_label (group, x); \
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio)); \
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);


	group = NULL;
	ADD_RADIO (_("Left"));
	ADD_RADIO (_("Center"));
	ADD_RADIO (_("Right"));

	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	return vbox;
}

void
paragraph_apply_cb (GtkHTMLControlData *cd)
{
}
