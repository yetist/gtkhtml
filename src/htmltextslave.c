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
#include <string.h>
#include "htmltextslave.h"


HTMLTextSlaveClass html_text_slave_class;


static HTMLFitType
fit_line (HTMLObject *o,
	  gboolean startOfLine,
	  gboolean firstRun,
	  gint widthLeft)
{
	gint newLen;
	gint newWidth;
	gchar *splitPtr;
	HTMLTextSlave *textslave = HTML_TEXT_SLAVE (o);
	HTMLTextMaster *textmaster = HTML_TEXT_MASTER (textslave->owner);
	HTMLText *ownertext = HTML_TEXT (textmaster);

	gchar *text = ownertext->text;

	/* Remove existing slaves */
	HTMLObject *next_obj = o->next;

	HTMLFitType return_value;

	if (next_obj && (HTML_OBJECT_TYPE (next_obj) == HTML_TYPE_TEXTSLAVE)) {
		do {
			o->next = next_obj->next;
			g_free (next_obj); /* FIXME FIXME FIXME */
			next_obj = o->next;
		} while (next_obj && (HTML_OBJECT_TYPE (next_obj)
				      == HTML_TYPE_TEXTSLAVE));
		textslave->posLen = textslave->owner->strLen - textslave->posStart;
	}
	
	if (startOfLine && (text[textslave->posStart] == ' ') && (widthLeft >= 0)) {
		/* Skip leading space */
		textslave->posStart++;
		textslave->posLen--;
	}
	text += textslave->posStart;
	
	o->width = html_font_calc_width (ownertext->font, text, textslave->posLen);
	if ((o->width <= widthLeft) || (textslave->posLen <= 1) || (widthLeft < 0)) {
		/* Text fits completely */
		if (!o->next || (o->next->flags & (HTML_OBJECT_FLAG_SEPARATOR
						   | HTML_OBJECT_FLAG_NEWLINE))) {
			return_value = HTMLCompleteFit;
			goto done;
		}

		/* Text is followed by more text...break it before the last word */
		splitPtr = rindex (text + 1, ' ');
		if (!splitPtr) {
			return_value = HTMLCompleteFit;
			goto done;
		}
	}
	else {
		splitPtr = index (text + 1, ' ');
	}
	
	if (splitPtr) {
		newLen = splitPtr - text;
		newWidth = html_font_calc_width (ownertext->font, text, newLen);
		if (newWidth > widthLeft) {
			/* Splitting doesn't make it fit */
			splitPtr = 0;
		}
		else {
			gint extraLen;
			gint extraWidth;

			for (;;) {
				gchar *splitPtr2 = index (splitPtr + 1, ' ');
				if (!splitPtr2)
					break;
				extraLen = splitPtr2 - splitPtr;
				extraWidth = html_font_calc_width (ownertext->font, splitPtr, extraLen);
				if (extraWidth + newWidth <= widthLeft) {
					/* We can break on the next separator cause it still fits */
					newLen += extraLen;
					newWidth += extraWidth;
					splitPtr = splitPtr2;
				}
				else {
					/* Using this separator would over-do it */
					break;
				}
			}
		}
	}
	else {
		newLen = textslave->posLen;
		newWidth = o->width;
	}
	
	if (!splitPtr) {
		/* No separator available */
		if (firstRun == FALSE) {
			/* Text does not fit, wait for next line */
			return_value = HTMLNoFit;
			goto done;
		}

		/* Text doesn't fit, too bad. 
		   newLen & newWidth are valid */
	}

	if (textslave->posLen - newLen > 0) {
		/* Move remaining text to our text-slave */
		HTMLObject *textSlave = html_text_slave_new (textmaster, textslave->posStart + newLen, textslave->posLen - newLen);

		textSlave->next = o->next;
		o->next = textSlave;
	}

	textslave->posLen = newLen;
	o->width = newWidth;

	return_value = HTMLPartialFit;

 done:
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

	return return_value;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLTextSlave *textslave = HTML_TEXT_SLAVE (o);
	HTMLText *ownertext = HTML_TEXT (textslave->owner);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	html_painter_set_pen (p, ownertext->font->textColor);
	html_painter_set_font (p, ownertext->font);

	html_painter_draw_text (p, o->x + tx, o->y + ty, 
				&(ownertext->text[textslave->posStart]), 
				textslave->posLen);

	{
		gchar *s = g_strndup (ownertext->text + textslave->posStart,
				      textslave->posLen);
		g_print ("(%d, %d) `%s'\n", o->x + tx, o->y + ty, s);
		g_free (s);
	}
}

static gint
calc_min_width (HTMLObject *o)
{
	return 0;
}

static gint
calc_perferred_width (HTMLObject *o)
{
	return 0;
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
	object_class->fit_line = fit_line;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_perferred_width;
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
	
	object->width = html_font_calc_width (owner_text->font,
					      owner_text->text + posStart,
					      posLen);
}

HTMLObject *
html_text_slave_new (HTMLTextMaster *owner, gint posStart, gint posLen)
{
	HTMLTextSlave *slave;

	slave = g_new (HTMLTextSlave, 1);
	html_text_slave_init (slave, &html_text_slave_class, owner,
			      posStart, posLen);

	return HTML_OBJECT (slave);
}

