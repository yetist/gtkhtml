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

#include "htmltext.h"
#include "htmllinktextmaster.h"


HTMLLinkTextMasterClass html_link_text_master_class;

static HTMLLinkTextMasterClass *parent_class;


/* HTMLObject methods.  */

static void
destroy (HTMLObject *object)
{
	HTMLLinkTextMaster *link_text_master;

	link_text_master = HTML_LINK_TEXT_MASTER (object);
	g_free (link_text_master->url);
	g_free (link_text_master->target);

	(* HTML_OBJECT_CLASS (parent_class)->destroy) (object);
}

static const gchar *
get_url (HTMLObject *object)
{
	printf ("%s: %s\n", __FUNCTION__, HTML_LINK_TEXT_MASTER (object)->url);
	return HTML_LINK_TEXT_MASTER (object)->url;
}

static const gchar *
get_target (HTMLObject *object)
{
	return HTML_LINK_TEXT_MASTER (object)->target;
}


void
html_link_text_master_type_init (void)
{
	html_link_text_master_class_init (&html_link_text_master_class,
					  HTML_TYPE_LINKTEXTMASTER);
}

void
html_link_text_master_class_init (HTMLLinkTextMasterClass *klass,
				  HTMLType type)
{
	HTMLObjectClass *object_class;
	HTMLTextMasterClass *text_master_class;

	object_class = HTML_OBJECT_CLASS (klass);
	text_master_class = HTML_TEXT_MASTER_CLASS (klass);

	html_text_master_class_init (text_master_class, type);

	object_class->destroy = destroy;
	object_class->get_url = get_url;
	object_class->get_target = get_target;

	parent_class = &html_link_text_master_class;
}

void
html_link_text_master_init (HTMLLinkTextMaster *link_text_master_object,
			    HTMLLinkTextMasterClass *klass,
			    gchar *text,
			    HTMLFont *font,
			    HTMLPainter *painter,
			    gchar *url,
			    gchar *target)
{
	HTMLTextMaster *text_master_object;

	text_master_object = HTML_TEXT_MASTER (link_text_master_object);
	html_text_master_init (text_master_object, HTML_TEXT_MASTER_CLASS (klass),
			       text, font, painter);

	link_text_master_object->url = url;
	link_text_master_object->target = target;
}

HTMLObject *
html_link_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter,
			   gchar *url, gchar *target)
{
	HTMLLinkTextMaster *link_text_master_object;

	link_text_master_object = g_new (HTMLLinkTextMaster, 1);

	html_link_text_master_init (link_text_master_object,
				    &html_link_text_master_class,
				    text, font, painter, url, target);

	return HTML_OBJECT (link_text_master_object);
}
