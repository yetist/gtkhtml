/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik (rodo@helixcode.com)
                       Ariel Rios   (ariel@arcavia.com)

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
#include "config.h"
#include "dialog.h"
#include "rule.h"
#include "htmlrule.h"
#include "htmlengine-edit-insert.h"

struct _GtkHTMLRuleDialog {
	GnomeDialog  *dialog;
	GtkHTML      *html;
	
	GtkWidget   *spin [3];
	GtkWidget   *check;
	GtkWidget   *combo;
	
	HTMLRule      *rule;
	gboolean       shade;
	HTMLHAlignType halign;
};


static void 
set_get_values (GtkWidget **spin, gint *val, gboolean flag)
{
	gint i;
	
	if (flag)
		for (i = 0; i < 3; i++)
			val [i] = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin [i]));
	else
		for (i = 0; i < 3; i++)
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin [i]), (gfloat) val [i]);
}

static void
button_rule_cb (GtkWidget *but, GtkHTMLRuleDialog *d)
{
	gint val [3];
	
	set_get_values (d->spin, val, TRUE);

	if (!d->rule) {
		html_engine_insert_rule (d->html->engine, val [0], val [1], val [2], TRUE, HTML_HALIGN_NONE);
		return;
	}
	
	d->rule->length  = val [0]; 
	d->rule->size    = val [2];
	d->rule->shade  = d->shade;
	d->rule->halign  = d->halign;
	
	html_engine_schedule_update (d->html->engine);
}

static void
button_shade_cb (GtkWidget *but, GtkHTMLRuleDialog *d)
{
	d->shade = !d->shade;
}

static void
combo_align_cb (GtkWidget *but, GtkHTMLRuleDialog *d)
{
}

GtkHTMLRuleDialog *
gtk_html_rule_dialog_new (GtkHTML *html)
{
	GtkHTMLRuleDialog *d = g_new (GtkHTMLRuleDialog, 1);
	GtkWidget *vbox  [4];
	GtkWidget *label [3];
	GtkWidget *hbox;
	gint i;
	gchar *name [] = {_("Length"), _("Percent"), _("Size")}; 

	d->dialog    = GNOME_DIALOG (gnome_dialog_new (_("Rule"), GNOME_STOCK_BUTTON_OK,
						    GNOME_STOCK_BUTTON_CANCEL, NULL));
	d->html       = html;
	d->check      = gtk_check_button_new_with_label (_("Set Shade"));
	d->combo      = gtk_combo_new ();
	d->shade     = TRUE;
	d->halign     = HTML_HALIGN_NONE;
	hbox          = gtk_hbox_new (FALSE, 3);

	for (i = 0; i < 3; i++) {

		label [i]   = gtk_label_new (name [i]);
		vbox [i]    = gtk_vbox_new (FALSE, 3);
		d->spin [i] = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 999, 1, 5, 10)), 1, 0);

		gtk_box_pack_start_defaults (GTK_BOX (vbox [i]), label [i]);
		gtk_box_pack_start_defaults (GTK_BOX (vbox [i]), d->spin [i]);
		gtk_box_pack_start_defaults (GTK_BOX (hbox), vbox [i]);
	}

	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (d->check), TRUE);
	
	gtk_box_pack_start_defaults (GTK_BOX (d->dialog->vbox), hbox);
	gtk_box_pack_start_defaults (GTK_BOX (d->dialog->vbox), d->check);
	gtk_box_pack_start_defaults (GTK_BOX (d->dialog->vbox), d->combo);

	gtk_widget_show_all (d->dialog->vbox);
	
	gnome_dialog_button_connect (d->dialog, 0, button_rule_cb, d);
	gnome_dialog_close_hides (d->dialog, TRUE);
	gnome_dialog_set_close (d->dialog, TRUE);
	gnome_dialog_set_default (d->dialog, 0);

	return d;
}
 
void
gtk_html_rule_dialog_destroy (GtkHTMLRuleDialog *d)
{
}

void
rule_insert (GtkHTMLControlData *cd)
{
	RUN_DIALOG (rule);
	cd->rule_dialog->rule = NULL;
}

void
rule_edit (GtkHTMLControlData *cd, HTMLRule *r)
{
	GtkHTMLRuleDialog *d = cd->rule_dialog;
	gint val [3];
	RUN_DIALOG (rule);
	
	d->rule = r;
	d->shade = r->shade;
	d->halign = r->halign;

	val [0] = r->length;
	val [1] = 0;
	val [2] = r->size;
		
	set_get_values (d->spin, val, FALSE);
       
}







