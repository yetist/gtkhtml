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

struct _HTMLGdkFontManager {
	GdkFont *font [GTK_HTML_FONT_STYLE_MAX];
	guint    size [GTK_HTML_FONT_STYLE_SIZE_MAX];

	gchar *family_var;
	gchar *family_fix;
	guint  size_var;
	guint  size_fix;
};
typedef struct _HTMLGdkFontManager HTMLGdkFontManager;


HTMLGdkFontManager *html_gdk_font_manager_new                        (void);
void                html_gdk_font_manager_destroy                    (HTMLGdkFontManager *manager);

GdkFont *           html_gdk_font_manager_get_font                   (HTMLGdkFontManager *manager,
								      GtkHTMLFontStyle style);
void                html_gdk_font_manager_set_family_var             (HTMLGdkFontManager *manager,
								      const gchar *family);
void                html_gdk_font_manager_set_family_fix             (HTMLGdkFontManager *manager,
								      const gchar *family_fixed);
void                html_gdk_font_manager_set_size_var               (HTMLGdkFontManager *manager,
								      gint size);
void                html_gdk_font_manager_set_size_fix               (HTMLGdkFontManager *manager,
								      gint size);
#endif /* _HTMLGDKFONTMANAGER_H_ */
