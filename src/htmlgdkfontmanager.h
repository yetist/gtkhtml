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

#ifndef _HTMLGDKFONTMANAGER_H_
#define _HTMLGDKFONTMANAGER_H_

#include <gdk/gdk.h>

#include "gtkhtmlfontstyle.h"
#include "htmlfontface.h"

struct _HTMLGdkFontFace {
	HTMLFontFace face;
	GdkFont *font [GTK_HTML_FONT_STYLE_MAX];
};
typedef struct _HTMLGdkFontFace HTMLGdkFontFace;

struct _HTMLGdkFontManager {
	HTMLGdkFontFace *variable;
	HTMLGdkFontFace *fixed;

	GHashTable *faces;
};
typedef struct _HTMLGdkFontManager HTMLGdkFontManager;


HTMLGdkFontManager *html_gdk_font_manager_new                        (void);
void                html_gdk_font_manager_destroy                    (HTMLGdkFontManager *manager);

HTMLGdkFontFace *   html_gdk_font_manager_find_face                  (HTMLGdkFontManager *manager,
								      const gchar *families);

HTMLGdkFontFace *   html_gdk_font_manager_get_variable               (HTMLGdkFontManager *manager);
HTMLGdkFontFace *   html_gdk_font_manager_get_fixed                  (HTMLGdkFontManager *manager);
GdkFont *           html_gdk_font_manager_get_font                   (HTMLGdkFontManager *manager,
								      GtkHTMLFontStyle style,
								      HTMLGdkFontFace *face);
void                html_gdk_font_manager_set_variable               (HTMLGdkFontManager *manager,
								      const gchar *family,
								      gint size);
void                html_gdk_font_manager_set_fixed                  (HTMLGdkFontManager *manager,
								      const gchar *family,
								      gint size);
void                html_gdk_font_manager_set_size                   (HTMLGdkFontManager *manager,
								      gint size);

/* UTF-8 Wrappers */

gint html_gdk_text_width (HTMLGdkFontManager *manager,
			  HTMLGdkFontFace *face,
			  GtkHTMLFontStyle style,
			  gchar *text, gint bytes);

void html_gdk_draw_text (HTMLGdkFontManager *manager,
			 HTMLGdkFontFace *face,
			 GtkHTMLFontStyle style,
			 GdkDrawable *drawable,
			 GdkGC *gc,
			 gint x, gint y,
			 gchar *text, gint bytes);

#endif /* _HTMLGDKFONTMANAGER_H_ */
