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

#ifndef _HTMLFONTSTYLE_H
#define _HTMLFONTSTYLE_H

enum _GtkHTMLFontStyle {
	GTK_HTML_FONT_STYLE_DEFAULT = 0,
	GTK_HTML_FONT_STYLE_SIZE_1 = 1,
	GTK_HTML_FONT_STYLE_SIZE_2 = 2,
	GTK_HTML_FONT_STYLE_SIZE_3 = 3,
	GTK_HTML_FONT_STYLE_SIZE_4 = 4,
	GTK_HTML_FONT_STYLE_SIZE_5 = 5,
	GTK_HTML_FONT_STYLE_SIZE_6 = 6,
	GTK_HTML_FONT_STYLE_SIZE_7 = 7,
	GTK_HTML_FONT_STYLE_SIZE_MASK = 0x7,
	GTK_HTML_FONT_STYLE_BOLD = 1 << 3,
	GTK_HTML_FONT_STYLE_ITALIC = 1 << 4,
	GTK_HTML_FONT_STYLE_UNDERLINE = 1 << 5,
	GTK_HTML_FONT_STYLE_STRIKEOUT = 1 << 6,
	GTK_HTML_FONT_STYLE_FIXED = 1 << 7,
};
typedef enum _GtkHTMLFontStyle GtkHTMLFontStyle;

#define GTK_HTML_FONT_STYLE_SIZE_MAX 7
#define GTK_HTML_FONT_STYLE_MAX									\
	(GTK_HTML_FONT_STYLE_SIZE_MASK | GTK_HTML_FONT_STYLE_BOLD | GTK_HTML_FONT_STYLE_ITALIC	\
	 | GTK_HTML_FONT_STYLE_UNDERLINE | GTK_HTML_FONT_STYLE_STRIKEOUT | GTK_HTML_FONT_STYLE_FIXED)
#define GTK_HTML_FONT_STYLE_MAX_FONT      ((GTK_HTML_FONT_STYLE_ITALIC) << 1)
#define GTK_HTML_FONT_STYLE_MAX_FONT_MASK (GTK_HTML_FONT_STYLE_MAX_FONT - 1)


GtkHTMLFontStyle gtk_html_font_style_merge (GtkHTMLFontStyle a, GtkHTMLFontStyle b);

#endif /* _HTMLFONTSTYLE_H */
