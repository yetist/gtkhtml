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

#ifndef _HTMLPRINTFONTMANAGER_H_
#define _HTMLPRINTFONTMANAGER_H_

#include <libgnomeprint/gnome-print.h>

#include "htmlfontstyle.h"

struct _HTMLPrintFontManager {
	GnomeFont *fonts[HTML_FONT_STYLE_MAX];
};
typedef struct _HTMLPrintFontManager HTMLPrintFontManager;


HTMLPrintFontManager *html_print_font_manager_new      (void);
void                  html_print_font_manager_destroy  (HTMLPrintFontManager *manager);

GnomeFont *html_print_font_manager_get_font  (HTMLPrintFontManager *manager,
					      HTMLFontStyle         style);

#endif /* _HTMLPRINTFONTMANAGER_H_ */
