/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

#include <gnome.h>
#include <bonobo.h>

#include "menubar.h"
#include "gtkhtml.h"
#include "control-data.h"
#include "properties.h"

static void
undo_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_undo (cd->html);
}

static void
redo_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_redo (cd->html);
}

static void
cut_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_cut (cd->html);
}

static void
copy_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_copy (cd->html);
}

static void
paste_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_paste (cd->html);
}

static void
search_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search (cd, TRUE);
}

static void
search_regex_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search (cd, FALSE);
}

static void
search_next_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search_next (cd);
}

static void
replace_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	replace (cd);
}

static void
insert_image_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);
	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, TRUE);
	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_LINK, _("Insert image"),
						   image_insertion,
						   image_insert_cb,
						   image_close_cb);
	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
insert_link_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, FALSE);

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_LINK, _("Link"),
						   link_properties,
						   link_apply_cb,
						   link_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
insert_rule_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	rule_insert (cd);
}

static void
properties_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gchar *argv[2] = {"gtkhtml-properties-capplet", NULL};
	if (gnome_execute_async (NULL, 1, argv) < 0)
		gnome_error_dialog (_("Cannot execute gtkhtml properties"));
}

BonoboUIVerb verbs [] = {
	BONOBO_UI_VERB ("EditUndo", undo_cb),
	BONOBO_UI_VERB ("EditRedo", redo_cb),
	BONOBO_UI_VERB ("EditCut", cut_cb),
	BONOBO_UI_VERB ("EditCopy", copy_cb),
	BONOBO_UI_VERB ("EditPaste", paste_cb),
	BONOBO_UI_VERB ("EditFind", search_cb),
	BONOBO_UI_VERB ("EditFindRegexp", search_regex_cb),
	BONOBO_UI_VERB ("EditFindAgain", search_next_cb),
	BONOBO_UI_VERB ("EditReplace", replace_cb),
	BONOBO_UI_VERB ("EditProperties", properties_cb),

	BONOBO_UI_VERB ("InsertImage", insert_image_cb),
	BONOBO_UI_VERB ("InsertLink", insert_link_cb),
	BONOBO_UI_VERB ("InsertRule", insert_rule_cb),
	
	BONOBO_UI_VERB_END
};

void
menubar_setup (BonoboUIHandler    *uih,
	       GtkHTMLControlData *cd)
{
	BonoboUIComponent *uic;
	Bonobo_UIContainer container;
	char *fname;
	xmlNode *ui;

	g_return_if_fail (cd->html != NULL);
	g_return_if_fail (GTK_IS_HTML (cd->html));
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));

	uic = bonobo_ui_compat_get_component (uih);
	g_return_if_fail (uic != NULL);

	container = bonobo_ui_compat_get_container (uih);
	g_return_if_fail (container != CORBA_OBJECT_NIL);

	bonobo_ui_component_add_verb_list_with_data (uic, verbs, cd);

	fname = bonobo_ui_util_get_ui_fname ("html-editor-control.xml");
	g_warning ("Loading ui from '%s'", fname);

	ui = bonobo_ui_util_new_ui (container, fname, "html-editor-control");

	bonobo_ui_component_set_tree (uic, container, "/", ui, NULL);
	g_free (fname);
	xmlFreeNode (ui);
}
