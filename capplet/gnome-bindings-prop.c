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
#include <string.h>
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkscrolledwindow.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-bindings-prop.h"

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint gnome_bindings_properties_signals [LAST_SIGNAL] = { 0 };

struct _KeymapEntry {
	gchar *name;
	gchar *bindings_name;
	gchar *signal_name;
	GtkType enum_type;
	gboolean editable;
	GList *bindings;
};
typedef struct _KeymapEntry KeymapEntry;

static void remove_snooper (GnomeBindingsProperties *prop);

GnomeBindingEntry *
gnome_binding_entry_new (guint keyval, guint modifiers, gchar *command)
{
	GnomeBindingEntry *be = g_new (GnomeBindingEntry, 1);

	be->command = g_strdup (command);
	be->keyval = keyval;
	be->modifiers = modifiers;

	return be;
}

void
gnome_binding_entry_destroy (GnomeBindingEntry *be)
{
	g_free (be->command);
	g_free (be);
}

GList *
gnome_binding_entry_list_copy (GList *list)
{
	GList *new_list = NULL;
	GnomeBindingEntry *be;

	list = g_list_last (list);
	while (list) {
		be = (GnomeBindingEntry *) list->data;
		new_list = g_list_prepend (new_list, gnome_binding_entry_new (be->keyval, be->modifiers, be->command));
		list = list->prev;
	}

	return new_list;
}

void
gnome_binding_entry_list_destroy (GList *list)
{
	GList *cur = list;

	while (cur) {
		gnome_binding_entry_destroy ((GnomeBindingEntry *) cur->data);
		cur = cur->next;
	}

	g_list_free (list);
}

static KeymapEntry *
keymap_entry_new (gchar *n, gchar *bn, gchar *sn, GtkType t, gboolean editable)
{
	KeymapEntry *ke = g_new (KeymapEntry, 1);
	GtkBindingSet *bset;
	GtkBindingEntry *bentry;

	ke->name          = g_strdup (n);
	ke->bindings_name = g_strdup (bn);
	ke->signal_name   = g_strdup (sn);
	ke->enum_type     = t;
	ke->editable      = editable;
	ke->bindings      = NULL;

	bset = gtk_binding_set_find (bn);
	if (bset) {
		for (bentry = bset->entries ;bentry; bentry = bentry->set_next) {
			if (!strcmp (bentry->signals->signal_name, sn)
			    && bentry->signals->args->arg_type == GTK_TYPE_IDENTIFIER) {

				ke->bindings = g_list_prepend
					(ke->bindings,
					 gnome_binding_entry_new (bentry->keyval,
								  bentry->modifiers,
								  bentry->signals->args->d.string_data));
			}
		}
	}

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
	return (keyval) ? g_strconcat ((mods & GDK_CONTROL_MASK) ? "C-" : "",
				       (mods & GDK_MOD1_MASK) ? "M-" : "",
				       gdk_keyval_name (keyval), NULL)
		: g_strdup (_("<None>"));
}

static gboolean
key_from_string (gchar *str, guint *keyval, guint *modifiers)
{
	size_t len = strlen (str);
	*modifiers = 0;
	if (len > 1 && str [1] == '-') {
		if (str [0] == 'c' || str [0] == 'C') {
			*modifiers |= GDK_CONTROL_MASK;
		} else if (str [0] == 'm' || str [0] == 'M') {
			*modifiers |= GDK_MOD1_MASK;
		} else
			return FALSE;
		str += 2;
		if (len > 3 && str [1] == '-') {
			if (str [0] == 'c' || str [0] == 'C') {
				*modifiers |= GDK_CONTROL_MASK;
			} else if (str [0] == 'm' || str [0] == 'M') {
				*modifiers |= GDK_MOD1_MASK;
			} else
				return FALSE;
			str += 2;
		}
	}

	*keyval = gdk_keyval_from_name (str);

	return (*keyval != 0);
}

