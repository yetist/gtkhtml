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

#include <string.h>
#include "gtkhtmlfontstyle.h"
#include "htmlfontmanager.h"

static void
html_font_set_init (HTMLFontSet *set, gchar *face)
{
	bzero (set, GTK_HTML_FONT_STYLE_MAX_FONT*sizeof (gpointer));
	set->ref_count = 1;
	set->face = g_strdup (face);
}

static HTMLFontSet *
html_font_set_new (gchar *face)
{
	HTMLFontSet *set;

	set = g_new (HTMLFontSet, 1);
	html_font_set_init (set, face);

	return set;
}

static gboolean
html_font_set_face (HTMLFontSet *set, gchar *face)
{
	if (!set->face || strcmp (set->face, face)) {
		if (set->face)
			g_free (set->face);
		set->face = g_strdup (face);
		return TRUE;
	}
	return FALSE;
}

static void
html_font_set_release (HTMLFontSet *set, GFreeFunc destroy)
{
	gint i;

	for (i=0; i<GTK_HTML_FONT_STYLE_MAX_FONT; i++) {
		if (set->font [i])
			(*destroy)(set->font [i]);
		set->font [i] = NULL;
	}
}

static void
html_font_set_unref (HTMLFontSet *set, GFreeFunc destroy)
{
	set->ref_count --;
	if (!set->ref_count) {
		html_font_set_release (set, destroy);
		if (set->face)
			g_free (set->face);

		g_free (set);
	}
}

void
html_font_manager_init (HTMLFontManager *manager, GFreeFunc destroy_font)
{
	manager->font_sets     = g_hash_table_new (g_str_hash, g_str_equal);
	manager->var_size      = 14;
	manager->fix_size      = 14;
	manager->destroy_font  = destroy_font;

	html_font_set_init (&manager->variable, NULL);
	html_font_set_init (&manager->fixed, NULL);
}

static gboolean
destroy_font_set_foreach (gpointer key, gpointer font_set, gpointer destroy)
{
	html_font_set_unref (font_set, (GFreeFunc) destroy);

	return TRUE;
}


static void
release_fonts (HTMLFontManager *manager)
{
	g_hash_table_foreach_remove (manager->font_sets, destroy_font_set_foreach, manager->destroy_font);
}

void
html_font_manager_finalize (HTMLFontManager *manager)
{
	html_font_set_release (&manager->variable, manager->destroy_font);
	html_font_set_release (&manager->fixed, manager->destroy_font);

	release_fonts (manager);
	g_hash_table_destroy (manager->font_sets);
}

void
html_font_manager_set_default (HTMLFontManager *manager, gchar *variable, gchar *fixed, gint var_size, gint fix_size)
{
	gboolean changed = FALSE;

	/* variable width fonts */
	changed = html_font_set_face (&manager->variable, variable);
	if (manager->var_size != var_size) {
		manager->var_size = var_size;
		release_fonts (manager);
		changed = TRUE;
	}
	if (changed) {
		html_font_set_release (&manager->variable, manager->destroy_font);
	}
	changed = FALSE;

	/* fixed width fonts */
	changed = html_font_set_face (&manager->fixed, fixed);
	if (manager->fix_size != fix_size) {
		manager->fix_size = fix_size;
		changed = TRUE;
	}
	if (changed)
		html_font_set_release (&manager->fixed, manager->destroy_font);
}

static gint
get_font_num (GtkHTMLFontStyle style)
{
	return (style == GTK_HTML_FONT_STYLE_DEFAULT)
		? GTK_HTML_FONT_STYLE_SIZE_3
		: (style & GTK_HTML_FONT_STYLE_MAX_FONT_MASK);
}

static gint
html_font_set_get_idx (GtkHTMLFontStyle style)
{
	return get_font_num (style) - 1;
}

static HTMLFontSet *
get_font_set (HTMLFontManager *manager, gchar *face, GtkHTMLFontStyle style)
{
	return (face)
		? g_hash_table_lookup (manager->font_sets, face)
		: ((style & GTK_HTML_FONT_STYLE_FIXED) ? &manager->fixed : &manager->variable);
}

static gdouble
get_real_font_size (HTMLFontManager *manager, GtkHTMLFontStyle style)
{
	return ((style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fix_size : manager->var_size) *
		(1.0 + .08333 * ((get_font_num (style) & GTK_HTML_FONT_STYLE_SIZE_MASK) - GTK_HTML_FONT_STYLE_SIZE_3));
}

static void
html_font_set_font (HTMLFontManager *manager, HTMLFontSet *set, GtkHTMLFontStyle style, gpointer font)
{
	gint idx;

	g_assert (font);
	g_assert (set);

	/* set font in font set */
	idx = html_font_set_get_idx (style);
	if (set->font [idx] && font != set->font [idx])
		(*manager->destroy_font) (set->font [idx]);
	set->font [idx] = font;
}

static gpointer
get_font (HTMLFontManager *manager, HTMLFontSet **set, gchar *face, GtkHTMLFontStyle style)
{
	gpointer font = NULL;

	*set = get_font_set (manager, face, style);
	if (*set)
		font = (*set)->font [html_font_set_get_idx (style)];
	return font;
}

static gpointer
alloc_new_font (HTMLFontManager *manager, HTMLFontSet **set, gchar *face_list, GtkHTMLFontStyle style,
		HTMLFontManagerAllocFont alloc_font)
{
	gpointer font;
	gchar **faces;
	gchar **face;

	if (!(*set)) {
		face = faces = g_strsplit (face_list, ",", 0);
		while (*face) {
			/* first try to get font from available sets */
			font = get_font (manager, set, *face, style);
			if (!font)
				font = alloc_font (*face, get_real_font_size (manager, style), style);
			if (font) {
				if (!(*set)) {
					*set = html_font_set_new (*face);
					g_hash_table_insert (manager->font_sets, *face, *set);
				}
				if (strcmp (face_list, *face)) {
					(*set)->ref_count ++;
					g_hash_table_insert (manager->font_sets, face_list, *set);
				}
				break;
			}
			face++;
		}
		g_strfreev (faces);
		if (!(*set)) {
			/* none of faces exist, so create empty set for him and let manager later set fixed font here */
			*set = html_font_set_new (*face);
			g_hash_table_insert (manager->font_sets, face_list, *set);
		}
	} else
		font = alloc_font ((*set)->face, get_real_font_size (manager, style), style);

	if ((*set) && font)
		html_font_set_font (manager, (*set), style, font);

	return font;
}

gpointer
html_font_manager_get_font (HTMLFontManager *manager, gchar *face_list, GtkHTMLFontStyle style,
			    HTMLFontManagerAllocFont alloc_font)
{
	HTMLFontSet *set;
	gpointer font = NULL;

	font = get_font (manager, &set, face_list, style);

	if (!font) {
		/* first try to alloc right one */
		font = alloc_new_font (manager, &set, face_list, style, alloc_font);
		if (!font) {
			g_assert (set);
			if (!face_list) {
				/* default font, so the last chance is to get fixed font */
				font = alloc_font (NULL, get_real_font_size (manager, style), style);
				if (!font)
					g_error ("Cannot allocate fixed font\n");
			} else {
				/* some unavailable non-default font => use default one */
				font = html_font_manager_get_font (manager, NULL, style, alloc_font);
				/* gdk_font_ref (font); */
			}
			html_font_set_font (manager, set, style, font);
		}
	}

	return font;
}
