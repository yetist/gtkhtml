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


HTMLHSpaceClass html_hspace_class;


static void
draw (HTMLObject *o, HTMLPainter *p, HTMLCursor *cursor,
      gint x, gint y, gint width, gint height,
      gint tx, gint ty)
{
}

static gboolean
accepts_cursor (HTMLObject *self)
{
	return TRUE;
}


void
html_hspace_type_init (void)
{
	html_hspace_class_init (&html_hspace_class, HTML_TYPE_HSPACE);
}

void
html_hspace_class_init (HTMLHSpaceClass *klass,
			HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	/* FIXME destroy? */

	object_class->draw = draw;
	object_class->accepts_cursor = accepts_cursor;
}

void
html_hspace_init (HTMLHSpace *hspace,
		  HTMLHSpaceClass *klass,
		  HTMLFont *font,
		  gboolean hidden)
{
	HTMLObject *object;

	object = HTML_OBJECT (hspace);

	html_object_init (object, HTML_OBJECT_CLASS (klass));
	
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font) + 1;

	hspace->font = font;

	if (!hidden)
		object->width = html_font_calc_width (font, " ", -1);
	else
		object->width = 1;

	object->flags |= HTML_OBJECT_FLAG_SEPARATOR;
	object->flags &= ~HTML_OBJECT_FLAG_HIDDEN;
}

HTMLObject *
html_hspace_new (HTMLFont *font, gboolean hidden)
{
	HTMLHSpace *hspace;

	hspace = g_new0 (HTMLHSpace, 1);
	html_hspace_init (hspace, &html_hspace_class, font, hidden);

	return HTML_OBJECT (hspace);
}
