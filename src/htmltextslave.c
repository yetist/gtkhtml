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
#include "htmlcursor.h"


HTMLTextSlaveClass html_text_slave_class;


/* Split this TextSlave at the specified offset.  */
static void
split (HTMLTextSlave *slave, gshort offset)
{
	HTMLObject *obj;
	HTMLObject *new;

	g_return_if_fail (offset >= 0 && offset < slave->posLen);

	obj = HTML_OBJECT (slave);

	new = html_text_slave_new (slave->owner,
				   slave->posStart + offset,
				   slave->posLen - offset);

	new->next = obj->next;
	new->prev = obj;
	new->parent = obj->parent;

	if (obj->next != NULL)
		obj->next->prev = new;

	obj->next = new;
}

/* Split this TextSlave at the first newline character.  */
static void
split_at_newline (HTMLTextSlave *slave)
{
	const gchar *text;
	const gchar *p;

	text = HTML_TEXT (slave)->text + slave->posStart;

	p = memchr (text, '\n', slave->posLen);
	if (p == NULL)
		return;

	split (slave, p - text + 1);
}


/* HTMLObject methods.  */

static void
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	HTMLText *owner;
	HTMLTextSlave *slave;
	HTMLFontStyle font_style;

	slave = HTML_TEXT_SLAVE (self);
	owner = HTML_TEXT (slave->owner);
	font_style = html_text_get_font_style (owner);

	self->ascent = html_painter_calc_ascent (painter, font_style);
	self->descent = html_painter_calc_descent (painter, font_style);

	self->width = html_painter_calc_text_width (painter,
						    owner->text + slave->posStart,
						    slave->posLen,
						    font_style);
}

