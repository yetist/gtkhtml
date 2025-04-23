/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
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
 *  along with this library; see the file COPYING.LIB.  If not, write to    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

#ifndef _HTMLFORM_H_
#define _HTMLFORM_H_

#include <gtk/gtk.h>
#include "htmlembedded.h"
#include "htmlhidden.h"

#define HTML_FORM(x) ((HTMLForm *) (x))

struct _HTMLForm {

	gchar *action;
	gchar *method;
	GList *elements;
	GList *hidden;

	/* Used by radio buttons */
	GHashTable *radio_group;

	HTMLEngine *engine;
};


HTMLForm *html_form_new (HTMLEngine *engine, const gchar *_action, const gchar *_method);
void html_form_add_element (HTMLForm *form, HTMLEmbedded *e);
void html_form_add_hidden (HTMLForm *form, HTMLHidden *h);
void html_form_remove_element (HTMLForm *form, HTMLEmbedded *e);
void html_form_submit (HTMLForm *form);
void html_form_reset (HTMLForm *form);
void html_form_destroy (HTMLForm *form);
void html_form_add_radio (HTMLForm *form, const gchar *name, GtkRadioButton *button);
void html_form_set_engine (HTMLForm *form, HTMLEngine *engine);

#endif /* _HTMLFORM_H_ */

