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
#include <gtkhtml-properties.h>
#include "gnome-bindings-prop.h"
#include "../src/gtkhtml.h"

#define CUSTOM_KEYMAP_NAME "Custom"
#define EMACS_KEYMAP_NAME "Emacs like"
#define MS_KEYMAP_NAME "MS like"

static GtkWidget *capplet, *check, *bi, *live_spell_check, *language, *live_spell_color;
static gboolean active = FALSE;
#ifdef GTKHTML_HAVE_GCONF
static GError      *error  = NULL;
static GConfClient *client = NULL;
#endif
static GtkHTMLClassProperties *saved_prop;
static GtkHTMLClassProperties *orig_prop;
static GtkHTMLClassProperties *actual_prop;

static GList *saved_bindings;
static GList *orig_bindings;

static gchar *home_rcfile;

static void
set_ui ()
{
	gchar *keymap_name;

	active = FALSE;

	/* set to current state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), actual_prop->magic_links);

	if (!strcmp (actual_prop->keybindings_theme, "emacs")) {
		keymap_name = EMACS_KEYMAP_NAME;
	} else if (!strcmp (actual_prop->keybindings_theme, "ms")) {
		keymap_name = MS_KEYMAP_NAME;
	} else
		keymap_name = CUSTOM_KEYMAP_NAME;
	gnome_bindings_properties_select_keymap (GNOME_BINDINGS_PROPERTIES (bi), keymap_name);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (live_spell_check), actual_prop->live_spell_check);
	gtk_widget_set_sensitive (live_spell_color, actual_prop->live_spell_check);
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (live_spell_color),
							actual_prop->spell_error_color.red,
							actual_prop->spell_error_color.green,
							actual_prop->spell_error_color.blue, 0);
	gtk_entry_set_text (GTK_ENTRY (language), actual_prop->language);

	active = TRUE;
}

static void
apply (void)
{
	gchar *keymap_id, *keymap_name;

	/* bindings */
	gnome_bindings_properties_save_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME, home_rcfile);
	gnome_binding_entry_list_destroy (saved_bindings);
	saved_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap (GNOME_BINDINGS_PROPERTIES (bi),
											      CUSTOM_KEYMAP_NAME));

	/* properties */
	actual_prop->magic_links = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	keymap_name = gnome_bindings_properties_get_keymap_name (GNOME_BINDINGS_PROPERTIES (bi));
	if (!strcmp (keymap_name, EMACS_KEYMAP_NAME)) {
		keymap_id = "emacs";
	} else if (!strcmp (keymap_name, MS_KEYMAP_NAME)) {
		keymap_id = "ms";
	} else
		keymap_id = "custom";

	g_free (actual_prop->keybindings_theme);
	actual_prop->keybindings_theme = g_strdup (keymap_id);

	actual_prop->live_spell_check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (live_spell_check));
	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (live_spell_color),
				    &actual_prop->spell_error_color.red,
				    &actual_prop->spell_error_color.green,
				    &actual_prop->spell_error_color.blue, NULL);
	g_free (actual_prop->language);
	actual_prop->language = g_strdup (gtk_entry_get_text (GTK_ENTRY (language)));

#ifdef GTKHTML_HAVE_GCONF
	gtk_html_class_properties_update (actual_prop, client, saved_prop);
#else
	gtk_html_class_properties_save (actual_prop);
#endif
	gtk_html_class_properties_copy   (saved_prop, actual_prop);

}

static void
revert (void)
{
	gnome_bindings_properties_set_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME, orig_bindings);
	gnome_binding_entry_list_destroy (saved_bindings);
	saved_bindings = gnome_binding_entry_list_copy (orig_bindings);
	gnome_bindings_properties_save_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME, home_rcfile);

#ifdef GTKHTML_HAVE_GCONF
	gtk_html_class_properties_update (orig_prop, client, saved_prop);
#else
	gtk_html_class_properties_save (orig_prop);
#endif
	gtk_html_class_properties_copy   (saved_prop, orig_prop);
	gtk_html_class_properties_copy   (actual_prop, orig_prop);

	set_ui ();
}

static void
changed (GtkWidget *widget)
{
	if (active)
		capplet_widget_state_changed (CAPPLET_WIDGET (capplet), TRUE);
}

