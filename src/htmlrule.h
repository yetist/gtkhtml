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
#ifndef _HTMLRULE_H_
#define _HTMLRULE_H_

#include "htmlobject.h"

typedef struct _HTMLRule HTMLRule;

#define HTML_RULE(x) ((HTMLRule *)(x))

struct _HTMLRule {
	HTMLObject parent;

	gint shade;
};

HTMLObject *html_rule_new (gint max_width, gint percent, gint size, gboolean shade);
gint        html_rule_calc_min_width (HTMLObject *o);
void        html_rule_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);
void        html_rule_set_max_width (HTMLObject *o, gint max_width);

#endif /* _HTMLRULE_H_ */
