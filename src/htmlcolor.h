/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#ifndef _HTMLCOLOR_H_
#define _HTMLCOLOR_H_

typedef struct _HTMLColorStack HTMLColorStack;

struct _HTMLColorStack {
	GList *list;
};

HTMLColorStack *html_color_stack_new     (void);
void            html_color_stack_destroy (HTMLColorStack *cs);
void            html_color_stack_push    (HTMLColorStack *cs, GdkColor *c);
void            html_color_stack_clear   (HTMLColorStack *cs);
GdkColor       *html_color_stack_top     (HTMLColorStack *cs);
GdkColor       *html_color_stack_pop     (HTMLColorStack *cs);

#endif /* _HTMLCOLOR_H_ */
