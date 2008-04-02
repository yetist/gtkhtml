/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-swatch.h
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

#ifndef GTKHTML_COLOR_SWATCH_H
#define GTKHTML_COLOR_SWATCH_H

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_COLOR_SWATCH \
	(gtkhtml_color_swatch_get_type ())
#define GTKHTML_COLOR_SWATCH(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_COLOR_SWATCH, GtkhtmlColorSwatch))
#define GTKHTML_COLOR_SWATCH_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_COLOR_SWATCH, GtkhtmlColorSwatchClass))
#define GTKHTML_IS_COLOR_SWATCH(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_COLOR_SWATCH))
#define GTKHTML_IS_COLOR_SWATCH_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_COLOR_SWATCH))
#define GTKHTML_COLOR_SWATCH_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_COLOR_SWATCH, GtkhtmlColorSwatchClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlColorSwatch GtkhtmlColorSwatch;
typedef struct _GtkhtmlColorSwatchClass GtkhtmlColorSwatchClass;
typedef struct _GtkhtmlColorSwatchPrivate GtkhtmlColorSwatchPrivate;

struct _GtkhtmlColorSwatch {
	GtkBin parent;
	GtkhtmlColorSwatchPrivate *priv;
};

struct _GtkhtmlColorSwatchClass {
	GtkBinClass parent_class;
};

GType		gtkhtml_color_swatch_get_type	(void);
GtkWidget *	gtkhtml_color_swatch_new	(void);
void		gtkhtml_color_swatch_get_color	(GtkhtmlColorSwatch *swatch,
						 GdkColor *color);
void		gtkhtml_color_swatch_set_color	(GtkhtmlColorSwatch *swatch,
						 const GdkColor *color);
GtkShadowType	gtkhtml_color_swatch_get_shadow_type
						(GtkhtmlColorSwatch *swatch);
void		gtkhtml_color_swatch_set_shadow_type
						(GtkhtmlColorSwatch *swatch,
						 GtkShadowType shadow_type);

G_END_DECLS

#endif /* GTKHTML_COLOR_SWATCH_H */
