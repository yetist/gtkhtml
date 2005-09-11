/* "a -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2002, Ximian Inc.

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

#include <string.h>
#include <ctype.h>
#include "htmlstyle.h"

/* Color handling.  */
gboolean
html_parse_color (const gchar *text,
		  GdkColor *color)
{
	gchar c [8];
	gint  len = strlen (text);

	if (gdk_color_parse (text, color))
		return TRUE;

	c [7] = 0;
	if (*text != '#') {
		c[0] = '#'; 
		strncpy (c + 1, text, 6);
		len++;
	} else {
		strncpy (c, text, MIN (7, len));
	}
	
	if (len < 7)
		memset (c + len, '\0', 7-len);

	return gdk_color_parse (c, color);
}

static HTMLLength *
parse_length (char *str) {
	char *cur = str;
	HTMLLength *len;
	
	len = g_new0 (HTMLLength, 1);
	
	if (!str)
		return len;

	/* g_warning ("begin \"%s\"", *str); */

	while (isspace (*cur)) cur++;

	len->val = atoi (cur);
	len->type = HTML_LENGTH_TYPE_PIXELS;

	while (isdigit (*cur) || *cur == '-') cur++;

	switch (*cur) {
	case '*':
		if (len->val == 0)
			len->val = 1;
		len->type = HTML_LENGTH_TYPE_FRACTION;
		cur++;
		break;
	case '%':
		len->type = HTML_LENGTH_TYPE_PERCENT;
                cur++;
		break;
	}
	
	if (cur <= str) {
		g_free (len);
		return NULL;
	} 

	/* g_warning ("length len->val=%d, len->type=%d", len->val, len->type); */

	return len;
}


HTMLStyle *
html_style_new (void) 
{
	HTMLStyle *style = g_new0 (HTMLStyle, 1);

	style->display = DISPLAY_NONE;

	style->color = NULL;
	style->mask = 0;
	style->settings = 0;

	/* BLOCK */
	style->text_align = HTML_HALIGN_NONE;
	style->clear = HTML_CLEAR_NONE;

	style->text_valign = HTML_VALIGN_NONE;

	return style;
}

void
html_style_free (HTMLStyle *style)
{
	if (!style)
		return;

	g_free (style->face);
	g_free (style->bg_image);
	g_free (style->width);
	g_free (style->height);

	if (style->color)
		html_color_unref (style->color);

	if (style->bg_color)
		html_color_unref (style->bg_color);

	g_free (style);
}

HTMLStyle *
html_style_add_color (HTMLStyle *style, HTMLColor *color)
{
	HTMLColor *old;
	
	if (!style)
		style = html_style_new ();

	old = style->color;

	style->color = color;

	if (color)
		html_color_ref (color);
	
	if (old)
		html_color_unref (old);

	return style;
}      

HTMLStyle *
html_style_unset_decoration (HTMLStyle *style, GtkHTMLFontStyle font_style)
{
	if (!style)
		style = html_style_new ();

	font_style &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	style->mask |= font_style;
	style->settings &= ~font_style;

	return style;
}

HTMLStyle *
html_style_set_decoration (HTMLStyle *style, GtkHTMLFontStyle font_style)
{
	if (!style)
		style = html_style_new ();

	font_style &= ~GTK_HTML_FONT_STYLE_SIZE_MASK;
	style->mask |= font_style;
	style->settings |= font_style;

	return style;
}

HTMLStyle *
html_style_set_font_size (HTMLStyle *style, GtkHTMLFontStyle font_style)
{
	if (!style)
		style = html_style_new ();

	font_style &= GTK_HTML_FONT_STYLE_SIZE_MASK;
	style->mask |= GTK_HTML_FONT_STYLE_SIZE_MASK;
	style->settings |= font_style;

	return style;
}

HTMLStyle *
html_style_add_font_face (HTMLStyle *style, const HTMLFontFace *face)
{
	if (!style)
		style = html_style_new ();

	g_free (style->face);
	style->face = g_strdup (face);

	return style;
}

HTMLStyle *
html_style_add_text_align (HTMLStyle *style, HTMLHAlignType type)
{
	if (!style)
		style = html_style_new ();

	style->text_align = type;

	return style;
}

HTMLStyle *
html_style_add_text_valign (HTMLStyle *style, HTMLVAlignType type)
{
	if (!style)
		style = html_style_new ();

	style->text_valign = type;

	return style;
}

