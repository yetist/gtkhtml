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

#include "htmltext.h"
#include "htmlcursor.h"

HTMLTextClass html_text_class;

#define HT_CLASS(x) HTML_TEXT_CLASS (HTML_OBJECT (x)->klass)


/* HTMLObject methods.  */

static void
draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor,
      gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLText *htmltext;
	gint x_offset;

	htmltext = HTML_TEXT (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	html_painter_set_font (p, htmltext->font);
	html_painter_set_pen (p, htmltext->font->textColor);
	html_painter_draw_text (p, o->x + tx, o->y + ty, htmltext->text, -1);

	if (cursor != NULL && cursor->object == o) {
		x_offset = gdk_text_width (htmltext->font->gdk_font, htmltext->text,
					   cursor->offset);

		html_painter_draw_cursor (p,
					  o->x + tx + x_offset, o->y + ty,
					  o->ascent, o->descent);
	}
}


/* HTMLText methods.  */

static void
insert_text (HTMLText *text, HTMLCursor *cursor, const gchar *s, guint len)
{
	gchar *new_buffer;
	guint old_len;
	guint new_len;

	/* The following code is very stupid, but it is just for interactive
           editing so most likely people won't even notice.  */

	old_len = strlen (text->text);
	new_len = old_len + len;

	if (cursor->offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::insert_text().");

		/* This should never happen, but this will make sure things are
                   always fixed up in a non-breaking way.  */
		cursor->offset = old_len;
	}

	new_buffer = g_malloc (new_len + 1);

	if (cursor->offset > 0)
		memcpy (new_buffer, text->text, cursor->offset);

	memcpy (new_buffer + cursor->offset, s, len);

	if (cursor->offset < old_len)
		memcpy (new_buffer + cursor->offset + len,
			text->text + cursor->offset,
			old_len - cursor->offset);

	new_buffer[new_len] = '\0';
}


void
html_text_type_init (void)
{
	html_text_class_init (&html_text_class, HTML_TYPE_TEXT);
}

void
html_text_class_init (HTMLTextClass *klass,
		      HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	/* FIXME destroy */

	object_class->draw = draw;
}

void
html_text_init (HTMLText *text_object,
		HTMLTextClass *klass,
		gchar *text,
		HTMLFont *font,
		HTMLPainter *painter)
{
	HTMLObject *object;

	object = HTML_OBJECT (text_object);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	object->width = html_font_calc_width (font, text, -1);
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	text_object->text = text;
	text_object->font = font;
}

HTMLObject *
html_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter)
{
	HTMLText *text_object;

	text_object = g_new (HTMLText, 1);

	html_text_init (text_object, &html_text_class, text, font, painter);

	return HTML_OBJECT (text_object);
}

void
html_text_insert_text (HTMLText *text, HTMLCursor *cursor, const gchar *p, guint len)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (p != NULL);
	g_return_if_fail (cursor->object != HTML_OBJECT (text));

	if (len == 0)
		return;

	(* HT_CLASS (text)->insert_text) (text, cursor, p, len);
}
