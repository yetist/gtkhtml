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

#include <string.h>
#include "gtkhtml.h"
#include "config.h"
#include "properties.h"
#include "dialog.h"
#include "rule.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-insert.h"
#include "htmlengine-edit-rule.h"

#define GTK_HTML_EDIT_RULE_WIDTH       0
#define GTK_HTML_EDIT_RULE_SIZE        1
#define GTK_HTML_EDIT_RULE_SPINS       2

struct _GtkHTMLEditRuleProperties {
	GtkHTMLControlData *cd;

	GtkWidget   *check [GTK_HTML_EDIT_RULE_SPINS];
	GtkWidget   *spin  [GTK_HTML_EDIT_RULE_SPINS];
	GtkObject   *adj   [GTK_HTML_EDIT_RULE_SPINS];
	gboolean     set   [GTK_HTML_EDIT_RULE_SPINS];

	gboolean percent;
	GtkWidget *width_measure;

	gboolean shaded;
	GtkWidget *shaded_check;

	HTMLHAlignType align;
	GtkWidget *align_option;

	GtkWidget *sample;

	gboolean disable_change;
};
typedef struct _GtkHTMLEditRuleProperties GtkHTMLEditRuleProperties;

#define CHANGE if (!d->disable_change) gtk_html_edit_properties_dialog_change (d->cd->properties_dialog)
#define FILL 	if (!d->disable_change) fill_sample (d)
#define VAL(x) (gint)GTK_ADJUSTMENT (d->adj [GTK_HTML_EDIT_RULE_ ## x])->value

static void
fill_sample (GtkHTMLEditRuleProperties *d)
{
	GtkHTMLStream *stream;
	gchar *body, *width, *size, *align, *noshade;

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
	body    = g_strconcat ("<br><hr", width, size, align, noshade, ">", NULL);

	printf ("body: %s\n", body);

	stream = gtk_html_begin (GTK_HTML (d->sample));
	gtk_html_write (GTK_HTML (d->sample), stream, body, strlen (body));
	gtk_html_end (GTK_HTML (d->sample), stream, GTK_HTML_STREAM_OK);

	g_free (width);
	g_free (size);
	g_free (noshade);
	g_free (align);
	g_free (body);
}

static void
check_toggled (GtkWidget *check, GtkHTMLEditRuleProperties *d)
{
	guint idx;

	for (idx = 0; idx < GTK_HTML_EDIT_RULE_SPINS; idx++)
		if (check == d->check [idx])
			break;
	if (check != d->check [idx])
		g_assert_not_reached ();

	d->set [idx] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	gtk_widget_set_sensitive (d->spin [idx], d->set [idx]);	
	FILL;
	CHANGE;
}

static void
adj_changed (GtkAdjustment *adj, GtkHTMLEditRuleProperties *d)
{
	FILL;
}

static void
checked_val (GtkHTMLEditRuleProperties *d, gint idx, const gchar *name)
{
	d->check [idx] = gtk_check_button_new_with_label (name);
	d->adj   [idx] = gtk_adjustment_new (0, 0, 32767, 1, 1, 1);
	d->spin  [idx] = gtk_spin_button_new (GTK_ADJUSTMENT (d->adj [idx]), 1, 0);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (d->check [idx]), d->set [idx]);
	gtk_widget_set_sensitive (d->spin [idx], d->set [idx]);

	gtk_signal_connect (GTK_OBJECT (d->check [idx]), "toggled", GTK_SIGNAL_FUNC (check_toggled), d);
	gtk_signal_connect (GTK_OBJECT (d->adj [idx]), "value_changed", adj_changed, d);
}

static void
align_menu_activate (GtkWidget *mi, GtkHTMLEditRuleProperties *d)
{
	d->align = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (mi), "idx"));
	FILL;
	CHANGE;
}

static void
percent_menu_activate (GtkWidget *mi, GtkHTMLEditRuleProperties *d)
{
	d->percent = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (mi), "idx"));
	FILL;
	CHANGE;
}

static void
width_toggled (GtkWidget *check, GtkHTMLEditRuleProperties *d)
{
	gtk_widget_set_sensitive (d->width_measure,
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
	FILL;
	CHANGE;
}

static void
shade_toggled (GtkWidget *check, GtkHTMLEditRuleProperties *d)
{
	d->shaded = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	FILL;
	CHANGE;
}

#define HBOX \
	hbox  = gtk_hbox_new (FALSE, 3); \
	gtk_container_border_width (GTK_CONTAINER (hbox), 3)

#undef ADD_VAL
#define ADD_VAL(x,y) \
	checked_val (data, x, _(y)); \
	gtk_box_pack_start (GTK_BOX (hbox), data->check [x], FALSE, FALSE, 0); \
	gtk_box_pack_start (GTK_BOX (hbox), data->spin  [x], FALSE, FALSE, 0);

#undef ADD_ITEM
#define ADD_ITEM(n,f) \
	menuitem = gtk_menu_item_new_with_label (_(n)); \
        gtk_menu_append (GTK_MENU (menu), menuitem); \
        gtk_widget_show (menuitem); \
        gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (f), data); \
        gtk_object_set_data (GTK_OBJECT (menuitem), "idx", GINT_TO_POINTER (mcounter)); \
        mcounter++;

