/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-state.h
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

#ifndef GTKHTML_COLOR_STATE_H
#define GTKHTML_COLOR_STATE_H

/* GtkhtmlColorState is for sharing state between multiple GtkhtmlColorCombo
 * widgets. This makes it easier to manage the same color value from various
 * points in a user interface. It embeds a GtkhtmlColorPalette so that
 * multiple color state objects can share a custom color palette. */

#include "gtkhtml-editor-common.h"
#include "gtkhtml-color-palette.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_COLOR_STATE \
	(gtkhtml_color_state_get_type ())
#define GTKHTML_COLOR_STATE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_COLOR_STATE, GtkhtmlColorState))
#define GTKHTML_COLOR_STATE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_COLOR_STATE, GtkhtmlColorStateClass))
#define GTKHTML_IS_COLOR_STATE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_COLOR_STATE))
#define GTKHTML_IS_COLOR_STATE_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_COLOR_STATE))
#define GTKHTML_COLOR_STATE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_COLOR_STATE, GtkhtmlColorStateClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlColorState GtkhtmlColorState;
typedef struct _GtkhtmlColorStateClass GtkhtmlColorStateClass;
typedef struct _GtkhtmlColorStatePrivate GtkhtmlColorStatePrivate;

struct _GtkhtmlColorState {
	GObject parent;
	GtkhtmlColorStatePrivate *priv;
};

struct _GtkhtmlColorStateClass {
	GObjectClass parent_class;
};

GType		gtkhtml_color_state_get_type	(void);
GtkhtmlColorState *
		gtkhtml_color_state_new		(void);
GtkhtmlColorState *
		gtkhtml_color_state_new_default	(GdkColor *default_color,
						 const gchar *default_label);
gboolean	gtkhtml_color_state_get_current_color
						(GtkhtmlColorState *state,
						 GdkColor *color);
void		gtkhtml_color_state_set_current_color
						(GtkhtmlColorState *state,
						 const GdkColor *color);
void		gtkhtml_color_state_get_default_color
						(GtkhtmlColorState *state,
						 GdkColor *color);
void		gtkhtml_color_state_set_default_color
						(GtkhtmlColorState *state,
						 const GdkColor *color);
const gchar *	gtkhtml_color_state_get_default_label
						(GtkhtmlColorState *state);
void		gtkhtml_color_state_set_default_label
						(GtkhtmlColorState *state,
						 const gchar *text);
gboolean	gtkhtml_color_state_get_default_transparent
						(GtkhtmlColorState *state);
void		gtkhtml_color_state_set_default_transparent
						(GtkhtmlColorState *state,
						 gboolean transparent);
GtkhtmlColorPalette *
		gtkhtml_color_state_get_palette	(GtkhtmlColorState *state);
void		gtkhtml_color_state_set_palette (GtkhtmlColorState *state,
						 GtkhtmlColorPalette *palette);

G_END_DECLS

#endif /* GTKHTML_COLOR_STATE_H */
