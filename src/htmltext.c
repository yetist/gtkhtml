/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999, 2000 Helix Code, Inc.
   
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

#include <stdio.h>

#include "htmltext.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"


HTMLTextClass html_text_class;
static HTMLObjectClass *parent_class = NULL;

#define HT_CLASS(x) HTML_TEXT_CLASS (HTML_OBJECT (x)->klass)


/* Calculcate the length of the string after collapsing multiple spaces into
   single ones and, optionally, removing leading/trailing space completely.  */
static guint
calc_actual_len (const gchar *s,
		 guint len,
		 gboolean remove_leading_space,
		 gboolean remove_trailing_space,
		 const gchar **first_return)
{
	guint i;
	guint actual_len;

	if (len == 0)
		return 0;

	i = 0;
	actual_len = 0;

	if (remove_leading_space) {
		while (i < len && s[i] == ' ')
			i++;
		len -= i;
		if (len == 0)
			return 0;
	} 

	*first_return = s + i;

	if (remove_trailing_space) {
		while (len > 0 && s[len - 1] == ' ')
			len--;
		if (len == 0)
			return 0;
	}

	while (i < len) {
		if (s[i] == ' ') {
			actual_len++;
			while (i < len && s[i] == ' ')
				i++;
		} else {
			actual_len++;
			i++;
		}
	}

	if (remove_trailing_space && s[i - 1] == ' ')
		actual_len--;

	return actual_len;
}

static void
copy_collapsing_spaces (gchar *dst,
			const gchar *src,
			guint actual_len)
{
	const gchar *sp;
	gchar *dp;
	guint count;

	if (actual_len == 0)
		return;

	sp = src;
	dp = dst;
	count = 0;

	while (count < actual_len) {
		if (*sp == ' ') {
			*dp = ' ';
			dp++, count++, sp++;
			if (count < actual_len) {
				while (*sp == ' ')
					sp++;
			}
		} else {
			*dp = *sp;
			dp++, count++, sp++;
		}
	}
}

static void
get_tags (const HTMLText *text,
	  gchar *opening_tags,
	  gchar *ending_tags)
{
	HTMLFontStyle font_style;
	gchar *opening_p, *ending_p;
	guint size;

	font_style = text->font_style;

	opening_p = opening_tags;
	ending_p = ending_tags;

	size = font_style & HTML_FONT_STYLE_SIZE_MASK;
	if (size != 0) {
		opening_p += sprintf (opening_p, "<FONT SIZE=%d>", size);
		ending_p += sprintf (ending_p, "</FONT SIZE=%d>", size);
	}

	if (font_style & HTML_FONT_STYLE_BOLD) {
		opening_p += sprintf (opening_p, "<B>");
		ending_p += sprintf (ending_p, "</B>");
	}

	if (font_style & HTML_FONT_STYLE_ITALIC) {
		opening_p += sprintf (opening_p, "<I>");
		ending_p += sprintf (ending_p, "</I>");
	}

	if (font_style & HTML_FONT_STYLE_UNDERLINE) {
		opening_p += sprintf (opening_p, "<U>");
		ending_p += sprintf (ending_p, "</U>");
	}

	if (font_style & HTML_FONT_STYLE_STRIKEOUT) {
		opening_p += sprintf (opening_p, "<S>");
		ending_p += sprintf (ending_p, "</S>");
	}

	if (font_style & HTML_FONT_STYLE_FIXED) {
		opening_p += sprintf (opening_p, "<TT>");
		ending_p += sprintf (ending_p, "</TT>");
	}

	*opening_p = 0;
	*ending_p = 0;
}


/* HTMLObject methods.  */

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	HTML_TEXT (dest)->text = g_strdup (HTML_TEXT (self)->text);
	HTML_TEXT (dest)->text_len = HTML_TEXT (self)->text_len;

	HTML_TEXT (dest)->font_style = HTML_TEXT (self)->font_style;
	HTML_TEXT (dest)->color = HTML_TEXT (self)->color;
	HTML_TEXT (dest)->color_allocated = HTML_TEXT (self)->color_allocated;
}

static void
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	HTMLText *text;
	HTMLFontStyle font_style;

	text = HTML_TEXT (self);
	font_style = html_text_get_font_style (text);

	self->ascent = html_painter_calc_ascent (painter, font_style);
	self->descent = html_painter_calc_descent (painter, font_style);
	self->width = html_painter_calc_text_width (painter, text->text, text->text_len, font_style);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLFontStyle font_style;
	HTMLText *htmltext;

	htmltext = HTML_TEXT (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	font_style = html_text_get_font_style (htmltext);
	html_painter_set_font_style (p, font_style);

	html_painter_set_pen (p, html_text_get_color (htmltext, p));
	html_painter_draw_text (p, o->x + tx, o->y + ty, htmltext->text, -1);
}

