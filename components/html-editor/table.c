/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik (rodo@helixcode.com)
                       Ariel Rios   (ariel@arcavia.com)

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

#include <unistd.h>
#include <string.h>
#include "config.h"
#include "dialog.h"
#include "htmlengine-edit-table.h"
#include "htmltable.h"

#define SPIN_INDEX 7


struct _GtkHTMLTableDialog {
	GnomeDialog *dialog;
	GtkHTML *html;
	GtkWidget *spin [SPIN_INDEX];
	
	gint values [SPIN_INDEX];
	
	/* 
	   The values stand for the following:
	   values: rows, cols, width, percent, 
	           padding, spacing, border;
	*/

};

void 
set_get_values (GtkHTMLDialog *dialog, gboolean flag)
{
	gint i = 0;
	
	if (flag)
		for (; i < SPIN_INDEX; i++)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->spin [i], (gfloat) values [i]));
	else	
		for (; i < SPIN_INDEX; i++)
			dialog->values [i] = gtk_spin_button_parse_as_int (GTK_SPIN_BUTTON (dialog->spin [i]));

}

void 
button_table_cb (GtkWidget *but, GtkHTMLLinkDialog *d)
{
	set_get_values (d, FALSE);
	
	html_engine_insert_table (d->html->engine, d->values);
}
		

GtkHTMLTableDialog *
gtk_html_table_dialog_new (GtkHTML *html)
{
	GtkHTMLTableDialog *dialog = g_new0 (GtkHTMLTableDialog, 1);
	GtkWidget *table           = gtk_table_new (5, 2, FALSE);
	GtkWidget *spin_label [SPIN_INDEX]   = { "Width", "Percent", "Padding", "Spacing", "Border" };
	gint i;
	
	dialog->dialog       =  GNOME_DIALOG (gnome_dialog_new (_("Table"), GNOME_STOCK_BUTTON_OK,
								GNOME_STOCK_BUTTON_CANCEL, NULL));
	dialog->html         =  html;

	for (i = 0; i < SPIN_INDEX; i++){	
		dialog->spin [i]  =  gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (i, 0, 999, 1, 5, 10)), 1, 0);
		spin_label [i]    = gtk_label_new (_(text_label [i]));
		gtk_table_attach_defaults (table, label [i], 0, 1, i, i+1);
		gtk_table_attach_defaults (table, spin [i],  1, 2, i, i+1); 
	}

	gtk_spin_button_set_value (GTKSPIN (spin [0], 2));
	gtk_spin_button_set_value (GTKSPIN (spin [1], 2));

	gtk_box_pack_start_defaults (GTK_BOX (box), table);
	
	gnome_dialog_button_connect (d->dialog, 0, button_link_cb, d);
	gnome_dialog_close_hides (d->dialog, TRUE);
	gnome_dialog_set_close (d->dialog, TRUE);
	gnome_dialog_set_default (d->dialog, 0);
	gtk_widget_grab_focus (d->link);
	gtk_widget_show_all (table);

	return table;

}

void 
gtk_html_link_dialog_destroy (GtkHTMLLinkDialog *d)
{
}
	
void
table_insert (GtkHTMLControlData *cd)
{
	if (cd->table_dialog)
		set_spin (cd->html->engine, cd->table_dialog->spin);

	RUN_DIALOG (table);

}

void 
table_edit (GtkHTMLControlData *cd)
{
	RUN_DIALOG (table);
	
	set_get_values (cd->table_dialog->table_dialog, TRUE);
	
}



