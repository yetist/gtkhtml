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
#ifndef _HTMLLIST_H_
#define _HTMLLIST_H_

#include "htmlobject.h"

typedef struct _HTMLList HTMLList;
typedef struct _HTMLListStack HTMLListStack;

typedef enum { Unordered, UnorderedPlain, Ordered, Menu, Dir } ListType;
typedef enum { Numeric = 0, LowAlpha, UpAlpha, LowRoman, UpRoman } ListNumType;

struct _HTMLList {
	ListType type;
	ListNumType numType;
	gint itemNumber;
};

struct _HTMLListStack {
	GList *list;
};

HTMLListStack *html_list_stack_new      (void);
void           html_list_stack_destroy  (HTMLListStack *ls);
gboolean       html_list_stack_is_empty (HTMLListStack *ls);
gint           html_list_stack_count    (HTMLListStack *ls);
void           html_list_stack_push     (HTMLListStack *ls, HTMLList *l);
void           html_list_stack_clear    (HTMLListStack *ls);
HTMLList      *html_list_stack_top      (HTMLListStack *ls);
HTMLList      *html_list_stack_pop      (HTMLListStack *ls);

HTMLList      *html_list_new     (ListType t, ListNumType nt);
void           html_list_destroy (HTMLList *list);

#endif /* _HTMLLIST_H_ */





