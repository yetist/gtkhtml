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
#ifndef _HTMLRADIO_H_
#define _HTMLRADIO_H_

#include "htmlembedded.h"

#define HTML_RADIO(x) ((HTMLRadio *) (x))
#define HTML_RADIO_CLASS(x) ((HTMLRadioClass *) (x))

struct _HTMLRadio {
	HTMLEmbedded element;
	gint default_checked;
};

struct _HTMLRadioClass {
	HTMLEmbeddedClass element_class;
};


extern HTMLRadioClass html_radio_class;


void        html_radio_type_init   (void);
void        html_radio_class_init  (HTMLRadioClass *klass,
				    HTMLType        type,
				    guint           object_size);
void        html_radio_init        (HTMLRadio      *radio,
				    HTMLRadioClass *klass,
				    GtkWidget      *parent,
				    gchar          *name,
				    gchar          *value,
				    gboolean        checked,
				    HTMLForm       *form);
HTMLObject *html_radio_new         (GtkWidget      *parent,
				    gchar          *name,
				    gchar          *value,
				    gboolean        checked,
				    HTMLForm       *form);

#endif /* _HTMLRADIO_H_ */
