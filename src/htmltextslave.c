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

#include <string.h>

#include "htmltextslave.h"
#include "htmlclue.h"
#include "htmlcursor.h"


/* #define HTML_TEXT_SLAVE_DEBUG */

HTMLTextSlaveClass html_text_slave_class;
static HTMLObjectClass *parent_class = NULL;


/* Split this TextSlave at the specified offset.  */
static void
split (HTMLTextSlave *slave, gshort offset)
{
	HTMLObject *obj;
	HTMLObject *new;

	g_return_if_fail (offset >= 0);
	g_return_if_fail (offset < slave->posLen);

	obj = HTML_OBJECT (slave);

	new = html_text_slave_new (slave->owner,
				   slave->posStart + offset,
				   slave->posLen - offset);
	html_clue_append_after (HTML_CLUE (obj->parent), new, obj);

	slave->posLen = offset;
}


/* HTMLObject methods.  */

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	/* FIXME it does not make much sense to me to share the owner.  */
	HTML_TEXT_SLAVE (dest)->owner = HTML_TEXT_SLAVE (self)->owner;
	HTML_TEXT_SLAVE (dest)->posStart = HTML_TEXT_SLAVE (self)->posStart;
	HTML_TEXT_SLAVE (dest)->posLen = HTML_TEXT_SLAVE (self)->posLen;
}

static gboolean
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	HTMLText *owner;
	HTMLTextSlave *slave;
	GtkHTMLFontStyle font_style;
	gint new_ascent, new_descent, new_width;
	gboolean changed;

	slave = HTML_TEXT_SLAVE (self);
	owner = HTML_TEXT (slave->owner);
	font_style = html_text_get_font_style (owner);

	new_ascent = html_painter_calc_ascent (painter, font_style, owner->face);
	new_descent = html_painter_calc_descent (painter, font_style, owner->face);

	new_width = html_painter_calc_text_width (painter,
						  html_text_get_text (HTML_TEXT (owner), slave->posStart),
						  slave->posLen,
						  font_style, owner->face);

	changed = FALSE;

	if (new_ascent != self->ascent) {
		self->ascent = new_ascent;
		changed = TRUE;
	}

	if (new_descent != self->descent) {
		self->descent = new_descent;
		changed = TRUE;
	}

	if (new_width != self->width) {
		self->width = new_width;
		changed = TRUE;
	}

	return changed;
}

#ifdef HTML_TEXT_SLAVE_DEBUG
static void
debug_print (HTMLFitType rv, gchar *text, gint len)
{
	gint i;

	printf ("Split text");
	switch (rv) {
	case HTML_FIT_PARTIAL:
		printf (" (Partial): `");
		break;
	case HTML_FIT_NONE:
		printf (" (NoFit): `");
		break;
	case HTML_FIT_COMPLETE:
		printf (" (Complete): `");
		break;
	}

	for (i = 0; i < len; i++)
		putchar (text [i]);

	printf ("'\n");
}
#endif

static gint
get_next_nb_width (HTMLTextSlave *slave, HTMLPainter *painter)
{
	
	gint width = 0;

	if (HTML_TEXT (slave->owner)->text_len == 0
	    || html_text_get_char (HTML_TEXT (slave->owner), slave->posStart + slave->posLen - 1) != ' ') {
		HTMLObject *obj;
		obj = html_object_next_not_slave (HTML_OBJECT (slave));
		if (obj && html_object_is_text (obj)
		    && html_text_get_char (HTML_TEXT (obj), 0) != ' ')
			width = html_text_get_nb_width (HTML_TEXT (obj), painter, TRUE);
	}

	return width;
}

static gboolean
could_remove_leading_space (HTMLTextSlave *slave, gboolean firstRun)
{
	HTMLObject *o = HTML_OBJECT (slave->owner);

	if (firstRun && (HTML_OBJECT (slave)->prev != o || o->prev))
		return TRUE;

	while (o->prev && HTML_OBJECT_TYPE (o->prev) == HTML_TYPE_CLUEALIGNED)
		o = o->prev;

	return o->prev ? FALSE : TRUE;
}

