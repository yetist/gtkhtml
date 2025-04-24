/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library
 *
 *  Copyright (C) 2000 Jonas Borgström <jonas_b@bitsmart.com>
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
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
#ifndef _HTMLTEXTAREA_H_
#define _HTMLTEXTAREA_H_

#include <gtk/gtk.h>
#include "htmlembedded.h"

#define HTML_TEXTAREA(x) ((HTMLTextArea *) (x))
#define HTML_TEXTAREA_CLASS(x) ((HTMLTextAreaClass *) (x))

struct _HTMLTextArea {
	HTMLEmbedded element;

	GtkWidget *text;
	GtkTextBuffer *buffer;

	gchar *default_text;
};

struct _HTMLTextAreaClass {
	HTMLEmbeddedClass element_class;
};


extern HTMLTextAreaClass html_textarea_class;


void        html_textarea_type_init   (void);
void        html_textarea_class_init  (HTMLTextAreaClass *klass,
				       HTMLType           type,
				       guint              object_size);
void        html_textarea_init        (HTMLTextArea      *ti,
				       HTMLTextAreaClass *klass,
				       GtkWidget         *parent,
				       gchar             *name,
				       gint               r,
				       gint               c);
HTMLObject *html_textarea_new         (GtkWidget         *parent,
				       gchar             *name,
				       gint               r,
				       gint               c);
void        html_textarea_set_text    (HTMLTextArea      *textarea,
				       gchar             *text);

#endif /* _HTMLTEXTAREA_H_ */