static GtkWidget *
setup_bindings (void)
{
	GtkWidget *vbox;
	guchar *base, *rcfile;

	vbox  = gtk_vbox_new (FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	/* hbox  = gtk_hbox_new (FALSE, 3);
	option = gtk_option_menu_new ();
	menu   = gtk_menu_new ();

#define ADD(l,v) \
	mi = gtk_menu_item_new_with_label (_(l)); \
	gtk_menu_append (GTK_MENU (menu), mi); \
        gtk_signal_connect (GTK_OBJECT (mi), "activate", changed, NULL); \
        gtk_object_set_data (GTK_OBJECT (mi), "theme", v);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
	gtk_option_menu_update_contents
	ADD ("Emacs like", "emacs");
	ADD ("MS like", "ms");
	ADD (CUSTOM_KEYMAP_NAME, "custom");

	gtk_box_pack_start (GTK_BOX (hbox), option, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0); */

#define LOAD(x) \
	base = g_strconcat ("gtkhtml/keybindingsrc.", x, NULL); \
	rcfile = gnome_unconditional_datadir_file (base); \
        gtk_rc_parse (rcfile); \
        g_free (base); \
	g_free (rcfile)

	home_rcfile = g_strconcat (gnome_util_user_home (), "/.gnome/gtkhtml-bindings-custom", NULL);
	gtk_rc_parse (home_rcfile);
	LOAD ("emacs");
	LOAD ("ms");

	bi = gnome_bindings_properties_new ();
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      EMACS_KEYMAP_NAME, "gtkhtml-bindings-emacs", "command",
					      GTK_TYPE_HTML_COMMAND, FALSE);
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      MS_KEYMAP_NAME, "gtkhtml-bindings-ms", "command",
					      GTK_TYPE_HTML_COMMAND, FALSE);
	gnome_bindings_properties_add_keymap (GNOME_BINDINGS_PROPERTIES (bi),
					      CUSTOM_KEYMAP_NAME, "gtkhtml-bindings-custom", "command",
					      GTK_TYPE_HTML_COMMAND, TRUE);
	gnome_bindings_properties_select_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME);
	orig_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap
						       (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME));
	saved_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap
							(GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME));
	gtk_signal_connect (GTK_OBJECT (bi), "changed", changed, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), bi);

	return vbox;
}

static void
live_changed (GtkWidget *w)
{
	gtk_widget_set_sensitive (live_spell_color, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (live_spell_check)));
	changed (w);
}

static GtkWidget *
setup_behaviour (void)
{
	GtkWidget *frame, *vbox, *hbox, *vb1;

	vbox  = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	frame = gtk_frame_new (_("Spell checking"));
	hbox  = gtk_hbox_new (FALSE, 0);
	vb1   = gtk_vbox_new (FALSE, 2);
	live_spell_check = gtk_check_button_new_with_label (_("live spell checking"));
	gtk_signal_connect (GTK_OBJECT (live_spell_check), "toggled", live_changed, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (vb1), 3);
	gtk_box_pack_start (GTK_BOX (hbox), live_spell_check, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), hbox, FALSE, FALSE, 0);

	live_spell_color = gnome_color_picker_new ();
	hbox  = gtk_hbox_new (FALSE, 5);
	gtk_signal_connect (GTK_OBJECT (live_spell_color), "color_set", changed, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("spell checking color")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), live_spell_color, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), hbox, FALSE, FALSE, 0);

	language = gtk_entry_new ();
	hbox  = gtk_hbox_new (FALSE, 5);
	gtk_signal_connect (GTK_OBJECT (language), "changed", changed, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("language")), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), language, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb1), hbox, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vb1);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	frame = gtk_frame_new (_("Miscellaneous"));
	hbox  = gtk_hbox_new (FALSE, 0);
	check = gtk_check_button_new_with_label (_("magic links"));
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
	gtk_signal_connect (GTK_OBJECT (check), "toggled", changed, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	return vbox;
}

static void
setup(void)
{
	GtkWidget *notebook;

        capplet  = capplet_widget_new ();
	notebook = gtk_notebook_new ();

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), setup_bindings (), gtk_label_new (_("Bindings")));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), setup_behaviour (), gtk_label_new (_("Behaviour")));
        gtk_container_add (GTK_CONTAINER (capplet), notebook);
        gtk_widget_show_all (capplet);

	set_ui ();
}

int
main (int argc, char **argv)
{
        bindtextdomain (PACKAGE, GNOMELOCALEDIR);
        textdomain (PACKAGE);

        if (gnome_capplet_init ("gtkhtml-editor-properties", VERSION, argc, argv, NULL, 0, NULL) < 0)
		return 1;

#ifdef GTKHTML_HAVE_GCONF
	if (!gconf_init(argc, argv, &error)) {
		g_assert (error != NULL);
		g_warning ("GConf init failed:\n  %s", error->message);
		return 1;
	}

	client = gconf_client_get_default ();
	gconf_client_add_dir(client, GTK_HTML_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
#endif
	orig_prop = gtk_html_class_properties_new ();
	saved_prop = gtk_html_class_properties_new ();
	actual_prop = gtk_html_class_properties_new ();
#ifdef GTKHTML_HAVE_GCONF
	gtk_html_class_properties_load (actual_prop, client);
#else
	gtk_html_class_properties_load (actual_prop);
#endif
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

	g_free (home_rcfile);

        return 0;
}
