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
#include <gdk/gdk.h>
#include "htmlbullet.h"

void html_bullet_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

HTMLObject *
html_bullet_new (gint height, gint level, GdkColor color)
{
	HTMLBullet *bullet = g_new0 (HTMLBullet, 1);
	HTMLObject *object = HTML_OBJECT (bullet);
	html_object_init (object, Bullet);

	/* HTMLObject functions */
	object->draw = html_bullet_draw;

	object->ascent = height;
	object->descent = 0;
	object->width = 14;

	bullet->level = level;
	
	return object;
}

void
html_bullet_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	gint xp, yp;

	if (y + height < o->y + o->ascent || y > o->y + o->descent)
		return;

	yp = o->y + ty - 9;
	xp = o->x + tx + 2;

	/* FIXME: should set correct color */

	switch (HTML_BULLET (o)->level) {
	case 1:
		html_painter_draw_ellipse (p, xp, yp, 7, 7);
		break;
	default:
		html_painter_draw_rect (p, xp, yp, 7, 7);
		g_print ("unknown bullet level: %d\n", HTML_BULLET (o)->level);
	}
}


