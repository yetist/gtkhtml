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

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo/gnome-bonobo.h>

#include "gtkhtml.h"
#include "persist-stream-impl.h"

#include "editor-control-factory.h"


static GnomeObject *
editor_control_factory (GnomeGenericFactory *factory,
			gpointer closure)
{
	GnomeControl *control;
	GtkWidget *html_widget;
	GtkWidget *scrolled_window;
	GnomePersistStream *stream_impl;

	html_widget = gtk_html_new ();
	gtk_widget_show (html_widget);
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), html_widget);
	gtk_widget_show (scrolled_window);

	control = gnome_control_new (scrolled_window);

	stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
	gnome_object_add_interface (GNOME_OBJECT (control), GNOME_OBJECT (stream_impl));

	g_warning ("Creating a new GtkHTML editor control.");

	return GNOME_OBJECT (control);
}

void
editor_control_factory_init (void)
{
	static GnomeGenericFactory *factory = NULL;

	if (factory != NULL)
		return;

	factory = gnome_generic_factory_new ("control-factory:html-editor",
					     editor_control_factory,
					     NULL);

	if (factory == NULL)
		g_error ("I could not register the html-editor-control factory.");
}
