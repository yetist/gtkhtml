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
#include <gal/widgets/e-unicode.h>
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmllinktext.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlselection.h"
#include "htmlsettings.h"

#include "properties.h"
#include "link.h"

struct _GtkHTMLEditLinkProperties {
	GtkHTMLControlData *cd;
	GtkWidget *entry_text;
	GtkWidget *entry_url;
};
typedef struct _GtkHTMLEditLinkProperties GtkHTMLEditLinkProperties;

static void
changed (GtkWidget *w, GtkHTMLEditLinkProperties *data)
{
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
}

GtkWidget *
link_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkWidget *vbox, *frame, *f1;
	GtkHTMLEditLinkProperties *data = g_new (GtkHTMLEditLinkProperties, 1);

	*set_data = data;
	data->cd = cd;

	vbox = gtk_vbox_new (FALSE, 3);

	data->entry_text = gtk_entry_new ();
	data->entry_url  = gtk_entry_new ();

	frame = gtk_frame_new (_("Link text"));
	f1    = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (f1), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (f1), 3);
	gtk_container_add (GTK_CONTAINER (f1), data->entry_text);
	gtk_container_add (GTK_CONTAINER (frame), f1);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	if (html_engine_is_selection_active (cd->html->engine)) {
		gchar *str, *str_gtk;

		str = html_engine_get_selection_string (cd->html->engine);
		str_gtk = e_utf8_to_gtk_string (data->entry_text, str);
		gtk_entry_set_text (GTK_ENTRY (data->entry_text), str_gtk);
		g_free (str);
		g_free (str_gtk);
	}

	frame = gtk_frame_new (_("Click will follow this URL"));
	f1    = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (f1), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (f1), 3);
	gtk_container_add (GTK_CONTAINER (f1), data->entry_url);
	gtk_container_add (GTK_CONTAINER (frame), f1);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	gtk_signal_connect (GTK_OBJECT (data->entry_text), "changed", changed, data);
	gtk_signal_connect (GTK_OBJECT (data->entry_url), "changed", changed, data);

	gtk_widget_show_all (vbox);

	return vbox;
}

void
link_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditLinkProperties *data = (GtkHTMLEditLinkProperties *) get_data;
	HTMLEngine *e = cd->html->engine;
	gchar *url;
	gchar *target;
	gchar *text;

	url  = gtk_entry_get_text (GTK_ENTRY (data->entry_url));
	text = gtk_entry_get_text (GTK_ENTRY (data->entry_text));
	if (url && text && *url && *text) {
		HTMLObject *new_link;
		gchar *url_copy;

		url  = e_utf8_from_gtk_string (data->entry_url, url);
		text = e_utf8_from_gtk_string (data->entry_text, text);

		target = strchr (url, '#');

		url_copy = target ? g_strndup (url, target - url) : g_strdup (url);
		new_link = html_link_text_new (text, GTK_HTML_FONT_STYLE_DEFAULT,
					       html_colorset_get_color (e->settings->color_set, HTMLLinkColor),
					       url_copy, target);

		html_engine_paste_object (e, new_link, g_utf8_strlen (text, -1));

		g_free (url_copy);
		g_free (url);
		g_free (text);
	}
}

void
link_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
