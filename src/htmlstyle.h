/* "a -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2002, Ximian Inc.

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

#ifndef __HTML_STYLE_H__
#define __HTML_STYLE_H__
#include "gtkhtml-enums.h"
#include "htmlenums.h"
#include "htmltypes.h"
#include "htmlcolor.h"

struct _HTMLLength {
	gint           val;
	HTMLLengthType type;
};

struct _HTMLStyle {
	HTMLColor          *color;
	HTMLFontFace       *face;
	GtkHTMLFontStyle    settings;
	GtkHTMLFontStyle    mask;		

	/* Block Level */
	HTMLHAlignType      text_align;
	HTMLClearType       clear;
	
	/* Cell Level */
	HTMLVAlignType      text_valign;
	
	/* box settings */
	HTMLLength     *width;
	HTMLLength     *height;

	char           *bg_image;
	HTMLColor      *bg_color;
	HTMLDisplayType display;

	/* border */
	int border_width;
	HTMLBorderStyle border_style;
	HTMLColor *border_color;
};	

HTMLStyle *html_style_new                  (void);
HTMLStyle *html_style_unset_decoration     (HTMLStyle *style, GtkHTMLFontStyle decoration);
HTMLStyle *html_style_set_decoration       (HTMLStyle *style, GtkHTMLFontStyle decoration);
HTMLStyle *html_style_set_font_size        (HTMLStyle *style, GtkHTMLFontStyle decoration);
HTMLStyle *html_style_set_size             (HTMLStyle *style, GtkHTMLFontStyle size);
HTMLStyle *html_style_set_display          (HTMLStyle *style, HTMLDisplayType display);
HTMLStyle *html_style_set_clear            (HTMLStyle *style, HTMLClearType clear);
HTMLStyle *html_style_set_border_style     (HTMLStyle *style, HTMLBorderStyle bstyle);
HTMLStyle *html_style_set_border_width     (HTMLStyle *style, int width);
HTMLStyle *html_style_set_border_color     (HTMLStyle *style, HTMLColor *color);
HTMLStyle *html_style_add_text_align       (HTMLStyle *style, HTMLHAlignType type);
HTMLStyle *html_style_add_text_valign      (HTMLStyle *style, HTMLVAlignType type);
HTMLStyle *html_style_add_font_face        (HTMLStyle *style, const HTMLFontFace *face);
HTMLStyle *html_style_add_color            (HTMLStyle *style, HTMLColor *face);
HTMLStyle *html_style_add_attribute        (HTMLStyle *style, const char *attr);
HTMLStyle *html_style_add_background_image (HTMLStyle *style, const char *url);
HTMLStyle *html_style_add_background_color (HTMLStyle *style, HTMLColor *color);
HTMLStyle *html_style_add_width            (HTMLStyle *style, char *width);
HTMLStyle *html_style_add_height           (HTMLStyle *style, char *height);
void       html_style_free                 (HTMLStyle *style);

gboolean   html_parse_color                (const gchar *text, GdkColor *color);

#endif /* __HTML_COLOR_H__ */