static HTMLFitType
fit_line (HTMLObject *o,
	  HTMLPainter *painter,
	  gboolean startOfLine,
	  gboolean firstRun,
	  gint widthLeft)
{
	GtkHTMLFontStyle font_style;
	HTMLFitType rv;
	HTMLTextSlave *slave;
	HTMLText *text;
	gint width, next_width = 0;

	/* printf ("fit_line %d left: %d\n", firstRun, widthLeft); */

	slave = HTML_TEXT_SLAVE (o);
	text  = HTML_TEXT (slave->owner);

	if (html_text_get_char (text, slave->posStart) == ' ' && could_remove_leading_space (slave, firstRun)) {
		slave->posStart ++;
		slave->posLen --;
	}

	/* try complete fit now */
	font_style = html_text_get_font_style (text);
	width      = html_painter_calc_text_width (painter, html_text_get_text (text, slave->posStart),
						   slave->posLen,
						   font_style, text->face);
	next_width = get_next_nb_width (slave, painter);

	if (width + next_width <= widthLeft)
		rv = HTML_FIT_COMPLETE;
	else {
		/* so try partial fit atleast */
		gchar *sep = NULL, *lsep;
		gchar *begin;
		gint width = 0, len;

		sep = begin = html_text_get_text (text, slave->posStart);

		do {
			lsep   = sep;
			sep    = unicode_strchr (lsep + 1, ' ');
			if (!sep || unicode_index_to_offset (begin, sep - begin) >= slave->posLen) {
				sep = begin + unicode_offset_to_index (begin, slave->posLen);
				break;
			}
			width += html_painter_calc_text_width
				(painter, lsep, unicode_index_to_offset (lsep, sep - lsep), font_style, text->face);
		} while (width <= widthLeft);

		g_assert (lsep);

		if (lsep == begin && !firstRun)
			rv = HTML_FIT_NONE;
		else {
			/* partial fit, we split this slave to this and new one which becomes next */
			rv = HTML_FIT_PARTIAL;
			/* if only one pass has been made */
			if (lsep == begin) lsep = sep;
			/* split + set attributes */
			len = unicode_index_to_offset (begin, lsep - begin);
			if (len < slave->posLen)
				split (slave, len);
			o->width   = width;
			o->ascent  = html_painter_calc_ascent (painter, font_style, text->face);
			o->descent = html_painter_calc_descent (painter, font_style, text->face);
		}
	}

#ifdef HTML_TEXT_SLAVE_DEBUG
	debug_print (rv, html_text_get_text (text, slave->posStart), slave->posLen);
#endif

	return rv;	
}

static gboolean
select_range (HTMLObject *self,
	      HTMLEngine *engine,
	      guint start, gint length,
	      gboolean queue_draw)
{
	return FALSE;
}

static guint
get_length (HTMLObject *self)
{
	return 0;
}


/* HTMLObject::draw() implementation.  */

static void
draw_spell_errors (HTMLTextSlave *slave, HTMLPainter *p, gint tx, gint ty)
{
	GList *cur = HTML_TEXT (slave->owner)->spell_errors;
	HTMLObject *obj = HTML_OBJECT (slave);
	SpellError *se;
	guint ma, mi;

	while (cur) {
		se = (SpellError *) cur->data;
		ma = MAX (se->off, slave->posStart);
		mi = MIN (se->off + se->len, slave->posStart + slave->posLen);
		if (ma < mi) {
			guint off = ma - slave->posStart;
			guint len = mi - ma;

			html_painter_set_pen (p, &html_colorset_get_color_allocated (p, HTMLSpellErrorColor)->color);
			/* printf ("spell error: %s\n", HTML_TEXT (slave->owner)->text + off); */
			html_painter_draw_spell_error (p, obj->x + tx, obj->y + ty,
						       html_text_get_text (HTML_TEXT (slave->owner), slave->posStart),
						       off, len);
		}
		if (se->off > slave->posStart + slave->posLen)
			break;
		cur = cur->next;
	}
}

