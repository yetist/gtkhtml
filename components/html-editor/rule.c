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
#include "htmlengine-edit-insert.h"

struct _GtkHTMLRuleDialog {
	GnomeDialog  *dialog;
	GtkHTML      *html;
	
	GtkWidget   *spin [3];
        GtkWidget   *combo;
	GtkWidget   *check;
	GtkWidget   *radio [4];
};


static void 
get_values (GtkWidget **spin, gint *val)
{
	gint i;
	
	for (i = 0; i < 3; i++)
		val [i] = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin [i]));

}
static void
button_rule_cb (GtkWidget *but, GtkHTMLRuleDialog *d)
{
	gint val [3];
	
	get_values (d->spin, val);
	
	html_engine_insert_rule (d->html->engine, val [0], val [1], val [2], TRUE, HTML_HALIGN_NONE);
	
}

static void
set_spin (HTMLEngine *e, GtkWidget *et)
{
}   

GtkHTMLRuleDialog *
gtk_html_rule_dialog_new (GtkHTML *html)
{
	GtkHTMLRuleDialog *d = g_new (GtkHTMLRuleDialog, 1);
	GtkWidget *vbox  [6];
	GtkWidget *label [3];
	GtkWidget *hbox;
	gint i;
	gchar *name [] = {_("Length"), _("Percent"), _("Size")}; 

	d->dialog    = GNOME_DIALOG (gnome_dialog_new (_("Rule"), GNOME_STOCK_BUTTON_OK,
						    GNOME_STOCK_BUTTON_CANCEL, NULL));
	d->html       = html;
	d->check      = gtk_check_button_new_with_label (_("Set Shade"));
	hbox          = gtk_hbox_new (FALSE, 3);

	for (i = 0; i < 3; i++) {
		label [i]   = gtk_label_new (name [i]);
		vbox [i]    = gtk_vbox_new (FALSE, 3);
		d->spin [i] = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (i, 0, 999, 1, 5, 10)), 1, 0);

		gtk_box_pack_start_defaults (GTK_BOX (vbox [i]), label [i]);
		gtk_box_pack_start_defaults (GTK_BOX (vbox [i]), d->spin [i]);
		gtk_box_pack_start_defaults (GTK_BOX (hbox), vbox [i]);
	}

	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (d->check), TRUE);
	vbox [3] = gtk_vbox_new (FALSE, 3);

	gtk_box_pack_start_defaults (GTK_BOX (vbox [3]), d->check);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), vbox [3]);
	gtk_box_pack_start_defaults (GTK_BOX (d->dialog->vbox), hbox);

	gtk_widget_show_all (hbox);
	
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
}

void
rule_edit (GtkHTMLControlData *cd)
{
	RUN_DIALOG (rule);
	
}







