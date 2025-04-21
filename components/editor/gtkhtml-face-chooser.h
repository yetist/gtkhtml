/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face-chooser.h
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

#ifndef GTKHTML_FACE_CHOOSER_H
#define GTKHTML_FACE_CHOOSER_H

#include "gtkhtml-editor-common.h"
#include "gtkhtml-face.h"

G_BEGIN_DECLS

#define GTKHTML_TYPE_FACE_CHOOSER gtkhtml_face_chooser_get_type ()
G_DECLARE_INTERFACE (GtkhtmlFaceChooser, gtkhtml_face_chooser, GTKHTML, FACE_CHOOSER, GObject)

struct _GtkhtmlFaceChooserInterface {
	GTypeInterface parent_iface;

	/* Methods */
	GtkhtmlFace *	(*get_current_face)	(GtkhtmlFaceChooser *chooser);
	void		(*set_current_face)	(GtkhtmlFaceChooser *chooser,
						 GtkhtmlFace *face);

	/* Signals */
	void		(*item_activated)	(GtkhtmlFaceChooser *chooser);
};

GtkhtmlFace *	gtkhtml_face_chooser_get_current_face
						(GtkhtmlFaceChooser *chooser);
void		gtkhtml_face_chooser_set_current_face
						(GtkhtmlFaceChooser *chooser,
						 GtkhtmlFace *face);
void		gtkhtml_face_chooser_item_activated
						(GtkhtmlFaceChooser *chooser);
GList *		gtkhtml_face_chooser_get_items	(GtkhtmlFaceChooser *chooser);

G_END_DECLS

#endif /* GTKHTML_FACE_CHOOSER_H */