static gboolean
accepts_cursor (HTMLObject *object)
{
	return TRUE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	/* OK, doing these nasty things is not in my style, but in this case
           it's so unlikely to break and it's so handy and fast that I think
           it's almost acceptable.  */
#define RIDICULOUS_BUFFER_SIZE 16384
	gchar opening_tags[RIDICULOUS_BUFFER_SIZE];
	gchar closing_tags[RIDICULOUS_BUFFER_SIZE];
#undef RIDICULOUS_BUFFER_SIZE
	HTMLText *text;

	text = HTML_TEXT (self);

	get_tags (text, opening_tags, closing_tags);

	if (! html_engine_save_output_string (state, opening_tags))
		return FALSE;

	if (! html_engine_save_encode (state, text->text, text->text_len))
		return FALSE;

	if (! html_engine_save_output_string (state, closing_tags))
		return FALSE;

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

static guint
insert_text (HTMLText *text,
	     HTMLEngine *engine,
	     guint offset,
	     const gchar *s,
	     guint len)
{
	gboolean remove_leading_space, remove_trailing_space;
	const gchar *start;
	gchar *new_buffer;
	guint old_len;
	guint new_len;
	guint actual_len;

	/* The following code is very stupid and quite inefficient, but it is
           just for interactive editing so most likely people won't even
           notice.  */

	old_len = text->text_len;
	if (offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::insert_text().");

		/* This should never happen, but the following will make sure
                   things are always fixed up in a non-segfaulting way.  */
		offset = old_len;
	}

	if (offset > 0 && text->text[offset - 1] == ' ')
		remove_leading_space = TRUE;
	else
		remove_leading_space = FALSE;

	if (text->text[offset] == ' ')
		remove_trailing_space = TRUE;
	else
		remove_trailing_space = FALSE;

	actual_len = calc_actual_len (s, len,
				      remove_leading_space,
				      remove_trailing_space,
				      &start);
	if (actual_len == 0)
		return 0;

	new_len = old_len + actual_len;
	new_buffer = g_malloc (new_len + 1);

	if (offset > 0)
		memcpy (new_buffer, text->text, offset);

	copy_collapsing_spaces (new_buffer + offset, start, actual_len);

	if (offset < old_len)
		memcpy (new_buffer + offset + actual_len,
			text->text + offset,
			old_len - offset);

	new_buffer[new_len] = '\0';

	g_free (text->text);

	text->text = new_buffer;
	text->text_len = new_len;

	if (! html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text)))
		html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);

	return actual_len;
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
		return 0;
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
	text->text_len = new_len;

	html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
	html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);

	return len;
}

static void
get_cursor (HTMLObject *self,
	    HTMLPainter *painter,
	    guint offset,
	    gint *x1, gint *y1,
	    gint *x2, gint *y2)
{
	html_object_get_cursor_base (self, painter, offset, x2, y2);

	*x1 = *x2;
	*y1 = *y2 - self->ascent;
	*y2 += self->descent - 1;
}

static void
get_cursor_base (HTMLObject *self,
		 HTMLPainter *painter,
		 guint offset,
		 gint *x, gint *y)
{
	HTMLFontStyle font_style;

	html_object_calc_abs_position (HTML_OBJECT (self), x, y);

	if (offset > 0) {
		font_style = html_text_get_font_style (HTML_TEXT (self));
		*x += html_painter_calc_text_width (painter,
						    HTML_TEXT (self)->text,
						    offset, font_style);
	}
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
	new = HTML_TEXT (html_text_new (s, self->font_style, &self->color));

	self->text = g_realloc (self->text, offset + 1);
	self->text[offset] = '\0';

	return new;
}

static void
merge (HTMLText *text,
       HTMLText **list)
{
	g_warning ("HTMLText::merge not implemented.");
}

/* This is necessary to merge the text-specified font style with that of the
   HTMLClueFlow parent.  */
static HTMLFontStyle
get_font_style (const HTMLText *text)
{
	HTMLObject *parent;
	HTMLFontStyle font_style;

	parent = HTML_OBJECT (text)->parent;

	if (HTML_OBJECT_TYPE (parent) == HTML_TYPE_CLUEFLOW) {
		HTMLFontStyle parent_style;

		parent_style = html_clueflow_get_default_font_style (HTML_CLUEFLOW (parent));
		font_style = html_font_style_merge (parent_style, text->font_style);
	} else {
		font_style = html_font_style_merge (HTML_FONT_STYLE_SIZE_3, text->font_style);
	}

	return font_style;
}

