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
#include "htmlengine.h"
#include "htmlstack.h"


HTMLStackElement*
html_stack_element_new (gint id, gint level, HTMLBlockFunc exitFunc, 
			gint miscData1, gint miscData2, HTMLStackElement *next)
{
	HTMLStackElement *se;

	se = g_new0 (HTMLStackElement, 1);
	se->id = id;
	se->level = level;
	se->miscData1 = miscData1;
	se->miscData2 = miscData2;
	se->next = next;
	se->exitFunc = exitFunc;
	return se;
}
