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
#include "body.h"

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
		html_engine_remove_link_object (cd->html->engine, cd->html->engine->cursor->object);
}

static void
prop_dialog (GtkWidget *mi, GtkHTMLControlData *cd)
{
	GList *cur;
	GtkHTMLEditPropertyType t;

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, FALSE);

	cur = cd->properties_types;
	while (cur) {
		t = GPOINTER_TO_INT (cur->data);
		switch (t) {
		case GTK_HTML_EDIT_PROPERTY_TEXT:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   t, _("Text"),
								   text_properties,
								   text_apply_cb,
								   text_close_cb);
			break;
		case GTK_HTML_EDIT_PROPERTY_IMAGE:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   t, _("Image"),
								   image_properties,
								   image_apply_cb,
								   image_close_cb);
								   break;
		case GTK_HTML_EDIT_PROPERTY_PARAGRAPH:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   t, _("Paragraph"),
								   paragraph_properties,
								   paragraph_apply_cb,
								   paragraph_close_cb);
			break;
		case GTK_HTML_EDIT_PROPERTY_LINK:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   t, _("Link"),
								   link_properties,
								   link_apply_cb,
								   link_close_cb);
								   break;
		case GTK_HTML_EDIT_PROPERTY_BODY:
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   t, _("Page"),
								   body_properties,
								   body_apply_cb,
								   body_close_cb);
								   break;
		}
		cur = cur->next;
	}

	t = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (mi), "type"));

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	if (t >= 0)
		gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, t);
}

#define ADD_ITEM(l,f,t) \
		menuitem = gtk_menu_item_new_with_label (_(l)); \
                gtk_object_set_data (GTK_OBJECT (menuitem), "type", GINT_TO_POINTER (t)); \
		gtk_menu_append (GTK_MENU (menu), menuitem); \
		gtk_widget_show (menuitem); \
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC (f), cd); \
                items++; items_sep++

#define ADD_SEP \
        if (items_sep) { \
                menuitem = gtk_menu_item_new (); \
                gtk_menu_append (GTK_MENU (menu), menuitem); \
                gtk_widget_show (menuitem); \
		items_sep = 0; \
        }

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
	guint items_sep = 0;

	obj  = cd->html->engine->cursor->object;
	menu = gtk_menu_new ();

	if (e->active_selection
	    || (obj
		&& (HTML_OBJECT_TYPE (obj) == HTML_TYPE_LINKTEXTMASTER
		    || (HTML_OBJECT_TYPE (obj) == HTML_TYPE_IMAGE
			&& (HTML_IMAGE (obj)->url
			    || HTML_IMAGE (obj)->target))))) {
		    ADD_ITEM (_("Remove link"), remove_link, -1);
	}

	if (e->active_selection) {
		ADD_SEP;
		ADD_ITEM ("Copy", copy, -1);
		ADD_ITEM ("Cut",  cut, -1);
	}
	if (e->cut_buffer) {
		if (!e->active_selection) {
			ADD_SEP;
		}
		ADD_ITEM ("Paste",  paste, -1);
	}
	if (e->active_selection) {
		ADD_SEP;
		ADD_ITEM ("Text...", prop_dialog, GTK_HTML_EDIT_PROPERTY_TEXT);
		ADD_PROP (TEXT);
		ADD_ITEM ("Paragraph...", prop_dialog, GTK_HTML_EDIT_PROPERTY_PARAGRAPH);
		ADD_PROP (PARAGRAPH);
		ADD_ITEM ("Link...", prop_dialog, GTK_HTML_EDIT_PROPERTY_LINK);
		ADD_PROP (LINK);
	}
	if (obj && !e->active_selection) {
		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_RULE:
			break;
		case HTML_TYPE_IMAGE:
			ADD_SEP;
			ADD_PROP (IMAGE);
			ADD_ITEM ("Image...", prop_dialog, GTK_HTML_EDIT_PROPERTY_PARAGRAPH);
			ADD_PROP (PARAGRAPH);
			ADD_ITEM ("Paragraph...", prop_dialog, GTK_HTML_EDIT_PROPERTY_PARAGRAPH);
			ADD_PROP (LINK);
			ADD_ITEM ("Link...", prop_dialog, GTK_HTML_EDIT_PROPERTY_LINK);
			break;
		case HTML_TYPE_LINKTEXTMASTER:
		case HTML_TYPE_TEXTMASTER:
			ADD_SEP;
			ADD_PROP (TEXT);
			ADD_ITEM ("Text...", prop_dialog, GTK_HTML_EDIT_PROPERTY_TEXT);
			ADD_PROP (PARAGRAPH);
			ADD_ITEM ("Paragraph...", prop_dialog, GTK_HTML_EDIT_PROPERTY_PARAGRAPH);
			ADD_PROP (LINK);
			ADD_ITEM ("Link...", prop_dialog, GTK_HTML_EDIT_PROPERTY_LINK);
			break;

		default:
		}
	}
	ADD_SEP;
	ADD_PROP (BODY);
	ADD_ITEM ("Page...", prop_dialog, GTK_HTML_EDIT_PROPERTY_BODY);

	gtk_widget_show (menu);
	if (items)
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	gtk_widget_unref (menu);

	return (items > 0);
}
