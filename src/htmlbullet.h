/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) Helix Code, Inc.

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
#ifndef _HTMLBULLET_H_
#define _HTMLBULLET_H_

#include <gdk/gdk.h>
#include "htmlobject.h"

#define HTML_BULLET(x) ((HTMLBullet *) (x))
#define HTML_BULLET_CLASS(x) ((HTMLBulletClass *) (x))

typedef struct _HTMLBullet HTMLBullet;
typedef struct _HTMLBulletClass HTMLBulletClass;

struct _HTMLBullet {
	HTMLObject parent;
	
	gchar level;
	GdkColor *color;
};

struct _HTMLBulletClass {
	HTMLObjectClass parent;
};


extern HTMLBulletClass html_bullet_class;


void html_bullet_type_init (void);
void html_bullet_class_init (HTMLBulletClass *klass, HTMLType type);
void html_bullet_init (HTMLBullet *bullet, HTMLBulletClass *klass, gint height, gint level, GdkColor *color);
HTMLObject *html_bullet_new (gint height, gint level, GdkColor *color);

#endif /* _HTMLBULLET_H_ */
