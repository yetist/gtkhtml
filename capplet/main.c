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

static GtkWidget *capplet, *check, *menu, *option, *bi, *variable, *fixed;
static gboolean active = FALSE;
#ifdef GTKHTML_HAVE_GCONF
static GConfError  *error  = NULL;
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
	guint idx;
	gchar *font_name;

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

	font_name = font_name = g_strdup_printf ("-*-%s-*-*-normal-*-%d-*-*-*-*-*-*-*",
						 actual_prop->font_var_family, actual_prop->font_var_size);
	gnome_font_picker_set_font_name (GNOME_FONT_PICKER (variable), font_name);
	g_free (font_name);
	font_name = font_name = g_strdup_printf ("-*-%s-*-*-normal-*-%d-*-*-*-*-*-*-*",
						 actual_prop->font_fix_family, actual_prop->font_fix_size);
	gnome_font_picker_set_font_name (GNOME_FONT_PICKER (fixed), font_name);
	g_free (font_name);

	active = TRUE;
}

static gchar *
get_attr (gchar *font_name, gint n)
{
    gchar *s, *end;

    /* Search paramether */
    for (s=font_name; n; n--,s++)
	    s = strchr (s,'-');

    if (s && *s != 0) {
	    end = strchr (s, '-');
	    if (end)
		    return g_strndup (s, end - s);
	    else
		    return g_strdup (s);
    } else
	    return g_strdup ("Unknown");
}

static void
apply_fonts ()
{
	gchar *size_str;

	g_free (actual_prop->font_var_family);
	actual_prop->font_var_family = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (variable)), 2);
	size_str = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (variable)), 7);
	actual_prop->font_var_size = atoi (size_str);
	g_free (size_str);

	g_free (actual_prop->font_fix_family);
	actual_prop->font_fix_family = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (fixed)), 2);
	size_str = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (fixed)), 7);
	actual_prop->font_fix_size = atoi (size_str);
	g_free (size_str);
}

static void
apply (void)
{
	/* bindings */
	gnome_bindings_properties_save_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME, home_rcfile);
	gnome_binding_entry_list_destroy (saved_bindings);
	saved_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap (GNOME_BINDINGS_PROPERTIES (bi),
											      CUSTOM_KEYMAP_NAME));

	/* properties */
	actual_prop->magic_links = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
	g_free (actual_prop->keybindings_theme);
	actual_prop->keybindings_theme = g_strdup (gtk_object_get_data (GTK_OBJECT (gtk_menu_get_active (GTK_MENU (menu))),
									"theme"));
	apply_fonts ();
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

static void
setup(void)
{
	GtkWidget *vbox, *hbox, *frame, *mi, *table, *label;
	guchar *base, *rcfile;

        capplet = capplet_widget_new();

	vbox  = gtk_vbox_new (FALSE, 3);

	frame = gtk_frame_new (_("Appearance"));
	hbox  = gtk_hbox_new (FALSE, 0);
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 5);
	gtk_container_set_border_width (GTK_CONTAINER (table), 3);

	label = gtk_label_new (_("Variable width font"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 0, 0);
	variable = gnome_font_picker_new ();
	gnome_font_picker_set_title (GNOME_FONT_PICKER (variable), _("Select HTML variable width font"));
	gnome_font_picker_set_mode (GNOME_FONT_PICKER (variable), GNOME_FONT_PICKER_MODE_FONT_INFO);
	gtk_signal_connect (GTK_OBJECT (variable), "font_set", changed, NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), variable, 1, 2, 0, 1);

	label = gtk_label_new (_("Fixed width font"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	fixed = gnome_font_picker_new ();
	gnome_font_picker_set_title (GNOME_FONT_PICKER (fixed), _("Select HTML fixed width font"));
	gnome_font_picker_set_mode (GNOME_FONT_PICKER (fixed), GNOME_FONT_PICKER_MODE_FONT_INFO);
	gtk_signal_connect (GTK_OBJECT (fixed), "font_set", changed, NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), fixed, 1, 2, 1, 2);

	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

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
	ADD (CUSTOM_KEYMAP_NAME, "custom");

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

	home_rcfile = g_strconcat (gnome_util_user_home (), "/.gnome/gtkhtml-bindings-custom", NULL);
	gtk_rc_parse (home_rcfile);
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
					      CUSTOM_KEYMAP_NAME, "gtkhtml-bindings-custom", "command",
					      GTK_TYPE_HTML_COMMAND, TRUE);
	gnome_bindings_properties_select_keymap (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME);
	orig_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap
						       (GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME));
	saved_bindings = gnome_binding_entry_list_copy (gnome_bindings_properties_get_keymap
							(GNOME_BINDINGS_PROPERTIES (bi), CUSTOM_KEYMAP_NAME));
	gtk_signal_connect (GTK_OBJECT (bi), "changed", changed, NULL);
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

#ifdef GTKHTML_HAVE_GCONF
	if (!gconf_init(argc, argv, &error)) {
		g_assert(error != NULL);
		g_warning("GConf init failed:\n  %s", error->str);
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
