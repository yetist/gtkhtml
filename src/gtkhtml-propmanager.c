/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000, 2001, 2002 Ximian Inc.
   Authors:           Larry Ewing <lewing@ximian.com>
                      Radek Doulik <rodo@ximian.com>

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

#include "gtkhtml-propmanager.h"
#include "gtkhtml-properties.h"

#define d(x) x;

static GtkObject *parent_class;

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

struct _GtkHTMLPropmanagerPrivate {
	GtkWidget *capplet;
	GtkWidget *variable;
	GtkWidget *variable_print;
	GtkWidget *fixed;
	GtkWidget *fixed_print;
	GtkWidget *anim_check;
	GtkWidget *bi; 
	GtkWidget *live_spell_check;
	GtkWidget *magic_check;
	GtkWidget *button_cfg_spell;
	
	GtkHTMLClassProperties *saved_prop;
	GtkHTMLClassProperties *orig_prop;
	GtkHTMLClassProperties *actual_prop;

	guint notify_id;
	gboolean active;
};

static void
gtk_html_propmanager_sync_ui (GtkHTMLPropmanager *pman)
{
	GtkHTMLPropmanagerPrivate *priv;

	g_return_if_fail (pman != NULL);
	priv = pman->priv;

	if (priv->anim_check)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->anim_check),
					      priv->actual_prop->animations);

	if (priv->magic_check)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->magic_check),
					      priv->actual_prop->magic_links);

	if (priv->live_spell_check)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->live_spell_check),
					      priv->actual_prop->live_spell_check);

	if (priv->button_cfg_spell) {	
		gtk_widget_set_sensitive (GTK_WIDGET (priv->button_cfg_spell), priv->actual_prop->live_spell_check);
	}


#define SET_FONT(f,w) \
        if (w) gnome_font_picker_set_font_name (GNOME_FONT_PICKER (w), priv->actual_prop-> f);

	SET_FONT (font_var,       priv->variable);
	SET_FONT (font_fix,       priv->fixed);
	SET_FONT (font_var_print, priv->variable_print);
	SET_FONT (font_fix_print, priv->fixed_print);
}
	
static void
propmanager_client_notify (GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer data)
{
	GtkHTMLPropmanager *pman = data;

	d (printf ("GOT MILK??"));

	gtk_html_class_properties_load (pman->priv->actual_prop, client);
	gtk_html_propmanager_sync_ui (pman);
}

static void
propmanager_changed (GtkWidget *widget, GtkHTMLPropmanager *pman)
{
	gtk_signal_emit (GTK_OBJECT (pman), signals[CHANGED]);
}

static void
propmanager_sensitivity_changed (GtkWidget *widget, GtkHTMLPropmanager *pman)
{
	GtkHTMLPropmanagerPrivate *priv;
	g_return_if_fail (pman != NULL);

	priv = pman->priv;
	gtk_widget_set_sensitive (priv->button_cfg_spell,
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->live_spell_check)));
}

#define SELECTOR(x) GTK_FONT_SELECTION_DIALOG (GNOME_FONT_PICKER (x)->font_dialog)

static void
propmanager_picker_clicked (GtkWidget *w, gpointer proportional)
{
	gchar *mono_spaced [] = { "c", "m", NULL };

	if (GPOINTER_TO_INT (proportional))
		gtk_font_selection_dialog_set_filter (SELECTOR (w),
						      GTK_FONT_FILTER_BASE, GTK_FONT_ALL,
						      NULL, NULL, NULL, NULL,
						      mono_spaced, NULL);
}

static void
propmanager_child_destroyed (GtkWidget *w, GtkHTMLPropmanager *pman)
{

}

static GtkWidget *
propmanager_add_picker (GtkWidget *picker, gboolean proportional, gboolean *found)
{
	if (picker) {
		gtk_signal_connect (GTK_OBJECT (picker), "clicked", propmanager_picker_clicked,
				    GINT_TO_POINTER (proportional));
		gtk_signal_connect (GTK_OBJECT (picker), "destroy", propmanager_child_destroyed,
				    NULL);
		*found = TRUE;
	}
	return picker;
}

