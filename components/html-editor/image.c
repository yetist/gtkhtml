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

#include <unistd.h>
#include <string.h>
#include "config.h"
#include "dialog.h"
#include "image.h"
#include "htmlengine-edit-images.h"
#include "htmlimage.h"
#include "properties.h"

#define GTK_HTML_EDIT_IMAGE_BWIDTH      0
#define GTK_HTML_EDIT_IMAGE_WIDTH       1
#define GTK_HTML_EDIT_IMAGE_HEIGHT      2
#define GTK_HTML_EDIT_IMAGE_HSPACE      3
#define GTK_HTML_EDIT_IMAGE_VSPACE      4
#define GTK_HTML_EDIT_IMAGE_SPINS       5

#define ALIGN_TOP    0
#define ALIGN_CENTER 1
#define ALIGN_BOTTOM 2

struct _GtkHTMLEditImageProperties {

	GtkHTMLControlData *cd;

	GtkWidget   *pentry;
	GtkWidget   *entry_alt;

	GtkWidget   *check [GTK_HTML_EDIT_IMAGE_SPINS];
	GtkWidget   *spin  [GTK_HTML_EDIT_IMAGE_SPINS];
	GtkObject   *adj   [GTK_HTML_EDIT_IMAGE_SPINS];
	gint         val   [GTK_HTML_EDIT_IMAGE_SPINS];
	gboolean     set   [GTK_HTML_EDIT_IMAGE_SPINS];

	GtkWidget   *check_percent;
	gboolean     percent;

	GtkWidget   *sel_align;
	HTMLImage   *image;
	guint        align;
};
typedef struct _GtkHTMLEditImageProperties GtkHTMLEditImageProperties;

static void
entry_changed (GtkWidget *entry, GtkHTMLEditImageProperties *d)
{
	gchar *text;

	text = gtk_entry_get_text (GTK_ENTRY (entry));
	gtk_html_edit_properties_dialog_change (d->cd->properties_dialog);
}

static void
menu_activate (GtkWidget *mi, GtkHTMLEditImageProperties *d)
{
	d->align = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (mi), "idx"));
}

static void
width_toggled (GtkWidget *check, GtkHTMLEditImageProperties *d)
{
	gtk_widget_set_sensitive (d->check_percent,
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
}

static void
percent_toggled (GtkWidget *check, GtkHTMLEditImageProperties *d)
{
	d->percent = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
}

static void
check_toggled (GtkWidget *check, GtkHTMLEditImageProperties *d)
{
	guint idx;

	for (idx = 0; idx < GTK_HTML_EDIT_IMAGE_SPINS; idx++)
		if (check == d->check [idx])
			break;
	if (check != d->check [idx])
		g_assert_not_reached ();

	d->set [idx] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	gtk_widget_set_sensitive (d->spin [idx], d->set [idx]);	
}

static void
checked_val (GtkHTMLEditImageProperties *d, gint idx, const gchar *name)
{
	d->check [idx] = gtk_check_button_new_with_label (name);
	d->adj   [idx] = gtk_adjustment_new (d->val [idx], 0, 32767, 1, 1, 1);
	d->spin  [idx] = gtk_spin_button_new (GTK_ADJUSTMENT (d->adj [idx]), 1, 0);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check [idx]), d->set [idx]);
	gtk_widget_set_sensitive (d->spin [idx], d->set [idx]);

	gtk_signal_connect (GTK_OBJECT (d->check [idx]), "toggled", GTK_SIGNAL_FUNC (check_toggled), d);
}

GtkWidget *
image_insertion (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkHTMLEditImageProperties *data = g_new0 (GtkHTMLEditImageProperties, 1);
	GtkWidget *hbox, *hb1, *mhb;
	GtkWidget *vbox, *vb1;
	GtkWidget *table;
	GtkWidget *frame;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gchar     *dir;
	guint      malign = 0;

	*set_data = data;
	data->cd = cd;
	data->image = NULL;

	mhb = gtk_hbox_new (FALSE, 3);
	vb1 = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (mhb), 3);

	data->pentry = gnome_pixmap_entry_new ("insert_image", _("Image selection"), TRUE);
	/* gnome_pixmap_entry_set_preview_size (GNOME_PIXMAP_ENTRY (data->pentry), 200, 200); */
	dir = getcwd (NULL, 0);
	gnome_pixmap_entry_set_pixmap_subdir (GNOME_PIXMAP_ENTRY (data->pentry), dir);
	free (dir);
	gtk_box_pack_start_defaults (GTK_BOX (mhb), data->pentry);

	hb1   = gtk_hbox_new (FALSE, 3);
	frame = gtk_frame_new (_("Border"));
	hbox = gtk_hbox_new (FALSE, 3);

