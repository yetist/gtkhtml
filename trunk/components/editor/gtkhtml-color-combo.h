/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-color-combo.h
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

#ifndef GTKHTML_COLOR_COMBO_H
#define GTKHTML_COLOR_COMBO_H

#include "gtkhtml-editor-common.h"
#include "gtkhtml-color-palette.h"
#include "gtkhtml-color-state.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_COLOR_COMBO \
	(gtkhtml_color_combo_get_type ())
#define GTKHTML_COLOR_COMBO(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_COLOR_COMBO, GtkhtmlColorCombo))
#define GTKHTML_COLOR_COMBO_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_COLOR_COMBO, GtkhtmlColorComboClass))
#define GTKHTML_IS_COLOR_COMBO(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_COLOR_COMBO))
#define GTKHTML_IS_COLOR_COMBO_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_COLOR_COMBO))
#define GTKHTML_COLOR_COMBO_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_COLOR_COMBO, GtkhtmlColorComboClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlColorCombo GtkhtmlColorCombo;
typedef struct _GtkhtmlColorComboClass GtkhtmlColorComboClass;
typedef struct _GtkhtmlColorComboPrivate GtkhtmlColorComboPrivate;

struct _GtkhtmlColorCombo {
	GtkBin parent;
	GtkhtmlColorComboPrivate *priv;
};

struct _GtkhtmlColorComboClass {
	GtkBinClass parent_class;

	void		(*popup)		(GtkhtmlColorCombo *combo);
	void		(*popdown)		(GtkhtmlColorCombo *combo);
};

GType		gtkhtml_color_combo_get_type	(void);
GtkWidget *	gtkhtml_color_combo_new		(void);
GtkWidget *	gtkhtml_color_combo_new_defaults
						(GdkColor *default_color,
						 const gchar *default_label);
void		gtkhtml_color_combo_popup	(GtkhtmlColorCombo *combo);
void		gtkhtml_color_combo_popdown	(GtkhtmlColorCombo *combo);
gboolean	gtkhtml_color_combo_get_current_color
						(GtkhtmlColorCombo *combo,
						 GdkColor *color);
void		gtkhtml_color_combo_set_current_color
						(GtkhtmlColorCombo *combo,
						 GdkColor *color);
void		gtkhtml_color_combo_get_default_color
						(GtkhtmlColorCombo *combo,
						 GdkColor *color);
void		gtkhtml_color_combo_set_default_color
						(GtkhtmlColorCombo *combo,
						 GdkColor *default_color);
const gchar *	gtkhtml_color_combo_get_default_label
						(GtkhtmlColorCombo *combo);
void		gtkhtml_color_combo_set_default_label
						(GtkhtmlColorCombo *combo,
						 const gchar *text);
gboolean	gtkhtml_color_combo_get_default_transparent
						(GtkhtmlColorCombo *combo);
void		gtkhtml_color_combo_set_default_transparent
						(GtkhtmlColorCombo *combo,
						 gboolean transparent);
GtkhtmlColorPalette *
		gtkhtml_color_combo_get_palette	(GtkhtmlColorCombo *combo);
void		gtkhtml_color_combo_set_palette	(GtkhtmlColorCombo *combo,
						 GtkhtmlColorPalette *palette);
GtkhtmlColorState *
		gtkhtml_color_combo_get_state	(GtkhtmlColorCombo *combo);
void		gtkhtml_color_combo_set_state	(GtkhtmlColorCombo *combo,
						 GtkhtmlColorState *state);

G_END_DECLS

#endif /* GTKHTML_COLOR_COMBO_H */
