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

#ifndef _HTML_FONT_MANAGER_H_
#define _HTML_FONT_MANAGER_H_

#define HTML_FONT_MANAGER(x) ((HTMLFontManager *)x)

#include <glib.h>

typedef struct     _HTMLFontManager             HTMLFontManager;
typedef gpointer (* HTMLFontManagerAllocFont)   (gchar *face_name, gdouble size, GtkHTMLFontStyle style);
typedef void     (* HTMLFontManagerRefFont)     (gpointer font);
typedef void     (* HTMLFontManagerUnrefFont)   (gpointer font);
typedef gchar       HTMLFontFace;
typedef struct     _HTMLFontSet                 HTMLFontSet;

#include "gtkhtmlfontstyle.h"

struct _HTMLFontSet {
	gpointer font [GTK_HTML_FONT_STYLE_MAX_FONT];
	gint     ref_count;
	gchar   *face;
};

struct _HTMLFontManager {
	HTMLFontManagerAllocFont alloc_font;
	HTMLFontManagerRefFont     ref_font;
	HTMLFontManagerUnrefFont unref_font;

	GHashTable *font_sets;
	HTMLFontSet variable;
	HTMLFontSet fixed;

	gint var_size;
	gint fix_size;
};

void                html_font_manager_init                    (HTMLFontManager *manager,
							       HTMLFontManagerAllocFont alloc_font,
							       HTMLFontManagerRefFont     ref_font,
							       HTMLFontManagerUnrefFont unref_font);

void                html_font_manager_finalize                (HTMLFontManager *manager);

void                html_font_manager_set_default             (HTMLFontManager *manager,
							       gchar *variable,
							       gchar *fixed,
							       gint var_size,
							       gint fix_size);
gpointer            html_font_manager_get_font                (HTMLFontManager *manager,
							       gchar *face,
							       GtkHTMLFontStyle style);

#endif
