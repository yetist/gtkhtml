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
#include <stdio.h>
#include <glib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "HTMLEditor.h"

#include "gtkhtml.h"
#include "htmlengine-edit.h"

#include "menubar.h"
#include "persist-file-impl.h"
#include "persist-stream-impl.h"
#include "popup.h"
#include "toolbar.h"
#include "properties.h"
#include "text.h"
#include "paragraph.h"
#include "link.h"
#include "body.h"
#include "spell.h"
#include "resolver-progressive-impl.h"

#include "editor-control-factory.h"
#include "gtkhtmldebug.h"

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

static BonoboGenericFactory *factory = NULL;
static gint active_controls = 0;

static void
set_frame_cb (BonoboControl *control,
	      gpointer data)
{
	Bonobo_UIContainer remote_ui_container;
	BonoboUIComponent *ui_component;
	GtkHTMLControlData *control_data;
	GtkWidget *toolbar;
	GtkWidget *scrolled_window;

	control_data = (GtkHTMLControlData *) data;

	remote_ui_container = bonobo_control_get_remote_ui_container (control);
	ui_component = bonobo_control_get_ui_component (control);
	bonobo_ui_component_set_container (ui_component, remote_ui_container);

	/* Setup the tool bar.  */

	toolbar = toolbar_style (control_data);
	gtk_box_pack_start (GTK_BOX (control_data->vbox), toolbar, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (control_data->html));
	gtk_widget_show_all (scrolled_window);

	gtk_box_pack_start (GTK_BOX (control_data->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Setup the menu bar.  */

	menubar_setup (ui_component, control_data);
	toolbar_setup (ui_component, control_data);
}

static gint
release (GtkWidget *widget, GdkEventButton *event, GtkHTMLControlData *cd)
{
	HTMLEngine *e = cd->html->engine;
	GtkHTMLEditPropertyType start = GTK_HTML_EDIT_PROPERTY_BODY;
	gboolean run_dialog = FALSE;

	if (cd->obj) {
		switch (HTML_OBJECT_TYPE (cd->obj)) {
		case HTML_TYPE_IMAGE:
		case HTML_TYPE_LINKTEXTMASTER:
		case HTML_TYPE_TEXTMASTER:
			run_dialog = TRUE;
			break;
		default:
		}
		if (run_dialog) {
			cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, FALSE, _("Properties"));

			html_cursor_jump_to (e->cursor, e, cd->obj, 0);
			html_engine_disable_selection (e);
			html_engine_set_mark (e);
			html_cursor_jump_to (e->cursor, e, cd->obj, html_object_get_length (cd->obj));
			html_engine_edit_selection_updater_update_now (e->selection_updater);

			switch (HTML_OBJECT_TYPE (cd->obj)) {
			case HTML_TYPE_IMAGE:
				gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
									   GTK_HTML_EDIT_PROPERTY_IMAGE, _("Image"),
									   image_properties,
									   image_apply_cb,
									   image_close_cb);
				start = GTK_HTML_EDIT_PROPERTY_IMAGE;
				break;
			case HTML_TYPE_LINKTEXTMASTER:
			case HTML_TYPE_TEXTMASTER:
				gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
									   GTK_HTML_EDIT_PROPERTY_TEXT, _("Text"),
									   text_properties,
									   text_apply_cb,
									   text_close_cb);
				start = (HTML_OBJECT_TYPE (cd->obj) == HTML_TYPE_TEXTMASTER)
					? GTK_HTML_EDIT_PROPERTY_TEXT
					: GTK_HTML_EDIT_PROPERTY_LINK;
						
				break;
			default:
			}
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   GTK_HTML_EDIT_PROPERTY_PARAGRAPH, _("Paragraph"),
								   paragraph_properties,
								   paragraph_apply_cb,
								   paragraph_close_cb);
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   GTK_HTML_EDIT_PROPERTY_LINK, _("Link"),
								   link_properties,
								   link_apply_cb,
								   link_close_cb);
			gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
								   GTK_HTML_EDIT_PROPERTY_BODY, _("Page"),
								   body_properties,
								   body_apply_cb,
								   body_close_cb);
			gtk_html_edit_properties_dialog_show (cd->properties_dialog);
			gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, start);
		}
	}
	gtk_signal_disconnect (GTK_OBJECT (widget), cd->releaseId);

	return FALSE;
}

static int
load_from_file (GtkHTML *html,
		const char *url,
		GtkHTMLStream *handle)
{
	unsigned char buffer[4096];
	int len;
	int fd;
        const char *path;

        if (strncmp (url, "file:", 5) != 0) {
		g_warning ("Unsupported image url: %s", url);
		return FALSE;
	} 
	path = url + 5; 

	if ((fd = open (path, O_RDONLY)) == -1) {
		g_warning ("%s", g_strerror (errno));
		return FALSE;
	}
      
       	while ((len = read (fd, buffer, 4096)) > 0) {
		gtk_html_write (html, handle, buffer, len);
	}

	if (len < 0) {
		/* check to see if we stopped because of an error */
		gtk_html_end (html, handle, GTK_HTML_STREAM_ERROR);
		g_warning ("%s", g_strerror (errno));
		return FALSE;
	}	
	/* done with no errors */
	gtk_html_end (html, handle, GTK_HTML_STREAM_OK);
	close (fd);
	return TRUE;
}

