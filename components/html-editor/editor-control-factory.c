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
#include <bonobo.h>

#include "gtkhtml.h"
#include "persist-stream-impl.h"

#include "editor-control-factory.h"


static BonoboObject *
editor_control_factory (BonoboGenericFactory *factory,
			gpointer closure)
{
	BonoboControl *control;
	GtkWidget *html_widget;
	GtkWidget *scrolled_window;
	BonoboPersistStream *stream_impl;

	html_widget = gtk_html_new ();
	gtk_widget_show (html_widget);
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), html_widget);
	gtk_widget_show (scrolled_window);

	control = bonobo_control_new (scrolled_window);

	stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
	bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (stream_impl));

	g_warning ("Creating a new GtkHTML editor control.");

	return BONOBO_OBJECT (control);
}

void
editor_control_factory_init (void)
{
	static BonoboGenericFactory *factory = NULL;

	if (factory != NULL)
		return;

	factory = bonobo_generic_factory_new ("control-factory:html-editor",
					      editor_control_factory,
					      NULL);

	if (factory == NULL)
		g_error ("I could not register the html-editor-control factory.");
}