HTMLStyle *
html_style_add_background_color (HTMLStyle *style, HTMLColor *color)
{
	HTMLColor *old;

	if (!style)
		style = html_style_new ();

	old = style->bg_color;

	style->bg_color = color;

	if (color)
		html_color_ref (color);

	if (old)
		html_color_unref (old);

	return style;
}

HTMLStyle *
html_style_set_display (HTMLStyle *style, HTMLDisplayType display)
{
	if (!style)
		style = html_style_new ();

	style->display = display;

	return style;
}

HTMLStyle *
html_style_set_clear (HTMLStyle *style, HTMLClearType clear)
{
	if (!style)
		style = html_style_new ();

	style->clear = clear;

	return style;
}

HTMLStyle *
html_style_add_width (HTMLStyle *style, char *len)
{
	if (!style)
		style = html_style_new ();

	g_free (style->width);

	style->width = parse_length (len);

	return style;
}

HTMLStyle *
html_style_add_height (HTMLStyle *style, char *len)
{
	if (!style)
		style = html_style_new ();

	g_free (style->height);

	style->height = parse_length (len);

	return style;
}

HTMLStyle *
html_style_add_background_image (HTMLStyle *style, const char *url)
{
	if (!style)
		style = html_style_new ();

	g_free (style->bg_image);
	style->bg_image = g_strdup (url);

	return style;
}

HTMLStyle *
html_style_set_border_style (HTMLStyle *style, HTMLBorderStyle bstyle)
{
	if (!style)
		style = html_style_new ();

	style->border_style = bstyle;

	return style;
}

HTMLStyle *
html_style_set_border_width (HTMLStyle *style, int width)
{
	if (!style)
		style = html_style_new ();

	style->border_width = width;

	return style;
}

HTMLStyle *
html_style_set_padding (HTMLStyle *style, int padding)
{
	if (!style)
		style = html_style_new ();

	style->padding = padding;

	return style;
}

HTMLStyle *
html_style_set_border_color (HTMLStyle *style, HTMLColor *color)
{
	HTMLColor *old;

	if (!style)
		style = html_style_new ();

	old = style->border_color;

	style->border_color = color;

	if (color)
		html_color_ref (color);

	if (old)
		html_color_unref (old);

	return style;
}

static HTMLStyle *
parse_border_style (HTMLStyle *style, char *value)
{
	while (isspace (*value))
		value ++;

	if (!g_ascii_strcasecmp (value, "solid"))
		style = html_style_set_border_style (style, HTML_BORDER_SOLID);
	else if (!g_ascii_strcasecmp (value, "inset"))
		style = html_style_set_border_style (style, HTML_BORDER_INSET);

	return style;
}

static HTMLStyle *
parse_border_color (HTMLStyle *style, char *value)
{
	GdkColor color;

	if (html_parse_color (value, &color)) {
		HTMLColor *hc = html_color_new_from_gdk_color (&color);
		style = html_style_set_border_color (style, hc);
		html_color_unref (hc);
	}

	return style;
}

static HTMLStyle *
parse_border_width (HTMLStyle *style, char *value)
{
	while (isspace (*value))
		value ++;

	if (!g_ascii_strcasecmp (value, "thin"))
		style = html_style_set_border_width (style, 1);
	else if (!g_ascii_strcasecmp (value, "medium"))
		style = html_style_set_border_width (style, 2);
	else if (!g_ascii_strcasecmp (value, "thick"))
		style = html_style_set_border_width (style, 5);
	else if (isdigit (*value))
		style = html_style_set_border_width (style, atoi (value));

	return style;
}

static HTMLStyle *
parse_border (HTMLStyle *style, char *value)
{
	while (value && *value) {
		char *next;
		int modified;
		char orig = 0;

		while (isspace (*value))
			value ++;

		next = value;
		while (*next && !isspace (*next))
			next ++;
		if (*next) {
			orig = *next;
			*next = 0;
			modified = 1;
		} else
			modified = 0;

		style = parse_border_style (style, value);
		style = parse_border_color (style, value);
		style = parse_border_width (style, value);

		if (modified) {
			*next = orig;
			next ++;
		}

		value = next;
	}

	return style;
}

