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
#ifndef _HTMLTEXTMASTER_H_
#define _HTMLTEXTMASTER_H_

#include "htmlobject.h"
#include "htmltext.h"

typedef struct _HTMLTextMaster HTMLTextMaster;

#define HTML_TEXT_MASTER(x) ((HTMLTextMaster *)(x))

struct _HTMLTextMaster {
	HTMLText parent;

	gint minWidth;
	gint prefWidth;
	gint strLen;
};

HTMLFitType html_text_master_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft);
HTMLObject *html_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter);


#endif /* _HTMLTEXTMASTER_H_ */
