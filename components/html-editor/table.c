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

#include "gtkhtml.h"

#include "htmlclue.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-save.h"
#include "htmlimage.h"
#include "htmltable.h"
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

	gboolean   changed_spacing;
	gint       spacing;
	GtkWidget *spin_spacing;

	gboolean   changed_padding;
	gint       padding;
	GtkWidget *spin_padding;

	gboolean   changed_border;
	gint       border;
	GtkWidget *spin_border;

	gboolean        changed_align;
	HTMLHAlignType  align;
	GtkWidget      *option_align;

	gboolean   has_width;
	gboolean   changed_width;
	gint       width;
	gboolean   width_percent;
	GtkWidget *spin_width;
	GtkWidget *check_width;
	GtkWidget *option_width;

	gboolean   changed_cols;
	gint       cols;
	GtkWidget *spin_cols;

	gboolean   changed_rows;
	gint       rows;
	GtkWidget *spin_rows;

	gboolean   disable_change;
} GtkHTMLEditTableProperties;

#define CHANGE if (!d->disable_change) gtk_html_edit_properties_dialog_change (d->cd->properties_dialog)
#define FILL   if (!d->disable_change && d->sample) fill_sample (d)

static void
fill_sample (GtkHTMLEditTableProperties *d)
{
	GString *cells;
	gchar *body, *html, *bg_color, *bg_pixmap, *spacing, *align, *width;
	gint r, c;

	body      = html_engine_save_get_sample_body (d->cd->html->engine, NULL);
	bg_color  = d->has_bg_color
		? g_strdup_printf (" bgcolor=\"#%02x%02x%02x\"",
				   d->bg_color.red >> 8,
				   d->bg_color.green >> 8,
				   d->bg_color.blue >> 8)
		: g_strdup ("");
	bg_pixmap = d->has_bg_pixmap && d->bg_pixmap
		? g_strdup_printf (" background=\"file://%s\"", d->bg_pixmap)
		: g_strdup ("");
	spacing = g_strdup_printf (" cellspacing=\"%d\" cellpadding=\"%d\" border=\"%d\"", d->spacing, d->padding, d->border);

	align   = d->align != HTML_HALIGN_NONE
		? g_strdup_printf (" align=\"%s\"",
				   d->align == HTML_HALIGN_CENTER ? "center"
				   : (d->align == HTML_HALIGN_RIGHT ? "right" : "left"))
		: g_strdup ("");
	width   = d->width != 0 && d->has_width
		? g_strdup_printf (" width=\"%d%s\"", d->width, d->width_percent ? "%" : "") : g_strdup ("");

	cells  = g_string_new (NULL);
	for (r = 0; r < d->rows; r ++) {
		g_string_append (cells, "<tr>");
		for (c = 0; c < d->cols; c ++) {
			gchar *cell;

			cell = g_strdup_printf ("<td>%d</td>", d->cols*r + c + 1);
			g_string_append (cells, cell);
			g_free (cell);
		}
		g_string_append (cells, "</tr>");
	}
			
	html      = g_strconcat (body, "<table", bg_color, bg_pixmap, spacing, align, width, ">",
				 cells->str, "</table>", NULL);
	/* printf ("html: %s\n", html); */
	gtk_html_load_from_string (d->sample, html, -1);

	g_string_free (cells, TRUE);
	g_free (body);
	g_free (bg_color);
	g_free (bg_pixmap);
	g_free (spacing);
	g_free (align);
	g_free (width);
	g_free (html);
}

static GtkHTMLEditTableProperties *
data_new (GtkHTMLControlData *cd)
{
	GtkHTMLEditTableProperties *data = g_new0 (GtkHTMLEditTableProperties, 1);

	/* fill data */
	data->cd                = cd;
	data->table             = NULL;

	data->bg_color          = html_colorset_get_color (data->cd->html->engine->settings->color_set,
							   HTMLBgColor)->color;
	data->bg_pixmap         = "";
	data->border            = 1;
	data->spacing           = 2;
	data->padding           = 1;
	data->align             = HTML_HALIGN_NONE;
	data->has_width         = TRUE;
	data->width_percent     = TRUE;
	data->width             = 100;
	data->cols              = 3;
	data->rows              = 3;

	return data;
}

static void
set_has_bg_color (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	d->has_bg_color = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_bg_color));
	FILL;
	CHANGE;
	if (!d->disable_change)
		d->changed_bg_color = TRUE;
}

static void
set_has_bg_pixmap (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	d->has_bg_pixmap = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap));
	FILL;
	CHANGE;
	if (!d->disable_change)
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
	if (!d->disable_change)
		d->changed_bg_color = TRUE;
	if (!d->has_bg_color)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_color), TRUE);
	else {
		FILL;
		CHANGE;
	}
}