static void
draw_normal (HTMLTextSlave *self,
	     HTMLPainter *p,
	     GtkHTMLFontStyle font_style,
	     gint x, gint y,
	     gint width, gint height,
	     gint tx, gint ty)
{
	HTMLObject *obj;

	obj = HTML_OBJECT (self);

	html_painter_set_font_style (p, font_style);
	html_painter_set_font_face  (p, HTML_TEXT (self->owner)->face);
	html_color_alloc (HTML_TEXT (self->owner)->color, p);
	html_painter_set_pen (p, &HTML_TEXT (self->owner)->color->color);
	html_painter_draw_text (p,
				obj->x + tx, obj->y + ty, 
				html_text_get_text (HTML_TEXT (self->owner), self->posStart),
				self->posLen);
}

static void
draw_highlighted (HTMLTextSlave *slave,
		  HTMLPainter *p,
		  GtkHTMLFontStyle font_style,
		  gint x, gint y,
		  gint width, gint height,
		  gint tx, gint ty)
{
	HTMLTextMaster *owner;
	HTMLObject *obj;
	guint start, end, len;
	guint offset_width, text_width;
	const gchar *text;

	obj = HTML_OBJECT (slave);
	owner = HTML_TEXT_MASTER (slave->owner);
	start = owner->select_start;
	end = start + owner->select_length;

	text = HTML_TEXT (owner)->text;

	if (start < slave->posStart)
		start = slave->posStart;
	if (end > slave->posStart + slave->posLen)
		end = slave->posStart + slave->posLen;
	len = end - start;

	offset_width = html_painter_calc_text_width (p, text + unicode_offset_to_index (text, slave->posStart),
						     start - slave->posStart,
						     font_style, HTML_TEXT (owner)->face);
	text_width = html_painter_calc_text_width (p, text + unicode_offset_to_index (text, start),
						   len, font_style, HTML_TEXT (owner)->face);

	html_painter_set_font_style (p, font_style);
	html_painter_set_font_face  (p, HTML_TEXT (owner)->face);

	/* Draw the highlighted part with a highlight background.  */

	html_painter_set_pen (p, &html_colorset_get_color_allocated (p, HTMLHighlightColor)->color);
	html_painter_fill_rect (p, obj->x + tx + offset_width, obj->y + ty - obj->ascent,
				text_width, obj->ascent + obj->descent);
	html_painter_set_pen (p, &html_colorset_get_color_allocated (p, HTMLHighlightTextColor)->color);
	html_painter_draw_text (p, obj->x + tx + offset_width, obj->y + ty, text + unicode_offset_to_index (text, start), len);

	/* Draw the non-highlighted part.  */

	html_color_alloc (HTML_TEXT (owner)->color, p);
	html_painter_set_pen (p, &HTML_TEXT (owner)->color->color);

	/* 1. Draw the leftmost non-highlighted part, if any.  */

	if (start > slave->posStart)
		html_painter_draw_text (p,
					obj->x + tx, obj->y + ty,
					text + unicode_offset_to_index (text, slave->posStart),
					start - slave->posStart);

	/* 2. Draw the rightmost non-highlighted part, if any.  */

	if (end < slave->posStart + slave->posLen)
		html_painter_draw_text (p,
					obj->x + tx + offset_width + text_width, obj->y + ty,
					text + unicode_offset_to_index (text, end),
					slave->posStart + slave->posLen - end);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLTextSlave *textslave;
	HTMLTextMaster *owner;
	HTMLText *ownertext;
	GtkHTMLFontStyle font_style;
	guint end;
	ArtIRect paint;

	html_object_calc_intersection (o, &paint, x, y, width, height);
	if (art_irect_empty (&paint))
		return;
	
	textslave = HTML_TEXT_SLAVE (o);
	owner = textslave->owner;
	ownertext = HTML_TEXT (owner);
	font_style = html_text_get_font_style (ownertext);

	end = textslave->posStart + textslave->posLen;

	if (owner->select_start + owner->select_length <= textslave->posStart
	    || owner->select_start >= end) {
		draw_normal (textslave, p, font_style, x, y, width, height, tx, ty);
	} else {
		draw_highlighted (textslave, p, font_style, x, y, width, height, tx, ty);
	}

	draw_spell_errors (textslave, p, tx ,ty);
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	return 0;
}

