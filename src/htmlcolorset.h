/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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

#ifndef _HTMLCOLORSET_H_
#define _HTMLCOLORSET_H_

typedef enum   _HTMLColor HTMLColor;
typedef struct _HTMLColorSet HTMLColorSet;

#include <gdk/gdk.h>
#include "htmlpainter.h"

enum _HTMLColor
{
	HTMLBgColor = 0,
	HTMLTextColor,
	HTMLLinkColor,
	HTMLVLinkColor,
	HTMLALinkColor,
	HTMLHighlightColor,
	HTMLHighlightTextColor,

	HTMLColors,
};

struct _HTMLColorSet
{
	GdkColor color [HTMLColors];
	gboolean color_allocated [HTMLColors];
	GSList  *colors_to_free;

	/* slave sets - they must be updated when setting this one
	   engine has master set and painters have slave ones
	 */
	GSList  *slaves;
};



/* ctor + dtor */
HTMLColorSet     *html_colorset_new                   (GtkWidget *w);
void              html_colorset_destroy               (HTMLColorSet *set);

/* slaves handling */
void              html_colorset_add_slave             (HTMLColorSet *set,
						       HTMLColorSet *slave);
/* colors set/get */
void              html_colorset_set_color             (HTMLColorSet *set,
						       GdkColor *color,
						       HTMLColor idx);
GdkColor         *html_colorset_get_color             (HTMLColorSet *set,
						       HTMLColor idx);
GdkColor         *html_colorset_get_color_allocated   (HTMLPainter *painter,
						       HTMLColor idx);

/* frees allocated colors */
void              html_colorset_free_colors           (HTMLColorSet *set,
						       HTMLPainter *painter,
						       gboolean all);

/* copy colors from one se to another, used for resetting to default values */
void              html_colorset_set_by                (HTMLColorSet *s, HTMLColorSet *o);

#endif
