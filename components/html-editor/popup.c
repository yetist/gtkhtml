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
#include <htmlengine-edit-insert.h>
#include "popup.h"
#include "properties.h"
#include "paragraph.h"
#include "image.h"
#include "text.h"
#include "link.h"

static void
insert_link (GtkWidget *mi, GtkHTMLControlData *cd)
{
	link_insert (cd);
}

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
	if (cd->html->engine->active_selection)
		html_engine_remove_link (cd->html->engine);
        else
		html_engine_remove_link_object (cd->html->engine, cd->obj);
}

static void
prop (GtkWidget *mi, GtkHTMLControlData *cd)
{
	printf ("prop\n");

	switch (HTML_OBJECT_TYPE (cd->obj)) {
	case HTML_TYPE_RULE:
		rule_edit (cd, HTML_RULE (cd->obj));
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

static void
prop_dialog (GtkWidget *mi, GtkHTMLControlData *cd)
{
	GList *cur;
	GtkHTMLEditPropertyType t;

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd);

	cur = cd->properties_types;
	while (cur) {
		t = GPOINTER_TO_INT (cur->data);
		switch (t) {
		case GTK_HTML_EDIT_PROPERTY_TEXT:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   text_properties (cd),
								   _("Text"),
								   text_apply_cb);
			break;
		case GTK_HTML_EDIT_PROPERTY_IMAGE:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   image_properties (cd),
								   _("Image"),
								   image_apply_cb);
			break;
		case GTK_HTML_EDIT_PROPERTY_PARAGRAPH:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   paragraph_properties (cd),
								   _("Paragraph"),
								   paragraph_apply_cb);
			break;
		case GTK_HTML_EDIT_PROPERTY_LINK:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   link_properties (cd),
								   _("Link"),
								   link_apply_cb);
			break;
		}
		cur = cur->next;
	}

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
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

#define ADD_PROP(x) \
        cd->properties_types = g_list_append (cd->properties_types, GINT_TO_POINTER (GTK_HTML_EDIT_PROPERTY_ ## x))

gint
popup_show (GtkHTMLControlData *cd, GdkEventButton *event)
{
	HTMLEngine *e = cd->html->engine;
	HTMLObject *obj;
	GtkWidget *menu;
	GtkWidget *menuitem;
	guint items = 0;

	printf ("popup\n");

	obj  = cd->html->engine->cursor->object;
	menu = gtk_menu_new ();

	/* clear properties list */
	g_list_free (cd->properties_types);
	cd->properties_types = NULL;

	ADD_ITEM ("WIP: Properties...", prop_dialog);
	ADD_ITEM ("Insert link...", insert_link);
	if (e->active_selection
	    || (obj
		&& (HTML_OBJECT_TYPE (obj) == HTML_TYPE_LINKTEXTMASTER
		    || (HTML_OBJECT_TYPE (obj) == HTML_TYPE_IMAGE
			&& (*HTML_IMAGE (obj)->url
			    || *HTML_IMAGE (obj)->target))))) {
		    ADD_ITEM (_("Remove link"), remove_link);
	}

	if (e->active_selection) {
		ADD_PROP (TEXT);
		ADD_PROP (PARAGRAPH);
		ADD_PROP (LINK);
		ADD_SEP;
		ADD_ITEM ("Copy", copy);
		ADD_ITEM ("Cut",  cut);
	}
	if (e->cut_buffer) {
		ADD_ITEM ("Paste",  paste);
	}

	if (obj && !e->active_selection) {
		gchar *text = NULL;

		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_RULE:
			text = N_("Rule...");
			break;
		case HTML_TYPE_IMAGE:
			text = N_("Image...");
			ADD_PROP (IMAGE);
			ADD_PROP (PARAGRAPH);
			ADD_PROP (LINK);
			break;
		case HTML_TYPE_LINKTEXTMASTER:
			text = N_("Link...");
		case HTML_TYPE_TEXTMASTER:
			ADD_PROP (TEXT);
			ADD_PROP (PARAGRAPH);
			ADD_PROP (LINK);
			break;

		default:
		}
		if (text) {
			if (items) {
				ADD_SEP;
			}
			ADD_ITEM (text, prop);
		}
	}
	gtk_widget_show (menu);
	if (items)
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	gtk_widget_unref (menu);

	return (items > 0);
}
