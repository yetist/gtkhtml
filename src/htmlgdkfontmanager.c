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
#include <unicode.h>

static void release_fonts (HTMLGdkFontFace *face);
static GdkFont * get_closest_font (HTMLGdkFontFace *face, const gchar *weight, const gchar *slant, guint html_size);

static HTMLGdkFontFace *
html_gdk_font_face_new (const gchar *family, gint size)
{
	HTMLGdkFontFace *face = g_new0 (HTMLGdkFontFace, 1);

	g_assert (family);

	face->face.family = g_strdup (family);
	face->face.size   = size;

	return face;
}

static void
html_gdk_font_face_destroy (HTMLGdkFontFace *face)
{
	release_fonts (face);

	if (face->face.family) g_free (face->face.family);

	g_free (face);
}

HTMLGdkFontManager *
html_gdk_font_manager_new  (void)
{
	HTMLGdkFontManager *new;

	new = g_new (HTMLGdkFontManager, 1);

	new->variable = html_gdk_font_face_new ("helvetica", 12);
	new->fixed    = html_gdk_font_face_new ("courier", 12);
	new->faces    = g_hash_table_new (g_str_hash, g_str_equal);

	return new;
}

static void
destroy_face (gchar *family, HTMLGdkFontFace *face)
{
	html_gdk_font_face_destroy (face);
}

void
html_gdk_font_manager_destroy (HTMLGdkFontManager *manager)
{
	html_gdk_font_face_destroy (manager->variable);
	html_gdk_font_face_destroy (manager->fixed);
	g_hash_table_foreach (manager->faces, (GHFunc) destroy_face, NULL);
	g_hash_table_destroy (manager->faces);

	g_free (manager);
}

/* fixme: Ugly-ugly hack */

static gboolean
font_face_exists (const gchar *name)
{
	gchar *font_name, **list;
	gint n;

	font_name = g_strdup_printf ("-*-%s-*-*-normal-*-*-*-*-*-*-*-*-*", name);
	list = XListFonts (GDK_DISPLAY (), font_name, 1, &n);
	XFreeFontNames (list);
	g_free (font_name);

	return n > 0;
}

HTMLGdkFontFace *
html_gdk_font_manager_find_face (HTMLGdkFontManager *manager, const gchar *families)
{
	HTMLGdkFontFace *face;
	gchar **names;
	gchar **iterate;

	names = g_strsplit (families, ",", 32);

	face = NULL;
	for (iterate = names; *iterate != NULL; iterate++) {
		face = (HTMLGdkFontFace *) g_hash_table_lookup (manager->faces, *iterate);
		if (face)
			break;
		if (font_face_exists (*iterate)) {
			face = html_gdk_font_face_new (*iterate, manager->variable->face.size);
			g_hash_table_insert (manager->faces, (gpointer) *iterate, face);
			break;
		}
	}

	g_strfreev (names);

	return (face) ? face : manager->variable;
}

HTMLGdkFontFace *
html_gdk_font_manager_get_variable (HTMLGdkFontManager *manager)
{
	return manager->variable;
}

HTMLGdkFontFace *
html_gdk_font_manager_get_fixed (HTMLGdkFontManager *manager)
{
	return manager->fixed;
}

static GdkFont *
html_gdk_font_face_get_font (HTMLGdkFontFace *face, GtkHTMLFontStyle style)
{
	g_assert (face);

	if (!face->font [style]) {
		const gchar *weight;
		const gchar *slant;
		gint html_size;


		html_size = (style & GTK_HTML_FONT_STYLE_SIZE_MASK) ? style & GTK_HTML_FONT_STYLE_SIZE_MASK : 3;
		weight    = (style & GTK_HTML_FONT_STYLE_BOLD)   ? "bold" : "medium";
		slant     = (style & GTK_HTML_FONT_STYLE_ITALIC) ? "i" : "r";

		face->font[style] = get_closest_font (face, weight, slant, html_size);
	}

	return face->font [style];
}

