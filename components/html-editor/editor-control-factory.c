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
#include <bonobo.h>

#include "gtkhtml.h"
#include "persist-stream-impl.h"
#include "menubar.h"
#include "toolbar.h"
#include "popup.h"

#include "editor-control-factory.h"

#ifdef USING_OAF
#define CONTROL_FACTORY_ID "OAFIID:control-factory:html-editor:97cebd11-9e8c-45c3-9a0a-5a269c760c00"
#else
#define CONTROL_FACTORY_ID "control-factory:html-editor"
#endif


/* This is the initialization that can only be performed after the
   control has been embedded (signal "set_frame").  */

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
	GtkHTMLControlData *control_data;
	GtkWidget *toolbar;
	GtkWidget *scrolled_window;

	control_data = (GtkHTMLControlData *) data;

	remote_uih = bonobo_control_get_remote_ui_handler (control);
	uih = bonobo_control_get_ui_handler (control);
	bonobo_ui_handler_set_container (uih, remote_uih);

	/* Setup the tool bar.  */

	toolbar = toolbar_setup (uih, control_data);
	gtk_box_pack_start (GTK_BOX (control_data->vbox), toolbar, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (control_data->html));
	gtk_widget_show_all (scrolled_window);

	gtk_box_pack_start (GTK_BOX (control_data->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Setup the menu bar.  */

	menubar_setup (uih, control_data);
}

static gint
html_button_pressed (GtkWidget *html, GdkEventButton *event, GtkHTMLControlData *cd)
{
	HTMLObject *obj;
	HTMLEngine *engine = cd->html->engine;

	printf ("button pressed %d\n", event->button);

	cd->obj = obj = html_engine_get_object_at (engine,
						   event->x + engine->x_offset, event->y + engine->y_offset,
						   NULL, FALSE);	

	if (obj && event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_IMAGE:
			image_edit (cd, HTML_IMAGE (obj));
			break;
		default:
		}
	} else if (event->button == 3) {
		if (popup_show (cd, event))
			gtk_signal_emit_stop_by_name (GTK_OBJECT (html), "button_press_event");
	}
	return FALSE;
}

static void
destroy_control_data_cb (GtkObject *control, GtkHTMLControlData *cd)
{
	gtk_html_control_data_destroy (cd);
}

static BonoboObject *
editor_control_factory (BonoboGenericFactory *factory,
			gpointer closure)
{
	BonoboControl *control;
	BonoboPersistStream *stream_impl;
	GtkWidget *html_widget;
	GtkWidget *vbox;
	GtkHTMLControlData *control_data;
		
	html_widget = gtk_html_new ();

	gtk_html_load_empty (GTK_HTML (html_widget));
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

	control = bonobo_control_new (vbox);

	/* Bonobo::PersistStream */

	stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
	bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (stream_impl));

	g_warning ("Creating a new GtkHTML editor control.");

	/* Part of the initialization must be done after the control is
	   embedded in its control frame.  We use the "set_frame" signal to
	   handle that.  */

	control_data = gtk_html_control_data_new (GTK_HTML (html_widget), vbox);

	gtk_signal_connect (GTK_OBJECT (control), "set_frame",
			    GTK_SIGNAL_FUNC (set_frame_cb), control_data);

	gtk_signal_connect (GTK_OBJECT (control), "destroy",
			    GTK_SIGNAL_FUNC (destroy_control_data_cb), control_data);

	gtk_signal_connect (GTK_OBJECT (html_widget), "button_press_event",
			    GTK_SIGNAL_FUNC (html_button_pressed), control_data);

	return BONOBO_OBJECT (control);
}

void
editor_control_factory_init (void)
{
	static BonoboGenericFactory *factory = NULL;

	if (factory != NULL)
		return;

	factory = bonobo_generic_factory_new (CONTROL_FACTORY_ID,
					      editor_control_factory,
					      NULL);

	if (factory == NULL)
		g_error ("I could not register the html-editor-control factory.");
}
