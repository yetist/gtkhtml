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
#include "htmltext.h"

HTMLObject *
html_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter)
{
	HTMLText *htmltext = g_new0 (HTMLText, 1);
	HTMLObject *object = HTML_OBJECT (htmltext);
	html_object_init (object, Text);

	/* Functions */
	object->draw = html_text_draw;

	object->width = html_font_calc_width (font, text, -1);
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	htmltext->text = text;
	htmltext->font = font;

	return object;
}

void
html_text_draw (HTMLObject *o, HTMLPainter *p,
		gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLText *htmltext = HTML_TEXT (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	html_painter_set_font (p, htmltext->font);
	html_painter_set_pen (p, htmltext->font->textColor);
	html_painter_draw_text (p, o->x + tx, o->y + ty, htmltext->text, -1);
}
