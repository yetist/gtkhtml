/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   
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

#include "htmlengine-cutbuffer.h"
#include "htmlengine-edit-copy.h"
#include "htmltext.h"


static void
copy_forall (HTMLObject *obj,
	     gpointer closure)
{
	HTMLObject *selection;
	GList **p;

	if (! obj->selected)
		return;

	p = (GList **) closure;

	selection = html_object_get_selection (obj);
	if (selection == NULL)
		return;

	g_print ("Adding object %p [%s] to selection.\n",
		 selection, html_type_name (HTML_OBJECT_TYPE (selection)));
	if (html_object_is_text (selection))
		g_print ("\ttext `%s'\n", HTML_TEXT (selection)->text);

	*p = g_list_prepend (*p, selection);
}

void
html_engine_copy (HTMLEngine *engine)
{
	GList *buffer;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->active_selection);
	g_return_if_fail (engine->clue != NULL);

	g_warning ("Copy!");

	html_engine_cut_buffer_destroy (engine, engine->cut_buffer);

	buffer = NULL;

	html_object_forall (engine->clue, copy_forall, &buffer);

	engine->cut_buffer = g_list_reverse (buffer);
}
