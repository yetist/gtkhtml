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
#ifndef HTMLELEMENT_H
#define HTMLELEMENT_H
#include "htmlobject.h"

#define HTML_ELEMENT(x) ((HTMLElement *)(x))
#define HTML_ELEMENT_CLASS(x) ((HTMLElementClass *)(x))

typedef struct _HTMLElement HTMLElement;
typedef struct _HTMLElementClass HTMLElementClass;

struct _HTMLElement {
	HTMLObject object;
	
	gchar *name;
	gchar *value;
	HTMLForm *form;
	GtkWidget *widget, *parent;

	gint abs_x, abs_y;
};
struct _HTMLElementClass {
	HTMLObjectClass object_class;

	void (*reset) (HTMLElement *element);
	gchar *(*encode) (HTMLElement *element);
};


extern HTMLElementClass html_element_class;


void   html_element_type_init      (void);
void   html_element_class_init     (HTMLElementClass *klass,
				    HTMLType          type,
				    guint             object_size);
void   html_element_init           (HTMLElement      *element,
				    HTMLElementClass *klass,
				    GtkWidget        *parent,
				    gchar            *name,
				    gchar            *value);
gchar *html_element_get_name       (HTMLElement      *element);
void   html_element_set_form       (HTMLElement      *element,
				    HTMLForm         *form);
void   html_element_reset          (HTMLElement      *element);
gchar *html_element_encode         (HTMLElement      *element);
gchar *html_element_encode_string  (gchar            *str);

#endif /* HTMLELEMENT_H */
