/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-palette.c
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2025 yetist <yetist@gmail.com>
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

/* XXX GtkhtmlColorPalette has a minimal API specifically designed for
 *     use with GtkhtmlColorCombo, but there's plenty of room to grow if
 *     this proves useful elsewhere. */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <string.h>
#include <glib/gi18n-lib.h>
#include "gtkhtml-color-palette.h"

enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GtkhtmlColorPalette
{
	GObject     parent;
	GHashTable *index;
	GSList     *list;
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (GtkhtmlColorPalette, gtkhtml_color_palette, G_TYPE_OBJECT);

static void
gtkhtml_color_palette_finalize (GObject *object)
{
	GtkhtmlColorPalette *palette;

	palette = GTKHTML_COLOR_PALETTE (object);

	g_hash_table_destroy (palette->index);
	g_slist_free (palette->list);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (gtkhtml_color_palette_parent_class)->finalize (object);
}

static void
gtkhtml_color_palette_class_init (GtkhtmlColorPaletteClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gtkhtml_color_palette_finalize;

	signals[CHANGED] = g_signal_new (
		"changed",
		G_OBJECT_CLASS_TYPE (object_class),
		G_SIGNAL_RUN_LAST,
		0, NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
gtkhtml_color_palette_init (GtkhtmlColorPalette *palette)
{
	GHashTable *index;

	index = g_hash_table_new_full (
		(GHashFunc) gdk_color_hash,
		(GEqualFunc) gdk_color_equal,
		(GDestroyNotify) gdk_color_free,
		(GDestroyNotify) NULL);

	palette->index = index;
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

	list = palette->list;
	link = g_hash_table_lookup (palette->index, color);
	if (link == NULL) {
		list = g_slist_prepend (list, gdk_color_copy (color));
		g_hash_table_insert (palette->index, list->data, list);
	} else {
		list = g_slist_remove_link (list, link);
		list = g_slist_concat (link, list);
	}
	palette->list = list;

	g_signal_emit (G_OBJECT (palette), signals[CHANGED], 0);
}

GSList *
gtkhtml_color_palette_list_colors (GtkhtmlColorPalette *palette)
{
	GSList *list, *iter;

	g_return_val_if_fail (GTKHTML_IS_COLOR_PALETTE (palette), NULL);

	list = g_slist_copy (palette->list);

	for (iter = list; iter != NULL; iter = iter->next)
		iter->data = gdk_color_copy (iter->data);

	return list;
}
