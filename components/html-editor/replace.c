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
#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>
#include "replace.h"
#include "dialog.h"
#include "htmlengine.h"

struct _GtkHTMLReplaceAskDialog {
	GtkDialog  *dialog;
	HTMLEngine *engine;
};

typedef struct _GtkHTMLReplaceAskDialog GtkHTMLReplaceAskDialog;

struct _GtkHTMLReplaceDialog {
	GtkDialog   *dialog;
	GtkHTML     *html;
	GtkWidget   *entry_search;
	GtkWidget   *entry_replace;
	GtkWidget   *backward;
	GtkWidget   *case_sensitive;

	GtkHTMLReplaceAskDialog *ask_dialog;
};

/* FIX2 static void
replace_do (GtkHTMLReplaceAskDialog *d, HTMLReplaceQueryAnswer answer)
{
	gtk_dialog_response (d->dialog, GTK_RESPONSE_CLOSE);
	html_engine_replace_do (d->engine, answer);
}

static void
replace_cb (GtkWidget *button, GtkHTMLReplaceAskDialog *d)
{
	replace_do (d, RQA_Replace);
}

static void
replace_all_cb (GtkWidget *button, GtkHTMLReplaceAskDialog *d)
{
	replace_do (d, RQA_ReplaceAll);
}

static void
next_cb (GtkWidget *button, GtkHTMLReplaceAskDialog *d)
{
	replace_do (d, RQA_Next);
}

static void
cancel_cb (GtkWidget *button, GtkHTMLReplaceAskDialog *d)
{
	replace_do (d, RQA_Cancel);
} */

static GtkHTMLReplaceAskDialog *
ask_dialog_new (HTMLEngine *e)
{
	GtkHTMLReplaceAskDialog *d;

	d = g_new (GtkHTMLReplaceAskDialog, 1);
	d->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Replace confirmation"), NULL, 0,
							     _("Replace"), 0,
							     _("Replace all"), 1,
							     _("Next"), 2,
							     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL));
	d->engine = e;

	gnome_window_icon_set_from_file (GTK_WINDOW (d->dialog), ICONDIR "/search-and-replace-24.png");
	/* FIX2 gnome_dialog_button_connect (d->dialog, 0, GTK_SIGNAL_FUNC (replace_cb), d);
	   gnome_dialog_button_connect (d->dialog, 1, GTK_SIGNAL_FUNC (replace_all_cb), d);
	   gnome_dialog_button_connect (d->dialog, 2, GTK_SIGNAL_FUNC (next_cb), d);
	   gnome_dialog_button_connect (d->dialog, 3, GTK_SIGNAL_FUNC (cancel_cb), d); */

	/* gnome_dialog_set_close (d->dialog, TRUE); */

	/* FIX2 gnome_dialog_set_default (d->dialog, 0); */

	return d;
}

static void
ask (HTMLEngine *e, gpointer data)
{
	GtkHTMLReplaceDialog *d = (GtkHTMLReplaceDialog *) data;

	if (!d->ask_dialog) {
		d->ask_dialog = ask_dialog_new (e);
		gtk_dialog_run (d->ask_dialog->dialog);
	} else {
		gtk_widget_show (GTK_WIDGET (d->ask_dialog->dialog));
		gdk_window_raise (GTK_WIDGET (d->ask_dialog->dialog)->window);
	}	
}

static void
button_replace_cb (GtkWidget *but, GtkHTMLReplaceDialog *d)
{
	gtk_dialog_response  (d->dialog, GTK_RESPONSE_CLOSE);
	html_engine_replace (d->html->engine,
			     gtk_entry_get_text (GTK_ENTRY (d->entry_search)),
			     gtk_entry_get_text (GTK_ENTRY (d->entry_replace)),
			     GTK_TOGGLE_BUTTON (d->case_sensitive)->active,
			     GTK_TOGGLE_BUTTON (d->backward)->active == 0, FALSE,
			     ask, d);
}

static void
entry_changed (GtkWidget *entry, GtkHTMLReplaceDialog *d)
{
}

static void
entry_activate (GtkWidget *entry, GtkHTMLReplaceDialog *d)
{
	button_replace_cb (NULL, d);
}

GtkHTMLReplaceDialog *
gtk_html_replace_dialog_new (GtkHTML *html)
{
	GtkHTMLReplaceDialog *dialog = g_new (GtkHTMLReplaceDialog, 1);
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *label;

	dialog->dialog         = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Replace"), NULL, 0,
									  _("Replace"), 0,
									  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL));

	table = gtk_table_new (2, 2, FALSE);
	dialog->entry_search   = gtk_entry_new ();
	dialog->entry_replace  = gtk_entry_new ();
	dialog->backward       = gtk_check_button_new_with_label (_("search backward"));
	dialog->case_sensitive = gtk_check_button_new_with_label (_("case sensitive"));
	dialog->html           = html;
	dialog->ask_dialog     = NULL;

	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	label = gtk_label_new (_("Replace"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, .5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	label = gtk_label_new (_("with"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, .5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);

	gtk_table_attach_defaults (GTK_TABLE (table), dialog->entry_search,  1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), dialog->entry_replace, 1, 2, 1, 2);

	hbox = gtk_hbox_new (FALSE, 0);

	gtk_box_pack_start_defaults (GTK_BOX (hbox), dialog->backward);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), dialog->case_sensitive);

	gtk_box_pack_start_defaults (GTK_BOX (dialog->dialog->vbox), table);
	gtk_box_pack_start_defaults (GTK_BOX (dialog->dialog->vbox), hbox);
	gtk_widget_show_all (table);
	gtk_widget_show_all (hbox);

	gnome_window_icon_set_from_file (GTK_WINDOW (dialog->dialog), ICONDIR "/search-and-replace-24.png");

	/* FIX2 gnome_dialog_button_connect (dialog->dialog, 0, GTK_SIGNAL_FUNC (button_replace_cb), dialog);
	   gnome_dialog_close_hides (dialog->dialog, TRUE);
	   gnome_dialog_set_close (dialog->dialog, TRUE);

	   gnome_dialog_set_default (dialog->dialog, 0); */
	gtk_widget_grab_focus (dialog->entry_search);

	g_signal_connect (dialog->entry_search, "changed", G_CALLBACK (entry_changed), dialog);
	g_signal_connect (dialog->entry_search, "activate", G_CALLBACK (entry_activate), dialog);
	g_signal_connect (dialog->entry_replace, "activate", G_CALLBACK (entry_activate), dialog);

	return dialog;
}

void
gtk_html_replace_dialog_destroy (GtkHTMLReplaceDialog *d)
{
	g_free (d);
}

void
replace (GtkHTMLControlData *cd)
{
	RUN_DIALOG (replace, _("Replace"));

	if (cd->replace_dialog)
		gtk_widget_grab_focus (cd->replace_dialog->entry_search);
}
