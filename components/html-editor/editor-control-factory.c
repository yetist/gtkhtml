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
#include "menubar.h"
#include "toolbar.h"

#include "editor-control-factory.h"

/* For some reason, enabling this slows things down *a lot*.  */
#define MAKE_VBOX_CONTROL


#ifdef MAKE_VBOX_CONTROL
/* This is the initialization that can only be performed after the
   control has been embedded (signal "set_frame').  */

struct _SetFrameData {
	GtkWidget *html;
	GtkWidget *vbox;
};
typedef struct _SetFrameData SetFrameData;

static void
set_frame_cb (BonoboControl *control,
	      gpointer data)
{
	Bonobo_UIHandler remote_uih;
	BonoboUIHandler *uih;
	SetFrameData *set_frame_data;
	GtkWidget *toolbar;
	GtkWidget *scrolled_window;

	set_frame_data = (SetFrameData *) data;

	remote_uih = bonobo_control_get_remote_ui_handler (control);
	uih = bonobo_control_get_ui_handler (control);
	bonobo_ui_handler_set_container (uih, remote_uih);

	/* Setup the tool bar.  */

	toolbar = toolbar_setup (uih, GTK_HTML (set_frame_data->html));
	gtk_box_pack_start (GTK_BOX (set_frame_data->vbox), toolbar, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), set_frame_data->html);
	gtk_widget_show (scrolled_window);

	gtk_box_pack_start (GTK_BOX (set_frame_data->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Setup the menu bar.  */

	menubar_setup (uih, GTK_HTML (set_frame_data->html));

	g_free (set_frame_data);
}
#endif

static BonoboObject *
editor_control_factory (BonoboGenericFactory *factory,
			gpointer closure)
{
	BonoboControl *control;
	BonoboPersistStream *stream_impl;
	GtkWidget *html_widget;
	GtkWidget *vbox;

	html_widget = gtk_html_new ();
	gtk_widget_show (html_widget);

	gtk_html_load_empty (GTK_HTML (html_widget));
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

#ifdef MAKE_VBOX_CONTROL
	control = bonobo_control_new (vbox);
#else
	{
		GtkWidget *scrolled_window;

		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (scrolled_window), html_widget);
		gtk_widget_show (scrolled_window);
		control = bonobo_control_new (scrolled_window);
	}
#endif

	/* Bonobo::PersistStream */

	stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
	bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (stream_impl));

	g_warning ("Creating a new GtkHTML editor control.");

#ifdef MAKE_VBOX_CONTROL
	{
		SetFrameData *set_frame_data;

		/* Part of the initialization must be done after the control is
		   embedded in its control frame.  We use the "set_frame" signal to
		   handle that.  */

		set_frame_data = g_new (SetFrameData, 1);
		set_frame_data->html = html_widget;
		set_frame_data->vbox = vbox;

		gtk_signal_connect (GTK_OBJECT (control), "set_frame",
				    GTK_SIGNAL_FUNC (set_frame_cb), set_frame_data);
	}
#endif

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
