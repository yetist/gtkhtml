/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-action.c
 *
 * Copyright (C) 2008 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gtkhtml-face-action.h"

#include "gtkhtml-face-chooser-menu.h"
#include "gtkhtml-face-tool-button.h"

#define GTKHTML_FACE_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), GTKHTML_TYPE_FACE_ACTION, GtkhtmlFaceActionPrivate))

struct _GtkhtmlFaceActionPrivate {
	GList *choosers;
	GtkhtmlFaceChooser *current_chooser;
};

enum {
	PROP_0,
	PROP_CURRENT_FACE
};

static gpointer parent_class;

static void
face_action_proxy_item_activated_cb (GtkhtmlFaceAction *action,
                                     GtkhtmlFaceChooser *chooser)
{
	action->priv->current_chooser = chooser;

	g_signal_emit_by_name (action, "item-activated");
}

static void
face_action_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CURRENT_FACE:
			gtkhtml_face_chooser_set_current_face (
				GTKHTML_FACE_CHOOSER (object),
				g_value_get_boxed (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
face_action_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_CURRENT_FACE:
			g_value_set_boxed (
				value, gtkhtml_face_chooser_get_current_face (
				GTKHTML_FACE_CHOOSER (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
face_action_finalize (GObject *object)
{
	GtkhtmlFaceActionPrivate *priv;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (object);

	g_list_free (priv->choosers);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
face_action_activate (GtkAction *action)
{
	GtkhtmlFaceActionPrivate *priv;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (action);

	priv->current_chooser = NULL;
}

static GtkWidget *
face_action_create_menu_item (GtkAction *action)
{
	GtkWidget *item;
	GtkWidget *menu;

	item = gtk_image_menu_item_new ();
	menu = gtk_action_create_menu (action);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	gtk_widget_show (menu);

	return item;
}

static GtkWidget *
face_action_create_tool_item (GtkAction *action)
{
	return GTK_WIDGET (gtkhtml_face_tool_button_new ());
}

static void
face_action_connect_proxy (GtkAction *action,
                           GtkWidget *proxy)
{
	GtkhtmlFaceActionPrivate *priv;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (action);

	if (!GTKHTML_IS_FACE_CHOOSER (proxy))
		goto chainup;

	if (g_list_find (priv->choosers, proxy) != NULL)
		goto chainup;

	g_signal_connect_swapped (
		proxy, "item-activated",
		G_CALLBACK (face_action_proxy_item_activated_cb), action);

chainup:
	/* Chain up to parent's connect_proxy() method. */
	GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);
}

static void
face_action_disconnect_proxy (GtkAction *action,
                              GtkWidget *proxy)
{
	GtkhtmlFaceActionPrivate *priv;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (action);

	priv->choosers = g_list_remove (priv->choosers, proxy);

	/* Chain up to parent's disconnect_proxy() method. */
	GTK_ACTION_CLASS (parent_class)->disconnect_proxy (action, proxy);
}

static GtkWidget *
face_action_create_menu (GtkAction *action)
{
	GtkhtmlFaceActionPrivate *priv;
	GtkWidget *widget;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (action);

	widget = gtkhtml_face_chooser_menu_new ();

	g_signal_connect_swapped (
		widget, "item-activated",
		G_CALLBACK (face_action_proxy_item_activated_cb), action);

	priv->choosers = g_list_prepend (priv->choosers, widget);

	return widget;
}

static GtkhtmlFace *
face_action_get_current_face (GtkhtmlFaceChooser *chooser)
{
	GtkhtmlFaceActionPrivate *priv;
	GtkhtmlFace *face = NULL;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (chooser);

	if (priv->current_chooser != NULL)
		face = gtkhtml_face_chooser_get_current_face (
			priv->current_chooser);

	return face;
}

static void
face_action_set_current_face (GtkhtmlFaceChooser *chooser,
                              GtkhtmlFace *face)
{
	GtkhtmlFaceActionPrivate *priv;
	GList *iter;

	priv = GTKHTML_FACE_ACTION_GET_PRIVATE (chooser);

	for (iter = priv->choosers; iter != NULL; iter = iter->next) {
		GtkhtmlFaceChooser *proxy_chooser = iter->data;

		gtkhtml_face_chooser_set_current_face (proxy_chooser, face);
	}
}

static void
face_action_class_init (GtkhtmlFaceActionClass *class)
{
	GObjectClass *object_class;
	GtkActionClass *action_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GtkhtmlFaceAction));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = face_action_set_property;
	object_class->get_property = face_action_get_property;
	object_class->finalize = face_action_finalize;

	action_class = GTK_ACTION_CLASS (class);
	action_class->activate = face_action_activate;
	action_class->create_menu_item = face_action_create_menu_item;
	action_class->create_tool_item = face_action_create_tool_item;
	action_class->connect_proxy = face_action_connect_proxy;
	action_class->disconnect_proxy = face_action_disconnect_proxy;
	action_class->create_menu = face_action_create_menu;

	g_object_class_override_property (
		object_class, PROP_CURRENT_FACE, "current-face");
}

static void
face_action_iface_init (GtkhtmlFaceChooserIface *iface)
{
	iface->get_current_face = face_action_get_current_face;
	iface->set_current_face = face_action_set_current_face;
}

static void
face_action_init (GtkhtmlFaceAction *action)
{
	action->priv = GTKHTML_FACE_ACTION_GET_PRIVATE (action);
}

GType
gtkhtml_face_action_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo type_info = {
			sizeof (GtkhtmlFaceActionClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) face_action_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,  /* class_data */
			sizeof (GtkhtmlFaceAction),
			0,     /* n_preallocs */
			(GInstanceInitFunc) face_action_init,
			NULL   /* value_table */
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) face_action_iface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL  /* interface_data */
		};

		type = g_type_register_static (
			GTK_TYPE_ACTION, "GtkhtmlFaceAction", &type_info, 0);

		g_type_add_interface_static (
			type, GTKHTML_TYPE_FACE_CHOOSER, &iface_info);
	}

	return type;
}

GtkAction *
gtkhtml_face_action_new (const gchar *name,
                         const gchar *label,
                         const gchar *tooltip,
                         const gchar *stock_id)
{
	g_return_val_if_fail (name != NULL, NULL);

	return g_object_new (
		GTKHTML_TYPE_FACE_ACTION, "name", name, "label", label,
		"tooltip", tooltip, "stock-id", stock_id, NULL);
}
