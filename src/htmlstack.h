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
#ifndef _HTMLSTACK_H_
#define _HTMLSTACK_H_

typedef struct _HTMLStackElement HTMLStackElement;

#include "htmlengine.h"
#include "htmlobject.h"

typedef void (*HTMLBlockFunc)(HTMLEngine *e, HTMLObject *clue, HTMLStackElement *el);



struct _HTMLStackElement {
	HTMLBlockFunc exitFunc;

	gint id;
	gint level;
	gint miscData1;
	gint miscData2;
	HTMLStackElement *next;
};

HTMLStackElement *html_stack_element_new (gint id, gint level, HTMLBlockFunc exitFunc, gint miscData1, gint miscData2, HTMLStackElement *next);

#endif /* _HTMLSTACK_H_ */
