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


#include "config.h"
#include "properties.h"

#define GTK_HTML_EDIT_IMAGE_BWIDTH      0
#define GTK_HTML_EDIT_IMAGE_WIDTH       1
#define GTK_HTML_EDIT_IMAGE_HEIGHT      2
#define GTK_HTML_EDIT_IMAGE_HSPACE      3
#define GTK_HTML_EDIT_IMAGE_VSPACE      4
#define GTK_HTML_EDIT_IMAGE_SPINS       5

#define ALIGN_TOP    0
#define ALIGN_CENTER 1
#define ALIGN_BOTTOM 2

struct _GtkHTMLEditPropertiesDialog {
	GtkWidget           *dialog;
	GtkHTMLControlData  *control_data;

	GList               *prop_apply_cb;
	GtkWidget           *notebook;
};


GtkHTMLEditPropertiesDialog *
gtk_html_edit_properties_dialog_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditPropertiesDialog *d = g_new (GtkHTMLEditPropertiesDialog, 1);

	d->prop_apply_cb  = NULL;
	d->dialog         = gnome_dialog_new (_("Image"),
					      GNOME_STOCK_BUTTON_OK,
					      GNOME_STOCK_BUTTON_APPLY,
					      GNOME_STOCK_BUTTON_CANCEL, NULL);
	d->notebook = gtk_notebook_new ();
	gtk_box_pack_start_defaults (GTK_BOX (GNOME_DIALOG (d->dialog)->vbox), d->notebook);

	gnome_dialog_set_close (GNOME_DIALOG (d->dialog), TRUE);

	return d;
}

void
gtk_html_edit_properties_dialog_destroy (GtkHTMLEditPropertiesDialog *d)
{
}

void
gtk_html_edit_properties_dialog_add_entry (GtkHTMLEditPropertiesDialog *d,
					   GtkWidget *w,
					   const gchar *name,
					   GtkHTMLEditPropertyApplyFunc apply_cb)
{
	d->prop_apply_cb = g_list_append (d->prop_apply_cb, apply_cb);
	gtk_notebook_append_page (GTK_NOTEBOOK (d->notebook), w, gtk_label_new (name));
}

void
gtk_html_edit_properties_dialog_show (GtkHTMLEditPropertiesDialog *d)
{
	gtk_window_set_modal (GTK_WINDOW (d->dialog), TRUE);
	gtk_widget_show_all (d->dialog);
}
