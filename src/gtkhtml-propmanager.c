/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2002 Ximian Inc.
   Authors:           Larry Ewing <lewing@ximian.com>

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
	GtkWidget *fixedl;
	GtkWidget *fixed_print;
	GtkWidget *anim_check;
	GtkWidget *bi; 
	GtkWidget *live_spell_check;
	GtkWidget *live_spell_frame; 
	GtkWidget *magic_check;
	GtkWidget *button_cfg_spell;
	
	GtkHTMLClassProperties *saved_prop;
	GtkHTMLClassProperties *orig_prop;
	GtkHTMLClassProperties *actual_prop;

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

	if (priv->live_spell_check) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->live_spell_check),
					      priv->actual_prop->live_spell_check);
	}

	if (priv->button_cfg_spell) {	
		gtk_widget_set_sensitive (GTK_WIDGET (priv->button_cfg_spell), priv->actual_prop->live_spell_check);
	}

#if 0
#define SET_FONT(f,w) \
        if (w) gnome_font_picker_set_font_name (GNOME_FONT_PICKER (w), priv->actual_prop-> f)

	SET_FONT (font_var,       priv->variable);
	SET_FONT (font_fix,       priv->fixed);
	SET_FONT (font_var_print, priv->variable_print);
	SET_FONT (font_fix_print, fixed_print);
#endif
}
	
gboolean
gtk_html_propmanager_set_xml (GtkHTMLPropmanager *pman, GladeXML *xml)
{
	GtkHTMLPropmanagerPrivate *priv;

	GConfClient *client;
	gboolean found_widget = FALSE;

	g_return_val_if_fail (pman != NULL, FALSE);
	priv = pman->priv;

	client = gconf_client_get_default ();
	gconf_client_add_dir (client, GTK_HTML_GCONF_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);

	priv->orig_prop = gtk_html_class_properties_new ();
	priv->saved_prop = gtk_html_class_properties_new ();
	priv->actual_prop = gtk_html_class_properties_new ();

#ifdef GTKHTML_HAVE_GCONF
	gtk_html_class_properties_load (priv->actual_prop, client);
#else
	gtk_html_class_properties_load (priv->actual_prop);
#endif
	gtk_html_class_properties_copy (priv->saved_prop, priv->actual_prop);
	gtk_html_class_properties_copy (priv->orig_prop, priv->actual_prop);

	if (priv->anim_check = glade_xml_get_widget (xml, "anim_check")) {
		found_widget = TRUE;
	}
	if (priv->magic_check = glade_xml_get_widget (xml, "magic_check")) {
		found_widget = TRUE;
	}
	if (priv->live_spell_check = glade_xml_get_widget (xml, "live_spell_check")) {
		found_widget = TRUE;
	}
	if (priv->live_spell_frame = glade_xml_get_widget (xml, "live_spell_frame")) {
		found_widget = TRUE;
	}
	if (priv->button_cfg_spell = glade_xml_get_widget (xml, "button_configure_spell_checking")) {
		found_widget = TRUE;
	}
	
	return found_widget;
}

static void
gtk_html_propmanager_init (GtkHTMLPropmanager *pman)
{
	GtkHTMLPropmanagerPrivate *priv;

	priv = g_new0 (GtkHTMLPropmanagerPrivate, 1);
	
	pman->priv = priv;
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
	
	//object_class->finalize = gtk_html_propmanager_finalize;
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
