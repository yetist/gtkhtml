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

	GList               *page_data;
	GtkWidget           *notebook;
};

struct _PageData {
	GtkHTMLEditPropertyType      type;
	GtkHTMLEditPropertyApplyFunc apply;
	GtkHTMLEditPropertyCloseFunc close;
	gpointer data;
};
typedef struct _PageData PageData;

static void
apply_cb (PageData *pd, GtkHTMLEditPropertiesDialog *d)
{
	(*pd->apply) (d->control_data, pd->data);
}

static void
apply (GtkWidget *w, GtkHTMLEditPropertiesDialog *d)
{
	g_list_foreach (d->page_data, (GFunc) apply_cb, d);
	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 0, FALSE);
	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 1, FALSE);
}

static void
prop_close (GtkWidget *w, GtkHTMLEditPropertiesDialog *d)
{
	gnome_dialog_close (GNOME_DIALOG (d->dialog));
	gtk_html_edit_properties_dialog_destroy (d);
}

static void
ok (GtkWidget *w, GtkHTMLEditPropertiesDialog *d)
{
	apply (w, d);
	prop_close (w,d);
}

GtkHTMLEditPropertiesDialog *
gtk_html_edit_properties_dialog_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditPropertiesDialog *d = g_new (GtkHTMLEditPropertiesDialog, 1);

	d->page_data      = NULL;
	d->control_data   = cd;
	d->dialog         = gnome_dialog_new (_("Properties"),
					      GNOME_STOCK_BUTTON_OK,
					      GNOME_STOCK_BUTTON_APPLY,
					      GNOME_STOCK_BUTTON_CLOSE, NULL);
	d->notebook = gtk_notebook_new ();
	gtk_box_pack_start_defaults (GTK_BOX (GNOME_DIALOG (d->dialog)->vbox), d->notebook);

	gnome_dialog_button_connect (GNOME_DIALOG (d->dialog), 0, ok, d);
	gnome_dialog_button_connect (GNOME_DIALOG (d->dialog), 1, apply, d);
	gnome_dialog_button_connect (GNOME_DIALOG (d->dialog), 2, prop_close, d);

	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 0, FALSE);
	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 1, FALSE);

	return d;
}

static void
destroy (PageData *pd, GtkHTMLEditPropertiesDialog *d)
{
	(*pd->close) (d->control_data, pd->data);
	g_free (pd);
}

void
gtk_html_edit_properties_dialog_destroy (GtkHTMLEditPropertiesDialog *d)
{
	g_list_foreach (d->page_data, (GFunc) destroy, d);
	g_list_free    (d->page_data);
	g_free (d);
}

void
gtk_html_edit_properties_dialog_add_entry (GtkHTMLEditPropertiesDialog *d,
					   GtkHTMLEditPropertyType t,
					   const gchar *name,
					   GtkHTMLEditPropertyCreateFunc create,
					   GtkHTMLEditPropertyApplyFunc apply_cb,
					   GtkHTMLEditPropertyCloseFunc close_cb)

{
	PageData *pd = g_new (PageData, 1);
	GtkWidget *page;

	page = (*create) (d->control_data, &pd->data);
	pd->apply = apply_cb;
	pd->close = close_cb;
	pd->type  = t;

	d->page_data = g_list_append (d->page_data, pd);
	gtk_notebook_append_page (GTK_NOTEBOOK (d->notebook), page, gtk_label_new (name));
}

void
gtk_html_edit_properties_dialog_show (GtkHTMLEditPropertiesDialog *d)
{
	gtk_window_set_modal (GTK_WINDOW (d->dialog), TRUE);
	gtk_widget_show_all (d->dialog);
}

void
gtk_html_edit_properties_dialog_change (GtkHTMLEditPropertiesDialog *d)
{
	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 0, TRUE);
	gnome_dialog_set_sensitive (GNOME_DIALOG (d->dialog), 1, TRUE);
}

static gint
find_type (PageData *d, gpointer data)
{
	return (d->type == GPOINTER_TO_INT (data)) ? 0 : 1;
}

void
gtk_html_edit_properties_dialog_set_page (GtkHTMLEditPropertiesDialog *d, GtkHTMLEditPropertyType t)
{
	gint pos = g_list_position (d->page_data, g_list_find_custom (d->page_data, GINT_TO_POINTER (t),
								      (GCompareFunc) find_type));
	if (pos >= 0)
		gtk_notebook_set_page (GTK_NOTEBOOK (d->notebook), pos);
}
