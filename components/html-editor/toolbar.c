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

#include "toolbar.h"


#define EDITOR_TOOLBAR_PATH "/HTMLEditor"


/* Toolbar button callbacks.  */

static void
editor_toolbar_bold_cb (gpointer data)
{
}

static void
editor_toolbar_italic_cb (gpointer data)
{
}

static void
editor_toolbar_underline_cb (gpointer data)
{
}

static void
editor_toolbar_strikeout_cb (gpointer data)
{
}


static void
editor_toolbar_left_align_cb (gpointer data)
{
}

static void
editor_toolbar_center_cb (gpointer data)
{
}

static void
editor_toolbar_right_align_cb (gpointer data)
{
}


/* Editor toolbar.  */

static GnomeUIInfo editor_toolbar_alignment_group[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Left align"), N_("Left justifies the cell contents"),
				editor_toolbar_left_align_cb, GNOME_STOCK_PIXMAP_ALIGN_LEFT),
	GNOMEUIINFO_ITEM_STOCK (N_("Center"), N_("Centers the cell contents"),
				editor_toolbar_center_cb, GNOME_STOCK_PIXMAP_ALIGN_CENTER),
	GNOMEUIINFO_ITEM_STOCK (N_("Right align"), N_("Right justifies the cell contents"),
				editor_toolbar_right_align_cb, GNOME_STOCK_PIXMAP_ALIGN_RIGHT),
	GNOMEUIINFO_END
};

static GnomeUIInfo editor_toolbar_data[] = {
	{ GNOME_APP_UI_TOGGLEITEM, N_("Bold"), N_("Sets the bold font"),
	  editor_toolbar_bold_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD },

	{ GNOME_APP_UI_TOGGLEITEM, N_("Italic"), N_("Make the selection italic"),
	  editor_toolbar_italic_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_ITALIC },

	{ GNOME_APP_UI_TOGGLEITEM, N_("Underline"), N_("Make the selection underlined"),
	  editor_toolbar_underline_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_UNDERLINE },

	{ GNOME_APP_UI_TOGGLEITEM, N_("Strikeout"), N_("Make the selection striked out"),
	  editor_toolbar_strikeout_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_RADIOLIST (editor_toolbar_alignment_group),

	GNOMEUIINFO_END
};

static Bonobo_Control
create_editor_toolbar (GtkHTML *html)
{
	GtkWidget *toolbar;
	BonoboControl *toolbar_control;

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar);

	gnome_app_fill_toolbar_with_data (GTK_TOOLBAR (toolbar), editor_toolbar_data, NULL, html);

#if 0
	gtk_toolbar_prepend_widget (GTK_TOOLBAR (toolbar), create_font_style_option_menu,
				    NULL, NULL);
#endif

	toolbar_control = bonobo_control_new (toolbar);
	return (Bonobo_Control) bonobo_object_corba_objref (BONOBO_OBJECT (toolbar_control));
}


void
toolbar_setup (BonoboUIHandler *uih,
	       GtkHTML *html)
{
	Bonobo_Control toolbar_control;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	bonobo_ui_handler_create_toolbar (uih, "HTMLEditor");

	toolbar_control = create_editor_toolbar (html);

	bonobo_ui_handler_toolbar_new_control (uih, EDITOR_TOOLBAR_PATH, 0, toolbar_control);
}