static void
binding_select_row (GtkCList       *clist,
		    gint            row,
		    gint            column,
		    GdkEvent       *event,
		    GnomeBindingsProperties *prop)
{
	GnomeBindingEntry *be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
	KeymapEntry *ke;
	gint command_row;
	gchar *key_text;

	if (!be)
		return;

	prop->commands_active = FALSE;

	ke = (KeymapEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->keymaps_clist),
						     GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data));

	command_row = gtk_clist_find_row_from_data (GTK_CLIST (prop->commands_clist),
						    (strcmp (be->command, _("<None>")))
						    ? GUINT_TO_POINTER (gtk_type_enum_find_value (ke->enum_type,
												  be->command)->value)
						    : GUINT_TO_POINTER (0));

	gtk_clist_select_row (GTK_CLIST (prop->commands_clist), command_row, 0);
	if (gtk_clist_row_is_visible (GTK_CLIST (prop->commands_clist), command_row) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto (GTK_CLIST (prop->commands_clist), command_row, -1, 0.0, 0.0);

	key_text = string_from_key (be->keyval, be->modifiers);
	gtk_entry_set_text (GTK_ENTRY (prop->key_entry), key_text);
	g_free (key_text);

	prop->commands_active = TRUE;
	remove_snooper (prop);
}

static void
command_select_row (GtkCList       *clist,
		    gint            row,
		    gint            column,
		    GdkEvent       *event,
		    GnomeBindingsProperties *prop)
{
	GnomeBindingEntry *be;
	gint binding_row;
	gint command_val;
	gchar *text;

	if (!prop->commands_active)
		return;

	command_val = GPOINTER_TO_INT (gtk_clist_get_row_data (clist, row));
	binding_row = GPOINTER_TO_INT (GTK_CLIST (prop->bindings_clist)->selection->data);
	be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->bindings_clist), binding_row);

	
	gtk_clist_get_text (clist, row, 0, &text);
	if (strcmp (text, be->command)) {
		g_free (be->command);
		be->command = g_strdup (text);
		gtk_clist_set_text (GTK_CLIST (prop->bindings_clist), binding_row, 1, be->command);
		gtk_signal_emit (GTK_OBJECT (prop), gnome_bindings_properties_signals [CHANGED]);
	}
	remove_snooper (prop);
}

static void
keymap_select_row (GtkCList       *clist,
		   gint            row,
		   gint            column,
		   GdkEvent       *event,
		   GnomeBindingsProperties *prop)
{
	KeymapEntry *ke = gtk_clist_get_row_data (clist, row);
	GnomeBindingEntry *be;
	GtkEnumValue *vals;
	GList *cur;
	gint num;
	gchar *val_name [1];

	if (!ke) return;

	prop->commands_active = FALSE;

	clist = GTK_CLIST (prop->commands_clist);
	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);
	gtk_widget_set_sensitive (GTK_WIDGET (clist), ke->editable);
	gtk_widget_set_sensitive (prop->add_button, ke->editable);
	gtk_widget_set_sensitive (prop->copy_button, ke->editable);
	gtk_widget_set_sensitive (prop->delete_button, ke->editable);
	gtk_widget_set_sensitive (prop->key_entry, ke->editable);
	gtk_widget_set_sensitive (prop->grab_button, ke->editable);
	vals  = gtk_type_enum_get_values (ke->enum_type);
	val_name [0] = _("<None>");
	gtk_clist_set_row_data (clist, gtk_clist_append (clist, val_name), GUINT_TO_POINTER (0));
	if (vals) {
		for (num=0; vals [num].value_name; num++) {
			val_name [0] = vals [num].value_nick;
			gtk_clist_set_row_data (clist, gtk_clist_append (clist, val_name),
						GUINT_TO_POINTER (vals [num].value));
		}
	}
	gtk_clist_columns_autosize (clist);
	gtk_clist_thaw (clist);

	clist = GTK_CLIST (prop->bindings_clist);
	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);
	cur = ke->bindings;
	while (cur) {
		gchar *name [2];

		be = (GnomeBindingEntry *) cur->data;
		name [0] = string_from_key (be->keyval, be->modifiers);
		name [1] = be->command;
		gtk_clist_set_row_data (clist, gtk_clist_append (clist, name), be);
		g_free (name [0]);
		
		cur = cur->next;
	}
	gtk_clist_columns_autosize (clist);
	gtk_clist_thaw (clist);
	gtk_clist_select_row (clist, 0, 0);

	if (!ke->bindings) {
		gtk_widget_set_sensitive (prop->copy_button, FALSE);
		gtk_widget_set_sensitive (prop->delete_button, FALSE);
		gtk_widget_set_sensitive (prop->grab_button, FALSE);
		gtk_widget_set_sensitive (prop->key_entry, FALSE);
		gtk_widget_set_sensitive (prop->commands_clist, FALSE);
	}

	prop->commands_active = TRUE;
	remove_snooper (prop);
}

