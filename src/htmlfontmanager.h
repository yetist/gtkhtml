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

#ifndef _HTMLFONTMANAGER_H_
#define _HTMLFONTMANAGER_H_

#include <gdk/gdk.h>

typedef struct _HTMLFont HTMLFont;

enum _HTMLFontStyle {
	HTML_FONT_STYLE_DEFAULT = 0,
	HTML_FONT_STYLE_SIZE_1 = 1,
	HTML_FONT_STYLE_SIZE_2 = 2,
	HTML_FONT_STYLE_SIZE_3 = 3,
	HTML_FONT_STYLE_SIZE_4 = 4,
	HTML_FONT_STYLE_SIZE_5 = 5,
	HTML_FONT_STYLE_SIZE_6 = 6,
	HTML_FONT_STYLE_SIZE_7 = 7,
	HTML_FONT_STYLE_SIZE_MASK = 0x7,
	HTML_FONT_STYLE_BOLD = 1 << 3,
	HTML_FONT_STYLE_ITALIC = 1 << 4,
	HTML_FONT_STYLE_UNDERLINE = 1 << 5,
	HTML_FONT_STYLE_STRIKEOUT = 1 << 6,
	HTML_FONT_STYLE_FIXED = 1 << 7,
	HTML_FONT_STYLE_MAX = 0xff
};
typedef enum _HTMLFontStyle HTMLFontStyle;

#define HTML_FONT_STYLE_SIZE_MAX 7

struct _HTMLFontManager {
	GdkFont *fonts[HTML_FONT_STYLE_MAX];
};
typedef struct _HTMLFontManager HTMLFontManager;


HTMLFontManager *html_font_manager_new  (void);
void html_font_manager_destroy (HTMLFontManager *manager);

GdkFont *html_font_manager_get_gdk_font (HTMLFontManager *manager, HTMLFontStyle style);

HTMLFontStyle html_font_style_merge (HTMLFontStyle a, HTMLFontStyle b);

#endif /* _HTMLFONTMANAGER_H_ */
