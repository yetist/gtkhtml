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

#include "htmlenginecolorset.h"


HTMLEngineColorSet *
html_engine_color_set_new (void)
{
	HTMLEngineColorSet *new;

	new = g_new (HTMLEngineColorSet, 1);

	new->background_color.red = 0xff;
	new->background_color.green = 0xff;
	new->background_color.blue = 0xff;

	new->foreground_color.red = 0x00;
	new->foreground_color.green = 0x00;
	new->foreground_color.blue = 0x00;

	new->link_color.red = 0x00;
	new->link_color.green = 0x00;
	new->link_color.blue = 0xff;

	new->highlight_color.red = 0xff;
	new->highlight_color.green = 0xff;
	new->highlight_color.blue = 0xff;

	new->highlight_foreground_color.red = 0xff;
	new->highlight_foreground_color.green = 0xff;
	new->highlight_foreground_color.blue = 0xff;

	new->realized = FALSE;

	return new;
}

void
html_engine_color_set_destroy (HTMLEngineColorSet *set)
{
	g_return_if_fail (set != NULL);

	html_engine_color_set_unrealize (set);
}

void
html_engine_color_set_realize (HTMLEngineColorSet *set,
			       GdkWindow *window)
{
	GdkColormap *colormap;

	g_return_if_fail (set != NULL);
	g_return_if_fail (window != NULL);
	g_return_if_fail (! set->realized);

	colormap = gdk_window_get_colormap (window);

	gdk_colormap_alloc_color (colormap, &set->background_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, &set->foreground_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, &set->link_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, &set->highlight_color, TRUE, TRUE);
	gdk_colormap_alloc_color (colormap, &set->highlight_foreground_color, TRUE, TRUE);

	set->realized = TRUE;
}

void
html_engine_color_set_unrealize (HTMLEngineColorSet *set)
{
	g_return_if_fail (set != NULL);
	g_return_if_fail (set->realized);

	/* FIXME: To really work as expected to, this should actually
           deallocate colors.  */
	set->realized = FALSE;
}
