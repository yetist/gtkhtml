/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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

HTMLFont *
html_font_new (gchar *family, gint size, gint *fontSizes, HTMLFontWeight weight, gboolean italic, gboolean underline)
{
	gchar *xlfd;
	HTMLFont *f;
	
	f = g_new0 (HTMLFont, 1);
	f->family = g_strdup (family);
	f->size = size;
	f->weight = weight;
	f->italic = italic;
	f->underline = underline;
	f->pointSize = fontSizes [size];

	xlfd = g_strdup_printf ("-*-%s-%s-r-normal-*-*-%d-*-*-p-*-iso8859-1", family, f->weight == Bold ? "bold" : "medium",
				fontSizes[size] * 10);

	/*	g_print ("xlfd is: %s\n", xlfd);*/

	if (f->gdk_font)
		gdk_font_unref (f->gdk_font);

	f->gdk_font = gdk_font_load (xlfd);

	return f;
}

HTMLFontStack *
html_font_stack_new (void)
{
	HTMLFontStack *fs;
	
	fs = g_new0 (HTMLFontStack, 1);

	return fs;
}

gint
html_font_calc_ascent (HTMLFont *f)
{
	return f->gdk_font->ascent;
}

gint
html_font_calc_descent (HTMLFont *f)
{
	return f->gdk_font->descent;
}

gint
html_font_calc_width (HTMLFont *f, gchar *text, gint len)
{
	gint width;

	if (len == -1)
		width = gdk_string_width (f->gdk_font, text);
	else
		width = gdk_text_width (f->gdk_font, text, len);

	return width;
}

void
html_font_stack_push (HTMLFontStack *fs, HTMLFont *f)
{
	fs->list = g_list_prepend (fs->list, (gpointer) f);
}

void
html_font_stack_clear (HTMLFontStack *fs)
{

	/* FIXME: Should destroy the fonts*/
}

HTMLFont *
html_font_stack_top (HTMLFontStack *fs)
{
	HTMLFont *f;

	f = (HTMLFont *)(g_list_first (fs->list))->data;

	return f;
}

HTMLFont *
html_font_stack_pop (HTMLFontStack *fs)
{
	HTMLFont *f;

	f = html_font_stack_top (fs);

	fs->list = g_list_remove (fs->list, f);

	return f;
}
