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

#include "htmlobject.h"
#include "htmltextmaster.h"
#include "htmltextslave.h"


HTMLTextMasterClass html_text_master_class;


static void
draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor,
      gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	/* Don't paint yourself.  */
}

static gint
calc_min_width (HTMLObject *o)
{
	return HTML_TEXT_MASTER (o)->minWidth;
}

static gint
calc_preferred_width (HTMLObject *o)
{
	return HTML_TEXT_MASTER (o)->prefWidth;
}

static HTMLFitType
fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun,
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
	text_slave = html_text_slave_new (textmaster, 0, textmaster->strLen);

	text_slave->next = o->next;
	text_slave->parent = o->parent;
	text_slave->prev = o;

	if (o->next != NULL)
		o->next->prev = text_slave;

	o->next = text_slave;

	return HTML_FIT_COMPLETE;
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
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
}

void
html_text_master_init (HTMLTextMaster *master,
		       HTMLTextMasterClass *klass,
		       gchar *text,
		       HTMLFont *font,
		       HTMLPainter *painter)
{
	HTMLText* html_text;
	HTMLObject *object;
	gint runWidth;
	gchar *textPtr;

	html_text = HTML_TEXT (master);
	object = HTML_OBJECT (master);

	html_text_init (html_text, HTML_TEXT_CLASS (klass),
			text, font, painter);

	object->width = 0;	/* FIXME why? */
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);
	
	master->prefWidth = html_font_calc_width (font, text, -1);
	master->minWidth = 0;

	runWidth = 0;
	textPtr = text;

	while (*textPtr) {
		if (*textPtr != ' ') {
			runWidth += html_font_calc_width (font, textPtr, 1);
		} else {
			if (runWidth > master->minWidth)
				master->minWidth = runWidth;
			runWidth = 0;
		}
		textPtr++;
	}
	
	if (runWidth > master->minWidth)
		master->minWidth = runWidth;

	master->strLen = strlen (text);
}

HTMLObject *
html_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter)
{
	HTMLTextMaster *master;

	master = g_new (HTMLTextMaster, 1);
	html_text_master_init (master, &html_text_master_class,
			       text, font, painter);

	return HTML_OBJECT (master);
}
