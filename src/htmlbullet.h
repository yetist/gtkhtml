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
#ifndef _HTMLBULLET_H_
#define _HTMLBULLET_H_

#include <gdk/gdk.h>
#include "htmlobject.h"

typedef struct _HTMLBullet HTMLBullet;

#define HTML_BULLET(x) ((HTMLBullet *)(x))

struct _HTMLBullet {
	HTMLObject parent;
	
	gchar level;
	GdkColor *color;
};

HTMLObject *html_bullet_new (gint height, gint level, GdkColor color);

#endif /* _HTMLBULLET_H_ */