static void
key_entry_changed (GtkEntry *entry, GnomeBindingsProperties *prop)
{
	if (prop->commands_active) {
		guint keyval;
		guint modifiers;

		if (key_from_string (gtk_entry_get_text (entry), &keyval, &modifiers)) {
			GnomeBindingEntry *be;
			gint binding_row;
			gchar *key_text;

			binding_row = GPOINTER_TO_INT (GTK_CLIST (prop->bindings_clist)->selection->data);
			be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->bindings_clist), binding_row);

			be->keyval = keyval;
			be->modifiers = modifiers;

			key_text = string_from_key (keyval, modifiers);

			gtk_clist_set_text (GTK_CLIST (prop->bindings_clist), binding_row, 0, key_text);
			gtk_signal_emit (GTK_OBJECT (prop), gnome_bindings_properties_signals [CHANGED]);
		}
	}
}

static void
remove_snooper (GnomeBindingsProperties *prop)
{
	KeymapEntry *ke = (KeymapEntry *) gtk_clist_get_row_data
		(GTK_CLIST (prop->keymaps_clist),
		 GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data));
	gtk_key_snooper_remove (prop->key_snooper);
	if (ke->editable && ke->bindings)
		gtk_widget_set_sensitive (prop->grab_button, TRUE);
	prop->key_snooper = 0;
}

static gint
key_snooper (GtkWidget *grab_widget,
	     GdkEventKey *event,
	     GnomeBindingsProperties *prop)
{
	if (event->keyval
	    && event->keyval != GDK_Control_L
	    && event->keyval != GDK_Control_R
	    && event->keyval != GDK_Shift_L
	    && event->keyval != GDK_Shift_R
	    && event->keyval != GDK_Meta_L
	    && event->keyval != GDK_Meta_R
	    && event->keyval != GDK_Alt_L
	    && event->keyval != GDK_Alt_R) {

		gchar *text;

		remove_snooper (prop);
		text = string_from_key (event->keyval, event->state);
		gtk_entry_set_text (GTK_ENTRY (prop->key_entry), text);
		g_free (text);
	}
	return FALSE;
}

static void
grab_button_clicked (GtkButton *button, GnomeBindingsProperties *prop)
{
	if (!prop->key_snooper) {
		gtk_widget_set_sensitive (prop->grab_button, FALSE);
		prop->key_snooper = gtk_key_snooper_install ((GtkKeySnoopFunc) key_snooper, prop);
	}
}

static void
add_button_clicked (GtkButton *button, GnomeBindingsProperties *prop)
{
	GnomeBindingEntry *be = NULL, *new_be;
	KeymapEntry *ke;
	GList *list;
	gchar *name [2];
	gint binding_row;

	prop->commands_active = FALSE;

	ke = (KeymapEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->keymaps_clist),
						     GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data));
	if (ke->bindings) {
		binding_row = GPOINTER_TO_INT (GTK_CLIST (prop->bindings_clist)->selection->data);
		be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->bindings_clist), binding_row);
	} else
		binding_row = 0;

	name [0] = _("<None>");
	name [1] = _("<None>");
	binding_row = gtk_clist_insert (GTK_CLIST (prop->bindings_clist), binding_row, name);

	new_be = gnome_binding_entry_new (0, 0, "<None>");
	gtk_clist_set_row_data (GTK_CLIST (prop->bindings_clist), binding_row, new_be);

	list = (be) ? g_list_find (ke->bindings, be) : NULL;
	list = g_list_prepend (list, new_be);
	if (!list->prev)
		ke->bindings = list;

	prop->commands_active = TRUE;
	gtk_clist_select_row (GTK_CLIST (prop->bindings_clist), binding_row, 0);
	gtk_signal_emit (GTK_OBJECT (prop), gnome_bindings_properties_signals [CHANGED]);

	gtk_widget_set_sensitive (prop->copy_button, TRUE);
	gtk_widget_set_sensitive (prop->delete_button, TRUE);
	gtk_widget_set_sensitive (prop->grab_button, TRUE);
	gtk_widget_set_sensitive (prop->key_entry, TRUE);
	gtk_widget_set_sensitive (prop->commands_clist, TRUE);
}

