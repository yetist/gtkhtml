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

#include "htmlengine-edit.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-save.h"
#include "text.h"
#include "properties.h"
#include "utils.h"

struct _GtkHTMLEditTextProperties {

	GtkHTMLControlData *cd;

	GtkWidget *color_picker;
	GtkWidget *style_option;
	GtkWidget *sel_size;
	GtkWidget *check [4];

	gboolean color_changed;
	gboolean style_changed;

	GtkHTMLFontStyle style_and;
	GtkHTMLFontStyle style_or;
	GdkColor color;

	GtkHTML *sample;
};
typedef struct _GtkHTMLEditTextProperties GtkHTMLEditTextProperties;

#define STYLES 4
static GtkHTMLFontStyle styles [STYLES] = {
	GTK_HTML_FONT_STYLE_BOLD,
	GTK_HTML_FONT_STYLE_ITALIC,
	GTK_HTML_FONT_STYLE_UNDERLINE,
	GTK_HTML_FONT_STYLE_STRIKEOUT,
};

#define CVAL(i) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (d->check [i])))

static gint get_size (GtkHTMLFontStyle s);

static void
fill_sample (GtkHTMLEditTextProperties *d)
{
	gchar *body, *size, *color, *bg;

	bg    = html_engine_save_get_sample_body (d->cd->html->engine, NULL);
	size  = g_strdup_printf ("<font size=%d>", get_size (d->style_or) + 1);
	color = g_strdup_printf ("<font color=#%2x%2x%2x>",
				 d->color.red >> 8,
				 d->color.green >> 8,
				 d->color.blue >> 8);
	body  = g_strconcat (bg,
			     CVAL (0) ? "<b>" : "",
			     CVAL (1) ? "<i>" : "",
			     CVAL (2) ? "<u>" : "",
			     CVAL (3) ? "<s>" : "",
			     size,
			     color,
			     "The quick brown fox jumps over the lazy dog.", NULL);
	printf ("body: %s\n", body);
	gtk_html_load_from_string (d->sample, body, -1);
	g_free (color);
	g_free (size);
	g_free (bg);
	g_free (body);
}

static void
set_color (GtkWidget *w, gushort r, gushort g, gushort b, gushort a, GtkHTMLEditTextProperties *data)
{
	data->color.red   = r;
	data->color.green = g;
	data->color.blue  = b;
	data->color_changed = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
	fill_sample (data);
}

static void
set_color_button (GtkWidget *w, GtkHTMLEditTextProperties *data)
{
	gdouble r, g, b, rn, gn, bn;

	gnome_color_picker_get_d (GNOME_COLOR_PICKER (data->color_picker), &r, &g, &b, NULL);

	rn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].red)  /0xffff;
	gn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].green)/0xffff;
	bn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].blue) /0xffff;

	if (r != rn || g != gn || b != bn) {
		gnome_color_picker_set_d (GNOME_COLOR_PICKER (data->color_picker), rn, gn, bn, 1.0);
		set_color (data->color_picker, rn*0xffff, gn*0xffff, bn*0xffff, 0xffff, data);
	}
}

static void
set_size (GtkWidget *w, GtkHTMLEditTextProperties *data)
{
	gint size = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (w), "size"));

	data->style_and &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	data->style_or  &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	data->style_or  |= size;
	data->style_changed = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
	fill_sample (data);
}

static void
set_style (GtkWidget *w, GtkHTMLEditTextProperties *d)
{
	GtkHTMLFontStyle style = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (w), "style"));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w))) {
		d->style_or  |= style;
		d->style_and |= style;
	} else
		d->style_and &= ~style;

	d->style_changed = TRUE;
	gtk_html_edit_properties_dialog_change (d->cd->properties_dialog);
	fill_sample (d);
}

static gint
get_size (GtkHTMLFontStyle s)
{
	return (s & GTK_HTML_FONT_STYLE_SIZE_MASK)
		? (s & GTK_HTML_FONT_STYLE_SIZE_MASK) - GTK_HTML_FONT_STYLE_SIZE_1
		: 2;
}

