/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "gtkhtml-properties.h"

GtkHTMLClassProperties *
gtk_html_class_properties_new (void)
{
	GtkHTMLClassProperties *p = g_new (GtkHTMLClassProperties, 1);

	/* default values */
	p->magic_links = TRUE;
	p->keybindings_theme = g_strdup ("emacs");

	return p;
}

void
gtk_html_class_properties_destroy (GtkHTMLClassProperties *p)
{
	g_free (p->keybindings_theme);
	g_free (p);
}

#define GET(t,x,prop,f,c) \
        key = g_strconcat (GTK_HTML_GCONF_DIR, x, NULL); \
        val = gconf_client_get_full (client, key, NULL, FALSE, &dflt, NULL); \
        if (!dflt) { f; p-> ## prop = gconf_value_ ## t (val); c; } \
        gconf_value_destroy (val); \
        g_free (key);

void
gtk_html_class_properties_load (GtkHTMLClassProperties *p, GConfClient *client)
{
	GConfValue *val;
	gchar *key;
	gboolean dflt;

	g_assert (client);

	GET (bool, "/magic_links", magic_links,,);
	GET (string, "/keybindings_theme", keybindings_theme,
	     g_free (p->keybindings_theme), p->keybindings_theme = g_strdup (p->keybindings_theme));
}

void
gtk_html_class_properties_copy (GtkHTMLClassProperties *p1,
				GtkHTMLClassProperties *p2)
{
	p1->magic_links = p2->magic_links;
	g_free (p1->keybindings_theme);
	p1->keybindings_theme = g_strdup (p2->keybindings_theme);
}

#define SET(t,x,prop) \
        key = g_strconcat (GTK_HTML_GCONF_DIR, x, NULL); \
        gconf_client_set_ ## t (client, key, p-> ## prop, NULL); \
        g_free (key);

void
gtk_html_class_properties_update (GtkHTMLClassProperties *p, GConfClient *client, GtkHTMLClassProperties *old)
{
	gchar *key;

	key = g_strconcat (GTK_HTML_GCONF_DIR, "/magic_links", NULL);

	if (p->magic_links != old->magic_links) {
		SET (bool, "/magic_links", magic_links);
	}

	if (strcmp (p->keybindings_theme, old->keybindings_theme)) {
		SET (string, "/keybindings_theme", keybindings_theme);
	}
}
