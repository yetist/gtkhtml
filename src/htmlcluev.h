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
#ifndef _HTMLCLUEV_H_
#define _HTMLCLUEV_H_

#include "htmlobject.h"
#include "htmlclue.h"

typedef struct _HTMLClueV HTMLClueV;

#define HTML_CLUEV(x) ((HTMLClueV *)(x))

struct _HTMLClueV {
	HTMLClue parent;
	
	/* fixme: htmlcluealigned */
	HTMLObject *alignLeftList;
	HTMLObject *alignRightList;
	gushort padding;
};

void       html_cluev_find_free_area (HTMLClue *clue, gint y, gint width, gint height, gint indent, gint *y_pos, gint *_lmargin, gint *_rmargin);
HTMLObject *html_cluev_new (int x, int y, int max_width, int percent);

void        html_cluev_set_max_width (HTMLObject *o, gint max_width);
void        html_cluev_reset (HTMLObject *clue);
void        html_cluev_calc_size (HTMLObject *clue, HTMLObject *parent);
gint        html_cluev_get_left_margin (HTMLClue *clue, gint y);
gint        html_cluev_get_right_margin (HTMLClue *o, gint y);
void        html_cluev_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

#endif /* _HTMLCLUEV_H_ */
