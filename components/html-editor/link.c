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
#include <string.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>

#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlimage.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmltext.h"

#include "properties.h"
#include "link.h"
#include "utils.h"

struct _GtkHTMLEditLinkProperties {
	GtkHTMLControlData *cd;
	GtkWidget *entry_description;
	GtkWidget *label_description;
	GtkWidget *entry_url;

	HTMLObject *object;
	gint offset;
	gboolean selection;
	gboolean description_empty;
	gboolean insert;
	int insert_start_offset;
	int insert_end_offset;
	HTMLObject *insert_object;

	gboolean disable_change;
};
typedef struct _GtkHTMLEditLinkProperties GtkHTMLEditLinkProperties;

static void
url_changed (GtkWidget *w, GtkHTMLEditLinkProperties *d)
{
	const char *url, *desc;

	if (d->disable_change)
		return;

	url = gtk_entry_get_text (GTK_ENTRY (d->entry_url));
	desc = gtk_entry_get_text (GTK_ENTRY (d->entry_description));
	if (d->selection)
		html_engine_set_link (d->cd->html->engine, url);
	else {
		if (!desc || !*desc || d->description_empty) {
			gtk_entry_set_text (GTK_ENTRY (d->entry_description), url);
			d->description_empty = TRUE;
		}
	}
}

static void
description_changed (GtkWidget *w, GtkHTMLEditLinkProperties *d)
{
	HTMLEngine *e = d->cd->html->engine;
	const char *text;
	int len;

	d->description_empty = FALSE;

	if (d->disable_change || !editor_has_html_object (d->cd, d->insert_object))
		return;

	html_cursor_jump_to (e->cursor, e, d->insert_object, d->insert_start_offset);
	html_engine_set_mark (e);
	html_cursor_jump_to (e->cursor, e, d->insert_object, d->insert_end_offset);
	html_engine_delete (e);
	text = gtk_entry_get_text (GTK_ENTRY (w));
	if (text && *text) {
		len = g_utf8_strlen (text, -1);
		html_engine_paste_link (e, text, len, gtk_entry_get_text (GTK_ENTRY (d->entry_url)));
		d->insert_object = e->cursor->object;
	} else
		len = 0;
	d->insert_end_offset = d->insert_start_offset + len;
}

static void
test_clicked (GtkWidget *w, GtkHTMLEditLinkProperties *data)
{
	const char *url = gtk_entry_get_text (GTK_ENTRY (data->entry_url));

	if (url)
		gnome_url_show (url, NULL);
}

static void
link_set_ui (GtkHTMLEditLinkProperties *d)
{
	HTMLEngine *e = d->cd->html->engine;
	gchar *url, *link_text;

	d->disable_change = TRUE;

	if (html_engine_is_selection_active (e)) {
		d->selection = TRUE;

		gtk_widget_hide (d->label_description);
		gtk_widget_hide (d->entry_description);
	} else {
		char *url = NULL;

		if (HTML_IS_TEXT (e->cursor->object))
			url = html_object_get_complete_url (e->cursor->object, e->cursor->offset);

		d->selection = FALSE;
		d->insert = TRUE;
		d->insert_object = e->cursor->object;

		if (url) {
			gtk_entry_set_text (GTK_ENTRY (d->entry_url), url);
			gtk_widget_hide (d->label_description);
			gtk_widget_hide (d->entry_description);
		
			if (HTML_IS_IMAGE (d->insert_object)) {
				d->insert_start_offset = 0;
				d->insert_end_offset = 1;
			} else {
				Link *link;
				link = html_text_get_link_at_offset (HTML_TEXT (d->insert_object), e->cursor->offset);

				if (link) {
					d->insert_start_offset = link->start_offset;
					d->insert_end_offset = link->end_offset;
				}
			}
		} else {
			gtk_entry_set_text (GTK_ENTRY (d->entry_url), "http://");

			d->insert_start_offset = d->insert_end_offset = e->cursor->offset;
		}
	}

	d->disable_change = FALSE;

	/* link_text = html_text_get_link_text (data->text, data->offset);
	gtk_entry_set_text (GTK_ENTRY (data->entry_text), link_text);
	g_free (link_text);

	url = html_object_get_complete_url (HTML_OBJECT (data->text), data->offset);
	gtk_entry_set_text (GTK_ENTRY (data->entry_url), url ? url : "");
	g_free (url); */
}

static void
test_url_clicked (GtkWidget *w, GtkHTMLEditLinkProperties *d)
{
	const char *url = gtk_entry_get_text (GTK_ENTRY (d->entry_url));

	if (url)
		gnome_url_show (url, NULL);
}

