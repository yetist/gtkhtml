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
#include "text.h"
#include "properties.h"

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
};
typedef struct _GtkHTMLEditTextProperties GtkHTMLEditTextProperties;

static void
set_color (GtkWidget *w, gushort r, gushort g, gushort b, gushort a, GtkHTMLEditTextProperties *data)
{
	data->color.red   = r;
	data->color.green = g;
	data->color.blue  = b;
	data->color_changed = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
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

	printf ("size %d\n", size);

	data->style_and &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	data->style_or  |= size;
	data->style_changed = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
}

static void
set_style (GtkWidget *w, GtkHTMLEditTextProperties *data)
{
	GtkHTMLFontStyle style = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (w), "style"));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
		data->style_or |= style;
	else
		data->style_and &= ~style;

	data->style_changed = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
}

static gint
get_size (GtkHTMLFontStyle s)
{
	return (s & GTK_HTML_FONT_STYLE_SIZE_MASK)
		? (s & GTK_HTML_FONT_STYLE_SIZE_MASK) - GTK_HTML_FONT_STYLE_SIZE_1
		: 2;
}

#define STYLES 4
static GtkHTMLFontStyle styles [STYLES] = {
	GTK_HTML_FONT_STYLE_BOLD,
	GTK_HTML_FONT_STYLE_ITALIC,
	GTK_HTML_FONT_STYLE_UNDERLINE,
	GTK_HTML_FONT_STYLE_STRIKEOUT,
};

GtkWidget *
text_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditTextProperties *data = g_new (GtkHTMLEditTextProperties, 1);
	GtkWidget *mvbox, *vbox, *hbox, *frame, *table, *table1, *button, *menu, *menuitem;
	GtkStyle *style;
	GdkColor *color;
	GtkHTMLFontStyle font_style;
	gint i, j;
	guint val, add_val;
	gdouble rn, gn, bn;

	*set_data = data;

	data->cd = cd;
	data->color_changed   = FALSE;
	data->style_changed   = FALSE;
	data->style_and       = GTK_HTML_FONT_STYLE_MAX;
	data->style_or        = 0;

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table), 3);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);

	frame = gtk_frame_new (_("Style"));
	vbox = gtk_vbox_new (FALSE, 2);

	font_style = html_engine_get_font_style (cd->html->engine);
	color      = html_engine_get_color      (cd->html->engine);

#define ADD_CHECK(x) \
	data->check [i] = gtk_check_button_new_with_label (x); \
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->check [i]), font_style & styles [i]); \
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
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->sel_size), get_size (font_style));
	gtk_container_add (GTK_CONTAINER (frame), data->sel_size);
	gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 1, 2);

	/* color selection */
	frame = gtk_frame_new (_("Color"));
	data->color_picker = gnome_color_picker_new ();

	rn = ((gdouble) color->red)  /0xffff;
	gn = ((gdouble) color->green)/0xffff;
	bn = ((gdouble) color->blue) /0xffff;

	gnome_color_picker_set_d (GNOME_COLOR_PICKER (data->color_picker), rn, gn, bn, 1.0);
        gtk_signal_connect (GTK_OBJECT (data->color_picker), "color_set", GTK_SIGNAL_FUNC (set_color), data);

	vbox = gtk_vbox_new (FALSE, 2);
	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);
	gtk_box_pack_start (GTK_BOX (hbox), data->color_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("text color")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	table1 = gtk_table_new (8, 8, TRUE);
	for (val=0, i=0; i<8; i++)
		for (j=0; j<8; j++, val++) {

			button = gtk_button_new ();
			gtk_widget_set_usize (button, 24, 16);
			style = gtk_style_copy (button->style);
			add_val = (val * 0x3fff / 0x3f);

			style->bg [GTK_STATE_NORMAL].red   = ((val & 12) << 12) | add_val;
			style->bg [GTK_STATE_NORMAL].green = ((((val & 16) >> 2) | (val & 2))<< 13) | add_val;
			style->bg [GTK_STATE_NORMAL].blue  = ((((val & 32) >> 4) | (val & 1))<< 14) | add_val;
			style->bg [GTK_STATE_ACTIVE] = style->bg [GTK_STATE_NORMAL];
			style->bg [GTK_STATE_PRELIGHT] = style->bg [GTK_STATE_NORMAL];

			gtk_signal_connect (GTK_OBJECT (button), "clicked", set_color_button, data);
			gtk_widget_set_style (button, style);
			gtk_table_attach_defaults (GTK_TABLE (table1), button, i, i+1, j, j+1);
		}

	gtk_box_pack_start (GTK_BOX (vbox), table1, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 2, GTK_FILL, GTK_FILL, 0, 0);

	mvbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mvbox), table, FALSE, FALSE, 0);

	return mvbox;
}

void
text_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditTextProperties *data = (GtkHTMLEditTextProperties *) get_data;
	gint i;
	printf ("text apply\n");

	if (data->style_changed) {
		for (i=0; i<STYLES; i++) {
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->check [i])))
				data->style_or |= styles [i];
			else
				data->style_and &= ~styles [i];
		}
		html_engine_set_font_style (cd->html->engine, data->style_and, data->style_or);
	}

	if (data->color_changed)
		html_engine_set_color (cd->html->engine, &data->color);

	data->color_changed = FALSE;
	data->style_changed = FALSE;
}

void
text_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	printf ("text close\n");

	g_free (get_data);
}
