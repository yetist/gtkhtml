/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#ifndef _HTMLFONT_H_
#define _HTMLFONT_H_

#include <gdk/gdk.h>

#define MAXFONTSIZES 7

typedef struct _HTMLFont HTMLFont;
typedef struct _HTMLFontStack HTMLFontStack;

struct _HTMLFont {
	gchar *family;
	gint size;
	gint pointSize;
	gboolean bold;
	gboolean italic;
	gboolean underline;

	GdkColor *textColor;
	GdkFont *gdk_font;
  
};

struct _HTMLFontStack {
	GList *list;
};

HTMLFont      *html_font_new           (gchar *family, gint size,
				       	gint *fontSizes, gboolean bold,
				       	gboolean italic, gboolean underline);
void           html_font_destroy       (HTMLFont *html_font);
gint  	       html_font_calc_width    (HTMLFont *f, gchar *text, gint len);
gint  	       html_font_calc_descent  (HTMLFont *f);
gint  	       html_font_calc_ascent   (HTMLFont *f);

HTMLFontStack *html_font_stack_new     (void);
void           html_font_stack_destroy (HTMLFontStack *stack);
void           html_font_stack_push    (HTMLFontStack *fs, HTMLFont *f);
void           html_font_stack_clear   (HTMLFontStack *fs);
HTMLFont      *html_font_stack_pop     (HTMLFontStack *fs);
HTMLFont      *html_font_stack_top     (HTMLFontStack *fs);

#endif /* _HTMLFONT_H_ */
