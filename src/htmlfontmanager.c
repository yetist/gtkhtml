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

struct _FontSet {
	gpointer font [GTK_HTML_FONT_STYLE_MAX_FONT];
};
typedef struct _FontSet FontSet;

struct _HTMLFontManager {
	GFreeFunc destroy_font;

	GHashTable *font_sets;
	FontSet variable;
	FontSet fixed;

	gchar *variable_face;
	gchar *fixed_face;
	gint var_size;
	gint fix_size;
};

static void
font_set_init (FontSet *set)
{
	bzero (set, GTK_HTML_FONT_STYLE_MAX_FONT*sizeof (gpointer));
}

static FontSet *
font_set_new ()
{
	FontSet *set;

	set = g_new (FontSet, 1);
	font_set_init (set);

	return set;
}

static void
font_set_destroy (gpointer font_set)
{
	g_free (font_set);
}

static void
font_set_release (FontSet *set, GFreeFunc destroy)
{
	gint i;

	for (i=0; i<GTK_HTML_FONT_STYLE_MAX_FONT; i++)
		(*destroy)(set->font [i]);
	font_set_init (set);
}

void
html_font_manager_init (HTMLFontManager *manager, GFreeFunc destroy_font)
{
	manager->font_sets     = g_hash_table_new (g_str_hash, g_str_equal);
	manager->var_size      = 14;
	manager->fix_size      = 14;
	manager->variable_face = NULL;
	manager->fixed_face    = NULL;
	manager->destroy_font  = destroy_font;

	font_set_init (&manager->variable);
	font_set_init (&manager->fixed);
}

static gboolean
destroy_font_set_foreach (gpointer key, gpointer font_set, gpointer destroy)
{
	font_set_release (font_set, (GFreeFunc) destroy);
	font_set_destroy (font_set);

	return TRUE;
}


static void
release_fonts (HTMLFontManager *manager)
{
	g_hash_table_foreach_remove (manager->font_sets, destroy_font_set_foreach, manager->destroy_font);
}

void
html_font_manager_finish (HTMLFontManager *manager)
{
	if (manager->variable_face)
		g_free (manager->variable_face);
	if (manager->fixed_face)
		g_free (manager->fixed_face);

	font_set_release (&manager->variable, manager->destroy_font);
	font_set_release (&manager->fixed, manager->destroy_font);

	release_fonts (manager);
	g_hash_table_destroy (manager->font_sets);
}

void
html_font_manager_set_default (HTMLFontManager *manager, gchar *variable, gchar *fixed, gint var_size, gint fix_size)
{
	gboolean changed = FALSE;

	/* variable width fonts */
	if (strcmp (manager->variable_face, variable)) {
		if (manager->variable_face)
			g_free (manager->variable_face);
		manager->variable_face = g_strdup (variable);
		changed = TRUE;
	}
	if (manager->var_size != var_size) {
		manager->var_size = var_size;
		release_fonts (manager);
		changed = TRUE;
	}
	if (changed) {
		font_set_release (&manager->variable, manager->destroy_font);
	}
	changed = FALSE;

	/* fixed width fonts */
	if (strcmp (manager->fixed_face, fixed)) {
		if (manager->fixed_face)
			g_free (manager->fixed_face);
		manager->fixed_face = g_strdup (fixed);
		changed = TRUE;
	}
	if (manager->fix_size != fix_size) {
		manager->fix_size = fix_size;
		changed = TRUE;
	}
	if (changed)
		font_set_release (&manager->fixed, manager->destroy_font);
}

static gint
font_set_get_idx (GtkHTMLFontStyle style)
{
	return style & GTK_HTML_FONT_STYLE_MAX_FONT;
}

static FontSet *
get_font_set (HTMLFontManager *manager, gchar *face, GtkHTMLFontStyle style)
{
	return (face)
		? g_hash_table_lookup (manager->font_sets, face)
		: ((style & GTK_HTML_FONT_STYLE_FIXED) ? &manager->fixed : &manager->variable);
}

static gchar *
get_font_face_name (HTMLFontManager *manager,
		    gchar *face,
		    GtkHTMLFontStyle style)
{
	return (face)
		? face
		: ((style & GTK_HTML_FONT_STYLE_FIXED) ? manager->variable_face : manager->fixed_face);
}

static gdouble
get_font_size (HTMLFontManager *manager, GtkHTMLFontStyle style)
{
	return ((style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fix_size : manager->var_size) *
		1.0 + .2 * (((gint) style & GTK_HTML_FONT_STYLE_SIZE_MASK) - 3);
}

static void
insert_font (HTMLFontManager *manager, gchar *face, GtkHTMLFontStyle style, gpointer font)
{
	FontSet *set;
	gint idx;

	g_assert (font);

	/* find font set or add new one */
	set = get_font_set (manager, face, style);
	if (!set) {
		g_assert (face);

		set = font_set_new ();
		g_hash_table_insert (manager->font_sets, face, set);
	}

	/* set font in font set */
	idx = font_set_get_idx (style);
	if (set->font [idx]) {
		(*manager->destroy_font) (set->font [idx]);
		set->font [idx] = font;
	}
}

gpointer
html_font_manager_get_font (HTMLFontManager *manager, gchar *face, GtkHTMLFontStyle style,
			    HTMLFontManagerAllocFont alloc_font, gpointer data)
{
	FontSet *set;
	gpointer font = NULL;

	set = get_font_set (manager, face, style);
	if (set)
		font = set->font [font_set_get_idx (style)];

	if (!font) {
		/* first try to alloc right one */
		font = alloc_font (get_font_face_name (manager, face, style), get_font_size (manager, style), style, data);
		if (font)
			insert_font (manager, face, style, font);
		else {
			if (!face) {
				/* default font, so we last chance is to get fixed font */
				font = alloc_font (NULL, get_font_size (manager, style), style, data);
				if (!font)
					g_error ("Cannot allocate fixed font\n");
				insert_font (manager, NULL, style, font);
			} else
				/* some unavailable non-default font => use default one */
				font = html_font_manager_get_font (manager, NULL, style, alloc_font, data);
		}
	}

	return font;
}