static void
copy_button_clicked (GtkButton *button, GnomeBindingsProperties *prop)
{
	GnomeBindingEntry *be, *new_be;
	KeymapEntry *ke;
	GList *list;
	gchar *name [2];
	gint binding_row;

	prop->commands_active = FALSE;

	binding_row = GPOINTER_TO_INT (GTK_CLIST (prop->bindings_clist)->selection->data);
	be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->bindings_clist), binding_row);
	ke = (KeymapEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->keymaps_clist),
						     GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data));

	name [0] = string_from_key (be->keyval, be->modifiers);
	name [1] = be->command;
	binding_row = gtk_clist_insert (GTK_CLIST (prop->bindings_clist), binding_row, name);
	g_free (name [0]);

	new_be = gnome_binding_entry_new (be->keyval, be->modifiers, be->command);
	gtk_clist_set_row_data (GTK_CLIST (prop->bindings_clist), binding_row, new_be);

	list = g_list_find (ke->bindings, be);
	list = g_list_prepend (list, new_be);
	if (!list->prev)
	ke->bindings = list;

	prop->commands_active = TRUE;
	gtk_clist_select_row (GTK_CLIST (prop->bindings_clist), binding_row, 0);
	gtk_signal_emit (GTK_OBJECT (prop), gnome_bindings_properties_signals [CHANGED]);
}

static void
delete_button_clicked (GtkButton *button, GnomeBindingsProperties *prop)
{
	GnomeBindingEntry *be;
	KeymapEntry *ke;
	gint binding_row;

	ke = (KeymapEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->keymaps_clist),
						     GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data));
	if (!ke->bindings)
		return;

	binding_row = GPOINTER_TO_INT (GTK_CLIST (prop->bindings_clist)->selection->data);
	be = (GnomeBindingEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->bindings_clist), binding_row);

	gtk_clist_remove (GTK_CLIST (prop->bindings_clist), binding_row);
	ke->bindings = g_list_remove (ke->bindings, be);
	gnome_binding_entry_destroy (be);
	gtk_signal_emit (GTK_OBJECT (prop), gnome_bindings_properties_signals [CHANGED]);

	if (!ke->bindings) {
		gtk_widget_set_sensitive (prop->copy_button, FALSE);
		gtk_widget_set_sensitive (prop->delete_button, FALSE);
		gtk_widget_set_sensitive (prop->grab_button, FALSE);
		gtk_widget_set_sensitive (prop->key_entry, FALSE);
		gtk_widget_set_sensitive (prop->commands_clist, FALSE);
	}
}

