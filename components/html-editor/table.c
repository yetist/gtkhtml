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
#include <gal/widgets/widget-color-combo.h>

#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine-save.h"
#include "htmlsettings.h"

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

	gboolean   has_bg_color;
	gboolean   changed_bg_color;
	GdkColor   bg_color;
	GtkWidget *combo_bg_color;
	GtkWidget *check_bg_color;

	gboolean   has_bg_pixmap;
	gboolean   changed_bg_pixmap;
	gchar     *bg_pixmap;
	GtkWidget *entry_bg_pixmap;
	GtkWidget *check_bg_pixmap;

} GtkHTMLEditTableProperties;

#define CHANGE gtk_html_edit_properties_dialog_change (d->cd->properties_dialog)
#define FILL   fill_sample (d)

static void
fill_sample (GtkHTMLEditTableProperties *d)
{
	gchar *body, *html, *bg_color, *bg_pixmap;

	body    = html_engine_save_get_sample_body (d->cd->html->engine, NULL);
	/* width   = d->set [GTK_HTML_EDIT_RULE_WIDTH]
		? g_strdup_printf (" width=%d%s", VAL (WIDTH),
				   d->percent ? "%" : "") : g_strdup ("");
	size    = d->set [GTK_HTML_EDIT_RULE_SIZE]
		? g_strdup_printf (" size=%d", VAL (SIZE))
		: g_strdup ("");
	noshade = g_strdup (d->shaded ? "" : " noshade");
	align   = d->align != HTML_HALIGN_CENTER ? g_strdup_printf (" align=%s",
								    d->align == HTML_HALIGN_LEFT ? "left" : "right")
								    : g_strdup (""); */
	bg_color  = d->has_bg_color
		? g_strdup_printf (" bgcolor=\"#%02x%02x%02x\"",
				   d->bg_color.red >> 8,
				   d->bg_color.green >> 8,
				   d->bg_color.blue >> 8)
		: g_strdup ("");
	bg_pixmap = d->has_bg_pixmap && d->bg_pixmap
		? g_strdup_printf (" background=\"file://%s\"", d->bg_pixmap)
		: g_strdup ("");

	html      = g_strconcat (body, "<table", bg_color, bg_pixmap, ">"
				 "<tr><th>Header</th><th>1</th></tr>"
				 "<tr><td>Normal</td><td>2</td></tr></table>", NULL);
	printf ("html: %s\n", html);
	gtk_html_load_from_string (d->sample, html, -1);

	g_free (body);
	g_free (bg_color);
	g_free (bg_pixmap);
	g_free (html);
}

static GtkHTMLEditTableProperties *
data_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditTableProperties *data = g_new0 (GtkHTMLEditTableProperties, 1);

	/* fill data */
	data->cd                = cd;
	data->table             = NULL;

	data->has_bg_color      = FALSE;
	data->changed_bg_color  = FALSE;
	data->bg_color          = html_colorset_get_color (data->cd->html->engine->defaultSettings->color_set,
							   HTMLBgColor)->color;
	data->has_bg_pixmap     = FALSE;
	data->changed_bg_pixmap = FALSE;

	/* default values
	data->width          = 100;
	data->size           = 2;
	data->percent        = TRUE;
	data->shaded         = TRUE;
	data->align          = HTML_HALIGN_CENTER; */

	return data;
}

static void
set_has_bg_color (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	d->has_bg_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_bg_color));
	gtk_widget_set_sensitive (d->combo_bg_color, d->has_bg_color);
	FILL;
	CHANGE;
	d->changed_bg_color = TRUE;
}

static void
set_has_bg_pixmap (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	d->has_bg_pixmap = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap));
	gtk_widget_set_sensitive (d->entry_bg_pixmap, d->has_bg_pixmap);
	FILL;
	CHANGE;
	d->changed_bg_pixmap = TRUE;
}

static void
changed_bg_color (GtkWidget *w, GdkColor *color, gboolean by_user, GtkHTMLEditTableProperties *d)
{
	/* If the color was changed programatically there's not need to set things */
	if (!by_user)
		return;
		
	d->bg_color = color
		? *color
		: html_colorset_get_color (d->cd->html->engine->defaultSettings->color_set, HTMLBgColor)->color;
	d->has_bg_color = TRUE;
	FILL;
	CHANGE;
}

static GtkWidget *
table_widget (GtkHTMLEditTableProperties *d)
{
	HTMLColor *color;
	GtkWidget *table_page;
	GladeXML *xml;

	xml = glade_xml_new (GLADE_DATADIR "/gtkhtml-editor-properties.glade", "table_page");
	if (!xml)
		g_error (_("Could not load glade file."));

	table_page = glade_xml_get_widget (xml, "table_page");

        color = html_colorset_get_color (d->cd->html->engine->defaultSettings->color_set, HTMLBgColor);
        html_color_alloc (color, d->cd->html->engine->painter);
	d->combo_bg_color = color_combo_new (NULL, _("Automatic"), &color->color,
					     color_group_fetch ("table_bg_color", d->cd));
        gtk_signal_connect (GTK_OBJECT (d->combo_bg_color), "changed", GTK_SIGNAL_FUNC (changed_bg_color), d);
	gtk_table_attach (GTK_TABLE (glade_xml_get_widget (xml, "bg_table")),
			  d->combo_bg_color,
			  1, 2, 0, 1, 0, 0, 0, 0);

	d->check_bg_color  = glade_xml_get_widget (xml, "check_table_bg_color");
	gtk_signal_connect (GTK_OBJECT (d->check_bg_color), "toggled", set_has_bg_color, d);
	d->check_bg_pixmap = glade_xml_get_widget (xml, "check_table_bg_pixmap");
	gtk_signal_connect (GTK_OBJECT (d->check_bg_pixmap), "toggled", set_has_bg_pixmap, d);
	d->entry_bg_pixmap = glade_xml_get_widget (xml, "entry_table_bg_pixmap");

	gtk_box_pack_start (GTK_BOX (table_page), sample_frame (&d->sample), FALSE, FALSE, 0);
	fill_sample (d);

	gtk_widget_show_all (table_page);
	gnome_pixmap_entry_set_preview (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap), FALSE);

	return table_page;
}

static void
set_ui (GtkHTMLEditTableProperties *d)
{
	gtk_widget_set_sensitive (d->combo_bg_color, d->has_bg_color);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_color), d->has_bg_color);
	gtk_widget_set_sensitive (d->entry_bg_pixmap, d->has_bg_pixmap);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap), d->has_bg_pixmap);
}

GtkWidget *
table_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTableProperties *data = data_new (cd);
	GtkWidget *rv;

	*set_data = data;
	rv        = table_widget (data);
	set_ui (data);

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
