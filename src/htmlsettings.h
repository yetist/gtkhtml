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

#ifndef _HTMLSETTINGS_H_
#define _HTMLSETTINGS_H_

#include <gdk/gdk.h>

typedef struct _HTMLSettings HTMLSettings;

#include "htmlengine.h"


#define HTML_NUM_FONT_SIZES 6

struct _HTMLSettings {
	gint fontSizes[HTML_NUM_FONT_SIZES];
	gint fontBaseSize;
	GdkColor fontBaseColor;
	gchar *fontBaseFace;
	gchar *fixedFontFace;

	GdkColor linkColor;
	GdkColor vLinkColor;
	GdkColor bgColor;

	gboolean underlineLinks : 1;
	gboolean forceDefault : 1;
};


HTMLSettings *html_settings_new (void);
void html_settings_destroy (HTMLSettings *settings);
void html_settings_set_bgcolor (HTMLSettings *settings, GdkColor *color);
void html_settings_set_font_sizes (HTMLSettings *settings, const gint *newFontSizes);
void html_settings_get_font_sizes (HTMLSettings *settings, gint *fontSizes);
void html_settings_reset_font_sizes (HTMLSettings *settings);
void html_settings_alloc_colors (HTMLSettings *settings, GdkColormap *colormap);
void html_settings_copy (HTMLSettings *dest, HTMLSettings *src);
void html_settings_set_font_base_face (HTMLSettings *settings, const gchar *face);
void html_settings_set_fixed_font_face (HTMLSettings *settings, const gchar *face);

#endif /* _HTMLSETTINGS_H_ */
