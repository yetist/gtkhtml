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

#include "htmlengine-save.h"

#include "config.h"
#include "properties.h"
#include "dialog.h"
#include "cell.h"
#include "utils.h"

typedef struct
{	
	GtkHTMLControlData *cd;
	HTMLTableCell *cell;

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

	gboolean   disable_change;

} GtkHTMLEditCellProperties;

#define CHANGE if (!d->disable_change) gtk_html_edit_properties_dialog_change (d->cd->properties_dialog)
#define FILL   if (!d->disable_change) fill_sample (d)

static void
fill_sample (GtkHTMLEditCellProperties *d)
{
	gchar *body, *html;

	body      = html_engine_save_get_sample_body (d->cd->html->engine, NULL);

	html      = g_strconcat (body, "<table border=1 cellpadding=4 cellspacing=2>"
				 "<tr><td>&nbsp;Other&nbsp;</td><td>&nbsp;Actual&nbsp;</td><td>&nbsp;Other&nbsp;</td></tr>"
				 "<tr><td>&nbsp;Other&nbsp;</td><td>&nbsp;Other&nbsp;</td><td>&nbsp;Other&nbsp;</td></tr>"
				 "</table>", NULL);
	printf ("html: %s\n", html);
	gtk_html_load_from_string (d->sample, html, -1);

	g_free (body);
	g_free (html);
}

static GtkHTMLEditCellProperties *
data_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditCellProperties *data = g_new0 (GtkHTMLEditCellProperties, 1);

	/* fill data */
	data->cd             = cd;
	data->cell          = NULL;

	/* default values
	data->width          = 100;
	data->size           = 2;
	data->percent        = TRUE;
	data->shaded         = TRUE;
	data->align          = HTML_HALIGN_CENTER; */

	return data;
}

static GtkWidget *
cell_widget (GtkHTMLEditCellProperties *d)
{
	GtkWidget *cell_page;
	GladeXML *xml;

	xml = glade_xml_new (GLADE_DATADIR "/gtkhtml-editor-properties.glade", "cell_page");
	if (!xml)
		g_error (_("Could not load glade file."));

	cell_page          = glade_xml_get_widget (xml, "cell_page");
	d->entry_bg_pixmap = glade_xml_get_widget (xml, "entry_cell_bg_pixmap");

	gtk_box_pack_start (GTK_BOX (cell_page), sample_frame (&d->sample), FALSE, FALSE, 0);

	gtk_widget_show_all (cell_page);
	gnome_pixmap_entry_set_preview (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap), FALSE);

	return cell_page;
}

static void
set_ui (GtkHTMLEditCellProperties *d)
{
	d->disable_change = TRUE;

	/* gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_color), d->has_bg_color);
	gdk_color_alloc (gdk_window_get_colormap (GTK_WIDGET (d->cd->html)->window), &d->bg_color);
	color_combo_set_color (COLOR_COMBO (d->combo_bg_color), &d->bg_color);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap), d->has_bg_pixmap);
	gtk_entry_set_text (GTK_ENTRY (gnome_pixmap_entry_gtk_entry (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap))),
			    d->bg_pixmap);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_spacing), d->spacing);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_padding), d->padding);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_border),  d->border);

	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_align), d->align - HTML_HALIGN_LEFT);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), d->has_width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width),  d->width);
	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_width), d->width_percent ? 1 : 0); */

	d->disable_change = FALSE;

	FILL;
}

static void
get_data (GtkHTMLEditCellProperties *d)
{
	/* d->table = html_engine_get_table (d->cd->html->engine);
	g_return_if_fail (d->table);

	if (d->table->bgColor) {
		d->has_bg_color = TRUE;
		d->bg_color     = *d->table->bgColor;
	}
	if (d->table->bgPixmap) {
		d->has_bg_pixmap = TRUE;
		d->bg_pixmap = strncasecmp ("file://", d->table->bgPixmap->url, 7)
			? (strncasecmp ("file://", d->table->bgPixmap->url, 5)
			   ? d->table->bgPixmap->url
			   : d->table->bgPixmap->url + 5)
			: d->table->bgPixmap->url + 7;
	}

	d->spacing = d->table->spacing;
	d->padding = d->table->padding;
	d->border  = d->table->border;

	g_return_if_fail (HTML_OBJECT (d->table)->parent);
	d->align   = HTML_CLUE (HTML_OBJECT (d->table)->parent)->halign;

	if (HTML_OBJECT (d->table)->percent) {
		d->width = HTML_OBJECT (d->table)->percent;
		d->width_percent = TRUE;
		d->has_width = TRUE;
	} else if (d->table->specified_width) {
		d->width = d->table->specified_width;
		d->width_percent = FALSE;
		d->has_width = TRUE;
		} */
}

GtkWidget *
cell_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditCellProperties *data = data_new (cd);
	GtkWidget *rv;

	get_data (data);
	*set_data = data;
	rv        = cell_widget (data);
	set_ui (data);

	return rv;
}

void
cell_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditCellProperties *d = (GtkHTMLEditCellProperties *) get_data;

	/* html_cell_set (d->rule, cd->html->engine, VAL (WIDTH), d->percent ? VAL (WIDTH) : 0, VAL (SIZE), d->shaded, d->align); */
}

void
cell_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
