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

static void undo_cb             (GtkWidget *widget, GtkHTMLControlData *cd);
static void redo_cb             (GtkWidget *widget, GtkHTMLControlData *cd);

static void cut_cb              (GtkWidget *widget, GtkHTMLControlData *cd);
static void copy_cb             (GtkWidget *widget, GtkHTMLControlData *cd);
static void paste_cb            (GtkWidget *widget, GtkHTMLControlData *cd);

static void search_cb           (GtkWidget *widget, GtkHTMLControlData *cd);
static void search_regex_cb     (GtkWidget *widget, GtkHTMLControlData *cd);
static void search_next_cb      (GtkWidget *widget, GtkHTMLControlData *cd);
static void replace_cb          (GtkWidget *widget, GtkHTMLControlData *cd);

static void insert_image_cb     (GtkWidget *widget, GtkHTMLControlData *cd);
static void insert_link_cb      (GtkWidget *widget, GtkHTMLControlData *cd);


static GnomeUIInfo format_subtree_info[] = {
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Bold"), N_("Set selection to bold face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD, 0, (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Italic"), N_("Set selection to italic face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_ITALIC, 0, (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Underline"), N_("Set selection to underlined face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_UNDERLINE, 0, (GdkModifierType) 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo edit_subtree_info[] = {
	GNOMEUIINFO_MENU_UNDO_ITEM (undo_cb, NULL),
	GNOMEUIINFO_MENU_REDO_ITEM (redo_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_CUT_ITEM (cut_cb, NULL),
	GNOMEUIINFO_MENU_COPY_ITEM (copy_cb, NULL),
	GNOMEUIINFO_MENU_PASTE_ITEM (paste_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_FIND_ITEM (search_cb, NULL),
	GNOMEUIINFO_ITEM_NONE (N_("Find Rege_x..."), N_("Regular expressions search..."), search_regex_cb),
	GNOMEUIINFO_MENU_FIND_AGAIN_ITEM (search_next_cb, NULL),
	GNOMEUIINFO_MENU_REPLACE_ITEM (replace_cb, NULL),
	GNOMEUIINFO_END	
};

static GnomeUIInfo insert_subtree_info[] = {
	GNOMEUIINFO_ITEM_NONE (N_("_Image..."), N_("Insert image into document..."), insert_image_cb),
	GNOMEUIINFO_ITEM_NONE (N_("_Link..."), N_("Insert HTML link into document..."), insert_link_cb),
	GNOMEUIINFO_END
};

static GnomeUIInfo menu_info[] = {
	GNOMEUIINFO_MENU_EDIT_TREE (edit_subtree_info),
	GNOMEUIINFO_SUBTREE (N_("_Insert"), insert_subtree_info),
	GNOMEUIINFO_SUBTREE (N_("F_ormat"), format_subtree_info),
	GNOMEUIINFO_END
};



static void
undo_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	gtk_html_undo (cd->html);
}

static void
redo_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	gtk_html_redo (cd->html);
}

static void
cut_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	gtk_html_cut (cd->html);
}

static void
copy_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	gtk_html_copy (cd->html);
}

static void
paste_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	gtk_html_paste (cd->html);
}

static void
search_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	search (cd, TRUE);
}

static void
search_regex_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	search (cd, FALSE);
}

static void
search_next_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	search_next (cd);
}

static void
replace_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	replace (cd);
}

static void
insert_image_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	image_insert (cd);
}

static void
insert_link_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	link_insert (cd);
}

void
menubar_setup (BonoboUIHandler *uih,
	       GtkHTMLControlData *cd)
{
	BonoboUIHandlerMenuItem *tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (cd->html != NULL);
	g_return_if_fail (GTK_IS_HTML (cd->html));

	tree = bonobo_ui_handler_menu_parse_uiinfo_list_with_data (menu_info, cd);
	bonobo_ui_handler_menu_add_list (uih, "/", tree);
	bonobo_ui_handler_menu_free_list (tree);
}
