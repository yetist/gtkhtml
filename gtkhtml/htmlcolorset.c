/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
 *
 *  Copyright (C) 2000 Helix Code, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlpainter.h"



HTMLColorSet *
html_colorset_new (GtkWidget *w)
{
	HTMLColorSet *s;

	s = g_new0 (HTMLColorSet, 1);

	/* these are default color settings */

	if (w && gtk_widget_get_style_context (w)) {
		html_colorset_set_style (s, w);
	} else {
		s->color[HTMLLinkColor]            = html_color_new_from_rgb (0, 0, 0xffff);
		s->color[HTMLALinkColor]           = html_color_new_from_rgb (0, 0, 0xffff);
		s->color[HTMLVLinkColor]           = html_color_new_from_rgb (0xffff, 0, 0);
		s->color[HTMLSpellErrorColor]      = html_color_new_from_rgb (0xffff, 0, 0);
		s->color[HTMLBgColor]              = html_color_new_from_rgb (0xffff, 0xffff, 0xffff);
		s->color[HTMLHighlightColor]       = html_color_new_from_rgb (0x7fff, 0x7fff, 0xffff);
		s->color[HTMLHighlightTextColor]   = html_color_new ();
		s->color[HTMLHighlightNFColor]     = html_color_new ();
		s->color[HTMLHighlightTextNFColor] = html_color_new ();
		s->color[HTMLTextColor]            = html_color_new ();
		s->color[HTMLCiteColor]            = html_color_new ();
	}

	return s;
}

void
html_colorset_destroy (HTMLColorSet *set)
{
	gint i;

	g_return_if_fail (set != NULL);

	for (i = 0; i < HTMLColors; i++) {
		if (set->color[i] != NULL)
			html_color_unref (set->color[i]);
	}

	if (set->slaves)
		g_slist_free (set->slaves);

	g_free (set);
}

void
html_colorset_add_slave (HTMLColorSet *set,
                         HTMLColorSet *slave)
{
	set->slaves = g_slist_prepend (set->slaves, slave);
}

void
html_colorset_set_color (HTMLColorSet *s,
                         GdkColor *color,
                         HTMLColorId idx)
{
	GSList *cur;
	HTMLColorSet *cs;

	html_color_set (s->color[idx], color);
	s->changed[idx] = TRUE;

	/* forward change to slaves */
	cur = s->slaves;
	while (cur) {
		cs  = (HTMLColorSet *) cur->data;
		html_colorset_set_color (cs, color, idx);
		cur = cur->next;
	}
}

HTMLColor *
html_colorset_get_color (HTMLColorSet *s,
                         HTMLColorId idx)
{
	return s->color[idx];
}

HTMLColor *
html_colorset_get_color_allocated (HTMLColorSet *s,
                                   HTMLPainter *painter,
                                   HTMLColorId idx)
{
	html_color_alloc (s->color[idx], painter);
	return s->color[idx];
}

void
html_colorset_set_by (HTMLColorSet *s,
                      HTMLColorSet *o)
{
	HTMLColorId i;

	for (i = 0; i < HTMLColors; i++) {
		html_colorset_set_color (s, &o->color[i]->color, i);
		/* unset the changed flag */
		s->changed[i] = FALSE;
	}
}

void
html_colorset_set_unchanged (HTMLColorSet *s,
                             HTMLColorSet *o)
{
	HTMLColorId i;

	for (i = 0; i < HTMLColors; i++) {
		if (!s->changed[i]) {
			html_colorset_set_color (s, &o->color[i]->color, i);
			s->changed[i] = FALSE;
		}
	}
}

static void
copy_to_rgba (GdkColor *in_color,
              GdkRGBA *out_rgba)
{
	g_return_if_fail (in_color != NULL);
	g_return_if_fail (out_rgba != NULL);

	out_rgba->alpha = 1.0;
	out_rgba->red = in_color->red / 65535.0;
	out_rgba->green = in_color->green / 65535.0;
	out_rgba->blue = in_color->blue / 65535.0;
}

static void
get_prop_color (GtkWidget *w,
                const gchar *name,
                const gchar *dv,
                gboolean silent_fallback,
                GdkRGBA *out_color)
{
	GdkColor *color = NULL;
	GtkStyleContext *style_context = gtk_widget_get_style_context (w);

	gtk_widget_style_get (w, name, &color, NULL);

	if (color) {
		copy_to_rgba (color, out_color);
		gdk_color_free (color);
		return;
	}

	if (dv && gdk_rgba_parse (out_color, dv))
		return;

	if (!silent_fallback)
		g_warning ("falling back to text color");

	gtk_style_context_get_color (style_context, GTK_STATE_FLAG_NORMAL, out_color);
}

void
html_colorset_set_style (HTMLColorSet *s,
                         GtkWidget *w)
{
#define SET_GCOLOR(t,rgba)										\
	if (!s->changed[HTML ## t ## Color]) {								\
		GdkColor gc;										\
													\
		gc.pixel = -1;										\
		gc.red = rgba.red * 65535.0;								\
		gc.green = rgba.green * 65535.0;							\
		gc.blue = rgba.blue * 65535.0;								\
													\
		if (s->color[HTML ## t ## Color]) html_color_unref (s->color[HTML ## t ## Color]);	\
		s->color[HTML ## t ## Color] = html_color_new_from_gdk_color (&gc);			\
	}
#define SET_COLOR_FUNC(t,st,func)									\
	if (!s->changed[HTML ## t ## Color]) {								\
		GdkRGBA color_rgba;									\
													\
		func (style_context, st, &color_rgba);							\
													\
		SET_GCOLOR (t, color_rgba);								\
	}

#define SET_COLOR_BG(t,st) SET_COLOR_FUNC (t, st, gtk_style_context_get_background_color)
#define SET_COLOR_FG(t,st) SET_COLOR_FUNC (t, st, gtk_style_context_get_color)

	GdkRGBA color;
	GtkStyleContext *style_context = gtk_widget_get_style_context (w);

	/* have text background and foreground colors same as GtkEntry */
	gtk_style_context_save (style_context);
	gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_ENTRY);
	SET_COLOR_BG (Bg,              GTK_STATE_FLAG_NORMAL);
	SET_COLOR_FG (Text,            GTK_STATE_FLAG_NORMAL);
	gtk_style_context_restore (style_context);

	SET_COLOR_BG (Highlight,       GTK_STATE_FLAG_SELECTED);
	SET_COLOR_FG (HighlightText,   GTK_STATE_FLAG_SELECTED);
	SET_COLOR_BG (HighlightNF,     GTK_STATE_FLAG_ACTIVE);
	SET_COLOR_FG (HighlightTextNF, GTK_STATE_FLAG_ACTIVE);
	get_prop_color (w, "link_color", "#0000ff", FALSE, &color);
	SET_GCOLOR (Link, color);
	get_prop_color (w, "alink_color", "#0000ff", FALSE, &color);
	SET_GCOLOR (ALink, color);
	get_prop_color (w, "vlink_color", "#ff0000", FALSE, &color);
	SET_GCOLOR (VLink, color);
	get_prop_color (w, "spell_error_color", "#ff0000", FALSE, &color);
	SET_GCOLOR (SpellError, color);
	get_prop_color (w, "cite_color", NULL, TRUE, &color);
	SET_GCOLOR (Cite, color);

#undef SET_COLOR_FG
#undef SET_COLOR_BG
#undef SET_COLOR_FUNC
#undef SET_GCOLOR
}