#undef ADD_VAL
#define ADD_VAL(x,y) \
	checked_val (data, x, _(y)); \
	gtk_box_pack_start (GTK_BOX (hbox), data->check [x], FALSE, FALSE, 0); \
	gtk_box_pack_start (GTK_BOX (hbox), data->spin  [x], FALSE, FALSE, 0);

	ADD_VAL (GTK_HTML_EDIT_IMAGE_BWIDTH, "width");

	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start_defaults (GTK_BOX (hb1), frame);

	menu = gtk_menu_new ();
	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_border_width (GTK_CONTAINER (hbox), 3);

#undef ADD_ITEM
#define ADD_ITEM(n) \
	menuitem = gtk_menu_item_new_with_label (_(n)); \
        gtk_menu_append (GTK_MENU (menu), menuitem); \
        gtk_widget_show (menuitem); \
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (menu_activate), data); \
        gtk_object_set_data (GTK_OBJECT (menuitem), "idx", GINT_TO_POINTER (malign)); \
        malign++;

	ADD_ITEM("Top");
	ADD_ITEM("Center");
	ADD_ITEM("Bottom");
	/* ADD_ITEM("Left",   GDK_F4);
	   ADD_ITEM("Right",  GDK_F5); */

	frame = gtk_frame_new (_("Alignment"));
	data->sel_align = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->sel_align), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->sel_align), data->align);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), data->sel_align);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start_defaults (GTK_BOX (hb1), frame);
	gtk_box_pack_start (GTK_BOX (vb1), hb1, FALSE, FALSE, 0);

	/* size and spacing */
	hbox = gtk_hbox_new (FALSE, 3);
	vbox = gtk_vbox_new (FALSE, 0);
	frame = gtk_frame_new (_("Size"));

#undef ADD_VAL
#define ADD_VAL(x, y, i, n) \
	checked_val (data, i, _(n)); \
	gtk_table_attach (GTK_TABLE (table), data->check [i], x,   x+1, y, y+1, GTK_FILL, 0, 0, 0); \
	gtk_table_attach (GTK_TABLE (table), data->spin [i],  x+1, x+2, y, y+1, GTK_FILL, 0, 0, 0);

	table = gtk_table_new (2, 3, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 3);

	ADD_VAL (0, 0, GTK_HTML_EDIT_IMAGE_WIDTH,  "width");
	ADD_VAL (0, 1, GTK_HTML_EDIT_IMAGE_HEIGHT, "height");

	data->check_percent = gtk_check_button_new_with_label ("%");
	gtk_widget_set_sensitive (data->check_percent, data->set [GTK_HTML_EDIT_IMAGE_WIDTH]);
	gtk_signal_connect (GTK_OBJECT (data->check [GTK_HTML_EDIT_IMAGE_WIDTH]), "toggled",
					GTK_SIGNAL_FUNC (width_toggled), data);
	gtk_signal_connect (GTK_OBJECT (data->check_percent), "toggled",
					GTK_SIGNAL_FUNC (percent_toggled), data);
	gtk_table_attach (GTK_TABLE (table), data->check_percent, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);

	/* spacing */
	vbox = gtk_vbox_new (FALSE, 0);
	frame = gtk_frame_new (_("Padding"));

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), 3);

	ADD_VAL (2, 0, GTK_HTML_EDIT_IMAGE_HSPACE, "horizontal");
	ADD_VAL (2, 1, GTK_HTML_EDIT_IMAGE_VSPACE, "vertical");

	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);

	gtk_box_pack_start (GTK_BOX (vb1), hbox, FALSE, FALSE, 0);

	gtk_signal_connect (GTK_OBJECT (gnome_pixmap_entry_gtk_entry (GNOME_PIXMAP_ENTRY (data->pentry))),
			    "changed", GTK_SIGNAL_FUNC (entry_changed), data);

	gtk_box_pack_start_defaults (GTK_BOX (mhb), vb1);

	return mhb;
}

GtkWidget *
image_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkWidget *w = image_insertion (cd, set_data);
	GtkHTMLEditImageProperties *d = (GtkHTMLEditImageProperties *) *set_data;
	HTMLImage *image = HTML_IMAGE (cd->html->engine->cursor->object);
	HTMLImagePointer *ip = image->image_ptr;
	gint off = 0;

	g_assert (HTML_OBJECT_TYPE (cd->html->engine->cursor->object) == HTML_TYPE_IMAGE);
	d->image = image;

        if (!strncmp (ip->url, "file:", 5))
		off = 5;

	gtk_entry_set_text (GTK_ENTRY (gnome_pixmap_entry_gtk_entry (GNOME_PIXMAP_ENTRY (d->pentry))),
			    ip->url + off);

