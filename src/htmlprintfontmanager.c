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

#include <libgnomeprint/gnome-print.h>

#include "htmlprintfontmanager.h"


static void
load_font (HTMLPrintFontManager *manager,
	   GtkHTMLFontStyle style)
{
	const gchar *family_string;
	GnomeFontWeight weight;
	gboolean italic;
	gint size;
	gint real_size;
	GnomeFont *font;

	if (manager->fonts[style] != NULL)
		return;

	size = style & GTK_HTML_FONT_STYLE_SIZE_MASK;
	if (size == 0)
		size = 3;

	real_size = 10 + 2 * (size - 3);

	if (style & GTK_HTML_FONT_STYLE_BOLD)
		weight = GNOME_FONT_BOLD;
	else
		weight = GNOME_FONT_BOOK;

	if (style & GTK_HTML_FONT_STYLE_ITALIC)
		italic = TRUE;
	else
		italic = FALSE;

	if (style & GTK_HTML_FONT_STYLE_FIXED)
		family_string = "Courier";
	else
		family_string = "Helvetica";

	font = gnome_font_new_closest (family_string, weight, italic, real_size);

	if (font == NULL){
		g_warning ("Font `%s' not found -- this should not happen", family_string);
		font = gnome_font_new ("Helvetica", 12);
	}

	manager->fonts[style] = font;
}


HTMLPrintFontManager *
html_print_font_manager_new (void)
{
	HTMLPrintFontManager *new;

	new = g_new (HTMLPrintFontManager, 1);
	memset (new->fonts, 0, sizeof (new->fonts));

	return new;
}

void
html_print_font_manager_destroy  (HTMLPrintFontManager *manager)
{
	guint i;

	g_return_if_fail (manager != NULL);

	for (i = 0; i < GTK_HTML_FONT_STYLE_MAX; i++) {
		if (manager->fonts[i] != NULL)
			gtk_object_unref (GTK_OBJECT (manager->fonts[i]));
	}

	g_free (manager);
}


GnomeFont *
html_print_font_manager_get_font  (HTMLPrintFontManager *manager,
				   GtkHTMLFontStyle style)
{
	g_return_val_if_fail (manager != NULL, NULL);
	g_return_val_if_fail (style < GTK_HTML_FONT_STYLE_MAX, NULL);

	load_font (manager, style);

	return manager->fonts[style];
}
