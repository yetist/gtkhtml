/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library
 *
 *  Copyright (C) 2000 Jonas Borgström <jonas_b@bitsmart.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/
#ifndef _HTMLBUTTON_H_
#define _HTMLBUTTON_H_

#include "htmltypes.h"
#include "htmlenums.h"
#include "htmlembedded.h"

#define HTML_BUTTON(x) ((HTMLButton *) (x))
#define HTML_BUTTON_CLASS(x) ((HTMLButtonClass *) (x))

struct _HTMLButton {
	HTMLEmbedded element;
	HTMLButtonType type;
	gint successful;
};

struct _HTMLButtonClass {
	HTMLEmbeddedClass element_class;
};


extern HTMLButtonClass html_button_class;


void        html_button_type_init   (void);
void        html_button_class_init  (HTMLButtonClass *klass,
				     HTMLType         type,
				     guint            object_size);
void        html_button_init        (HTMLButton      *button,
				     HTMLButtonClass *klass,
				     GtkWidget       *parent,
				     gchar           *name,
				     gchar           *value,
				     HTMLButtonType   type);
HTMLObject *html_button_new         (GtkWidget       *parent,
				     gchar           *name,
				     gchar           *value,
				     HTMLButtonType   type);

#endif /* _HTMLBUTTON_H_ */
