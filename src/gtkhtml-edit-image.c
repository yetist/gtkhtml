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

#include <unistd.h>
#include "config.h"
#include "gtkhtml-edit-image.h"

static void
insert (GtkWidget *w, GtkHTMLImageDialog *d)
{
	html_engine_insert_image (d->html->engine, gnome_pixmap_entry_get_filename (GNOME_PIXMAP_ENTRY (d->pentry)));
}

static void
entry_activate (GtkWidget *entry, GtkHTMLImageDialog *d)
{
	printf ("entry activate\n");
	// replace (NULL, d);
}

GtkHTMLImageDialog *
gtk_html_image_dialog_new (GtkHTML *html)
{
	GtkHTMLImageDialog *dialog = g_new (GtkHTMLImageDialog, 1);
	GtkWidget *hbox;
	GtkWidget *label;
	gchar     *dir;

	dialog->dialog         = GNOME_DIALOG (gnome_dialog_new (_("Image"), GNOME_STOCK_BUTTON_OK,
								 GNOME_STOCK_BUTTON_CANCEL, NULL));

	dialog->entry_alt      = gtk_entry_new_with_max_length (20);
	dialog->html           = html;

	dialog->pentry = gnome_pixmap_entry_new ("insert_image", _("Image selection"), TRUE);
	dir = getcwd (NULL, 0);
	gnome_pixmap_entry_set_pixmap_subdir (GNOME_PIXMAP_ENTRY (dialog->pentry), dir);
	free (dir);

	hbox = gtk_hbox_new (FALSE, 0);

	label = gtk_label_new (_("Description"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, .5);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), dialog->entry_alt);

	gtk_box_pack_start_defaults (GTK_BOX (dialog->dialog->vbox), dialog->pentry);
	gtk_box_pack_start_defaults (GTK_BOX (dialog->dialog->vbox), hbox);
	gtk_widget_show_all (hbox);
	gtk_widget_show_all (dialog->pentry);

	gnome_dialog_button_connect (dialog->dialog, 0, insert, dialog);
	gnome_dialog_close_hides (dialog->dialog, TRUE);
	gnome_dialog_set_close (dialog->dialog, TRUE);

	gnome_dialog_set_default (dialog->dialog, 0);

	gtk_signal_connect (GTK_OBJECT (dialog->entry_alt), "activate",
			    entry_activate, dialog);

	return dialog;
}
