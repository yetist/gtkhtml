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

#include "htmlvspace.h"


HTMLVSpaceClass html_vspace_class;


void
html_vspace_type_init (void)
{
	html_vspace_class_init (&html_vspace_class, HTML_TYPE_VSPACE);
}

void
html_vspace_class_init (HTMLVSpaceClass *klass,
			HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);
}

void
html_vspace_init (HTMLVSpace *vspace,
		  HTMLVSpaceClass *klass,
		  gint space,
		  HTMLClearType clear)
{
	HTMLObject *object;

	object = HTML_OBJECT (vspace);

	html_object_init (object, HTML_OBJECT_CLASS (klass));
	
	object->ascent = space;
	object->descent = 0;
	object->width = 1;
	object->flags |= HTML_OBJECT_FLAG_NEWLINE;
	
	vspace->clear = clear;
}

HTMLObject *
html_vspace_new (gint space,
		 HTMLClearType clear)
{
	HTMLVSpace *vspace;

	vspace = g_new (HTMLVSpace, 1);
	html_vspace_init (vspace, &html_vspace_class, space, clear);

	return HTML_OBJECT (vspace);
}