HTMLStyle *
html_style_add_attribute (HTMLStyle *style, const char *attr)
{
	gchar **prop;

	prop = g_strsplit (attr, ";", 100);
	if (prop) {
		gint i;
		for (i = 0; prop[i]; i++) {
			char *text;
			
			text = g_strstrip (prop[i]);
			if (!g_ascii_strncasecmp ("color: ", text, 7)) {
				GdkColor color;

				if (html_parse_color (g_strstrip (text + 7), &color)) {
					HTMLColor *hc = html_color_new_from_gdk_color (&color);
					style = html_style_add_color (style, hc);
				        html_color_unref (hc);
				
				}
			} else if (!g_ascii_strncasecmp ("background: ", text, 12)) {
				GdkColor color;
				
				if (html_parse_color (text + 12, &color)) {
					HTMLColor *hc = html_color_new_from_gdk_color (&color);
					style = html_style_add_background_color (style, hc);
				        html_color_unref (hc);
				}
			} else if (!g_ascii_strncasecmp ("background-color: ", text, 18)) {
				GdkColor color;

				if (html_parse_color (text + 18, &color)) {
					HTMLColor *hc = html_color_new_from_gdk_color (&color);
					style = html_style_add_background_color (style, hc);
				        html_color_unref (hc);
				}
			} else if (!g_ascii_strncasecmp ("background-image: ", text, 18)) {
				style = html_style_add_background_image (style, text + 18);
				
			} else if (!g_ascii_strncasecmp ("border: ", text, 8)) {
				style = parse_border (style, text + 8);
			} else if (!g_ascii_strncasecmp ("border-style: ", text, 14)) {
				style = parse_border_style (style, text + 14);
			} else if (!g_ascii_strncasecmp ("border-color: ", text, 14)) {
				style = parse_border_color (style, text + 14);
			} else if (!g_ascii_strncasecmp ("border-width: ", text, 14)) {
				style = parse_border_width (style, text + 14);
			} else if (!g_ascii_strncasecmp ("padding: ", text, 9)) {
				char *value = text + 9;

				style = html_style_set_padding (style, atoi (value));
			} else if (!g_ascii_strncasecmp ("white-space: ", text, 13)) {
				/* normal, pre, nowrap, pre-wrap, pre-line, inherit  */
				/*
				if (!g_ascii_strcasecmp ("normal", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_NORMAL);
				} else if (!g_ascii_strcasecmp ("pre", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_PRE);
				} else if (!g_ascii_strcasecmp ("nowrap", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_NOWRAP);
				} else if (!g_ascii_strcasecmp ("pre-wrap", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_PRE_WRAP);
				} else if (!g_ascii_strcasecmp ("pre-line", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_PRE_LINE);
				} else if (!g_ascii_strcasecmp ("inherit", text + 13)) {
					style = html_style_set_white_space (style, HTML_WHITE_SPACE_INHERIT);
				}
				*/
			} else if (!g_ascii_strncasecmp ("text-decoration: none", text, 21)) {
				style = html_style_unset_decoration (style, ~GTK_HTML_FONT_STYLE_SIZE_MASK);
			} else if (!g_ascii_strncasecmp ("display: ", text, 9)) {
				char *value = text + 9;
				if (!g_ascii_strcasecmp ("block", value)) { 
					style = html_style_set_display (style, DISPLAY_BLOCK);
				} else if (!g_ascii_strcasecmp ("inline", value)) {
					style = html_style_set_display (style, DISPLAY_INLINE);
				} else if (!g_ascii_strcasecmp ("none", value)) {
					style = html_style_set_display (style, DISPLAY_NONE);
				} else if (!g_ascii_strcasecmp ("inline-table", value)) {
					style = html_style_set_display (style, DISPLAY_INLINE_TABLE);
				}
			} else if (!g_ascii_strncasecmp ("text-align: center", text, 18)) {
				style = html_style_add_text_align (style, HTML_HALIGN_CENTER);
			} else if (!g_ascii_strncasecmp ("width: ", text, 7)) {
				style = html_style_add_width (style, text + 7);
			} else if (!g_ascii_strncasecmp ("height: ", text, 8)) {
				style = html_style_add_height (style, text + 8);
			} else if (!g_ascii_strncasecmp ("clear: ", text, 7)) {
				char *value = text + 7;

				if (!g_ascii_strcasecmp ("left", value)) { 
					style = html_style_set_clear (style, HTML_CLEAR_LEFT); 
				} else if (!g_ascii_strcasecmp ("right", value)) {
					style = html_style_set_clear (style, HTML_CLEAR_RIGHT); 
				} else if (!g_ascii_strcasecmp ("both", value)) {
					style = html_style_set_clear (style, HTML_CLEAR_ALL); 
				} else if (!g_ascii_strcasecmp ("inherit", value)) {
					style = html_style_set_clear (style, HTML_CLEAR_INHERIT);
				} else if (!g_ascii_strcasecmp ("none", value)) {
					style = html_style_set_clear (style, HTML_CLEAR_NONE);
				}
			}
		}
		g_strfreev (prop);
	}
	return style;
}
