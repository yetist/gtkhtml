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

#include "htmlcolorset.h"



#define SET_COLOR(w,r,g,b) \
        s->color [HTML ## w ## Color].red   = r; \
        s->color [HTML ## w ## Color].green = g; \
        s->color [HTML ## w ## Color].blue  = b

#define SET_GCOLOR(t,c) \
        s->color [HTML ## t ## Color].red   = c.red; \
        s->color [HTML ## t ## Color].green = c.green; \
        s->color [HTML ## t ## Color].blue  = c.blue

HTMLColorSet *
html_colorset_new (GtkWidget *w)
{
	HTMLColorSet *s;

	s = g_new0 (HTMLColorSet, 1);

	/* these are default color settings */
	SET_COLOR  (Link,  0, 0, 0xffff);
	SET_COLOR  (ALink, 0, 0, 0xffff);
	SET_COLOR  (VLink, 0, 0, 0xffff);

	if (w) {
		GtkStyle *style = gtk_widget_get_style (w);

		SET_GCOLOR (Bg,            style->bg [GTK_STATE_NORMAL]);
		SET_GCOLOR (Text,          style->text [GTK_STATE_NORMAL]);
		SET_GCOLOR (Highlight,     style->bg [GTK_STATE_SELECTED]);
		SET_GCOLOR (HighlightText, style->text [GTK_STATE_SELECTED]);
	} else {
		SET_COLOR  (Bg, 0xffff, 0xffff, 0xffff);
		SET_COLOR  (Highlight, 0x7fff, 0x7fff, 0xffff);
		SET_COLOR  (HighlightText, 0, 0, 0);
	}

	return s;
}

void
html_colorset_destroy (HTMLColorSet *set)
{
	g_return_if_fail (set != NULL);

	if (set->slaves)
		g_slist_free (set->slaves);

	g_free (set);
}

void
html_colorset_add_slave (HTMLColorSet *set, HTMLColorSet *slave)
{
	set->slaves = g_slist_prepend (set->slaves, slave);
}

void
html_colorset_set_color (HTMLColorSet *s, GdkColor *color, HTMLColor idx)
{
	GSList *cur;
	HTMLColorSet *cs;

	if (s->color_allocated [idx])
		s->colors_to_free = g_slist_prepend (s->colors_to_free, gdk_color_copy (&s->color [idx]));
	s->color [idx] = *color;
	s->color_allocated [idx] = FALSE;

	/* forward change to slaves */
	cur = s->slaves;
	while (cur) {
		cs  = (HTMLColorSet *) cur->data;
		html_colorset_set_color (cs, color, idx);
		cur = cur->next;
	}
}

GdkColor *
html_colorset_get_color (HTMLColorSet *s, HTMLColor idx)
{
	return &s->color [idx];
}

GdkColor *
html_colorset_get_color_allocated (HTMLPainter *painter, HTMLColor idx)
{
	HTMLColorSet *s = painter->color_set;

	if (s->colors_to_free)
		html_colorset_free_colors (s, painter, FALSE);

	if (!s->color_allocated [idx]) {
		html_painter_alloc_color (painter, &s->color [idx]);
		s->color_allocated [idx] = TRUE;
	}

	return &s->color [idx];
}

void
html_colorset_set_by (HTMLColorSet *s, HTMLColorSet *o)
{
	HTMLColor i;

	for (i=0; i < HTMLColors; i++)
		html_colorset_set_color (s, &o->color [i], i);
}

void
html_colorset_free_colors (HTMLColorSet *s, HTMLPainter *painter, gboolean all)
{
	GSList *cur = s->colors_to_free;
	GdkColor *c;

	/* free colors to free */
	if (cur) {
		while (cur) {
			c = (GdkColor *) cur->data;
			html_painter_free_color (painter, c);
			gdk_color_free (c);
			cur = cur->next;
		}
		g_slist_free (s->colors_to_free);
		s->colors_to_free = NULL;
	}

	/* all means that we free all allocated colors */
	if (all) {
		HTMLColor i;

		for (i=0; i < HTMLColors; i++) {
			if (s->color_allocated [i]) {
				html_painter_free_color (painter, &s->color [i]);
				s->color_allocated [i] = FALSE;
			}
		}
	}
}
