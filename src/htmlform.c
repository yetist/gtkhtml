/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML widget.

    Copyright (C) 2000 Jonas Borgström <jonas_b@bitsmart.com>

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

#include "htmlform.h"


HTMLForm *
html_form_new (gchar *_action, gchar *_method) {
	HTMLForm *new;
	
	new = g_new (HTMLForm, 1);
	new->action = g_strdup(_action);
	new->method = g_strdup(_method);

	new->elements = NULL;

	g_print("html_form_new action = '%s' method = '%s'\n", new->action, new->method);
	
	return new;
}

