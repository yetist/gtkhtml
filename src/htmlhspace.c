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
#include "htmlhspace.h"

void
html_hspace_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLHSpace *hspace = HTML_HSPACE (o);

	html_painter_set_font (p, hspace->font);

	/* FIXME: Sane Check */
	
	html_painter_draw_text (p, o->x + tx, o->y + ty, " ", 1);
}

HTMLObject *
html_hspace_new (HTMLFont *font, HTMLPainter *painter, gboolean hidden)
{
	HTMLHSpace *hspace = g_new0 (HTMLHSpace, 1);
	HTMLObject *object = HTML_OBJECT (hspace);
	html_object_init (object, HSpace);
	
	/* HTMLObject functions */
	object->draw = html_hspace_draw;

	object->ObjectType = HSpace;
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font) + 1;

	hspace->font = font;

	if (!hidden)
		object->width = html_font_calc_width (font, " ", -1);
	else
		object->width = 0;

	object->flags |= Separator;
	object->flags &= ~Hidden;

	return object;
}
