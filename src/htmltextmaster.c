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

#include "htmlobject.h"
#include "htmltextmaster.h"
#include "htmltextslave.h"


HTMLTextMasterClass html_text_master_class;


/* HTMLObject methods.  */

static void
draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor,
      gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	/* Don't paint yourself.  */
}

static gint
calc_min_width (HTMLObject *self,
		HTMLPainter *painter)
{
	HTMLTextMaster *master;
	HTMLFontStyle font_style;
	HTMLText *text;
	const gchar *p;
	gint min_width, run_width;

	master = HTML_TEXT_MASTER (self);
	text = HTML_TEXT (self);

	font_style = html_text_get_font_style (text);

	min_width = 0;
	run_width = 0;
	p = text->text;

	while (1) {
		if (*p != ' ' && *p != 0) {
			run_width += html_painter_calc_text_width (painter, p, 1, font_style);
		} else {
			if (run_width > min_width)
				min_width = run_width;
			run_width = 0;
			if (*p == 0)
				break;
		}
		p++;
	}

	return min_width;
}

static gint
calc_preferred_width (HTMLObject *self,
		      HTMLPainter *painter)
{
	HTMLText *text;
	HTMLFontStyle font_style;
	gint width;

	text = HTML_TEXT (self);
	font_style = html_text_get_font_style (text);

	width = html_painter_calc_text_width (painter,
					      text->text, text->text_len,
					      font_style);

	return width;
}

static void
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	self->width = 0;
	self->ascent = 0;
	self->descent = 0;
}

static HTMLFitType
fit_line (HTMLObject *o,
	  HTMLPainter *painter,
	  gboolean startOfLine,
	  gboolean firstRun,
	  gint widthLeft) 
{
	HTMLTextMaster *textmaster; 
	HTMLObject *next_obj;
	HTMLObject *text_slave;

	textmaster = HTML_TEXT_MASTER (o);

	if (o->flags & HTML_OBJECT_FLAG_NEWLINE)
		return HTML_FIT_COMPLETE;
	
	/* Remove existing slaves */
	next_obj = o->next;
	while (next_obj != NULL
	       && (HTML_OBJECT_TYPE (next_obj) == HTML_TYPE_TEXTSLAVE)) {
		o->next = next_obj->next;
		html_object_destroy (next_obj);
		next_obj = o->next;
	}
	
	/* Turn all text over to our slaves */
	text_slave = html_text_slave_new (textmaster, 0, HTML_TEXT (textmaster)->text_len);

	text_slave->next = o->next;
	text_slave->parent = o->parent;
	text_slave->prev = o;

	if (o->next != NULL)
		o->next->prev = text_slave;

	o->next = text_slave;

	return HTML_FIT_COMPLETE;
}

static HTMLText *
split (HTMLText *self,
       guint offset)
{
	HTMLTextMaster *master;
	HTMLObject *new;
	gchar *s;

	master = HTML_TEXT_MASTER (self);

	if (offset >= HTML_TEXT (self)->text_len || offset == 0)
		return NULL;

	s = g_strdup (self->text + offset);
	new = html_text_master_new (s, self->font_style, &self->color);

	self->text = g_realloc (self->text, offset + 1);
	self->text[offset] = '\0';
	HTML_TEXT (self)->text_len = offset;

	return HTML_TEXT (new);
}


/* HTMLText methods.  */

static void
queue_draw (HTMLText *text,
	    HTMLEngine *engine,
	    guint offset,
	    guint len)
{
	HTMLObject *obj;

	for (obj = HTML_OBJECT (text)->next; obj != NULL; obj = obj->next) {
		HTMLTextSlave *slave;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_TEXTSLAVE)
			continue;

		slave = HTML_TEXT_SLAVE (obj);

		if (offset < slave->posStart + slave->posLen
		    && (len == 0 || offset + len >= slave->posStart)) {
			html_engine_queue_draw (engine, obj);
			if (len != 0 && slave->posStart + slave->posLen > offset + len)
				break;
		}
	}
}

static void
calc_char_position (HTMLText *text,
		    guint offset,
		    gint *x_return, gint *y_return)
{
	HTMLObject *obj;

	for (obj = HTML_OBJECT (text)->next; obj != NULL; obj = obj->next) {
		HTMLTextSlave *slave;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_TEXTSLAVE)
			continue;

		slave = HTML_TEXT_SLAVE (obj);

#if 0
		if (offset < slave->posStart + slave->posLen
		    || obj->next == NULL
		    || HTML_OBJECT_TYPE (obj->next) != HTML_TYPE_TEXTSLAVE) {
			html_object_calc_abs_position (obj, x_return, y_return);
			if (offset != slave->posStart)
				*x_return += gdk_text_width (text->font->gdk_font,
							     text->text + slave->posStart,
							     offset - slave->posStart);
			return;
		}
#endif
	}
}


void
html_text_master_type_init (void)
{
	html_text_master_class_init (&html_text_master_class,
				     HTML_TYPE_TEXTMASTER);
}

void
html_text_master_class_init (HTMLTextMasterClass *klass,
			     HTMLType type)
{
	HTMLObjectClass *object_class;
	HTMLTextClass *text_class;

	object_class = HTML_OBJECT_CLASS (klass);
	text_class = HTML_TEXT_CLASS (klass);

	html_text_class_init (text_class, type);

	object_class->draw = draw;
	object_class->fit_line = fit_line;
	object_class->calc_size = calc_size;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;

	text_class->queue_draw = queue_draw;
	text_class->calc_char_position = calc_char_position;
	text_class->split = split;
}

void
html_text_master_init (HTMLTextMaster *master,
		       HTMLTextMasterClass *klass,
		       gchar *text,
		       HTMLFontStyle font_style,
		       const GdkColor *color)
{
	HTMLText* html_text;
	HTMLObject *object;

	html_text = HTML_TEXT (master);
	object = HTML_OBJECT (master);

	html_text_init (html_text, HTML_TEXT_CLASS (klass), text, font_style, color);
}

HTMLObject *
html_text_master_new (gchar *text,
		      HTMLFontStyle font_style,
		      const GdkColor *color)
{
	HTMLTextMaster *master;

	master = g_new (HTMLTextMaster, 1);
	html_text_master_init (master, &html_text_master_class, text, font_style, color);

	return HTML_OBJECT (master);
}


HTMLObject *
html_text_master_get_slave (HTMLTextMaster *master, guint offset)
{
	HTMLObject *p;

	for (p = HTML_OBJECT (master)->next; p != NULL; p = p->next) {
		HTMLTextSlave *slave;

		slave = HTML_TEXT_SLAVE (p);
		if (slave->posStart <= offset
		    && slave->posStart + slave->posLen > offset)
			break;
	}

	return p;
}
