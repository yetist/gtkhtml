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

#include <string.h>
#include "config.h"
#include "properties.h"
#include "dialog.h"
#include "link.h"
#include "htmlengine-edit-insert.h"

struct _GtkHTMLLinkDialog {
	GnomeDialog        *dialog;
	GtkHTML            *html;
	HTMLLinkTextMaster *html_link;

	GtkWidget   *link;
};

static void
button_link_cb (GtkWidget *but, GtkHTMLLinkDialog *d)
{
	gchar *link = gtk_entry_get_text (GTK_ENTRY (d->link));

	if (d->html_link) {
		gboolean changed = FALSE;

		html_link_text_master_set_url (d->html_link, link);
		if (changed)
			html_engine_schedule_update (d->html->engine);
	} else
		html_engine_insert_link (d->html->engine, link, "");
}

static void
set_entries (HTMLEngine *e, GtkWidget *el)
{
	gchar *text;
	gchar *found;

	text = html_engine_get_selection_string (e);

	/* use only text part to \n */
	found = strchr (text, '\n');
	if (found) *found=0;

	printf ("set_entries %s\n", text);

	/* if it contains mailto: and '@'  we assume it is href */
	if (text == (found = strstr (text, "mailto:")) && strchr (text, '@') > found) {
		gtk_entry_set_text (GTK_ENTRY (el), text);
		goto end;
	}

	/* if it contains http: or ftp: we assume it's href */
	if (text == strstr (text, "http:") || text == strstr (text, "ftp:")) {
		gtk_entry_set_text (GTK_ENTRY (el), text);
		goto end;
	}

	/* mailto addition */
	if (strchr (text, '@')) {
		gchar *link;

		link = g_strconcat ("mailto:", text, NULL);
		gtk_entry_set_text (GTK_ENTRY (el), link);
		g_free (link);
		goto end;
	}

 end:
	g_free (text);
	return;
}

GtkHTMLLinkDialog *
gtk_html_link_dialog_new (GtkHTML *html)
{
	GtkHTMLLinkDialog *d = g_new (GtkHTMLLinkDialog, 1);
	GtkWidget *table;
	GtkWidget *label;

	d->dialog = GNOME_DIALOG (gnome_dialog_new (_("Link"), GNOME_STOCK_BUTTON_OK,
						    GNOME_STOCK_BUTTON_CANCEL, NULL));
	d->html = html;
	d->link = gtk_entry_new ();

	set_entries (html->engine, d->link);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);

	label = gtk_label_new (_("Link"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, .5);
	gtk_table_attach_defaults (GTK_TABLE (table), label,   0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), d->link, 1, 2, 1, 2);

	gtk_box_pack_start_defaults (GTK_BOX (d->dialog->vbox), table);
	gtk_widget_show_all (table);

	gnome_dialog_button_connect (d->dialog, 0, button_link_cb, d);
	gnome_dialog_close_hides (d->dialog, TRUE);
	gnome_dialog_set_close (d->dialog, TRUE);

	gnome_dialog_set_default (d->dialog, 0);
	gtk_widget_grab_focus (d->link);

	return d;
}

void
gtk_html_link_dialog_destroy (GtkHTMLLinkDialog *d)
{
}

void
link_insert (GtkHTMLControlData *cd)
{
	if (cd->link_dialog)
		set_entries (cd->html->engine, cd->link_dialog->link);
	RUN_DIALOG (link);
	cd->link_dialog->html_link = NULL;
}

void
link_edit (GtkHTMLControlData *cd, HTMLLinkTextMaster *link)
{
	RUN_DIALOG (link);
	cd->link_dialog->html_link = link;
	gtk_entry_set_text (GTK_ENTRY (cd->link_dialog->link), link->url);
}

struct _GtkHTMLEditLinkProperties {
	GtkHTMLControlData *cd;
	GtkWidget *entry;
};
typedef struct _GtkHTMLEditLinkProperties GtkHTMLEditLinkProperties;

static void
set_link (GtkWidget *w, GtkHTMLEditLinkProperties *data)
{
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
}

GtkWidget *
link_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkWidget *vbox, *hbox;
	GtkHTMLEditLinkProperties *data = g_new (GtkHTMLEditLinkProperties, 1);

	*set_data = data;
	data->cd = cd;

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);
	hbox = gtk_hbox_new (FALSE, 3);

	data->entry = gtk_entry_new ();
	gtk_signal_connect (GTK_OBJECT (data->entry), "changed", set_link, data);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("URL")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), data->entry, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	return vbox;
}

void
link_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	printf ("link apply\n");
}

void
link_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	printf ("link close\n");
	g_free (get_data);
}
