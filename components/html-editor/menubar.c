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


static GnomeUIInfo format_subtree_info[] = {
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Bold"), N_("Set selection to bold face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD, 0, (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Italic"), N_("Set selection to italic face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD, 0, (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, N_("_Underline"), N_("Set selection to underlined face"), NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD, 0, (GdkModifierType) 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo menu_info[] = {
	GNOMEUIINFO_SUBTREE (N_("F_ormat"), format_subtree_info),
	GNOMEUIINFO_END
};


void
menubar_setup (BonoboUIHandler *uih,
	       GtkHTML *html)
{
	BonoboUIHandlerMenuItem *tree;

	g_return_if_fail (uih != NULL);
	g_return_if_fail (BONOBO_IS_UI_HANDLER (uih));
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	tree = bonobo_ui_handler_menu_parse_uiinfo_tree (menu_info);
	bonobo_ui_handler_menu_add_tree (uih, "/", tree);
	bonobo_ui_handler_menu_free_tree (tree);
}
