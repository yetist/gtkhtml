/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 2000 Helix Code, Inc.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include "htmlgdkfontmanager.h"
#include <gdk/gdkx.h>
#include <string.h>
#include <stdlib.h>

HTMLGdkFontManager *
html_gdk_font_manager_new  (void)
{
	HTMLGdkFontManager *new;

	new = g_new (HTMLGdkFontManager, 1);

	new->variable = html_font_face_new ("helvetica", 12);
	new->fixed    = html_font_face_new ("courier", 12);
	new->faces    = g_hash_table_new (g_str_hash, g_str_equal);

	return new;
}

static void
destroy_face (gchar *family, HTMLFontFace *face)
{
	html_font_face_destroy (face);
}

void
html_gdk_font_manager_destroy (HTMLGdkFontManager *manager)
{
	html_font_face_destroy (manager->variable);
	html_font_face_destroy (manager->fixed);
	g_hash_table_foreach (manager->faces, (GHFunc) destroy_face, NULL);
	g_hash_table_destroy (manager->faces);

	g_free (manager);
}

HTMLFontFace *
html_gdk_font_manager_get_face (HTMLGdkFontManager *manager, const gchar *family)
{
	HTMLFontFace *face;

	face = (HTMLFontFace *) g_hash_table_lookup (manager->faces, family);
	if (!face) {
		face = html_font_face_new (family, manager->variable->size);
		g_hash_table_insert (manager->faces, (gpointer) family, face);
	}

	return face;
}

HTMLFontFace *
html_gdk_font_manager_get_variable (HTMLGdkFontManager *manager)
{
	return manager->variable;
}

HTMLFontFace *
html_gdk_font_manager_get_fixed (HTMLGdkFontManager *manager)
{
	return manager->fixed;
}

GdkFont *
html_gdk_font_manager_get_font (HTMLGdkFontManager *manager, GtkHTMLFontStyle style, HTMLFontFace *face)
{
	if (!face)
		face = (style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fixed : manager->variable;

	return html_font_face_get_font (face, style);
}