static gint
calc_preferred_width (HTMLObject *o,
		      HTMLPainter *painter)
{
	return 0;
}

static const gchar *
get_url (HTMLObject *o)
{
	HTMLTextSlave *slave;

	slave = HTML_TEXT_SLAVE (o);
	return html_object_get_url (HTML_OBJECT (slave->owner));
}

static HTMLObject *
check_point (HTMLObject *self,
	     HTMLPainter *painter,
	     gint x, gint y,
	     guint *offset_return,
	     gboolean for_cursor)
{
	return NULL;
}


void
html_text_slave_type_init (void)
{
	html_text_slave_class_init (&html_text_slave_class, HTML_TYPE_TEXTSLAVE, sizeof (HTMLTextSlave));
}

void
html_text_slave_class_init (HTMLTextSlaveClass *klass,
			    HTMLType type,
			    guint object_size)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type, object_size);

	object_class->select_range = select_range;
	object_class->copy = copy;
	object_class->draw = draw;
	object_class->calc_size = calc_size;
	object_class->fit_line = fit_line;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->get_url = get_url;
	object_class->get_length = get_length;
	object_class->check_point = check_point;

	parent_class = &html_object_class;
}

void
html_text_slave_init (HTMLTextSlave *slave,
		      HTMLTextSlaveClass *klass,
		      HTMLTextMaster *owner,
		      gint posStart,
		      gint posLen)
{
	HTMLText *owner_text;
	HTMLObject *object;

	object = HTML_OBJECT (slave);
	owner_text = HTML_TEXT (owner);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	object->ascent = HTML_OBJECT (owner)->ascent;
	object->descent = HTML_OBJECT (owner)->descent;

	slave->posStart = posStart;
	slave->posLen = posLen;
	slave->owner = owner;

	/* text slaves has always min_width 0 */
	object->min_width = 0;
	object->change   &= ~HTML_CHANGE_MIN_WIDTH;
}

HTMLObject *
html_text_slave_new (HTMLTextMaster *owner, gint posStart, gint posLen)
{
	HTMLTextSlave *slave;

	slave = g_new (HTMLTextSlave, 1);
	html_text_slave_init (slave, &html_text_slave_class, owner, posStart, posLen);

	return HTML_OBJECT (slave);
}


guint
html_text_slave_get_offset_for_pointer (HTMLTextSlave *slave,
					HTMLPainter *painter,
					gint x, gint y)
{
	HTMLText *owner;
	GtkHTMLFontStyle font_style;
	guint width, prev_width;
	guint i;

	g_return_val_if_fail (slave != NULL, 0);

	owner = HTML_TEXT (slave->owner);
	font_style = html_text_get_font_style (owner);

	x -= HTML_OBJECT (slave)->x;

	prev_width = 0;
	for (i = 1; i <= slave->posLen; i++) {
		width = html_painter_calc_text_width (painter,
						      html_text_get_text (owner, slave->posStart),
						      i, font_style, owner->face);

		if ((width + prev_width) / 2 >= x)
			return i - 1;

		prev_width = width;
	}

	return slave->posLen;
}
