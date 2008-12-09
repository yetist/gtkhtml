/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-chooser-menu.h
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

#ifndef GTKHTML_FACE_CHOOSER_MENU_H
#define GTKHTML_FACE_CHOOSER_MENU_H

#include "gtkhtml-editor-common.h"
#include "gtkhtml-face-chooser.h"

/* Standard GObject macros */
#define GTKHTML_TYPE_FACE_CHOOSER_MENU \
	(gtkhtml_face_chooser_menu_get_type ())
#define GTKHTML_FACE_CHOOSER_MENU(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), GTKHTML_TYPE_FACE_CHOOSER_MENU, GtkhtmlFaceChooserMenu))
#define GTKHTML_FACE_CHOOSER_MENU_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), GTKHTML_TYPE_FACE_CHOOSER_MENU, GtkhtmlFaceChooserMenuClass))
#define GTKHTML_IS_FACE_CHOOSER_MENU(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), GTKHTML_TYPE_FACE_CHOOSER_MENU))
#define GTKHTML_IS_FACE_CHOOSER_MENU_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), GTKHTML_TYPE_FACE_CHOOSER_MENU))
#define GTKHTML_FACE_CHOOSER_MENU_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), GTKHTML_TYPE_FACE_CHOOSER_MENU, GtkhtmlFaceChooserMenuClass))

G_BEGIN_DECLS

typedef struct _GtkhtmlFaceChooserMenu GtkhtmlFaceChooserMenu;
typedef struct _GtkhtmlFaceChooserMenuClass GtkhtmlFaceChooserMenuClass;
typedef struct _GtkhtmlFaceChooserMenuPrivate GtkhtmlFaceChooserMenuPrivate;

struct _GtkhtmlFaceChooserMenu {
	GtkMenu parent;
	GtkhtmlFaceChooserMenuPrivate *priv;
};

struct _GtkhtmlFaceChooserMenuClass {
	GtkMenuClass parent_class;
};

GType		gtkhtml_face_chooser_menu_get_type	(void);
GtkWidget *	gtkhtml_face_chooser_menu_new		(void);

G_END_DECLS

#endif /* GTKHTML_FACE_CHOOSER_MENU_H */