static void
changed_bg_pixmap (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->bg_pixmap = gtk_entry_get_text (GTK_ENTRY (w));
	if (!d->disable_change)
		d->changed_bg_pixmap = TRUE;
	if (!d->has_bg_pixmap && d->bg_pixmap && *d->bg_pixmap)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap), TRUE);
	else {
		if (!d->bg_pixmap || !*d->bg_pixmap)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_pixmap), FALSE);
		FILL;
		CHANGE;
	}
}

static void
changed_spacing (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->spacing = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_spacing));
	if (!d->disable_change)
		d->changed_spacing = TRUE;
	FILL;
	CHANGE;
}

static void
changed_padding (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->padding = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_padding));
	if (!d->disable_change)
		d->changed_padding = TRUE;
	FILL;
	CHANGE;
}

static void
changed_border (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->border = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_border));
	if (!d->disable_change)
		d->changed_border = TRUE;
	FILL;
	CHANGE;
}

static void
changed_align (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->align = g_list_index (GTK_MENU_SHELL (w)->children, gtk_menu_get_active (GTK_MENU (w))) + HTML_HALIGN_LEFT;
	if (!d->disable_change)
		d->changed_align = TRUE;
	FILL;
	CHANGE;	
}

static void
changed_width (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_width));
	if (!d->disable_change) {
		d->disable_change = TRUE;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), TRUE);
		d->disable_change = FALSE;
		d->changed_width = TRUE;
	}
	FILL;
	CHANGE;
}

static void
set_has_width (GtkWidget *check, GtkHTMLEditTableProperties *d)
{
	d->has_width = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check_width));
	if (!d->disable_change)
		d->changed_width = TRUE;
	FILL;
	CHANGE;
}

static void
changed_width_percent (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->width_percent = g_list_index (GTK_MENU_SHELL (w)->children, gtk_menu_get_active (GTK_MENU (w))) ? TRUE : FALSE;
	if (!d->disable_change)
		d->changed_width = TRUE;
	FILL;
	CHANGE;	
}

static void
changed_cols (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->cols = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_cols));
	if (!d->disable_change)
		d->changed_cols = TRUE;
	FILL;
	CHANGE;
}

static void
changed_rows (GtkWidget *w, GtkHTMLEditTableProperties *d)
{
	d->rows = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (d->spin_rows));
	if (!d->disable_change)
		d->changed_rows = TRUE;
	FILL;
	CHANGE;
}

static GtkWidget *
table_widget (GtkHTMLEditTableProperties *d)
{
	HTMLColor *color;
	GtkWidget *table_page;
	GladeXML *xml;
	gchar     *dir;

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
	gtk_signal_connect (GTK_OBJECT (gnome_pixmap_entry_gtk_entry (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap))),
			    "changed", GTK_SIGNAL_FUNC (changed_bg_pixmap), d);
	dir = getcwd (NULL, 0);
	gnome_pixmap_entry_set_pixmap_subdir (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap), dir);
	free (dir);

	d->spin_spacing = glade_xml_get_widget (xml, "spin_spacing");
	gtk_signal_connect (GTK_OBJECT (d->spin_spacing), "changed", changed_spacing, d);
	d->spin_padding = glade_xml_get_widget (xml, "spin_padding");
	gtk_signal_connect (GTK_OBJECT (d->spin_padding), "changed", changed_padding, d);
	d->spin_border  = glade_xml_get_widget (xml, "spin_border");
	gtk_signal_connect (GTK_OBJECT (d->spin_border), "changed", changed_border, d);

	d->option_align = glade_xml_get_widget (xml, "option_table_align");
	gtk_signal_connect (GTK_OBJECT (gtk_option_menu_get_menu (GTK_OPTION_MENU (d->option_align))), "selection-done",
			    changed_align, d);

	d->spin_width   = glade_xml_get_widget (xml, "spin_table_width");
	gtk_signal_connect (GTK_OBJECT (d->spin_width), "changed", changed_width, d);
	d->check_width  = glade_xml_get_widget (xml, "check_table_width");
	gtk_signal_connect (GTK_OBJECT (d->check_width), "toggled", set_has_width, d);
	d->option_width = glade_xml_get_widget (xml, "option_table_width");
	gtk_signal_connect (GTK_OBJECT (gtk_option_menu_get_menu (GTK_OPTION_MENU (d->option_width))), "selection-done",
			    changed_width_percent, d);

	d->spin_cols = glade_xml_get_widget (xml, "spin_table_columns");
	gtk_signal_connect (GTK_OBJECT (d->spin_cols), "changed", changed_cols, d);
	d->spin_rows = glade_xml_get_widget (xml, "spin_table_rows");
	gtk_signal_connect (GTK_OBJECT (d->spin_rows), "changed", changed_rows, d);

	gtk_box_pack_start (GTK_BOX (table_page), sample_frame (&d->sample), FALSE, FALSE, 0);

	gtk_widget_show_all (table_page);
        gdk_color_alloc (gdk_window_get_colormap (d->cd->html->engine->window), &d->bg_color);
	gnome_pixmap_entry_set_preview (GNOME_PIXMAP_ENTRY (d->entry_bg_pixmap), FALSE);

	return table_page;
}

