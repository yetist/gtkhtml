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

#include "htmlengine-edit-clueflowstyle.h"
#include "body.h"
#include "properties.h"

struct _GtkHTMLEditBodyProperties {
	GtkHTMLControlData *cd;

	GtkWidget *pixmap_entry;
	GtkWidget *selected_color_picker;

	GdkColor   color [HTMLColors];
	gboolean   color_changed [HTMLColors];
};
typedef struct _GtkHTMLEditBodyProperties GtkHTMLEditBodyProperties;

static void
set_color (GtkWidget *w, gushort r, gushort g, gushort b, gushort a, GtkHTMLEditBodyProperties *data)
{
	gint idx;

	idx = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (w), "type"));
	data->color [idx].red   = r;
	data->color [idx].green = g;
	data->color [idx].blue  = b;
	data->color_changed [idx] = TRUE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);
}

static void
set_color_button (GtkWidget *w, GtkHTMLEditBodyProperties *data)
{
	gdouble r, g, b, rn, gn, bn;

	gnome_color_picker_get_d (GNOME_COLOR_PICKER (data->selected_color_picker), &r, &g, &b, NULL);

	rn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].red)  /0xffff;
	gn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].green)/0xffff;
	bn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].blue) /0xffff;

	if (r != rn || g != gn || b != bn) {
		gnome_color_picker_set_d (GNOME_COLOR_PICKER (data->selected_color_picker), rn, gn, bn, 1.0);
		set_color (data->selected_color_picker, rn*0xffff, gn*0xffff, bn*0xffff, 0xffff, data);
	}
}

static void
select_color (GtkWidget *w, GtkHTMLEditBodyProperties *data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w))) {
		data->selected_color_picker = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (w), "cpicker"));
	}
}

GtkWidget *
body_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditBodyProperties *data = g_new0 (GtkHTMLEditBodyProperties, 1);
	GtkWidget *hbox, *vbox, *table, *button, *check, *frame, *radio, *cpicker, *hbox1;
	GtkStyle *style;
	GSList *group;
	GdkColor *color;
	guint val, i, j, add_val;

	*set_data = data;
	data->cd = cd;

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_border_width (GTK_CONTAINER (hbox), 3);
	vbox = gtk_vbox_new (FALSE, 2);
	check = gtk_check_button_new_with_label (_("use background image"));
	data->pixmap_entry = gnome_pixmap_entry_new ("background_image", _("Background image"), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), data->pixmap_entry);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	frame = gtk_frame_new (_("Colors"));
	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	group = NULL;
#define ADD_RADIO(x, ct) \
	radio = gtk_radio_button_new (group); \
	cpicker = gnome_color_picker_new (); \
	gtk_container_border_width (GTK_CONTAINER (radio), 0); \
        gtk_object_set_data (GTK_OBJECT (cpicker), "type", GINT_TO_POINTER (ct)); \
        gtk_object_set_data (GTK_OBJECT (radio), "cpicker", cpicker); \
        gtk_signal_connect (GTK_OBJECT (radio), "toggled", select_color, data); \
        gtk_signal_connect (GTK_OBJECT (cpicker), "color_set", GTK_SIGNAL_FUNC (set_color), data); \
	group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio)); \
        color = &html_colorset_get_color (cd->html->engine->settings->color_set, ct)->color; \
        gnome_color_picker_set_d (GNOME_COLOR_PICKER (cpicker), (gdouble)color->red/0xffff, (gdouble)color->green/0xffff, (gdouble)color->blue/0xffff, 1.0); \
	hbox1 = gtk_hbox_new (FALSE, 3); \
	gtk_box_pack_start (GTK_BOX (hbox1), cpicker, FALSE, FALSE, 0); \
	gtk_box_pack_start (GTK_BOX (hbox1), gtk_label_new (_(x)), FALSE, FALSE, 0); \
	gtk_container_add (GTK_CONTAINER (radio), hbox1); \
	gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);

	ADD_RADIO ("Text color", HTMLTextColor); data->selected_color_picker = cpicker;
	ADD_RADIO ("Link color", HTMLLinkColor);
	ADD_RADIO ("Background color", HTMLBgColor);

	table = gtk_table_new (8, 8, TRUE);
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
			gtk_table_attach_defaults (GTK_TABLE (table), button, i, i+1, j, j+1);
		}
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

	return hbox;
}

void
body_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditBodyProperties *data = (GtkHTMLEditBodyProperties *) get_data;
	gboolean redraw = FALSE;

#define APPLY_COLOR(c) \
	if (data->color_changed [c]) { \
		html_colorset_set_color (cd->html->engine->settings->color_set, &data->color [c], c); \
                redraw = TRUE; \
        }

	APPLY_COLOR (HTMLTextColor);
	APPLY_COLOR (HTMLLinkColor);
	APPLY_COLOR (HTMLBgColor);

	if (redraw)
		gtk_widget_queue_draw (GTK_WIDGET (cd->html));
}

void
body_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
