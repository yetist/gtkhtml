/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.
    Authors:           Radek Doulik (rodo@helixcode.com)

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

#include <config.h>
#include <htmlengine.h>
#include <htmllinktextmaster.h>
#include <htmlengine-edit-copy.h>
#include <htmlengine-edit-cut.h>
#include <htmlengine-edit-paste.h>
#include "popup.h"
#include "image.h"
#include "link.h"

static void
copy (GtkWidget *mi, GtkHTMLControlData *cd)
{
	html_engine_copy (cd->html->engine);
}

static void
cut (GtkWidget *mi, GtkHTMLControlData *cd)
{
	html_engine_cut (cd->html->engine, TRUE);
}

static void
paste (GtkWidget *mi, GtkHTMLControlData *cd)
{
	html_engine_paste (cd->html->engine, TRUE);
}

static void
remove_link (GtkWidget *mi, GtkHTMLControlData *cd)
{
	html_link_text_master_to_text (HTML_LINK_TEXT_MASTER (cd->obj), cd->html->engine);
}

static void
prop (GtkWidget *mi, GtkHTMLControlData *cd)
{
	printf ("prop\n");

	switch (HTML_OBJECT_TYPE (cd->obj)) {
	case HTML_TYPE_RULE:
		rule_edit (cd);
		break;
	case HTML_TYPE_IMAGE:
		image_edit (cd, HTML_IMAGE (cd->obj));
		break;
       	case HTML_TYPE_LINKTEXTMASTER:
		link_edit (cd, HTML_LINK_TEXT_MASTER (cd->obj));
		break;
	default:
	}
}

#define ADD_ITEM(l,f) \
		menuitem = gtk_menu_item_new_with_label (_(l)); \
		gtk_menu_append (GTK_MENU (menu), menuitem); \
		gtk_widget_show (menuitem); \
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (f), cd); \
                items++;

#define ADD_SEP \
                menuitem = gtk_menu_item_new (); \
                gtk_menu_append (GTK_MENU (menu), menuitem); \
                gtk_widget_show (menuitem)

gint
popup_show (GtkHTMLControlData *cd, GdkEventButton *event)
{
	HTMLEngine *e = cd->html->engine;
	GtkWidget *menu;
	GtkWidget *menuitem;
	guint items = 0;

	printf ("popup\n");

	menu = gtk_menu_new ();

	if (e->active_selection) {
		ADD_ITEM ("Copy", copy);
		ADD_ITEM ("Cut",  cut);
	}
	if (e->cut_buffer) {
		ADD_ITEM ("Paste",  paste);
	}

	if (cd->obj) {
		gchar *text = NULL;

		switch (HTML_OBJECT_TYPE (cd->obj)) {
		case HTML_TYPE_RULE:
			text = N_("Rule...");
			break;
		case HTML_TYPE_IMAGE:
			text = N_("Image...");
			break;
		case HTML_TYPE_LINKTEXTMASTER:
			text = N_("Link...");
			break;
		default:
		}
		if (text) {
			if (items) {
				ADD_SEP;
			}
			ADD_ITEM (text, prop);
			switch (HTML_OBJECT_TYPE (cd->obj)) {
			
			case HTML_TYPE_LINKTEXTMASTER:
				ADD_ITEM (_("Remove link"), remove_link);
			default:
			}
		}
	}
	gtk_widget_show (menu);
	if (items)
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	gtk_widget_unref (menu);

	return (items > 0);
}
