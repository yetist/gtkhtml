/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

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

#ifndef HTMLENGINE_EDIT_FONTSTYLE_H
#define HTMLENGINE_EDIT_FONTSTYLE_H

#include "htmlengine.h"
#include "gtkhtmlfontstyle.h"

/* font style */
gboolean           html_engine_update_insertion_font_style  (HTMLEngine       *engine);
GtkHTMLFontStyle   html_engine_get_font_style               (HTMLEngine       *engine);
GtkHTMLFontStyle   html_engine_get_document_font_style      (HTMLEngine       *engine);
void               html_engine_set_font_style               (HTMLEngine       *engine,
							     GtkHTMLFontStyle  and_mask,
							     GtkHTMLFontStyle  or_mask);
void               html_engine_font_style_toggle            (HTMLEngine *engine,
							     GtkHTMLFontStyle style);

/* colors */
void               html_engine_set_color                    (HTMLEngine *engine,
							     GdkColor *color);
#endif