static int
load_from_corba (BonoboControl *control,
		 GtkHTML *html,
		 const char *url, 
		 GtkHTMLStream *handle)
{
	HTMLEditor_Resolver resolver;
	Bonobo_ControlFrame Frame;
	CORBA_Environment ev;	

	CORBA_exception_init (&ev);

	/* FIXME: is this going to work ? it looks extremely strange */
	Frame = bonobo_control_get_control_frame (control);
	if (Frame != CORBA_OBJECT_NIL) {
		resolver = Bonobo_Unknown_query_interface (Frame, "IDL:HTMLEditor/Resolver:1.0", &ev);
		
		if (resolver == CORBA_OBJECT_NIL) {
			/* g_warning ("Unable to aquire resolver interface"); */
		} else {
			/* g_warning ("found resolver - rejoice the masses"); */
			Bonobo_ProgressiveDataSink sink;

			sink = bonobo_object_corba_objref (BONOBO_OBJECT (resolver_sink (html, url, handle)));
			
			HTMLEditor_Resolver_loadURL (resolver, sink, url, &ev);
			if (ev._major != CORBA_NO_EXCEPTION){
				/* g_warning ("Got exception!!!"); */
			} else {
				/* g_warning ("No Exceptions made"); */
			}		
			
		}
	}
	CORBA_exception_free (&ev);

	return FALSE;
}

static void
url_requested_cb (GtkHTML *html, const char *url, GtkHTMLStream *handle, gpointer data)
{
	BonoboControl *control;

	g_return_if_fail (data != NULL);
	g_return_if_fail (url != NULL);
	g_return_if_fail (handle != NULL);

	control = BONOBO_CONTROL (data);

	if (load_from_corba (control, html, url, handle))
		;
		/* g_warning ("valid corba reponse"); */
	else if (load_from_file (html, url, handle))
		;
		/* g_warning ("valid local reponse"); */
	else
		g_warning ("unable to resolve url: %s", url);

}

static gint
html_button_pressed (GtkWidget *html, GdkEventButton *event, GtkHTMLControlData *cd)
{
	HTMLEngine *engine = cd->html->engine;

	cd->obj = html_engine_get_object_at (engine,
					     event->x + engine->x_offset,
					     event->y + engine->y_offset,
					     NULL, FALSE);
	switch (event->button) {
	case 1:
		if (event->type == GDK_2BUTTON_PRESS && cd->obj && event->state & GDK_CONTROL_MASK)
			cd->releaseId = gtk_signal_connect (GTK_OBJECT (html), "button_release_event",
							    GTK_SIGNAL_FUNC (release), cd);
		else
			return TRUE;
		break;
	case 2:
		/* pass this for pasting */
		return TRUE;
	case 3:
		if (popup_show (cd, event))
			gtk_signal_emit_stop_by_name (GTK_OBJECT (html), "button_press_event");
		break;
	default:
	}

	return FALSE;
}

static void
destroy_control_data_cb (GtkObject *control, GtkHTMLControlData *cd)
{
	gtk_html_control_data_destroy (cd);

	printf ("active--\n");
	active_controls --;

	printf ("control_destroy %d\n", active_controls);

	if (active_controls)
		return;
	bonobo_object_unref (BONOBO_OBJECT (factory));
	gtk_main_quit ();
}

static BonoboObject *
editor_control_factory (BonoboGenericFactory *factory,
			gpointer closure)
{
	BonoboControl *control;
	BonoboPersistStream *stream_impl;
	BonoboPersistFile *file_impl;
	GtkWidget *html_widget;
	GtkWidget *vbox;
	GtkHTMLControlData *control_data;

	html_widget = gtk_html_new ();
	gtk_html_load_empty (GTK_HTML (html_widget));
	gtk_html_set_editable (GTK_HTML (html_widget), TRUE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

	/* g_warning ("Creating a new GtkHTML editor control."); */
	control = bonobo_control_new (vbox);

	if (control) {

		active_controls++;
		printf ("active++\n");

		/* Bonobo::PersistStream */

		stream_impl = persist_stream_impl_new (GTK_HTML (html_widget));
		bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (stream_impl));

		/* Bonobo::PersistFile */

		file_impl = persist_file_impl_new (GTK_HTML (html_widget));
		bonobo_object_add_interface (BONOBO_OBJECT (control), BONOBO_OBJECT (file_impl));

		/* Part of the initialization must be done after the control is
		   embedded in its control frame.  We use the "set_frame" signal to
		   handle that.  */

		control_data = gtk_html_control_data_new (GTK_HTML (html_widget), vbox);

		gtk_signal_connect (GTK_OBJECT (control), "set_frame",
				    GTK_SIGNAL_FUNC (set_frame_cb), control_data);

		gtk_signal_connect (GTK_OBJECT (control), "destroy",
				    GTK_SIGNAL_FUNC (destroy_control_data_cb), control_data);

		gtk_signal_connect (GTK_OBJECT (html_widget), "url_requested",
				    GTK_SIGNAL_FUNC (url_requested_cb), control);

		gtk_signal_connect (GTK_OBJECT (html_widget), "button_press_event",
				    GTK_SIGNAL_FUNC (html_button_pressed), control_data);

#ifdef GTKHTML_HAVE_PSPELL
		gtk_signal_connect (GTK_OBJECT (html_widget), "spell_suggestion_request",
				    GTK_SIGNAL_FUNC (spell_suggestion_request_cb), control_data);
#endif
	}

	return BONOBO_OBJECT (control);
}

void
editor_control_factory_init (void)
{
	if (factory != NULL)
		return;

	gdk_rgb_init ();
	factory = bonobo_generic_factory_new (CONTROL_FACTORY_ID,
					      editor_control_factory,
					      NULL);

	if (factory == NULL)
		g_error ("I could not register the html-editor-control factory.");
}
