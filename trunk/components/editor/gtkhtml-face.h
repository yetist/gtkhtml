/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-face.h
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

#ifndef GTKHTML_FACE_H
#define GTKHTML_FACE_H

#include "gtkhtml-editor-common.h"

#define GTKHTML_TYPE_FACE \
	(gtkhtml_face_get_type ())

G_BEGIN_DECLS

typedef struct _GtkhtmlFace GtkhtmlFace;

struct _GtkhtmlFace {
	gchar *label;
	gchar *icon_name;
	gchar *text_face;
};

GType		gtkhtml_face_get_type		(void);
gboolean	gtkhtml_face_equal		(GtkhtmlFace *face_a,
						 GtkhtmlFace *face_b);
GtkhtmlFace *	gtkhtml_face_copy		(GtkhtmlFace *face);
void		gtkhtml_face_free		(GtkhtmlFace *face);

G_END_DECLS

#endif /* GTKHTML_FACE_H */