static const GdkColor *
get_color (HTMLText *text,
	   HTMLPainter *painter)
{
	if (! text->color_allocated)
		html_painter_alloc_color (painter, &text->color);

	return &text->color;
}

static void
set_font_style (HTMLText *text,
		HTMLEngine *engine,
		HTMLFontStyle style)
{
	if (text->font_style == style)
		return;

	text->font_style = style;

	if (engine != NULL) {
		html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
		html_engine_queue_draw (engine, HTML_OBJECT (text));
	}
}

static void
set_color (HTMLText *text,
	   HTMLEngine *engine,
	   const GdkColor *color)
{
	if (gdk_color_equal (&text->color, color))
		return;

	text->color = *color;
	text->color_allocated = FALSE;

	if (engine != NULL) {
		html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
		html_engine_queue_draw (engine, HTML_OBJECT (text));
	}
}


void
html_text_type_init (void)
{
	html_text_class_init (&html_text_class, HTML_TYPE_TEXT, sizeof (HTMLText));
}

void
html_text_class_init (HTMLTextClass *klass,
		      HTMLType type,
		      guint object_size)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type, object_size);

	/* FIXME destroy */

	object_class->copy = copy;
	object_class->draw = draw;
	object_class->accepts_cursor = accepts_cursor;
	object_class->calc_size = calc_size;
	object_class->get_cursor = get_cursor;
	object_class->get_cursor_base = get_cursor_base;
	object_class->save = save;

	/* HTMLText methods.  */

	klass->insert_text = insert_text;
	klass->remove_text = remove_text;
	klass->queue_draw = queue_draw;
	klass->split = split;
	klass->get_font_style = get_font_style;
	klass->get_color = get_color;
	klass->set_font_style = set_font_style;
	klass->set_color = set_color;
	klass->merge = merge;

	parent_class = &html_object_class;
}

void
html_text_init (HTMLText *text_object,
		HTMLTextClass *klass,
		gchar *text,
		HTMLFontStyle font_style,
		const GdkColor *color)
{
	HTMLObject *object;

	object = HTML_OBJECT (text_object);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	text_object->text = text;
	text_object->font_style = font_style;
	text_object->text_len = strlen (text);

	if (color != NULL) {
		text_object->color = *color;
	} else {
		text_object->color.red = 0;
		text_object->color.green = 0;
		text_object->color.blue = 0;
	}

	text_object->color_allocated = FALSE;
}

HTMLObject *
html_text_new (gchar *text,
	       HTMLFontStyle font,
	       const GdkColor *color)
{
	HTMLText *text_object;

	text_object = g_new (HTMLText, 1);

	html_text_init (text_object, &html_text_class, text, font, color);

	return HTML_OBJECT (text_object);
}

guint
html_text_insert_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       const gchar *p,
		       guint len)
{
	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (engine != NULL, 0);
	g_return_val_if_fail (p != NULL, 0);

	if (len == 0)
		return 0;

	return (* HT_CLASS (text)->insert_text) (text, engine, offset, p, len);
}

guint
html_text_remove_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       guint len)
{
	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (engine != NULL, 0);

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

HTMLText *
html_text_split (HTMLText *text,
		 guint offset)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (offset > 0, NULL);

	return (* HT_CLASS (text)->split) (text, offset);
}

void
html_text_merge (HTMLText *text,
		 HTMLText **list)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (list != NULL);

	return (* HT_CLASS (text)->merge) (text, list);
}


HTMLFontStyle
html_text_get_font_style (const HTMLText *text)
{
	g_return_val_if_fail (text != NULL, HTML_FONT_STYLE_DEFAULT);

	return (* HT_CLASS (text)->get_font_style) (text);
}

const GdkColor *
html_text_get_color (HTMLText *text,
		     HTMLPainter *painter)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (painter != NULL, NULL);

	return (* HT_CLASS (text)->get_color) (text, painter);
}

void
html_text_set_font_style (HTMLText *text,
			  HTMLEngine *engine,
			  HTMLFontStyle style)
{
	g_return_if_fail (text != NULL);

	return (* HT_CLASS (text)->set_font_style) (text, engine, style);
}

void  html_text_set_color (HTMLText *text,
			   HTMLEngine *engine,
			   const GdkColor *color)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (color != NULL);

	return (* HT_CLASS (text)->set_color) (text, engine, color);
}
