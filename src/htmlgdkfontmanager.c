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


/* FIXME this should be dynamically done, and based on the base font name.  */


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
test_font (gchar *font_name, gint *req_size)
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
get_font (const gchar *family_string, const gchar *weight_string, const gchar *slant_string, guint fsize)
{
	GdkFont *font;
	gchar *font_name;

	font      = NULL;
	font_name = g_strdup_printf ("-*-%s-%s-%s-normal-*-*-*-*-*-*-*-*-*",
				     family_string, weight_string, slant_string);
	if (test_font (font_name, &fsize)) {
		g_free (font_name);
		font_name = g_strdup_printf ("-*-%s-%s-%s-normal-*-%d-*-*-*-*-*-*-*",
					     family_string, weight_string, slant_string, fsize);
		font = gdk_font_load (font_name);
	}
	g_free (font_name);

	return font;
}

static GdkFont *
get_closest_font (const gchar *family_string, const gchar *weight_string,
		  const gchar *slant_string, guint html_size, guint size)
{
	GdkFont *font;
	guint font_size;

	font_size = size + (((gdouble) size / 5.0) * (((gint) html_size) - 3));
	font = get_font (family_string, weight_string, slant_string, font_size);

	/* try use obligue instead of italic */
	if (!font && *slant_string == 'i')
		font = get_closest_font (family_string, weight_string, "o", html_size, size);

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
load_font (HTMLGdkFontManager *manager,
	   GtkHTMLFontStyle style)
{
	const gchar *weight_string;
	const gchar *slant_string;
	const gchar *family_string;
	gint html_size, size;

	if (manager->font [style] != NULL)
		return;

	html_size     = (style & GTK_HTML_FONT_STYLE_SIZE_MASK) ? style & GTK_HTML_FONT_STYLE_SIZE_MASK : 3;
	size          = (style & GTK_HTML_FONT_STYLE_FIXED)  ? manager->size_fix : manager->size_var;
	weight_string = (style & GTK_HTML_FONT_STYLE_BOLD)   ? "bold" : "medium";
	slant_string  = (style & GTK_HTML_FONT_STYLE_ITALIC) ? "i" : "r";
	family_string = (style & GTK_HTML_FONT_STYLE_FIXED)  ? manager->family_fix : manager->family_var;

	manager->font[style] = get_closest_font (family_string, weight_string, slant_string, html_size, size);
}

static void
release_fonts (HTMLGdkFontManager *manager)
{
	guint i;

	g_assert (manager != NULL);

	for (i = 0; i < GTK_HTML_FONT_STYLE_MAX; i++) {
		if (manager->font [i] != NULL) {
			gdk_font_unref (manager->font [i]);
			manager->font [i] = NULL;
		}
	}
}


HTMLGdkFontManager *
html_gdk_font_manager_new  (void)
{
	HTMLGdkFontManager *new;

	new = g_new (HTMLGdkFontManager, 1);
	memset (new->font, 0, sizeof (new->font));

	new->family_var = g_strdup ("lucida");
	new->family_fix = g_strdup ("courier");
	new->size_var   = 12;
	new->size_fix   = 12;

	return new;
}

void
html_gdk_font_manager_destroy (HTMLGdkFontManager *manager)
{
	release_fonts (manager);

	g_free (manager->family_var);
	g_free (manager->family_fix);
	g_free (manager);
}

GdkFont *
html_gdk_font_manager_get_font (HTMLGdkFontManager *manager,
				GtkHTMLFontStyle style)
{
	g_return_val_if_fail (manager != NULL, NULL);
	g_return_val_if_fail (style < GTK_HTML_FONT_STYLE_MAX, NULL);

	load_font (manager, style);

	return gdk_font_ref (manager->font [style]);
}

void
html_gdk_font_manager_set_family_var (HTMLGdkFontManager *manager, const gchar *family)
{
	if (strcmp (family, manager->family_var)) {
		release_fonts (manager);
		g_free (manager->family_var);
		manager->family_var = g_strdup (family);
	}
}

void
html_gdk_font_manager_set_family_fix (HTMLGdkFontManager *manager, const gchar *family)
{
	if (strcmp (family, manager->family_fix)) {
		release_fonts (manager);
		g_free (manager->family_fix);
		manager->family_fix = g_strdup (family);
	}
}

void
html_gdk_font_manager_set_size_var (HTMLGdkFontManager *manager, gint size)
{
	if (size != manager->size_var) {
		release_fonts (manager);
		manager->size_var = size;
	}
}

void
html_gdk_font_manager_set_size_fix (HTMLGdkFontManager *manager, gint size)
{
	if (size != manager->size_fix) {
		release_fonts (manager);
		manager->size_fix = size;
	}
}
