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
#include <gnome.h>
#include <capplet-widget.h>
#include <gconf/gconf-client.h>
#include <gtkhtml-properties.h>
#include "gnome-bindings-prop.h"
#include "../src/gtkhtml.h"

static GtkWidget *capplet, *check, *menu, *option;
static gboolean active = FALSE;
static GConfError  *error  = NULL;
static GConfClient *client = NULL;

static GtkHTMLClassProperties *saved_prop;
static GtkHTMLClassProperties *orig_prop;
static GtkHTMLClassProperties *actual_prop;

static void
set_ui ()
{
	guint idx;

	active = FALSE;

	/* set to current state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), actual_prop->magic_links);

	if (!strcmp (actual_prop->keybindings_theme, "emacs")) {
		idx = 0;
	} else if (!strcmp (actual_prop->keybindings_theme, "ms")) {
		idx = 1;
	} else
		idx = 2;
	gtk_option_menu_set_history (GTK_OPTION_MENU (option), idx);

	active = TRUE;
}

static void
apply (void)
{
	actual_prop->magic_links = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	g_free (actual_prop->keybindings_theme);
	actual_prop->keybindings_theme = g_strdup (gtk_object_get_data (GTK_OBJECT (gtk_menu_get_active (GTK_MENU (menu))),
									"theme"));
	gtk_html_class_properties_update (actual_prop, client, saved_prop);
	gtk_html_class_properties_copy   (saved_prop, actual_prop);
}

static void
revert (void)
{
	gtk_html_class_properties_update (orig_prop, client, saved_prop);
	gtk_html_class_properties_copy   (saved_prop, orig_prop);
	gtk_html_class_properties_copy   (actual_prop, orig_prop);

	set_ui ();
}

static void
changed (GtkWidget *widget, gpointer data)
{
	if (active)
		capplet_widget_state_changed (CAPPLET_WIDGET (capplet), TRUE);
}

static void
setup(void)
{
	GtkWidget *vbox, *hbox, *frame, *mi, *bi;
	guchar *base, *rcfile;

        capplet = capplet_widget_new();

	vbox  = gtk_vbox_new (FALSE, 3);
	hbox  = gtk_hbox_new (FALSE, 3);
	frame = gtk_frame_new (_("Behaviour"));
	check = gtk_check_button_new_with_label (_("magic links"));

	gtk_signal_connect (GTK_OBJECT (check), "toggled", changed, NULL);

	gtk_container_add (GTK_CONTAINER (frame), check);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);

	frame  = gtk_frame_new (_("Keybindings"));
	option = gtk_option_menu_new ();
	menu   = gtk_menu_new ();

#define ADD(l,v) \
	mi = gtk_menu_item_new_with_label (_(l)); \
	gtk_menu_append (GTK_MENU (menu), mi); \
        gtk_signal_connect (GTK_OBJECT (mi), "activate", changed, NULL); \
        gtk_object_set_data (GTK_OBJECT (mi), "theme", v);

	ADD ("Emacs like", "emacs");
	ADD ("MS like", "ms");
	ADD ("Custom", "custom");
	/* to be implemented */
	gtk_widget_set_sensitive (mi, FALSE);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
	gtk_container_add (GTK_CONTAINER (frame), option);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), frame);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

#define LOAD(x) \
	base = g_strconcat ("gtkhtml/keybindingsrc.", x, NULL); \
	rcfile = gnome_unconditional_datadir_file (base); \
        gtk_rc_parse (rcfile); \
        g_free (base); \
	g_free (rcfile)

	rcfile = g_strconcat (gnome_util_user_home (), "/.gnome/gtkhtml-bindings-custom", NULL);
	gtk_rc_parse (rcfile);
	g_free (rcfile);
	LOAD ("emacs");
	LOAD ("ms");

	bi = gnome_bindings_properties_new ();
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      "Emacs like", "gtkhtml-bindings-emacs", "command",
					      GTK_TYPE_HTML_COMMAND, FALSE);
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      "MS like", "gtkhtml-bindings-ms", "command",
					      GTK_TYPE_HTML_COMMAND, FALSE);
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      "Custom", "gtkhtml-bindings-custom", "command",
					      GTK_TYPE_HTML_COMMAND, TRUE);
	gnome_bindings_properties_select_keymap (GNOME_BINDINGS_PROPERTIES (bi), "Custom");
	gtk_box_pack_start_defaults (GTK_BOX (vbox), bi);

        gtk_container_add (GTK_CONTAINER (capplet), vbox);
        gtk_widget_show_all (capplet);

	set_ui ();
}

int
main (int argc, char **argv)
{
        bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        textdomain (PACKAGE);

        if (gnome_capplet_init ("gtkhtml-properties", VERSION, argc, argv, NULL, 0, NULL) < 0)
		return 1;

	if (!gconf_init(argc, argv, &error)) {
		g_assert(error != NULL);
		g_warning("GConf init failed:\n  %s", error->str);
		return 1;
	}

	client = gconf_client_new ();
	gconf_client_add_dir(client, GTK_HTML_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);

	orig_prop = gtk_html_class_properties_new ();
	saved_prop = gtk_html_class_properties_new ();
	actual_prop = gtk_html_class_properties_new ();
	gtk_html_class_properties_load (actual_prop, client);
	gtk_html_class_properties_copy (saved_prop, actual_prop);
	gtk_html_class_properties_copy (orig_prop, actual_prop);

        setup ();

	/* connect signals */
        gtk_signal_connect (GTK_OBJECT (capplet), "try",
                            GTK_SIGNAL_FUNC (apply), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "revert",
                            GTK_SIGNAL_FUNC (revert), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "ok",
                            GTK_SIGNAL_FUNC (apply), NULL);
        gtk_signal_connect (GTK_OBJECT (capplet), "cancel",
                            GTK_SIGNAL_FUNC (revert), NULL);

        capplet_gtk_main ();

        return 0;
}
