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

#ifndef _GTK_HTML_PROPERTIES_H_
#define _GTK_HTML_PROPERTIES_H_

#define GTK_HTML_GCONF_DIR "/GNOME/Documents/HTML_Editor"

typedef enum _GtkHTMLClassPropertiesItem GtkHTMLClassPropertiesItem;
typedef struct _GtkHTMLClassProperties GtkHTMLClassProperties;

#include <glib.h>
#include <gconf/gconf-client.h>

enum _GtkHTMLClassPropertiesItem {
	GTK_HTML_CLASS_PROPERTIES_MAGIC_LINKS,
	GTK_HTML_CLASS_PROPERTIES_KEYBINDINGS,
};

struct _GtkHTMLClassProperties {
	gboolean  magic_links;
	gchar    *keybindings_theme;
};


GtkHTMLClassProperties * gtk_html_class_properties_new       (void);
void                     gtk_html_class_properties_destroy   (GtkHTMLClassProperties *p);
void                     gtk_html_class_properties_load      (GtkHTMLClassProperties *p,
							      GConfClient *client);
void                     gtk_html_class_properties_copy      (GtkHTMLClassProperties *p1,
							      GtkHTMLClassProperties *p2);
void                     gtk_html_class_properties_update    (GtkHTMLClassProperties *p,
							      GConfClient *client,
							      GtkHTMLClassProperties *old);

#endif