#define SET_ADJ(which, nval, val) \
        if (image->## val != nval) { \
		gtk_adjustment_set_value (GTK_ADJUSTMENT (d->adj [which]), image->## val); \
                gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check [which]), TRUE); \
	} else { \
		gtk_adjustment_set_value (GTK_ADJUSTMENT (d->adj [which]), 0); \
                gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check [which]), FALSE); \
	}

	SET_ADJ (GTK_HTML_EDIT_IMAGE_BWIDTH,  0, border);
	SET_ADJ (GTK_HTML_EDIT_IMAGE_WIDTH,  -1, specified_width);
	SET_ADJ (GTK_HTML_EDIT_IMAGE_HEIGHT, -1, specified_height);
	SET_ADJ (GTK_HTML_EDIT_IMAGE_HSPACE,  0, hspace);
	SET_ADJ (GTK_HTML_EDIT_IMAGE_VSPACE,  0, vspace);

	if (HTML_OBJECT (image)->percent > 0) {
		gtk_adjustment_set_value (GTK_ADJUSTMENT (d->adj [GTK_HTML_EDIT_IMAGE_WIDTH]),
					  HTML_OBJECT (image)->percent);
		gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check [GTK_HTML_EDIT_IMAGE_WIDTH]), TRUE);
		gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check_percent), TRUE);
		d->percent = TRUE;
	} else
		gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check_percent), FALSE);

	switch (image->valign) {
	case HTML_VALIGN_TOP:
		d->align = ALIGN_TOP;
		break;
	case HTML_VALIGN_CENTER:
		d->align = ALIGN_CENTER;
		break;
	case HTML_VALIGN_BOTTOM:
		d->align = ALIGN_BOTTOM;
		break;
	default:
		g_assert_not_reached ();
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (d->sel_align), d->align);
	

	return w;
}

static void
insert_or_apply (GtkHTMLControlData *cd, gpointer get_data, gboolean insert)
{	
	GtkHTMLEditImageProperties *data = (GtkHTMLEditImageProperties *) get_data;
	gchar *file;
	gint16 width;
	gint16 height;
	gint8 percent;
	gint8 border;
	gint8 hspace;
	gint8 vspace;
	HTMLHAlignType halign = HTML_HALIGN_NONE;
	HTMLVAlignType valign = HTML_VALIGN_BOTTOM;

	file    = g_strconcat ("file:", gnome_pixmap_entry_get_filename (GNOME_PIXMAP_ENTRY (data->pentry)), NULL);
	width   = -1;
	percent =  0;
	if (data->set [GTK_HTML_EDIT_IMAGE_WIDTH]) {
		if (data->percent) {
			percent = GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_WIDTH])->value;
		} else {
			width   = GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_WIDTH])->value;
		}
	}
	height = (data->set [GTK_HTML_EDIT_IMAGE_HEIGHT])
		? GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_HEIGHT])->value : -1;
	hspace = (data->set [GTK_HTML_EDIT_IMAGE_HSPACE])
		? GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_HSPACE])->value : 0;
	vspace = (data->set [GTK_HTML_EDIT_IMAGE_VSPACE])
		? GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_VSPACE])->value : 0;
	border = (data->set [GTK_HTML_EDIT_IMAGE_BWIDTH])
		? GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_IMAGE_BWIDTH])->value : 0;

	switch (data->align) {
	case ALIGN_TOP:
		valign = HTML_VALIGN_TOP;
		break;
	case ALIGN_CENTER:
		valign = HTML_VALIGN_CENTER;
		break;
	case ALIGN_BOTTOM:
		valign = HTML_VALIGN_BOTTOM;
		break;
	default:
		g_assert_not_reached ();
	}

	if (insert)
		html_engine_insert_image (data->cd->html->engine,
					  file,
					  NULL, NULL,
					  width, height, percent, border,
					  html_colorset_get_color (data->cd->html->engine->settings->color_set,
								   HTMLLinkColor),
					  halign, valign,
					  hspace, vspace);
	else {
		HTMLImage *image = HTML_IMAGE (data->image);

		g_assert (HTML_OBJECT_TYPE (data->image) == HTML_TYPE_IMAGE);

		if (data->set [GTK_HTML_EDIT_IMAGE_BWIDTH])
			html_image_set_border (image, border);
		html_image_set_size     (image, width, percent, height);
		html_image_set_url      (image, file);
		html_image_set_spacing  (image, hspace, vspace);
		html_image_set_valign   (image, valign);
	}

	g_free (file);
}

void
image_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	insert_or_apply (cd, get_data, FALSE);
}

void
image_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	insert_or_apply (cd, get_data, TRUE);
}

void
image_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
