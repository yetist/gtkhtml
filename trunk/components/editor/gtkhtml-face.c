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

#include "gtkhtml-face.h"

static GtkhtmlFace *
face_copy (GtkhtmlFace *face)
{
	GtkhtmlFace *copy;

	copy = g_slice_new (GtkhtmlFace);
	copy->label = g_strdup (face->label);
	copy->icon_name = g_strdup (face->icon_name);
	copy->text_face = g_strdup (face->text_face);

	return copy;
}

static void
face_free (GtkhtmlFace *face)
{
	g_free (face->label);
	g_free (face->icon_name);
	g_free (face->text_face);
	g_slice_free (GtkhtmlFace, face);
}

GType
gtkhtml_face_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
		type = g_boxed_type_register_static (
			"GtkhtmlFace",
			(GBoxedCopyFunc) face_copy,
			(GBoxedFreeFunc) face_free);

	return type;
}

gboolean
gtkhtml_face_equal (GtkhtmlFace *face_a,
                    GtkhtmlFace *face_b)
{
	if (face_a == face_b)
		return TRUE;

	if (g_strcmp0 (face_a->label, face_b->label) != 0)
		return FALSE;

	if (g_strcmp0 (face_a->icon_name, face_b->icon_name) != 0)
		return FALSE;

	if (g_strcmp0 (face_a->text_face, face_b->text_face) != 0)
		return FALSE;

	return TRUE;
}

GtkhtmlFace *
gtkhtml_face_copy (GtkhtmlFace *face)
{
	return g_boxed_copy (GTKHTML_TYPE_FACE, face);
}

void
gtkhtml_face_free (GtkhtmlFace *face)
{
	g_boxed_free (GTKHTML_TYPE_FACE, face);
}
