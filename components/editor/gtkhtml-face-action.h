/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-action.h
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

#ifndef GTKHTML_FACE_ACTION_H
#define GTKHTML_FACE_ACTION_H

#include "gtkhtml-editor-common.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_FACE_ACTION \
	(gtkhtml_face_action_get_type ())
#define GTKHTML_FACE_ACTION(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_FACE_ACTION, GtkhtmlFaceAction))
#define GTKHTML_FACE_ACTION_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_FACE_ACTION, GtkhtmlFaceActionClass))
#define GTKHTML_IS_FACE_ACTION(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_FACE_ACTION))
#define GTKHTML_IS_FACE_ACTION_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_FACE_ACTION))
#define GTKHTML_FACE_ACTION_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_FACE_ACTION, GtkhtmlFaceActionClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlFaceAction GtkhtmlFaceAction;
typedef struct _GtkhtmlFaceActionClass GtkhtmlFaceActionClass;
typedef struct _GtkhtmlFaceActionPrivate GtkhtmlFaceActionPrivate;

struct _GtkhtmlFaceAction {
	GtkAction parent;
	GtkhtmlFaceActionPrivate *priv;
};

struct _GtkhtmlFaceActionClass {
	GtkActionClass parent_class;
};

GType		gtkhtml_face_action_get_type	(void);
GtkAction *	gtkhtml_face_action_new		(const gchar *name,
						 const gchar *label,
						 const gchar *tooltip,
						 const gchar *stock_id);

G_END_DECLS

#endif /* GTKHTML_FACE_ACTION_H */
