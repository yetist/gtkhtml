/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
   Copyright (C) 1997 Martin Jones (mjones@kde.org)
	     (C) 1997 Torben Weis (weis@kde.org)
	     (C) 1999 Ettore Perazzoli (ettore@gnu.org)

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

#include "htmlanchor.h"


HTMLAnchorClass html_anchor_class;
static HTMLObjectClass *parent_class = NULL;


/* HTMLObject methods.  */

static void
destroy (HTMLObject *object)
{
	HTMLAnchor *anchor;

	anchor = HTML_ANCHOR (object);

	g_string_free (anchor->name, TRUE);
}

static void
set_max_ascent (HTMLObject *object,
		gint a)
{
	object->y -= a;
}


void
html_anchor_type_init (void)
{
	html_anchor_class_init (&html_anchor_class, HTML_TYPE_ANCHOR);
}

void
html_anchor_class_init (HTMLAnchorClass *klass,
			HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	object_class->destroy = destroy;
	object_class->set_max_ascent = set_max_ascent;

	parent_class = &html_object_class;
}

void
html_anchor_init (HTMLAnchor *anchor,
		  HTMLAnchorClass *klass,
		  const gchar *name)
{
	html_object_init (HTML_OBJECT (anchor), HTML_OBJECT_CLASS (klass));

	anchor->name = g_string_new (name);
}

HTMLObject *
html_anchor_new (const gchar *name)
{
	HTMLAnchor *anchor;

	anchor = g_new (HTMLAnchor, 1);
	html_anchor_init (anchor, &html_anchor_class, name);

	return HTML_OBJECT (anchor);
}


const gchar *
html_anchor_get_name (HTMLAnchor *anchor)
{
	return anchor->name->str;
}