GdkFont *
html_gdk_font_manager_get_font (HTMLGdkFontManager *manager, GtkHTMLFontStyle style, HTMLGdkFontFace *face)
{
	if (!face)
		face = (style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fixed : manager->variable;

	return html_gdk_font_face_get_font (face, style);
}

static void
html_gdk_font_face_set_size (HTMLGdkFontFace *face, gint size)
{
	if (size != face->face.size) {
		release_fonts (face);
		face->face.size = size;
	}
}

static void
html_gdk_font_face_set_family (HTMLGdkFontFace *face, const gchar *family)
{
	g_assert (family);

	if (strcmp (family, face->face.family)) {
		release_fonts (face);
		g_free (face->face.family);
		face->face.family = g_strdup (family);
	}
}

void
html_gdk_font_manager_set_variable (HTMLGdkFontManager *manager,
				    const gchar *family,
				    gint size)
{
	if (family) html_gdk_font_face_set_family (manager->variable, family);
	if (size > 0) html_gdk_font_face_set_size (manager->variable, size);
}

void
html_gdk_font_manager_set_fixed (HTMLGdkFontManager *manager,
				 const gchar *family,
				 gint size)
{
	if (family) html_gdk_font_face_set_family (manager->fixed, family);
	if (size > 0) html_gdk_font_face_set_size (manager->fixed, size);
}

static void
set_size (gchar *family, HTMLGdkFontFace *face, gpointer size)
{
	html_gdk_font_face_set_size (face, GPOINTER_TO_INT (size));
}

void
html_gdk_font_manager_set_size (HTMLGdkFontManager *manager, gint size)
{
	g_hash_table_foreach (manager->faces, (GHFunc) set_size, GINT_TO_POINTER (size));
}

gint html_gdk_text_width (HTMLGdkFontManager *manager,
			  HTMLGdkFontFace *face,
			  GtkHTMLFontStyle style,
			  gchar *text, gint bytes)
{
	GdkFont *font;
	gchar *p;
	gint width;

	g_return_val_if_fail (manager != NULL, 0);

	if (!text) return 0;

	if (!face) face = (style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fixed : manager->variable;

	font = html_gdk_font_face_get_font (face, style);

	width = 0;

	for (p = text; p && *p && p - text < bytes; p = unicode_next_utf8 (p)) {
		unicode_char_t unival;
		guchar c;
		unicode_get_utf8 (p, &unival);
		if (unival < 0) unival = ' ';
		if (unival > 255) unival = ' ';
		c = unival;
		width += gdk_text_width (font, &c, 1);
	}

	return width;
}

void html_gdk_draw_text (HTMLGdkFontManager *manager,
			 HTMLGdkFontFace *face,
			 GtkHTMLFontStyle style,
			 GdkDrawable *drawable,
			 GdkGC *gc,
			 gint x, gint y,
			 gchar *text, gint bytes)
{
	GdkFont *font;
	gchar *p;
	guchar *b;
	gint len;

	g_return_if_fail (manager != NULL);

	if (!text) return;

	if (!face) face = (style & GTK_HTML_FONT_STYLE_FIXED) ? manager->fixed : manager->variable;

	font = html_gdk_font_face_get_font (face, style);

	b = alloca (bytes);
	len = 0;

	for (p = text; p && *p && p - text < bytes; p = unicode_next_utf8 (p)) {
		unicode_char_t unival;
		unicode_get_utf8 (p, &unival);
		if (unival < 0) unival = ' ';
		if (unival > 255) unival = ' ';
		b[len++] = unival;
	}

	gdk_draw_text (drawable, font, gc, x, y, b, len);
}


/* static helper functions */

static gint
get_size (gchar *font_name)
{
    gchar *s, *end;
    gint n;

    /* Search paramether */
    for (s=font_name, n=7; n; n--,s++)
	    s = strchr (s,'-');

    if (s && *s != 0) {
	    end = strchr (s, '-');
	    if (end) {
		    *end = 0;
		    n = atoi (s);
		    *end = '-';
		    return n;
	    } else
		    return 0;
    } else
	    return 0;
}

static gboolean
find_font (gchar *font_name, gint *req_size)
{
	gint n, size, smaller, bigger;
	gchar **list;
	gboolean rv = FALSE;

	/* list available sizes */
	smaller = bigger = size = 0;
	list = XListFonts (GDK_DISPLAY (), font_name, 0xffff, &n);
	/* look for right one */
	while (n) {
		n--;
		size = get_size (list [n]);
		if (size == *req_size) {
			rv = TRUE;
			break;
		} else if (size < *req_size && size > smaller)
			smaller = size;
		else if (size > *req_size && (size < bigger || !bigger))
			bigger = size;
	}
	XFreeFontNames (list);

	/* if not found use closest one */
	if (!rv && (bigger || smaller)) {
		if (!bigger)
			size = smaller;
		if (!smaller)
			size = bigger;
		if (bigger && smaller)
			size = (bigger - *req_size <= smaller - *req_size) ? bigger : smaller;
		*req_size = size;
		rv = TRUE;
	}

	return rv;
}

static GdkFont *
get_font (const gchar *family, const gchar *weight, const gchar *slant, guint fsize)
{
	GdkFont *font;
	gchar *font_name;

	font      = NULL;
	font_name = g_strdup_printf ("-*-%s-%s-%s-normal-*-*-*-*-*-*-*-*-*",
				     family, weight, slant);
	if (find_font (font_name, &fsize)) {
		g_free (font_name);
		font_name = g_strdup_printf ("-*-%s-%s-%s-normal-*-%d-*-*-*-*-*-*-*",
					     family, weight, slant, fsize);
		font = gdk_font_load (font_name);
	}
	g_free (font_name);

	return font;
}

static GdkFont *
get_closest_font (HTMLGdkFontFace *face, const gchar *weight,
		  const gchar *slant, guint html_size)
{
	GdkFont *font;
	guint font_size;

	font_size = face->face.size + (((gdouble) face->face.size / 5.0) * (((gint) html_size) - 3));
	font = get_font (face->face.family, weight, slant, font_size);

	/* try use obligue instead of italic */
	if (!font && *slant == 'i')
		font = get_closest_font (face, weight, "o", html_size);

	/* last try - use fixed font */
	if (font == NULL){
		g_warning ("suitable font not found, trying to use fixed");
		font = gdk_font_load ("fixed");
		/* give up */
		g_assert (font);
	}

	return font;
}

static void
release_fonts (HTMLGdkFontFace *face)
{
	guint i;

	g_assert (face != NULL);

	for (i = 0; i < GTK_HTML_FONT_STYLE_MAX; i++) {
		if (face->font [i] != NULL) {
			gdk_font_unref (face->font [i]);
			face->font [i] = NULL;
		}
	}
}
