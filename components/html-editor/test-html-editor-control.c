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

    Authors: Ettore Perazzoli <ettore@helixcode.com>
*/

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <bonobo/gnome-bonobo.h>


static GtkWidget *file_selection_widget = NULL;


struct _OpenInfo {
	GnomeBonoboWidget *control;
	GtkWidget *app;
	GtkWidget *file_selection_widget;
};
typedef struct _OpenInfo OpenInfo;


static void
load_through_stream (const gchar *filename,
		     GNOME_PersistStream pstream)
{
	GnomeStream *stream;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	stream = gnome_stream_fs_open (filename, GNOME_Storage_READ);
	if (stream == NULL) {
		g_warning ("Couldn't load `%s'\n", filename);
	} else {
		GnomeObject *stream_object;
		GNOME_Stream corba_stream;

		stream_object = GNOME_OBJECT (stream);
		corba_stream = gnome_object_corba_objref (stream_object);
		GNOME_PersistStream_load (pstream, corba_stream, &ev);
	}

	GNOME_Unknown_unref (pstream, &ev);
	CORBA_Object_release (pstream, &ev);

	CORBA_exception_free (&ev);
}

static void
file_selection_open_cb (GtkWidget *widget,
			gpointer data)
{
	OpenInfo *info;
	CORBA_Object interface;
	GnomeObjectClient *object_client;

	info = (OpenInfo *) data;

	object_client = gnome_bonobo_widget_get_server (info->control);
	interface = gnome_object_client_query_interface (object_client,
							 "IDL:GNOME/PersistStream:1.0",
							 NULL);

	if (interface != CORBA_OBJECT_NIL) {
		const gchar *fname;

		fname = gtk_file_selection_get_filename (GTK_FILE_SELECTION (info->file_selection_widget));
		load_through_stream (fname, interface);
	} else {
		g_warning ("The Control does not implement any of the supported interfaces.");
	}

	gtk_widget_destroy (info->file_selection_widget);

	g_free (info);
}

static void
file_selection_destroy_cb (GtkWidget *widget,
			   gpointer data)
{
	file_selection_widget = NULL;
}

static void
open_cb (GtkWidget *widget,
	 gpointer data)
{
	GnomeApp *app;
	GnomeBonoboWidget *control;
	OpenInfo *open_info;

	app = GNOME_APP (data);
	control = GNOME_BONOBO_WIDGET (app->contents);

	if (file_selection_widget != NULL) {
		gdk_window_show (GTK_WIDGET (file_selection_widget)->window);
		return;
	}

	file_selection_widget = gtk_file_selection_new (_("Open file..."));
	gtk_window_set_transient_for (GTK_WINDOW (file_selection_widget), GTK_WINDOW (app));

	open_info = g_new (OpenInfo, 1);
	open_info->app = GTK_WIDGET (app);
	open_info->control = control;
	open_info->file_selection_widget = file_selection_widget;

	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (file_selection_widget)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (file_selection_widget));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selection_widget)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (file_selection_open_cb),
			    open_info);

	gtk_signal_connect (GTK_OBJECT (file_selection_widget), "destroy",
			    GTK_SIGNAL_FUNC (file_selection_destroy_cb), NULL);

	gtk_widget_show (file_selection_widget);
}


static void
exit_cb (GtkWidget *widget,
	 gpointer data)
{
	gtk_main_quit ();
}


static GnomeUIInfo file_menu_info[] = {
	GNOMEUIINFO_MENU_OPEN_ITEM (open_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menu_info[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu_info),
	GNOMEUIINFO_END
};


static guint
container_create (void)
{
	GtkWidget *app;
	GtkWidget *control;

	app = gnome_app_new ("test-html-editor-control",
			     "HTML Editor Control Test");
	gtk_window_set_default_size (GTK_WINDOW (app), 500, 440);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	control = gnome_bonobo_widget_new_control ("control:html-editor");
	if (control == NULL)
		g_error ("Cannot get `control:html-editor'.");

	gnome_app_set_contents (GNOME_APP (app), control);
	gnome_app_create_menus_with_data (GNOME_APP (app), menu_info, app);

	gtk_widget_show_all (app);

	return FALSE;
}

int
main (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_ORB orb;

	CORBA_exception_init (&ev);

	gnome_CORBA_init ("test-html-editor-control", "1.0", &argc, argv, 0, &ev);

	CORBA_exception_free (&ev);

	orb = gnome_CORBA_ORB ();

	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error ("Could not initialize Bonobo\n");

	/*
	 * We can't make any CORBA calls unless we're in the main
	 * loop.  So we delay creating the container here.
	 */
	gtk_idle_add ((GtkFunction) container_create, NULL);

	bonobo_main ();

	return 0;
}
