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
#include "dialog.h"
#include "link.h"
#include "htmlengine-edit-insert.h"

struct _GtkHTMLLinkDialog {
	GnomeDialog        *dialog;
	GtkHTML            *html;
	HTMLLinkTextMaster *html_link;

	GtkWidget   *text;
	GtkWidget   *text_label;
	GtkWidget   *link;
};

static void
button_link_cb (GtkWidget *but, GtkHTMLLinkDialog *d)
{
	gchar *text = gtk_entry_get_text (GTK_ENTRY (d->text));
	gchar *link = gtk_entry_get_text (GTK_ENTRY (d->link));

	if (d->html_link) {
		gboolean changed = FALSE;

		if (strcmp (text, HTML_TEXT (d->html_link)->text)) {
			changed = TRUE;
			html_text_set_text (HTML_TEXT (d->html_link), text);
		}
		html_link_text_master_set_url (d->html_link, link);
		if (changed)
			html_engine_schedule_update (d->html->engine);
	} else
		html_engine_insert_link (d->html->engine, text, link);
}

static void
set_entries (HTMLEngine *e, GtkWidget *et, GtkWidget *el)
{
	gchar *text;
	gchar *found;

	text = html_engine_get_selection_string (e);

	printf ("set_entries %s\n", text);

	/* if it contains mailto: and '@'  we assume it is href */
	if (text == (found = strstr (text, "mailto:")) && strchr (text, '@') > found) {
		gtk_entry_set_text (GTK_ENTRY (el), text);
		gtk_entry_set_text (GTK_ENTRY (et), found+7);
		return;
	}

	/* if it contains http: or ftp: we assume it's href */
	if (text == strstr (text, "http:") || text == strstr (text, "ftp:")) {
		gtk_entry_set_text (GTK_ENTRY (el), text);
		gtk_entry_set_text (GTK_ENTRY (et), text);
		return;
	}

	/* mailto addition */
	if (strchr (text, '@')) {
		gchar *link;

		link = g_strconcat ("mailto:", text, NULL);
		gtk_entry_set_text (GTK_ENTRY (el), link);
		gtk_entry_set_text (GTK_ENTRY (et), text);
		g_free (link);
		return;
	}

	/* for other cases fill text and leave link blank */
	gtk_entry_set_text (GTK_ENTRY (et), text);	
	gtk_entry_set_text (GTK_ENTRY (el), "");
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
	d->text = gtk_entry_new ();

	set_entries (html->engine, d->text, d->link);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	d->text_label = label = gtk_label_new (_("Text"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, .5);
	gtk_table_attach_defaults (GTK_TABLE (table), label,   0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), d->text, 1, 2, 0, 1);

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
		set_entries (cd->html->engine, cd->link_dialog->text, cd->link_dialog->link);
	RUN_DIALOG (link);
	gtk_widget_show (cd->link_dialog->text);
	gtk_widget_show (cd->link_dialog->text_label);
	cd->link_dialog->html_link = NULL;
}

void
link_edit (GtkHTMLControlData *cd, HTMLLinkTextMaster *link)
{
	RUN_DIALOG (link);
	cd->link_dialog->html_link = link;
	gtk_widget_hide (cd->link_dialog->text);
	gtk_widget_hide (cd->link_dialog->text_label);
	gtk_entry_set_text (GTK_ENTRY (cd->link_dialog->text), HTML_TEXT (link)->text);
	gtk_entry_set_text (GTK_ENTRY (cd->link_dialog->link), link->url);
}
