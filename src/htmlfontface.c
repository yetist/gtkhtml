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

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkx.h>
#include "htmlfontface.h"

#if 0
static void      release_fonts      (HTMLFontFace *face);
static GdkFont * get_closest_font   (HTMLFontFace *face, const gchar *weight,
				     const gchar *slant, guint html_size);

HTMLFontFace *
html_font_face_new (const gchar *family, gint size)
{
	HTMLFontFace *face = g_new0 (HTMLFontFace, 1);

	g_assert (family);

	face->family = g_strdup (family);
	face->size   = size;

	return face;
}

void
html_font_face_destroy (HTMLFontFace *face)
{
	if (face->family)
		g_free (face->family);
	g_free (face);
}

GdkFont *
html_font_face_get_font (HTMLFontFace *face, GtkHTMLFontStyle style)
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

#if 0
void
html_font_face_set_family (HTMLFontFace *face, const gchar *family)
{
	g_assert (family);

	if (strcmp (family, face->family)) {
		release_fonts (face);
		g_free (face->family);
		face->family = g_strdup (family);
	}
}

void
html_font_face_set_size (HTMLFontFace *face, gint size)
{
	if (size != face->size) {
		release_fonts (face);
		face->size = size;
	}
}
#endif

#if 0
gboolean
html_font_face_family_exists (const gchar *family)
{
	gchar *font_name, **list;
	gint n;

	font_name = g_strdup_printf ("-*-%s-*-*-normal-*-*-*-*-*-*-*-*-*", family);
	list = XListFonts (GDK_DISPLAY (), font_name, 1, &n);
	g_free (font_name);

	return n > 0;
}
#endif

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
	smaller = bigger = 0;
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
get_closest_font (HTMLFontFace *face, const gchar *weight,
		  const gchar *slant, guint html_size)
{
	GdkFont *font;
	guint font_size;

	font_size = face->size + (((gdouble) face->size / 5.0) * (((gint) html_size) - 3));
	font = get_font (face->family, weight, slant, font_size);

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
release_fonts (HTMLFontFace *face)
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
#endif
