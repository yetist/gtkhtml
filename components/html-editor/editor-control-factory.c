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


struct _UIHHackStruct {
	BonoboControl *control;
	GtkHTML *html;
};
typedef struct _UIHHackStruct UIHHackStruct;

static gboolean
uih_hack_cb (gpointer data)
{
	UIHHackStruct *hack_struct;
	Bonobo_UIHandler remote_uih;
	BonoboUIHandler *uih;

	hack_struct = (UIHHackStruct *) data;

	puts (__FUNCTION__);

	/* FIXME CORBA_Object_release()? */

	remote_uih = bonobo_control_get_remote_ui_handler (hack_struct->control);
	if (remote_uih == CORBA_OBJECT_NIL) {
		/* We must not generate errors here, because we must allow
                   containers not to have a UIHandler.  */
		return TRUE;
	}

	uih = bonobo_control_get_ui_handler (hack_struct->control);
	bonobo_ui_handler_set_container (uih, remote_uih);

	toolbar_setup (uih, hack_struct->html);
	menubar_setup (uih, hack_struct->html);

	g_free (hack_struct);

	return FALSE;
}


static void
set_frame_cb (BonoboControl *control,
	      gpointer data)
{
	GtkHTML *html;
	UIHHackStruct *hack_struct;

	puts (__FUNCTION__);

	html = GTK_HTML (data);

	/* FIXME: We have to do this because the Bonobo behavior is currently
           broken.  */
	hack_struct = g_new (UIHHackStruct, 1);
	hack_struct->control = control;
	hack_struct->html = html;
	gtk_timeout_add (1000, uih_hack_cb, hack_struct);
}


static BonoboObject *
editor_control_factory (BonoboGenericFactory *factory,
			gpointer closure)
{
	BonoboControl *control;
	GtkWidget *html_widget;
	GtkWidget *scrolled_window;
	BonoboPersistStream *stream_impl;
	BonoboUIHandler *uih;

	html_widget = gtk_html_new ();
	gtk_widget_show (html_widget);
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), html_widget);
	gtk_widget_show (scrolled_window);

	control = bonobo_control_new (scrolled_window);

	/* Bonobo::PersistStream */

	stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
	bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (stream_impl));

	/* UIHandler.  */

	uih = bonobo_ui_handler_new ();
	bonobo_control_set_ui_handler (control, uih);

	/* Part of the initialization must be done after the control is
           embedded in its control frame.  We use the "set_frame" signal to
           handle that.  */
	gtk_signal_connect (GTK_OBJECT (control), "set_frame",
			    GTK_SIGNAL_FUNC (set_frame_cb), html_widget);

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