gboolean
gtk_html_propmanager_set_xml (GtkHTMLPropmanager *pman, GladeXML *xml)
{
	GtkHTMLPropmanagerPrivate *priv;
	GError      *gconf_error  = NULL;

	GConfClient *client;
	gboolean found_widget = FALSE;

	g_return_val_if_fail (pman != NULL, FALSE);
	priv = pman->priv;

#ifdef USE_CLIENT 
	client = gconf_client_get_default ();
#else 
	client = pman->client;
#endif
	gconf_client_add_dir (client, GTK_HTML_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);

	priv->orig_prop = gtk_html_class_properties_new ();
	priv->saved_prop = gtk_html_class_properties_new ();
	priv->actual_prop = gtk_html_class_properties_new ();

	gtk_html_class_properties_load (priv->actual_prop, client);

	gtk_html_class_properties_copy (priv->saved_prop, priv->actual_prop);
	gtk_html_class_properties_copy (priv->orig_prop, priv->actual_prop);

	if ((priv->anim_check = glade_xml_get_widget (xml, "anim_check"))) {
		found_widget = TRUE;
	}
	if ((priv->magic_check = glade_xml_get_widget (xml, "magic_check"))) {
		found_widget = TRUE;
	}
	if ((priv->live_spell_check = glade_xml_get_widget (xml, "live_spell_check"))) {
		found_widget = TRUE;
	}
	if ((priv->button_cfg_spell = glade_xml_get_widget (xml, "button_configure_spell_checking"))) {
		found_widget = TRUE;
	}

	priv->variable = propmanager_add_picker (glade_xml_get_widget (xml, "screen_variable"),
						 TRUE, &found_widget);
	priv->variable_print = propmanager_add_picker (glade_xml_get_widget (xml, "print_variable"),
						       TRUE, &found_widget);
	priv->fixed = propmanager_add_picker (glade_xml_get_widget (xml, "screen_fixed"),
					      FALSE, &found_widget);
	priv->fixed_print = propmanager_add_picker (glade_xml_get_widget (xml, "print_fixed"),
						    FALSE, &found_widget);

	priv->notify_id = gconf_client_notify_add (client, GTK_HTML_GCONF_DIR, 
						   propmanager_client_notify, 
						   pman, NULL, &gconf_error);
	if (gconf_error)
		g_warning ("gconf error: %s\n", gconf_error->message);
				 
	gtk_html_propmanager_sync_ui (pman);
	glade_xml_signal_connect_data (xml, "changed", 
				       GTK_SIGNAL_FUNC (propmanager_changed), 
				       pman);
	
	return found_widget;
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

void
gtk_html_propmanager_apply (GtkHTMLPropmanager *pman)
{
	GtkHTMLPropmanagerPrivate *priv;
	gchar *size_str;

	g_return_if_fail (pman != NULL);

	priv = pman->priv;
	
	if (priv->anim_check) {
		priv->actual_prop->animations = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->anim_check));
		if (priv->actual_prop->animations) {
			d (g_warning ("FUCKING HELL"));
		} else {
			d (g_warning ("FUCKING HEAVEN"));
		}
	}
	if (priv->magic_check)
		priv->actual_prop->magic_links = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->magic_check));

	if (priv->live_spell_check)
		priv->actual_prop->live_spell_check = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->live_spell_check));

#define APPLY(f,s,w) \
        if (w) { \
	        g_free (priv->actual_prop-> f); \
	        priv->actual_prop-> f = g_strdup (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (w))); \
	        size_str = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (w)), 7); \
                if (!strcmp (size_str, "*")) { \
                          g_free (size_str); \
	                  size_str = get_attr (gnome_font_picker_get_font_name (GNOME_FONT_PICKER (w)), 8); \
                          priv->actual_prop-> f ## _points = TRUE; \
                } else { \
                          priv->actual_prop-> f ## _points = FALSE; \
                } \
	        priv->actual_prop-> s = atoi (size_str); \
	        g_free (size_str); \
	}

	APPLY (font_var,       font_var_size,       priv->variable);
	APPLY (font_fix,       font_fix_size,       priv->fixed);
	APPLY (font_var_print, font_var_size_print, priv->variable_print);
	APPLY (font_fix_print, font_fix_size_print, priv->fixed_print);

	gtk_html_class_properties_update (priv->actual_prop, pman->client,
					  priv->saved_prop);
	gtk_html_class_properties_copy (priv->saved_prop, priv->actual_prop);
}

void
gtk_html_propmanager_reset (GtkHTMLPropmanager *pman)
{

}

static void
gtk_html_propmanager_init (GtkHTMLPropmanager *pman)
{
	GtkHTMLPropmanagerPrivate *priv;

	d(g_warning ("init called"));
	priv = g_new0 (GtkHTMLPropmanagerPrivate, 1);
	
	pman->priv = priv;
}

GtkObject *
gtk_html_propmanager_new (GladeXML *xml)
{
	GtkHTMLPropmanager *pman;
	
	pman = GTK_HTML_PROPMANAGER ( gtk_type_new (gtk_html_propmanager_get_type ()));

	if (xml)
		gtk_html_propmanager_set_xml (pman, xml);

	return (GtkObject *)pman;
}

static void
gtk_html_propmanager_finalize (GtkObject *object)
{
	GtkHTMLPropmanagerPrivate *priv = GTK_HTML_PROPMANAGER (object)->priv;

	if (priv->orig_prop) {
		gtk_html_class_properties_destroy (priv->orig_prop);
		gtk_html_class_properties_destroy (priv->actual_prop);
		gtk_html_class_properties_destroy (priv->saved_prop);
	}

	if (priv->notify_id)
		gconf_client_notify_remove (GTK_HTML_PROPMANAGER (object)->client, priv->notify_id);

	g_free (priv);
}

static void
gtk_html_propmanager_class_init (GtkHTMLPropmanagerClass *class)
{
	GtkObjectClass *object_class;
	GtkHTMLPropmanagerClass *propmanager_class;

	object_class = (GtkObjectClass *)class;
	propmanager_class = (GtkHTMLPropmanagerClass *)class;

	parent_class = gtk_type_class (gtk_object_get_type ());

	signals [CHANGED] = 
		gtk_signal_new ("changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLPropmanagerClass, changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
	
	object_class->finalize = gtk_html_propmanager_finalize;
}

	
GtkType
gtk_html_propmanager_get_type (void)
{
	static GtkType propmanager_type = 0;

	if (!propmanager_type) {
		GtkTypeInfo propmanager_type_info = {
			"GtkHTMLPropmanager",
			sizeof (GtkHTMLPropmanager),
			sizeof (GtkHTMLPropmanagerClass),
			(GtkClassInitFunc) gtk_html_propmanager_class_init,
			(GtkObjectInitFunc) gtk_html_propmanager_init,
			NULL,
			NULL
		};
		
		propmanager_type = gtk_type_unique (gtk_object_get_type (), &propmanager_type_info);
	}

	return propmanager_type;
}
