/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-tool-button.h
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

#ifndef GTKHTML_FACE_TOOL_BUTTON_H
#define GTKHTML_FACE_TOOL_BUTTON_H

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_FACE_TOOL_BUTTON \
	(gtkhtml_face_tool_button_get_type ())
#define GTKHTML_FACE_TOOL_BUTTON(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_FACE_TOOL_BUTTON, GtkhtmlFaceToolButton))
#define GTKHTML_FACE_TOOL_BUTTON_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_FACE_TOOL_BUTTON, GtkhtmlFaceToolButtonClass))
#define GTKHTML_IS_FACE_TOOL_BUTTON(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_FACE_TOOL_BUTTON))
#define GTKHTML_IS_FACE_TOOL_BUTTON_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_FACE_TOOL_BUTTON))
#define GTKHTML_FACE_TOOL_BUTTON_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_FACE_TOOL_BUTTON, GtkhtmlFaceToolButtonClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlFaceToolButton GtkhtmlFaceToolButton;
typedef struct _GtkhtmlFaceToolButtonClass GtkhtmlFaceToolButtonClass;
typedef struct _GtkhtmlFaceToolButtonPrivate GtkhtmlFaceToolButtonPrivate;

struct _GtkhtmlFaceToolButton {
	GtkToggleToolButton parent;
	GtkhtmlFaceToolButtonPrivate *priv;
};

struct _GtkhtmlFaceToolButtonClass {
	GtkToggleToolButtonClass parent_class;

	void	(*popup)		(GtkhtmlFaceToolButton *button);
	void	(*popdown)		(GtkhtmlFaceToolButton *button);
};

GType		gtkhtml_face_tool_button_get_type	(void);
GtkToolItem *	gtkhtml_face_tool_button_new		(void);
void		gtkhtml_face_tool_button_popup		(GtkhtmlFaceToolButton *button);
void		gtkhtml_face_tool_button_popdown	(GtkhtmlFaceToolButton *button);

G_END_DECLS

#endif /* GTKHTML_FACE_TOOL_BUTTON_H */
