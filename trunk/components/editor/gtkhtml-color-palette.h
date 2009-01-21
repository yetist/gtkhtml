/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-palette.h
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

#ifndef GTKHTML_COLOR_PALETTE_H
#define GTKHTML_COLOR_PALETTE_H

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_COLOR_PALETTE \
	(gtkhtml_color_palette_get_type ())
#define GTKHTML_COLOR_PALETTE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_COLOR_PALETTE, GtkhtmlColorPalette))
#define GTKHTML_COLOR_PALETTE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_COLOR_PALETTE, GtkhtmlColorPaletteClass))
#define GTKHTML_IS_COLOR_PALETTE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_COLOR_PALETTE))
#define GTKHTML_IS_COLOR_PALETTE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_COLOR_PALETTE))
#define GTKHTML_COLOR_PALETTE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_COLOR_PALETTE, GtkhtmlColorPaletteClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlColorPalette GtkhtmlColorPalette;
typedef struct _GtkhtmlColorPaletteClass GtkhtmlColorPaletteClass;
typedef struct _GtkhtmlColorPalettePrivate GtkhtmlColorPalettePrivate;

struct _GtkhtmlColorPalette {
	GObject parent;
	GtkhtmlColorPalettePrivate *priv;
};

struct _GtkhtmlColorPaletteClass {
	GObjectClass parent_class;
};

GType		gtkhtml_color_palette_get_type	(void);
GtkhtmlColorPalette *
		gtkhtml_color_palette_new	(void);
void		gtkhtml_color_palette_add_color
						(GtkhtmlColorPalette *palette,
						 const GdkColor *color);
GSList *	gtkhtml_color_palette_list_colors
						(GtkhtmlColorPalette *palette);

G_END_DECLS

#endif /* GTKHTML_COLOR_PALETTE_H */
