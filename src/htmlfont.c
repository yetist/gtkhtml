/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
             (C) 1997 Torben Weis (weis@kde.org)
             (C) 1999 Anders Carlson (andersca@gnu.org)
	     (C) 1999, 2000 Helix Code, Inc.

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

#include "htmlfont.h"


static GdkFont *
create_gdk_font (const gchar *family, gint size, gboolean bold, gboolean italic)
{
	gchar *boldstr;
	gchar *italicstr;
	gchar *fontname;
	gint realsize;
	GdkFont *font;

	realsize = size;

	if (bold)
		boldstr = "bold";
	else
		boldstr = "medium";

	if (italic)
		italicstr = "i";
	else
		italicstr = "r";
	
	fontname = g_strdup_printf ("-*-%s-%s-%s-normal-*-%d-*-*-*-*-*-*-*",
				    family, boldstr, italicstr, realsize);
	
	font = gdk_font_load (fontname);
	g_free (fontname);
	if (font){
		return font;
	} else {
		g_warning ("font not found, using helvetica");
		fontname = g_strdup_printf ("-*-helvetica-medium-r-normal-*-*-%d-*-*-*-*-*-*",
					    realsize);
		font = gdk_font_load (fontname);
		g_free (fontname);
	}
	return font;
}


HTMLFont *
html_font_new (const gchar *family,
	       gint size,
	       gint *fontSizes,
	       gboolean bold,
	       gboolean italic,
	       gboolean underline)
{
	HTMLFont *f;

	g_return_val_if_fail (family != NULL, NULL);

	f = g_new0 (HTMLFont, 1);
	f->family = g_strdup (family);
	f->size = size;
	f->bold = bold;
	f->italic = italic;
	f->underline = underline;
	f->pointSize = fontSizes [size];

	f->gdk_font = create_gdk_font (family, fontSizes[size], bold, italic);

	f->textColor = NULL;

	return f;
}


HTMLFont *
html_font_dup (HTMLFont *f)
{
	HTMLFont *new;

	if (f == NULL)
		return NULL;

	new = g_new (HTMLFont, 1);

	new->family = g_strdup (f->family);
	new->size = f->size;
	new->pointSize = f->pointSize;
	new->bold = f->bold;
	new->italic = f->italic;
	new->underline = f->underline;
	new->textColor = gdk_color_copy (f->textColor);
	
	new->gdk_font = gdk_font_ref (f->gdk_font);

	return new;
}

void
html_font_destroy (HTMLFont *html_font)
{
	g_return_if_fail (html_font != NULL);

	if (html_font->gdk_font != NULL)
		gdk_font_unref (html_font->gdk_font);
	g_free (html_font->family);

	if (html_font->textColor)
		gdk_color_free (html_font->textColor);

	g_free (html_font);
}

void
html_font_set_color (HTMLFont *html_font,
		     const GdkColor *color)
{
	if (html_font->textColor != NULL)
		gdk_color_free (html_font->textColor);

	/* Evil but safe cast: the prototype for `GdkColor' is wrong.  */
	html_font->textColor = gdk_color_copy ((GdkColor *) color);
}


gint
html_font_calc_ascent (HTMLFont *f)
{
	g_return_val_if_fail (f != NULL, 0);
	
	return f->gdk_font->ascent;
}

gint
html_font_calc_descent (HTMLFont *f)
{
	g_return_val_if_fail (f != NULL, 0);
	
	return f->gdk_font->descent;
}

gint
html_font_calc_width (HTMLFont *f, const gchar *text, gint len)
{
	gint width;

	g_return_val_if_fail (f != NULL, 0);
	g_return_val_if_fail (text != NULL, 0);
	
	if (len == -1)
		width = gdk_string_width (f->gdk_font, text);
	else
		width = gdk_text_width (f->gdk_font, text, len);

	return width;
}
