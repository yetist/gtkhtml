/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-palette.c
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

#include "gtkhtml-color-palette.h"

/* XXX GtkhtmlColorPalette has a minimal API specifically designed for
 *     use with GtkhtmlColorCombo, but there's plenty of room to grow if
 *     this proves useful elsewhere. */

#include <string.h>
#include <glib/gi18n-lib.h>

#define GTKHTML_COLOR_PALETTE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), GTKHTML_TYPE_COLOR_PALETTE, GtkhtmlColorPalettePrivate))

enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GtkhtmlColorPalettePrivate {
	GHashTable *index;
	GSList *list;
};

static gpointer parent_class;
static guint signals[LAST_SIGNAL];

static void
color_palette_finalize (GObject *object)
{
	GtkhtmlColorPalettePrivate *priv;

	priv = GTKHTML_COLOR_PALETTE_GET_PRIVATE (object);

	g_hash_table_destroy (priv->index);
	g_slist_free (priv->list);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
color_palette_class_init (GtkhtmlColorPaletteClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (class, sizeof (GtkhtmlColorPalettePrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize = color_palette_finalize;

	signals[CHANGED] = g_signal_new (
		"changed",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		0, NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
color_palette_init (GtkhtmlColorPalette *palette)
{
	GHashTable *index;

	index = g_hash_table_new_full (
		(GHashFunc) gdk_color_hash,
		(GEqualFunc) gdk_color_equal,
		(GDestroyNotify) gdk_color_free,
		(GDestroyNotify) NULL);

	palette->priv = GTKHTML_COLOR_PALETTE_GET_PRIVATE (palette);
	palette->priv->index = index;
}

GType
gtkhtml_color_palette_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		static const GTypeInfo type_info = {
			sizeof (GtkhtmlColorPaletteClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) color_palette_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,  /* class_data */
			sizeof (GtkhtmlColorPalette),
			0,     /* n_preallocs */
			(GInstanceInitFunc) color_palette_init,
			NULL   /* value_table */
		};

		type = g_type_register_static (
			G_TYPE_OBJECT, "GtkhtmlColorPalette", &type_info, 0);
	}

	return type;
}

GtkhtmlColorPalette *
gtkhtml_color_palette_new (void)
{
	return g_object_new (GTKHTML_TYPE_COLOR_PALETTE, NULL);
}

void
gtkhtml_color_palette_add_color (GtkhtmlColorPalette *palette,
                                 const GdkColor *color)
{
	GSList *list, *link;

	g_return_if_fail (GTKHTML_IS_COLOR_PALETTE (palette));
	g_return_if_fail (color != NULL);

	list = palette->priv->list;
	link = g_hash_table_lookup (palette->priv->index, color);
	if (link == NULL) {
		list = g_slist_prepend (list, gdk_color_copy (color));
		g_hash_table_insert (palette->priv->index, list->data, list);
	} else {
		list = g_slist_remove_link (list, link);
		list = g_slist_concat (link, list);
	}
	palette->priv->list = list;

	g_signal_emit (G_OBJECT (palette), signals[CHANGED], 0);
}

GSList *
gtkhtml_color_palette_list_colors (GtkhtmlColorPalette *palette)
{
	GSList *list, *iter;

	g_return_val_if_fail (GTKHTML_IS_COLOR_PALETTE (palette), NULL);

	list = g_slist_copy (palette->priv->list);

	for (iter = list; iter != NULL; iter = iter->next)
		iter->data = gdk_color_copy (iter->data);

	return list;
}
