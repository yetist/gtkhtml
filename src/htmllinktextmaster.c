/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999 Helix Code, Inc.

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

static HTMLTextMasterClass *parent_class;


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
	return HTML_LINK_TEXT_MASTER (object)->url;
}

static const gchar *
get_target (HTMLObject *object)
{
	return HTML_LINK_TEXT_MASTER (object)->target;
}

static HTMLText *
split (HTMLText *self,
       guint offset)
{
	HTMLTextMaster *master;
	HTMLLinkTextMaster *link_master;
	HTMLObject *new;
	gchar *s;

	master = HTML_TEXT_MASTER (self);
	link_master = HTML_LINK_TEXT_MASTER (self);

	if (offset >= HTML_TEXT (self)->text_len || offset == 0)
		return NULL;

	s = g_strdup (self->text + offset);

	new = html_link_text_master_new (s,
					 self->font_style,
					 &self->color,
					 link_master->url,
					 link_master->target);

	self->text = g_realloc (self->text, offset + 1);
	self->text[offset] = '\0';
	HTML_TEXT (self)->text_len = offset;

	return HTML_TEXT (new);
}

static HTMLFontStyle
get_font_style (const HTMLText *text)
{
	HTMLFontStyle font_style;

	font_style = HTML_TEXT_CLASS (parent_class)->get_font_style (text);
	font_style = html_font_style_merge (font_style, HTML_FONT_STYLE_UNDERLINE);

	return font_style;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	if (! html_engine_save_output_string (state, "<A HREF=\"")
	    || ! html_engine_save_output_string (state, HTML_LINK_TEXT_MASTER (self)->url)
	    || ! html_engine_save_output_string (state, "\">"))
		return FALSE;

	if (! HTML_OBJECT_CLASS (&html_text_class)->save (self, state))
		return FALSE;

	if (! html_engine_save_output_string (state, "</A>"))
		return FALSE;

	return TRUE;
}


void
html_link_text_master_type_init (void)
{
	html_link_text_master_class_init (&html_link_text_master_class,
					  HTML_TYPE_LINKTEXTMASTER,
					  sizeof (HTMLLinkTextMaster));
}

void
html_link_text_master_class_init (HTMLLinkTextMasterClass *klass,
				  HTMLType type,
				  guint size)
{
	HTMLObjectClass *object_class;
	HTMLTextClass *text_class;
	HTMLTextMasterClass *text_master_class;

	object_class = HTML_OBJECT_CLASS (klass);
	text_class = HTML_TEXT_CLASS (klass);
	text_master_class = HTML_TEXT_MASTER_CLASS (klass);

	html_text_master_class_init (text_master_class, type, size);

	object_class->destroy = destroy;
	object_class->get_url = get_url;
	object_class->get_target = get_target;
	object_class->save = save;

	text_class->split = split;
	text_class->get_font_style = get_font_style;

	parent_class = &html_text_master_class;
}

void
html_link_text_master_init (HTMLLinkTextMaster *link_text_master_object,
			    HTMLLinkTextMasterClass *klass,
			    gchar *text,
			    HTMLFontStyle font_style,
			    const GdkColor *color,
			    const gchar *url,
			    const gchar *target)
{
	HTMLTextMaster *text_master_object;

	text_master_object = HTML_TEXT_MASTER (link_text_master_object);
	html_text_master_init (text_master_object,
			       HTML_TEXT_MASTER_CLASS (klass),
			       text,
			       font_style,
			       color);

	link_text_master_object->url = g_strdup (url);
	link_text_master_object->target = g_strdup (target);
}

HTMLObject *
html_link_text_master_new (gchar *text,
			   HTMLFontStyle font_style,
			   const GdkColor *color,
			   const gchar *url,
			   const gchar *target)
{
	HTMLLinkTextMaster *link_text_master_object;

	link_text_master_object = g_new (HTMLLinkTextMaster, 1);

	html_link_text_master_init (link_text_master_object,
				    &html_link_text_master_class,
				    text,
				    font_style,
				    color,
				    url,
				    target);

	return HTML_OBJECT (link_text_master_object);
}