static void
init (GnomeBindingsProperties *prop)
{
	GtkWidget *clist, *sw, *widget = GTK_WIDGET (prop), *hbox, *button, *vbox, *hbox1;
	gchar *cols1 [2] = {_("Key"), _("Command")};
	gchar *cols2 [1] = {_("Commands")};
	gchar *cols3 [1] = {_("Keymaps")};

	prop->bindingsets = g_hash_table_new (g_str_hash, g_str_equal);
	prop->commands_active = FALSE;
	prop->key_snooper = 0;

	/* bindings */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->bindings_clist = clist = gtk_clist_new_with_titles (2, cols1);
	gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_BROWSE);
	gtk_signal_connect (GTK_OBJECT (clist), "select_row", binding_select_row, prop);
	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start (GTK_BOX (widget), sw, TRUE, TRUE, 2);

	/* buttons */
	hbox = gtk_hbox_new (FALSE, 3);
	prop->add_button = button = gtk_button_new_with_label (_("Add"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked", add_button_clicked, prop);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), button);
	prop->copy_button = button = gtk_button_new_with_label (_("Copy"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked", copy_button_clicked, prop);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), button);
	prop->delete_button = button = gtk_button_new_with_label (_("Delete"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked", delete_button_clicked, prop);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), button);
	gtk_box_pack_start (GTK_BOX (widget), hbox, FALSE, FALSE, 2);

	hbox = gtk_hbox_new (FALSE, 3);

	/* keymaps */
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->keymaps_clist = clist = gtk_clist_new_with_titles (1, cols3);
	gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_BROWSE);
	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_signal_connect (GTK_OBJECT (clist), "select_row", keymap_select_row, prop);
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), sw);

	/* command names */
	vbox = gtk_vbox_new (FALSE, 2);
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	prop->commands_clist = clist = gtk_clist_new_with_titles (1, cols2);
	gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_BROWSE);
	gtk_clist_columns_autosize (GTK_CLIST (clist));
	gtk_signal_connect (GTK_OBJECT (clist), "select_row", command_select_row, prop);
	gtk_container_add (GTK_CONTAINER (sw), clist);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), sw);

	hbox1 = gtk_hbox_new (FALSE, 3);
	prop->key_entry = gtk_entry_new ();
	gtk_signal_connect (GTK_OBJECT (prop->key_entry), "changed", key_entry_changed, prop);
	prop->grab_button = gtk_button_new_with_label (_("Grab key"));
	gtk_signal_connect (GTK_OBJECT (prop->grab_button), "clicked", grab_button_clicked, prop);
	gtk_box_pack_start_defaults (GTK_BOX (hbox1), prop->key_entry);
	gtk_box_pack_start_defaults (GTK_BOX (hbox1), prop->grab_button);
	gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);

	gtk_box_pack_start_defaults (GTK_BOX (hbox), vbox);
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
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

	object_class->destroy = destroy;
	klass->changed        = NULL;

	gnome_bindings_properties_signals [CHANGED] =
		gtk_signal_new ("changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeBindingsPropertiesClass, changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, gnome_bindings_properties_signals, LAST_SIGNAL);
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
				      GtkType arg_enum_type,
				      gboolean editable)
{
	KeymapEntry *ke = keymap_entry_new (name, bindings, signal_name, arg_enum_type, editable);
	gchar *n [1];
	gint row;

	n [0] = name;
	row = gtk_clist_append (GTK_CLIST (prop->keymaps_clist), n);
	gtk_clist_set_row_data (GTK_CLIST (prop->keymaps_clist), row, ke);

	g_hash_table_insert (prop->bindingsets, name, ke);
}

GList *
gnome_bindings_properties_get_keymap (GnomeBindingsProperties *prop, gchar *name)
{
	return ((KeymapEntry *) g_hash_table_lookup (prop->bindingsets, name))->bindings;
}

void
gnome_bindings_properties_set_keymap (GnomeBindingsProperties *prop, gchar *name, GList *list)
{
	KeymapEntry *ke, *cur_ke;
	gboolean need_update = FALSE;
	gint row;

	row = GPOINTER_TO_INT (GTK_CLIST (prop->keymaps_clist)->selection->data);
	cur_ke = (KeymapEntry *) gtk_clist_get_row_data (GTK_CLIST (prop->keymaps_clist), row);
	ke = (KeymapEntry *) g_hash_table_lookup (prop->bindingsets, name);
	if (ke->bindings == cur_ke->bindings)
		need_update = TRUE;
	gnome_binding_entry_list_destroy (ke->bindings);
	ke->bindings = gnome_binding_entry_list_copy (list);

	if (need_update)
		gtk_clist_select_row (GTK_CLIST (prop->keymaps_clist), row, 0);
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

static gchar *
bind_from_key (guint keyval, guint modifiers)
{
	return g_strconcat ((modifiers & GDK_CONTROL_MASK) ? "<Ctrl>" : "",
			    (modifiers & GDK_MOD1_MASK) ? "<Alt>" : "",
			    gdk_keyval_name (keyval), NULL);
}

void
gnome_bindings_properties_save_keymap (GnomeBindingsProperties *prop,
				       gchar *name,
				       gchar *filename)
{
	FILE *file = fopen (filename, "w");
	KeymapEntry *ke = (KeymapEntry *) g_hash_table_lookup (prop->bindingsets, name);
	GnomeBindingEntry *be;
	GList *cur = ke->bindings;
	gchar *str;

	fprintf (file, "#\n# this file is autogenerated by GNOME bindings properties widget\n#\n\n"
		 "binding \"%s\"\n{\n", ke->bindings_name);

	while (cur) {
		be = (GnomeBindingEntry *) cur->data;
		str = bind_from_key (be->keyval, be->modifiers);
		fprintf (file, "  bind \"%s\"    { \"%s\" (%s) }\n", str, ke->signal_name, be->command);
		g_free (str);
		cur = cur->next;
	}

	fprintf (file, "}\n");
	fclose (file);
}
