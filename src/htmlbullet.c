/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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
#include <gdk/gdk.h>
#include "htmlbullet.h"


HTMLBulletClass html_bullet_class;


/* HTMLObject methods.  */

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      HTMLCursor *cursor,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	gint xp, yp;

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	yp = o->y + ty - 9;
	xp = o->x + tx + 2;

	html_painter_set_pen (p, HTML_BULLET (o)->color);

	switch (HTML_BULLET (o)->level) {
	case 1:
		html_painter_draw_ellipse (p, xp, yp, 7, 7);
		break;
	default:
		html_painter_draw_rect (p, xp, yp, 7, 7);
		g_print ("unknown bullet level: %d\n", HTML_BULLET (o)->level);
	}
}


void
html_bullet_type_init (void)
{
	html_bullet_class_init (&html_bullet_class, HTML_TYPE_BULLET);
}

void
html_bullet_class_init (HTMLBulletClass *klass,
			HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	object_class->draw = draw;
}

void
html_bullet_init (HTMLBullet *bullet,
		  HTMLBulletClass *klass,
		  gint height,
		  gint level,
		  GdkColor *color)
{
	HTMLObject *object;

	object = HTML_OBJECT (bullet);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	object->ascent = height;
	object->descent = 0;
	object->width = 14;

	bullet->level = level;
	bullet->color = color;
}

HTMLObject *
html_bullet_new (gint height, gint level, GdkColor *color)
{
	HTMLBullet *bullet;

	bullet = g_new0 (HTMLBullet, 1);
	html_bullet_init (bullet, &html_bullet_class, height, level, color);

	return HTML_OBJECT (bullet);
}
