/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-bindings-prop.h"

struct _KeymapEntry {
	gchar *name;
	gchar *bindings_name;
	gchar *signal_name;
	GtkType enum_type;

	
};
typedef struct _KeymapEntry KeymapEntry;

static KeymapEntry *
keymap_entry_new (gchar *n, gchar *bn, gchar *sn, GtkType t)
{
	KeymapEntry *ke = g_new (KeymapEntry, 1);

	ke->name          = g_strdup (n);
	ke->bindings_name = g_strdup (bn);
	ke->signal_name   = g_strdup (sn);
	ke->enum_type     = t;

	return ke;
}

static void
keymap_entry_destroy (KeymapEntry *ke)
{
	g_free (ke->name);
	g_free (ke->bindings_name);
	g_free (ke->signal_name);

	g_free (ke);
}

static gchar *
string_from_key (guint keyval, guint mods)
{
	return g_strconcat ((mods & GDK_CONTROL_MASK) ? "C-" : "",
			    (mods & GDK_MOD1_MASK) ? "M-" : "",
			    gdk_keyval_name (keyval), NULL);
}

static void
select_row (GtkCList       *clist,
	    gint            row,
	    gint            column,
	    GdkEvent       *event,
	    GnomeBindingsProperties *prop)
{
	KeymapEntry *ke = gtk_clist_get_row_data (clist, row);
	GtkBindingSet *bset;
	GtkBindingEntry *bentry;
	GtkEnumValue *vals;
	gint num;

	g_assert (ke);

	clist = GTK_CLIST (prop->commands_clist);
	gtk_clist_clear (clist);
	vals  = gtk_type_enum_get_values (ke->enum_type);
	if (vals) {
		gchar *name [1];

		for (num=0; vals [num].value_name; num++) {
			name [0] = vals [num].value_nick;
			gtk_clist_append (clist, name);
		}
		gtk_clist_columns_autosize (GTK_CLIST (clist));
	}

	clist = GTK_CLIST (prop->bindings_clist);
	gtk_clist_clear (clist);
	bset = gtk_binding_set_find (ke->bindings_name);
	if (bset) {
		for (bentry = bset->entries ;bentry; bentry = bentry->set_next) {
			gchar *name [2];
			if (!strcmp (bentry->signals->signal_name, ke->signal_name)
			    && bentry->signals->args->arg_type == GTK_TYPE_IDENTIFIER) {
				name [0] = string_from_key (bentry->keyval, bentry->modifiers);
				name [1] = bentry->signals->args->d.string_data;
				gtk_clist_prepend (GTK_CLIST (clist), name);
				g_free (name [0]);
			}
		}
	}
	gtk_clist_columns_autosize (GTK_CLIST (clist));
}

static void
init (GnomeBindingsProperties *prop)
{
	GtkWidget *clist, *sw, *widget = GTK_WIDGET (prop), *hbox;
	gchar *cols1 [2] = {N_("Key"), N_("Command")};
	gchar *cols2 [1] = {_("Commands")};
	gchar *cols3 [1] = {_("Keymaps")};

	/* bindings */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->bindings_clist = clist = gtk_clist_new_with_titles (2, cols1);

	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start (GTK_BOX (widget), sw, TRUE, TRUE, 2);

	hbox = gtk_hbox_new (FALSE, 3);

	/* keymaps */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->keymaps_clist = clist = gtk_clist_new_with_titles (1, cols3);
	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_signal_connect (GTK_OBJECT (clist), "select_row", select_row, prop);
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), sw);

	/* command names */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->commands_clist = clist = gtk_clist_new_with_titles (1, cols2);
	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), sw);
	gtk_box_pack_start (GTK_BOX (widget), hbox, TRUE, TRUE, 2);

}

static void
destroy (GtkObject *prop)
{
	GList *cur = GTK_CLIST (GNOME_BINDINGS_PROPERTIES (prop)->keymaps_clist)->row_list;

	while (cur) {
		keymap_entry_destroy (GTK_CLIST_ROW (cur)->data);
		cur = cur->next;
	}
}

static void
class_init (GnomeBindingsPropertiesClass *klass)
{
	GTK_OBJECT_CLASS (klass)->destroy = destroy;
}

GtkType
gnome_bindings_properties_get_type (void)
{
	static guint bindings_properties_type = 0;

	if (!bindings_properties_type) {
		static const GtkTypeInfo bindings_properties_info = {
			"GnomeBindingsProperties",
			sizeof (GnomeBindingsProperties),
			sizeof (GnomeBindingsPropertiesClass),
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		bindings_properties_type = gtk_type_unique (GTK_TYPE_VBOX, &bindings_properties_info);
	}

	return bindings_properties_type;
}

GtkWidget *
gnome_bindings_properties_new ()
{
	return gtk_type_new (gnome_bindings_properties_get_type ());
}

void
gnome_bindings_properties_add_keymap (GnomeBindingsProperties *prop,
				      gchar *name,
				      gchar *bindings,
				      gchar *signal_name,
				      GtkType arg_enum_type)
{
	KeymapEntry *ke = keymap_entry_new (name, bindings, signal_name, arg_enum_type);
	gchar *n [1];

	n [0] = name;
	gtk_clist_set_row_data (GTK_CLIST (prop->keymaps_clist),
				gtk_clist_append (GTK_CLIST (prop->keymaps_clist), n),
				ke);
}

void
gnome_bindings_properties_select_keymap (GnomeBindingsProperties *prop,
					 gchar *name)
{
	GtkCList *clist = GTK_CLIST (GNOME_BINDINGS_PROPERTIES (prop)->keymaps_clist);
	GList *cur = clist->row_list;
	KeymapEntry *ke;

	while (cur) {
		ke = (KeymapEntry *) GTK_CLIST_ROW (cur)->data;

		if (!strcmp (ke->name, name))
			gtk_clist_select_row (clist, gtk_clist_find_row_from_data (clist, ke), 0);

		cur = cur->next;
	}	
}