static GtkWidget *
table_insert_widget (GtkHTMLEditTableProperties *d)
{
	GtkWidget *table_page;
	GladeXML *xml;

	xml = glade_xml_new (GLADE_DATADIR "/gtkhtml-editor-properties.glade", "table_insert_page");
	if (!xml)
		g_error (_("Could not load glade file."));

	table_page = glade_xml_get_widget (xml, "table_insert_page");

	d->spin_cols = glade_xml_get_widget (xml, "spin_table_columns");
	gtk_signal_connect (GTK_OBJECT (d->spin_cols), "changed", changed_cols, d);
	d->spin_rows = glade_xml_get_widget (xml, "spin_table_rows");
	gtk_signal_connect (GTK_OBJECT (d->spin_rows), "changed", changed_rows, d);

	d->spin_width   = glade_xml_get_widget (xml, "spin_table_width");
	gtk_signal_connect (GTK_OBJECT (d->spin_width), "changed", changed_width, d);
	d->check_width  = glade_xml_get_widget (xml, "check_table_width");
	gtk_signal_connect (GTK_OBJECT (d->check_width), "toggled", set_has_width, d);
	d->option_width = glade_xml_get_widget (xml, "option_table_width");
	gtk_signal_connect (GTK_OBJECT (gtk_option_menu_get_menu (GTK_OPTION_MENU (d->option_width))), "selection-done",
			    changed_width_percent, d);

	gtk_widget_show_all (table_page);

	return table_page;
}

static void
set_ui (GtkHTMLEditTableProperties *d)
{
	d->disable_change = TRUE;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_bg_color), d->has_bg_color);
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
	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_width), d->width_percent ? 1 : 0);

	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_width), d->width_percent ? 1 : 0);
	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_width), d->width_percent ? 1 : 0);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_cols),  d->cols);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_rows),  d->rows);

	d->disable_change = FALSE;

	FILL;
}

static void
set_insert_ui (GtkHTMLEditTableProperties *d)
{
	d->disable_change = TRUE;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (d->check_width), d->has_width);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_width),  d->width);
	gtk_option_menu_set_history (GTK_OPTION_MENU (d->option_width), d->width_percent ? 1 : 0);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_cols),  d->cols);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (d->spin_rows),  d->rows);

	d->disable_change = FALSE;
}

static void
get_data (GtkHTMLEditTableProperties *d)
{
	d->table = html_engine_get_table (d->cd->html->engine);
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
	d->cols    = d->table->totalCols;
	d->rows    = d->table->totalRows;

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
	} else
		d->has_width = FALSE;
}


GtkWidget *
table_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTableProperties *data = data_new (cd);
	GtkWidget *rv;

	get_data (data);
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
	rv = table_insert_widget (data);
	set_insert_ui (data);
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);

	return rv;
}

void
table_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTableProperties *d = (GtkHTMLEditTableProperties *) get_data;

	html_engine_insert_table (cd->html->engine, d->cols, d->rows,
				  d->has_width ? (d->width_percent ? 0 : d->width) : 0,
				  d->has_width ? (d->width_percent ? d->width : 0) : 0,
				  d->padding, d->spacing, d->border);
}

void
table_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTableProperties *d = (GtkHTMLEditTableProperties *) get_data;

	if (d->changed_bg_color) {
		html_engine_table_set_bg_color (d->cd->html->engine, d->table, d->has_bg_color ? &d->bg_color : NULL);
		d->changed_bg_color = FALSE;
	}
	if (d->changed_bg_pixmap) {
		gchar *url = d->has_bg_pixmap ? g_strconcat ("file://", d->bg_pixmap, NULL) : NULL;

		html_engine_table_set_bg_pixmap (d->cd->html->engine, d->table, url);
		g_free (url);
		d->changed_bg_pixmap = FALSE;
	}
	if (d->changed_spacing) {
		html_engine_table_set_spacing (d->cd->html->engine, d->table, d->spacing);
		d->changed_spacing = FALSE;
	}
	if (d->changed_padding) {
		html_engine_table_set_padding (d->cd->html->engine, d->table, d->padding);
		d->changed_padding = FALSE;
	}
	if (d->changed_border) {
		html_engine_table_set_border_width (d->cd->html->engine, d->table, d->border, FALSE);
		d->changed_border = FALSE;
	}
	if (d->changed_align) {
		html_engine_table_set_align (d->cd->html->engine, d->table, d->align);
		d->changed_align = FALSE;
	}
	if (d->changed_width) {
		html_engine_table_set_width (d->cd->html->engine, d->table,
					     d->has_width ? d->width : 0, d->has_width ? d->width_percent : FALSE);
		d->changed_width = FALSE;
	}
}

void
table_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