GtkWidget *
text_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTextProperties *data = g_new (GtkHTMLEditTextProperties, 1);
	GtkWidget *mvbox, *vbox, *hbox, *frame, *table, *table1, *menu, *menuitem;
	gint i;
	gdouble rn, gn, bn;

	*set_data = data;

	data->cd = cd;
	data->color_changed   = FALSE;
	data->style_changed   = FALSE;
	data->style_and       = GTK_HTML_FONT_STYLE_MAX;
	data->style_or        = html_engine_get_font_style (cd->html->engine);
	data->color           = html_engine_get_color (cd->html->engine)->color;

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);

	frame = gtk_frame_new (_("Style"));
	vbox = gtk_vbox_new (FALSE, 2);

#define ADD_CHECK(x) \
	data->check [i] = gtk_check_button_new_with_label (x); \
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->check [i]), data->style_or & styles [i]); \
        gtk_object_set_data (GTK_OBJECT (data->check [i]), "style", GUINT_TO_POINTER (styles [i])); \
        gtk_signal_connect (GTK_OBJECT (data->check [i]), "toggled", set_style, data); \
	gtk_box_pack_start (GTK_BOX (vbox), data->check [i], FALSE, FALSE, 0); i++

	i=0;
	ADD_CHECK (_("Bold"));
	ADD_CHECK (_("Italic"));
	ADD_CHECK (_("Underline"));
	ADD_CHECK (_("Strikeout"));

	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);

	frame = gtk_frame_new (_("Size"));
	menu = gtk_menu_new ();

#undef ADD_ITEM
#define ADD_ITEM(n) \
	menuitem = gtk_menu_item_new_with_label (_(n)); \
        gtk_menu_append (GTK_MENU (menu), menuitem); \
        gtk_widget_show (menuitem); \
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (set_size), data); \
        gtk_object_set_data (GTK_OBJECT (menuitem), "size", GINT_TO_POINTER (i)); i++;

	i=GTK_HTML_FONT_STYLE_SIZE_1;
	ADD_ITEM("-2");
	ADD_ITEM("-1");
	ADD_ITEM("+0");
	ADD_ITEM("+1");
	ADD_ITEM("+2");
	ADD_ITEM("+3");
	ADD_ITEM("+4");

	data->sel_size = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->sel_size), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->sel_size), get_size (data->style_or));
	gtk_container_add (GTK_CONTAINER (frame), data->sel_size);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 1, 2);

	/* color selection */
	frame = gtk_frame_new (_("Color"));
	data->color_picker = gnome_color_picker_new ();

	rn = ((gdouble) data->color.red)  /0xffff;
	gn = ((gdouble) data->color.green)/0xffff;
	bn = ((gdouble) data->color.blue) /0xffff;

	gnome_color_picker_set_d (GNOME_COLOR_PICKER (data->color_picker), rn, gn, bn, 1.0);
        gtk_signal_connect (GTK_OBJECT (data->color_picker), "color_set", GTK_SIGNAL_FUNC (set_color), data);

	vbox = gtk_vbox_new (FALSE, 2);
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);
	gtk_box_pack_start (GTK_BOX (hbox), data->color_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("text color")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	table1 = color_table_new (set_color_button, data);
	gtk_box_pack_start (GTK_BOX (vbox), table1, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 2, GTK_FILL, GTK_FILL, 0, 0);

	mvbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mvbox), table, FALSE, FALSE, 0);

	/* sample */
	gtk_box_pack_start_defaults (GTK_BOX (mvbox), sample_frame (&data->sample));
	fill_sample (data);

	return mvbox;
}

void
text_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTextProperties *data = (GtkHTMLEditTextProperties *) get_data;

	if (data->style_changed) {
		printf ("and: %x or: %x\n", data->style_and, data->style_or);
		html_engine_set_font_style (cd->html->engine, data->style_and, data->style_or);
	}

	if (data->color_changed) {
		HTMLColor *color = html_color_new_from_gdk_color (&data->color);
		html_engine_set_color (cd->html->engine, color);
		html_color_unref (color);
	}

	data->color_changed = FALSE;
	data->style_changed = FALSE;
}

void
text_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
