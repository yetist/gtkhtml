/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999 Helix Code, Inc.
   
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
#include "htmltextslave.h"
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
		x_offset = gdk_text_width (htmltext->font->gdk_font,
					   htmltext->text,
					   cursor->offset);
		html_painter_draw_cursor (p,
					  o->x + tx + x_offset, o->y + ty,
					  o->ascent, o->descent);
	}
}

static gboolean
accepts_cursor (HTMLObject *object)
{
	return TRUE;
}


/* HTMLText methods.  */

static void
queue_draw (HTMLText *text,
	    HTMLEngine *engine,
	    guint offset,
	    guint len)
{
	html_engine_queue_draw (engine, HTML_OBJECT (text));
}

static void
insert_text (HTMLText *text,
	     HTMLEngine *engine,
	     guint offset,
	     const gchar *s,
	     guint len)
{
	gchar *new_buffer;
	guint old_len;
	guint new_len;

	/* The following code is very stupid and quite inefficient, but it is
           just for interactive editing so most likely people won't even
           notice.  */

	old_len = strlen (text->text);
	new_len = old_len + len;

	if (offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::insert_text().");

		/* This should never happen, but the following will make sure
                   things are always fixed up in a non-segfaulting way.  */
		offset = old_len;
	}

	new_buffer = g_malloc (new_len + 1);

	if (offset > 0)
		memcpy (new_buffer, text->text, offset);

	memcpy (new_buffer + offset, s, len);

	if (offset < old_len)
		memcpy (new_buffer + offset + len, text->text + offset,
			old_len - offset);

	new_buffer[new_len] = '\0';

	g_free (text->text);
	text->text = new_buffer;

	if (! html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text)))
		html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);
}

static guint
remove_text (HTMLText *text,
	     HTMLEngine *engine,
	     guint offset,
	     guint len)
{
	gchar *new_buffer;
	guint old_len;
	guint new_len;

	/* The following code is very stupid and quite inefficient, but it is
           just for interactive editing so most likely people won't even
           notice.  */

	old_len = strlen (text->text);

	if (offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::remove_text().");
		return;
	}

	if (offset + len > old_len || len == 0)
		len = old_len - offset;

	new_len = old_len - len;

	new_buffer = g_malloc (new_len + 1);

	if (offset > 0)
		memcpy (new_buffer, text->text, offset);

	memcpy (new_buffer + offset, text->text + offset + len, old_len - offset - len + 1);

	g_free (text->text);
	text->text = new_buffer;

	if (! html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text)))
		html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);

	return len;
}

static void
calc_char_position (HTMLText *self,
		    guint offset,
		    gint *x_return, gint *y_return)
{
	html_object_calc_abs_position (HTML_OBJECT (self), x_return, y_return);

	*x_return += gdk_text_width (self->font->gdk_font,
				     self->text,
				     offset);
}

static HTMLText *
split (HTMLText *self,
       guint offset)
{
	HTMLText *new;
	guint len;
	gchar *s;

	len = strlen (self->text);
	if (offset >= len || offset == 0)
		return NULL;

	s = g_strdup (self->text + offset);
	new = HTML_TEXT (html_text_new (s, self->font));

	self->text = g_realloc (self->text, offset + 1);
	self->text[offset] = '\0';

	return new;
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
	object_class->accepts_cursor = accepts_cursor;

	/* HTMLText methods.  */

	klass->insert_text = insert_text;
	klass->remove_text = remove_text;
	klass->queue_draw = queue_draw;
	klass->calc_char_position = calc_char_position;
	klass->split = split;
}

void
html_text_init (HTMLText *text_object,
		HTMLTextClass *klass,
		gchar *text,
		HTMLFont *font)
{
	HTMLObject *object;

	g_return_if_fail (font != NULL);

	object = HTML_OBJECT (text_object);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	object->width = html_font_calc_width (font, text, -1);
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	text_object->text = text;
	text_object->font = font;
}

HTMLObject *
html_text_new (gchar *text, HTMLFont *font)
{
	HTMLText *text_object;

	text_object = g_new (HTMLText, 1);

	html_text_init (text_object, &html_text_class, text, font);

	return HTML_OBJECT (text_object);
}

void
html_text_insert_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       const gchar *p,
		       guint len)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (engine != NULL);
	g_return_if_fail (p != NULL);

	if (len == 0)
		return;

	(* HT_CLASS (text)->insert_text) (text, engine, offset, p, len);
}

guint
html_text_remove_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       guint len)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (engine != NULL);

	return (* HT_CLASS (text)->remove_text) (text, engine, offset, len);
}

void
html_text_queue_draw (HTMLText *text,
		      HTMLEngine *engine,
		      guint offset,
		      guint len)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (engine != NULL);

	(* HT_CLASS (text)->queue_draw) (text, engine, offset, len);
}

void
html_text_calc_char_position (HTMLText *text,
			      guint offset,
			      gint *x_return, gint *y_return)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (x_return != NULL);
	g_return_if_fail (y_return != NULL);

	(* HT_CLASS (text)->calc_char_position) (text, offset,
						 x_return, y_return);
}

HTMLText *
html_text_split (HTMLText *text,
		 guint offset)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (offset > 0, NULL);

	return (* HT_CLASS (text)->split) (text, offset);
}
