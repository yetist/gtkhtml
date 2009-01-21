/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-combo-box.h
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

#ifndef GTKHTML_COMBO_BOX_H
#define GTKHTML_COMBO_BOX_H

/* This is a GtkComboBox that is driven by a group of GtkRadioActions.
 * Just plug in a GtkRadioAction and the widget will handle the rest. */

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_COMBO_BOX \
	(gtkhtml_combo_box_get_type ())
#define GTKHTML_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_COMBO_BOX, GtkhtmlComboBox))
#define GTKHTML_COMBO_BOX_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_COMBO_BOX, GtkhtmlComboBoxClass))
#define GTKHTML_IS_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_COMBO_BOX))
#define GTKHTML_IS_COMBO_BOX_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_COMBO_BOX))
#define GTKHTML_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_COMBO_BOX, GtkhtmlComboBoxClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlComboBox GtkhtmlComboBox;
typedef struct _GtkhtmlComboBoxClass GtkhtmlComboBoxClass;
typedef struct _GtkhtmlComboBoxPrivate GtkhtmlComboBoxPrivate;

struct _GtkhtmlComboBox {
	GtkComboBox parent;
	GtkhtmlComboBoxPrivate *priv;
};

struct _GtkhtmlComboBoxClass {
	GtkComboBoxClass parent_class;
};

GType		gtkhtml_combo_box_get_type	(void);
GtkWidget *	gtkhtml_combo_box_new		(void);
GtkWidget *	gtkhtml_combo_box_new_with_action
						(GtkRadioAction *action);
GtkRadioAction *gtkhtml_combo_box_get_action	(GtkhtmlComboBox *combo_box);
void		gtkhtml_combo_box_set_action	(GtkhtmlComboBox *combo_box,
						 GtkRadioAction *action);
gint		gtkhtml_combo_box_get_current_value
						(GtkhtmlComboBox *combo_box);
void		gtkhtml_combo_box_set_current_value
						(GtkhtmlComboBox *combo_box,
						 gint current_value);

G_END_DECLS

#endif /* GTKHTML_COMBO_BOX_H */