static GtkWidget *
link_widget (GtkHTMLEditLinkProperties *d, gboolean insert)
{
	GtkHTMLControlData *cd = d->cd;
	GtkWidget *link_page, *button;
	GladeXML *xml;

	xml = glade_xml_new (GLADE_DATADIR "/gtkhtml-editor-properties.glade", "link_page", NULL);
	if (!xml)
		g_error (_("Could not load glade file."));

	link_page = glade_xml_get_widget (xml, "link_page");

	editor_check_stock ();
	button = gtk_button_new_from_stock (GTKHTML_STOCK_TEST_URL);
	g_signal_connect (button, "clicked", G_CALLBACK (test_url_clicked), d);
	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (glade_xml_get_widget (xml, "table_link")), button, 2, 3, 0, 1, 0, 0, 0, 0);

	d->entry_url = glade_xml_get_widget (xml, "entry_url");
	g_signal_connect (d->entry_url, "changed", G_CALLBACK (url_changed), d);

	d->entry_description = glade_xml_get_widget (xml, "entry_description");
	g_signal_connect (d->entry_description, "changed", G_CALLBACK (description_changed), d);

	d->label_description = glade_xml_get_widget (xml, "label_description");

	/*GtkWidget *vbox, *hbox, *button, *frame, *f1;

	vbox = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	data->entry_text = gtk_entry_new ();
	data->entry_url  = gtk_entry_new ();

	gtk_box_pack_start (GTK_BOX (vbox), editor_hig_vbox (_("Link Text"), data->entry_text), FALSE, FALSE, 0);

	if (html_engine_is_selection_active (cd->html->engine)) {
		gchar *str;

		str = html_engine_get_selection_string (cd->html->engine);
		gtk_entry_set_text (GTK_ENTRY (data->entry_text), str);
		g_free (str);
	}

	hbox = gtk_hbox_new (FALSE, 5);
	editor_check_stock ();
	button = gtk_button_new_from_stock (GTKHTML_STOCK_TEST_URL);
	gtk_box_pack_start (GTK_BOX (hbox), data->entry_url, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor_hig_vbox (_("Click Will Follow This URL"), hbox), FALSE, FALSE, 0);

	if (!insert) {
		gtk_widget_set_sensitive (data->entry_text, FALSE);
		set_ui (data);
	}

	g_signal_connect (data->entry_text, "changed", G_CALLBACK (changed), data);
	g_signal_connect (data->entry_url, "changed", G_CALLBACK (changed), data);
	g_signal_connect (button, "clicked", G_CALLBACK (test_clicked), data);*/

	gtk_widget_show_all (link_page);
	link_set_ui (d);

	return link_page;
}

GtkWidget *
link_insert (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditLinkProperties *data = g_new0 (GtkHTMLEditLinkProperties, 1);

	*set_data = data;
	data->cd = cd;

	return link_widget (data, TRUE);
}

GtkWidget *
link_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditLinkProperties *data = g_new0 (GtkHTMLEditLinkProperties, 1);

	g_return_val_if_fail (cd->html->engine->cursor->object, NULL);
	g_return_val_if_fail (HTML_IS_LINK_TEXT (cd->html->engine->cursor->object), NULL);

	*set_data = data;
	data->cd = cd;
	//data->text = HTML_TEXT (cd->html->engine->cursor->object);
	//data->offset = cd->html->engine->cursor->offset;

	return link_widget (data, FALSE);
}

gboolean
link_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditLinkProperties *data = (GtkHTMLEditLinkProperties *) get_data;
	HTMLEngine *e = data->cd->html->engine;

	const gchar *url;
	gchar *target;

	/* if (data->url_changed) {
		gchar *url_copy;
		gint position;

		position = e->cursor->position;

		if (e->cursor->object != HTML_OBJECT (data->text))
			if (!html_cursor_jump_to (e->cursor, e, HTML_OBJECT (data->text), data->offset)) {
				GtkWidget *dialog;
				printf ("d: %p\n", data->cd->properties_dialog);
				dialog = gtk_message_dialog_new (GTK_WINDOW (data->cd->properties_dialog->dialog),
								 GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
								 _("The editted link was removed from the document.\nCannot apply your changes."));
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				html_cursor_jump_to_position (e->cursor, e, position);
				return FALSE;
			}

		url  = gtk_entry_get_text (GTK_ENTRY (data->entry_url));

		target = strchr (url, '#');

		url_copy = target ? g_strndup (url, target - url) : g_strdup (url);
		html_link_set_url_and_target (html_text_get_link_at_offset (data->text, data->offset), url_copy, target);
		html_engine_update_insertion_url_and_target (e);
		g_free (url_copy);
		html_cursor_jump_to_position (e->cursor, e, position);
		} */

	return TRUE;
}

gboolean
link_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditLinkProperties *data = (GtkHTMLEditLinkProperties *) get_data;
	HTMLEngine *e = cd->html->engine;
	const gchar *url;
	gchar *target;
	const gchar *text;

	/* url  = gtk_entry_get_text (GTK_ENTRY (data->entry_url));
	text = gtk_entry_get_text (GTK_ENTRY (data->entry_text));
	if (url && text && *url && *text) {
		HTMLObject *new_text;
		gchar *url_copy;

		target = strchr (url, '#');

		url_copy = target ? g_strndup (url, target - url) : g_strdup (url);
		new_text = html_text_new (text, GTK_HTML_FONT_STYLE_DEFAULT,
					  html_colorset_get_color (e->settings->color_set, HTMLLinkColor));
		html_text_add_link (HTML_TEXT (new_text), e, url_copy, target, 0, HTML_TEXT (new_text)->text_len);

		html_engine_paste_object (e, new_text, HTML_TEXT (new_text)->text_len);

		g_free (url_copy);

		return TRUE;
	} else {
		GtkWidget *toplevel;
		GtkWidget *dialog;

		toplevel = gtk_widget_get_toplevel (cd->properties_dialog->dialog);
		if (!GTK_WIDGET_TOPLEVEL (toplevel))
			toplevel = NULL;

		dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
						 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						 _("Please fill 'Link text' and 'URL' entries"));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		} */

	return FALSE;
}

void
link_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