static HTMLFitType
fit_line (HTMLObject *o,
	  HTMLPainter *painter,
	  gboolean startOfLine,
	  gboolean firstRun,
	  gint widthLeft)
{
	HTMLTextSlave *textslave;
	HTMLTextMaster *textmaster;
	HTMLText *ownertext;
	HTMLObject *next_obj;
	HTMLFitType return_value;
	HTMLFontStyle font_style;
	gint newLen;
	gint newWidth;
	gchar *splitPtr;
	gchar *text;

	textslave = HTML_TEXT_SLAVE (o);
	textmaster = HTML_TEXT_MASTER (textslave->owner);
	ownertext = HTML_TEXT (textmaster);

	font_style = html_text_get_font_style (ownertext);

	next_obj = o->next;
	text = ownertext->text;

	/* Remove following existing slaves.  */

	if (next_obj != NULL
	    && (HTML_OBJECT_TYPE (next_obj) == HTML_TYPE_TEXTSLAVE)) {
		do {
			o->next = next_obj->next;
			html_object_destroy (next_obj);
			next_obj = o->next;
			if (next_obj != NULL)
				next_obj->prev = o;
		} while (next_obj && (HTML_OBJECT_TYPE (next_obj)
				      == HTML_TYPE_TEXTSLAVE));
		textslave->posLen = HTML_TEXT (textslave->owner)->text_len - textslave->posStart;
	}

	split_at_newline (HTML_TEXT_SLAVE (o));

	text += textslave->posStart;

	o->width = html_painter_calc_text_width (painter, text, textslave->posLen, font_style);
	if (o->width <= widthLeft || textslave->posLen <= 1 || widthLeft < 0) {
		/* Text fits completely */
		if (o->next == NULL
		    || text[textslave->posLen - 1] == '\n'
		    || (o->next->flags & (HTML_OBJECT_FLAG_SEPARATOR
					  | HTML_OBJECT_FLAG_NEWLINE))) {
			return_value = HTML_FIT_COMPLETE;
			goto done;
		}

		/* Text is followed by more text...break it before the last word */
		splitPtr = rindex (text + 1, ' ');
		if (!splitPtr) {
			return_value = HTML_FIT_COMPLETE;
			goto done;
		}
	} else {
		splitPtr = index (text + 1, ' ');
	}
	
	if (splitPtr) {
		newLen = splitPtr - text + 1;
		newWidth = html_painter_calc_text_width (painter, text, newLen, font_style);
		if (newWidth > widthLeft) {
			/* Splitting doesn't make it fit */
			splitPtr = 0;
		} else {
			gint extraLen;
			gint extraWidth;

			for (;;) {
				gchar *splitPtr2 = index (splitPtr + 1, ' ');
				if (!splitPtr2)
					break;
				extraLen = splitPtr2 - splitPtr;
				extraWidth = html_painter_calc_text_width (painter, splitPtr, extraLen,
									   font_style);
				if (extraWidth + newWidth <= widthLeft) {
					/* We can break on the next separator cause it still fits */
					newLen += extraLen;
					newWidth += extraWidth;
					splitPtr = splitPtr2;
				} else {
					/* Using this separator would over-do it */
					break;
				}
			}
		}
	} else {
		newLen = textslave->posLen;
		newWidth = o->width;
	}
	
	if (!splitPtr) {
		/* No separator available */
		if (firstRun == FALSE) {
			/* Text does not fit, wait for next line */
			return_value = HTML_FIT_NONE;
			goto done;
		}

		/* Text doesn't fit, too bad. 
		   newLen & newWidth are valid */
	}

	if (textslave->posLen - newLen > 0)
		split (textslave, newLen);

	textslave->posLen = newLen;

	o->width = newWidth;
	o->ascent = html_painter_calc_ascent (painter, font_style);
	o->descent = html_painter_calc_descent (painter, font_style);

	return_value = HTML_FIT_PARTIAL;

 done:
#ifdef HTML_TEXT_SLAVE_DEBUG
	/* FIXME */
	{
		gint i;

		printf ("Split text");
		switch (return_value) {
		case HTMLPartialFit:
			printf (" (Partial): `");
			break;
		case HTMLNoFit:
			printf (" (NoFit): `");
			break;
		case HTMLCompleteFit:
			printf (" (Complete): `");
			break;
		}

		for (i = 0; i < textslave->posLen; i++)
			putchar (ownertext->text[textslave->posStart + i]);

		printf ("'\n");
	}
#endif

	return return_value;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      HTMLCursor *cursor,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLTextSlave *textslave;
	HTMLText *ownertext;
	HTMLFontStyle font_style;

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	textslave = HTML_TEXT_SLAVE (o);
	ownertext = HTML_TEXT (textslave->owner);
	font_style = html_text_get_font_style (ownertext);

	html_painter_set_font_style (p, font_style);
	html_painter_set_pen (p, html_text_get_color (ownertext, p));

	html_painter_draw_text (p,
				o->x + tx, o->y + ty, 
				ownertext->text + textslave->posStart,
				textslave->posLen);

#ifdef HTML_TEXT_SLAVE_DEBUG
	{
		gchar *s = g_strndup (ownertext->text + textslave->posStart,
				      textslave->posLen);
		g_print ("(%d, %d) `%s'\n", o->x + tx, o->y + ty, s);
		g_free (s);
	}
#endif

#if 0
	if (cursor != NULL
	    && cursor->object == HTML_OBJECT (textslave->owner)
	    && ((cursor->offset >= textslave->posStart
		 && cursor->offset < textslave->posStart + textslave->posLen)
		|| cursor->offset == HTML_TEXT (textslave->owner)->text_len)) {
		gint x_offset;

		x_offset = gdk_text_width (ownertext->font->gdk_font,
					   ownertext->text + textslave->posStart,
					   cursor->offset - textslave->posStart);

		html_painter_draw_cursor (p,
					  o->x + tx + x_offset, o->y + ty,
					  o->ascent, o->descent);
	}
#endif
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


void
html_text_slave_type_init (void)
{
	html_text_slave_class_init (&html_text_slave_class,
				    HTML_TYPE_TEXTSLAVE);
}

void
html_text_slave_class_init (HTMLTextSlaveClass *klass,
			    HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	object_class->draw = draw;
	object_class->calc_size = calc_size;
	object_class->fit_line = fit_line;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->get_url = get_url;
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
}

HTMLObject *
html_text_slave_new (HTMLTextMaster *owner, gint posStart, gint posLen)
{
	HTMLTextSlave *slave;

	slave = g_new (HTMLTextSlave, 1);
	html_text_slave_init (slave, &html_text_slave_class, owner, posStart, posLen);

	return HTML_OBJECT (slave);
}
