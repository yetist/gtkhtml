/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2001 Ximian, Inc.
    Authors:           Radek Doulik (rodo@ximian.com)

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

#include <glade/glade.h>

#include "config.h"
#include "properties.h"
#include "dialog.h"
#include "table.h"
#include "utils.h"

typedef struct
{	
	GtkHTMLControlData *cd;

	HTMLTable *table;
	GtkHTML *sample;

} GtkHTMLEditTableProperties;

static void
fill_sample (GtkHTMLEditTableProperties *d)
{
	/*	gchar *body, *width, *size, *align, *noshade, *bg;

	bg      = html_engine_save_get_sample_body (d->cd->html->engine, NULL);
	width   = d->set [GTK_HTML_EDIT_RULE_WIDTH]
		? g_strdup_printf (" width=%d%s", VAL (WIDTH),
				   d->percent ? "%" : "") : g_strdup ("");
	size    = d->set [GTK_HTML_EDIT_RULE_SIZE]
		? g_strdup_printf (" size=%d", VAL (SIZE))
		: g_strdup ("");
	noshade = g_strdup (d->shaded ? "" : " noshade");
	align   = d->align != HTML_HALIGN_CENTER ? g_strdup_printf (" align=%s",
								    d->align == HTML_HALIGN_LEFT ? "left" : "right")
		: g_strdup ("");
	body    = g_strconcat (bg, "<br><hr", width, size, align, noshade, ">", NULL);

	printf ("body: %s\n", body); */

	gtk_html_load_from_string (d->sample,
				   "<table bgcolor=\"lightgray\" border=\"1\"><tr><th>Header</th><th>1</th></tr>"
				   "<tr><td>Normal</td><td>2</td></tr></table>", -1);

	/* g_free (bg);
	g_free (width);
	g_free (size);
	g_free (noshade);
	g_free (align);
	g_free (body); */
}

static GtkHTMLEditTableProperties *
data_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditTableProperties *data = g_new0 (GtkHTMLEditTableProperties, 1);

	/* fill data */
	data->cd             = cd;
	data->table          = NULL;

	/* default values
	data->width          = 100;
	data->size           = 2;
	data->percent        = TRUE;
	data->shaded         = TRUE;
	data->align          = HTML_HALIGN_CENTER; */

	return data;
}

static GtkWidget *
table_widget (GtkHTMLEditTableProperties *d)
{
	GtkWidget *table_page;
	GladeXML *xml;

	xml = glade_xml_new (GLADE_DATADIR "/gtkhtml-editor-properties.glade", "table_page");
	if (!xml)
		g_error (_("Could not load glade file."));

	table_page = glade_xml_get_widget (xml, "table_page");


	gtk_box_pack_start (GTK_BOX (table_page), sample_frame (&d->sample), FALSE, FALSE, 0);
	fill_sample (d);

	return table_page;
}

GtkWidget *
table_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTableProperties *data = data_new (cd);
	GtkWidget *rv;

	*set_data = data;
	rv        = table_widget (data);

	return rv;
}

GtkWidget *
table_insert (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTableProperties *data = data_new (cd);
	GtkWidget *rv;

	*set_data = data;
	/* rv = table_widget (data); */
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);

	return rv;
}

void
table_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTableProperties *d = (GtkHTMLEditTableProperties *) get_data;

	/* html_engine_insert_rule (cd->html->engine,
				 VAL (WIDTH), d->percent ? VAL (WIDTH) : 0, VAL (SIZE),
				 d->shaded, d->align); */
}

void
table_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTableProperties *d = (GtkHTMLEditTableProperties *) get_data;

	/* html_table_set (d->rule, cd->html->engine, VAL (WIDTH), d->percent ? VAL (WIDTH) : 0, VAL (SIZE), d->shaded, d->align); */
}

void
table_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
