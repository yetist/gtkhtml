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

#ifndef _HTML_FONT_FACE_H_
#define _HTML_FONT_FACE_H_

#include <gdk/gdk.h>
#include "gtkhtmlfontstyle.h"

struct _HTMLFontFace {
	GdkFont *font [GTK_HTML_FONT_STYLE_MAX];

	gchar *family;
	guint  size;
};
typedef struct _HTMLFontFace HTMLFontFace;

HTMLFontFace *      html_font_face_new                        (const gchar *family,
							       gint size);
void                html_font_face_destroy                    (HTMLFontFace *face);
GdkFont *           html_font_face_get_font                   (HTMLFontFace *face,
							       GtkHTMLFontStyle style);
void                html_font_face_set_family                 (HTMLFontFace *face,
							       const gchar *family);
void                html_font_face_set_size                   (HTMLFontFace *face,
							       gint size);
gboolean            html_font_face_family_exists              (const gchar *family);

#endif