GtkWidget *
rule_properties (GtkHTMLControlData *cd, gpointer *set_data)
{
	GtkWidget *vbox, *mhb, *hbox, *frame, *sw, *menu, *menuitem, *vb1, *vb2;
	GtkHTMLEditRuleProperties *data = g_new0 (GtkHTMLEditRuleProperties, 1);
	gint mcounter;

	/* fill data */
	*set_data            = data;
	data->cd             = cd;
	data->percent        = TRUE;
	data->align          = HTML_HALIGN_CENTER;
	data->shaded         = TRUE;
	data->disable_change = TRUE;

	/* prepare content */
	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_border_width (GTK_CONTAINER (vbox), 3);

	mhb  = gtk_hbox_new (FALSE, 3);
	vb2  = gtk_vbox_new (FALSE, 2);

	/* style */
	frame = gtk_frame_new (_("Style"));
	HBOX;
	menu  = gtk_menu_new ();
	mcounter = 0;
	data->shaded_check = gtk_check_button_new_with_label (_("shaded"));
	gtk_signal_connect (GTK_OBJECT (data->shaded_check), "toggled", shade_toggled, data);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->shaded_check), data->shaded);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), data->shaded_check);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start_defaults (GTK_BOX (vb2), frame);

	/* size */
	frame = gtk_frame_new (_("Size"));
	vb1   = gtk_vbox_new (FALSE, 2);
	menu  = gtk_menu_new ();
	mcounter = 0;
	ADD_ITEM ("Pixels", percent_menu_activate);
	ADD_ITEM ("Percent %", percent_menu_activate);
	data->width_measure = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->width_measure), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->width_measure), data->percent);
	gtk_box_pack_start_defaults (GTK_BOX (vb1), data->width_measure);

	HBOX;
	ADD_VAL (GTK_HTML_EDIT_RULE_WIDTH, "length");
	gtk_adjustment_set_value (GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_RULE_WIDTH]), 100);
	gtk_widget_set_sensitive (data->width_measure, data->set [GTK_HTML_EDIT_RULE_WIDTH]);
	gtk_signal_connect (GTK_OBJECT (data->check [GTK_HTML_EDIT_RULE_WIDTH]), "toggled",
			    GTK_SIGNAL_FUNC (width_toggled), data);
	gtk_box_pack_start_defaults (GTK_BOX (vb1), hbox);
	gtk_container_add (GTK_CONTAINER (frame), vb1);
	gtk_box_pack_start_defaults (GTK_BOX (vb2), frame);
	gtk_box_pack_start (GTK_BOX (mhb), vb2, FALSE, FALSE, 0);
	vb2  = gtk_vbox_new (FALSE, 2);

	/* align */
	frame = gtk_frame_new (_("Align"));
	vb1   = gtk_vbox_new (FALSE, 2);
	menu  = gtk_menu_new ();
	mcounter = HTML_HALIGN_LEFT;
	gtk_container_border_width (GTK_CONTAINER (vb1), 3);
	ADD_ITEM ("Left", align_menu_activate);
	ADD_ITEM ("Center", align_menu_activate);
	ADD_ITEM ("Right", align_menu_activate);
	data->align_option = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (data->align_option), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->align_option), data->align);
	gtk_box_pack_start (GTK_BOX (vb1), data->align_option, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vb1);
	gtk_box_pack_start_defaults (GTK_BOX (vb2), frame);

	/* weight */
	frame = gtk_frame_new (_("Weight"));
	HBOX;
	ADD_VAL (GTK_HTML_EDIT_RULE_SIZE, "width");
	gtk_adjustment_set_value (GTK_ADJUSTMENT (data->adj [GTK_HTML_EDIT_RULE_SIZE]), 2);
	vb1 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vb1), hbox, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vb1);
	gtk_box_pack_start (GTK_BOX (vb2), frame, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (mhb), vb2, FALSE, FALSE, 0);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), mhb);

	/* sample */
	frame = gtk_frame_new (_("Sample"));
	data->sample = gtk_html_new ();
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_border_width (GTK_CONTAINER (sw), 3);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw), data->sample);
	gtk_container_add (GTK_CONTAINER (frame), sw);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), frame);
	fill_sample (data);

	data->disable_change = FALSE;
	gtk_html_edit_properties_dialog_change (data->cd->properties_dialog);

	return vbox;
}

void
rule_insert_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	GtkHTMLEditRuleProperties *d = (GtkHTMLEditRuleProperties *) get_data;

	html_engine_insert_rule (cd->html->engine,
				 VAL(WIDTH), d->percent ? VAL(WIDTH) : 0, VAL(SIZE),
				 d->shaded, d->align);
}

void
rule_apply_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	/* GtkHTMLEditRuleProperties *data = (GtkHTMLEditRuleProperties *) get_data;
	   HTMLEngine *e = cd->html->engine;
	   gchar *url;
	   gchar *target = "";

	   if (!data->url_changed)
	   return;

	   url = gtk_entry_get_text (GTK_ENTRY (data->entry));
	   if (*url)
	   html_engine_insert_rule (e, url, target);
	   else
	   html_engine_remove_rule (e); */
}

void
rule_close_cb (GtkHTMLControlData *cd, gpointer get_data)
{
	g_free (get_data);
}
