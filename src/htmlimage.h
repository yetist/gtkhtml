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
#ifndef _HTMLIMAGE_H_
#define _HTMLIMAGE_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include "htmlobject.h"
#include "htmlengine.h"

typedef struct _HTMLImage HTMLImage;

#define HTML_IMAGE(x) ((HTMLImage *)(x))

struct _HTMLImage {
	HTMLObject parent;

	gboolean predefinedWidth;
	gboolean predefinedHeight;
	gchar *url;
	gint border;
	HTMLEngine *engine;

	GdkPixbuf *pixmap;
        GdkPixbufLoader *loader;
};

HTMLObject *html_image_new (HTMLEngine *e, gchar *filename, gint max_width, gint width, gint height,
			    gint percent, gint border);

#endif /* _HTMLIMAGE_H_ */
