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

#include <config.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include "htmlengine-edit-clueflowstyle.h"
#include "htmlengine-save.h"
#include "htmlselection.h"
#include "paragraph.h"
#include "paragraph-style.h"
#include "properties.h"
#include "utils.h"

struct _GtkHTMLEditParagraphProperties {
	GtkHTMLControlData *cd;
	GtkWidget *style_option;
};
typedef struct _GtkHTMLEditParagraphProperties GtkHTMLEditParagraphProperties;

static void
set_align (GtkWidget *w, GtkHTMLEditParagraphProperties *data)
{
	GtkHTMLParagraphAlignment align = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (w), "align"));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)) && gtk_html_get_paragraph_alignment (data->cd->html) != align)
		gtk_html_set_paragraph_alignment (data->cd->html, align);
}

GtkWidget *
paragraph_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditParagraphProperties *data = g_new0 (GtkHTMLEditParagraphProperties, 1);
	GtkWidget *hbox, *vbox, *radio, *table, *icon;
	GSList *group;

	*set_data = data;
	data->cd = cd;

	table = gtk_table_new (2, 1, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 18);
	gtk_table_set_row_spacings (GTK_TABLE (table), 18);

	data->style_option = paragraph_style_combo_box_new (cd);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new_with_mnemonic (_("_Style:")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), data->style_option, FALSE, FALSE, 0);

	gtk_table_attach (GTK_TABLE (table), editor_hig_vbox (_("General"), hbox), 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	hbox = gtk_hbox_new (FALSE, 12);

#define ADD_RADIO(x,a,stock_id) \
	radio = gtk_radio_button_new_with_label (group, x); \
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio)); \
	icon = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU); \
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0); \
	gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0); \
        if (a == gtk_html_get_paragraph_alignment (data->cd->html)) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE); \
        g_signal_connect (radio, "toggled", G_CALLBACK (set_align), data); \
        g_object_set_data (G_OBJECT (radio), "align", GINT_TO_POINTER (a));

	group = NULL;
	ADD_RADIO (_("Left"), GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT, GTK_STOCK_JUSTIFY_LEFT);
	ADD_RADIO (_("Center"), GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER, GTK_STOCK_JUSTIFY_CENTER);
	ADD_RADIO (_("Right"), GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT, GTK_STOCK_JUSTIFY_RIGHT);

	gtk_table_attach (GTK_TABLE (table), editor_hig_vbox (_("Alignment"), hbox), 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	gtk_widget_show_all (vbox);

	return vbox;
}

void
paragraph_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
