/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
	      (C) 1999 Anders Carlson (andersca@gnu.org)

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

/* FIXME: Fix a better one */
gint defaultFontSizes [7] = {8, 10, 12, 14, 18, 24, 24};

static GdkFont *create_gdk_font (gchar *family, gint size, gboolean bold, gboolean italic);

HTMLFont *
html_font_new (gchar *family, gint size, gint *fontSizes, gboolean bold, gboolean italic, gboolean underline)
{
	gchar *xlfd;
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

	return f;
}

static GdkFont *
create_gdk_font (gchar *family, gint size, gboolean bold, gboolean italic)
{
	gboolean loaded = FALSE;
	gchar *boldstr;
	gchar *italicstr;
	gchar *fontname;
	gint realsize;
	GdkFont *font;

	/* FIXME: a better way to find out the size */
	realsize = size * 10;

	if (bold)
		boldstr = "bold";
	else
		boldstr = "medium";
	if (italic)
		italicstr = "i";
	else
		italicstr = "r";
	
	
	fontname = g_strdup_printf ("-*-%s-%s-%s-normal-*-*-%d-*-*-*-*-*-*",
				    family, boldstr, italicstr, realsize);
	g_print ("trying: %s\n", fontname);
	font = gdk_font_load (fontname);
	if (font)
		return font;
	else {
		g_free (fontname);
		g_warning ("font not found, using helvetica");
		fontname = g_strdup_printf ("-*-helvetica-medium-r-normal-*-*-%d-*-*-*-*-*-*",
					    realsize);
		font = gdk_font_load (fontname);
	}
	return font;
}

void
html_font_destroy (HTMLFont *html_font)
{
	g_return_if_fail (html_font != NULL);

	gdk_font_unref (html_font->gdk_font);
	g_free (html_font->family);
	g_free (html_font);
}

HTMLFontStack *
html_font_stack_new (void)
{
	HTMLFontStack *fs;
	
	fs = g_new0 (HTMLFontStack, 1);

	return fs;
}

void
html_font_stack_destroy (HTMLFontStack *stack)
{
	g_return_if_fail (stack != NULL);
	
	html_font_stack_clear (stack);
	g_free (stack);
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
html_font_calc_width (HTMLFont *f, gchar *text, gint len)
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

void
html_font_stack_push (HTMLFontStack *fs, HTMLFont *f)
{
	g_return_if_fail (fs != NULL);
	g_return_if_fail (f != NULL);
	
	fs->list = g_list_prepend (fs->list, (gpointer) f);
}

void
html_font_stack_clear (HTMLFontStack *fs)
{
	GList *stack;

	g_return_if_fail (fs != NULL);
	
	for (stack = fs->list; stack; stack = stack->next){
		HTMLFont *html_font = stack->data;

		html_font_destroy (html_font);
	}
	g_list_free (fs->list);
	fs->list = NULL;
}

HTMLFont *
html_font_stack_top (HTMLFontStack *fs)
{
	HTMLFont *f;

	g_return_val_if_fail (fs != NULL, NULL);
	
	f = (HTMLFont *)(g_list_first (fs->list))->data;

	return f;
}

HTMLFont *
html_font_stack_pop (HTMLFontStack *fs)
{
	HTMLFont *f;

	g_return_val_if_fail (fs != NULL, NULL);
	
	f = html_font_stack_top (fs);

	fs->list = g_list_remove (fs->list, f);

	return f;
}
