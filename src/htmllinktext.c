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
#include "htmllinktext.h"


HTMLLinkTextClass html_link_text_class;

static HTMLTextClass *parent_class;


/* HTMLObject methods.  */

static void
destroy (HTMLObject *object)
{
	HTMLLinkText *link_text;

	link_text = HTML_LINK_TEXT (object);
	g_free (link_text->url);
	g_free (link_text->target);

	(* HTML_OBJECT_CLASS (parent_class)->destroy) (object);
}

static const gchar *
get_url (HTMLObject *object)
{
	return HTML_LINK_TEXT (object)->url;
}

static const gchar *
get_target (HTMLObject *object)
{
	return HTML_LINK_TEXT (object)->target;
}


void
html_link_text_type_init (void)
{
	html_link_text_class_init (&html_link_text_class, HTML_TYPE_LINKTEXT);
}

void
html_link_text_class_init (HTMLLinkTextClass *klass,
			   HTMLType type)
{
	HTMLObjectClass *object_class;
	HTMLTextClass *text_class;

	object_class = HTML_OBJECT_CLASS (klass);
	text_class = HTML_TEXT_CLASS (klass);

	html_text_class_init (text_class, type);

	object_class->destroy = destroy;
	object_class->get_url = get_url;
	object_class->get_target = get_target;

	parent_class = &html_text_class;
}

void
html_link_text_init (HTMLLinkText *link_text_object,
		     HTMLLinkTextClass *klass,
		     gchar *text,
		     HTMLFont *font,
		     HTMLPainter *painter,
		     const gchar *url,
		     const gchar *target)
{
	HTMLText *text_object;

	text_object = HTML_TEXT (link_text_object);
	html_text_init (text_object, HTML_TEXT_CLASS (klass),
			text, font, painter);

	link_text_object->url = g_strdup (url);
	link_text_object->target = g_strdup (target);
}

HTMLObject *
html_link_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter,
		    const gchar *url, const gchar *target)
{
	HTMLLinkText *link_text_object;

	link_text_object = g_new (HTMLLinkText, 1);

	printf ("%s (", __FUNCTION__);
	if (url != NULL)
		printf ("url = `%s', ", url);
	if (target != NULL)
		printf ("target = `%s'", url);
	printf (")\n");

	html_link_text_init (link_text_object, &html_link_text_class,
			     text, font, painter, url, target);

	return HTML_OBJECT (link_text_object);
}
