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
#include <bonobo.h>


#ifdef USING_OAF
#define HTML_EDITOR_CONTROL_ID "OAFIID:control:html-editor:63c5499b-8b0c-475a-9948-81ec96a9662c"
#else
#define HTML_EDITOR_CONTROL_ID "control:html-editor"
#endif


/* Saving/loading through PersistStream.  */

static void
load_through_persist_stream (const gchar *filename,
			     Bonobo_PersistStream pstream)
{
	BonoboStream *stream;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	stream = bonobo_stream_fs_open (filename, Bonobo_Storage_READ);
	if (stream == NULL) {
		g_warning ("Couldn't load `%s'\n", filename);
	} else {
		BonoboObject *stream_object;
		Bonobo_Stream corba_stream;

		stream_object = BONOBO_OBJECT (stream);
		corba_stream = bonobo_object_corba_objref (stream_object);
		Bonobo_PersistStream_load (pstream, corba_stream,
					   "text/html", &ev);
	}

	Bonobo_Unknown_unref (pstream, &ev);
	CORBA_Object_release (pstream, &ev);

	CORBA_exception_free (&ev);
}

static void
save_through_persist_stream (const gchar *filename,
			     Bonobo_PersistStream pstream)
{
	BonoboStream *stream;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	stream = bonobo_stream_fs_create (filename);

	if (stream == NULL) {
		g_warning ("Couldn't create `%s'\n", filename);
	} else {
		BonoboObject *stream_object;
		Bonobo_Stream corba_stream;

		stream_object = BONOBO_OBJECT (stream);
		corba_stream = bonobo_object_corba_objref (stream_object);
		Bonobo_PersistStream_save (pstream, corba_stream,
					   "text/html", &ev);
	}

	Bonobo_Unknown_unref (pstream, &ev);
	CORBA_Object_release (pstream, &ev);

	CORBA_exception_free (&ev);
}


/* Loading/saving through PersistFile.  */

static void
load_through_persist_file (const gchar *filename,
			   Bonobo_PersistFile pfile)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_PersistFile_load (pfile, filename, &ev);

	if (ev._major != CORBA_NO_EXCEPTION)
		g_warning ("Cannot load.");

	CORBA_exception_free (&ev);
}

static void
save_through_persist_file (const gchar *filename,
			   Bonobo_PersistFile pfile)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	Bonobo_PersistFile_save (pfile, filename, &ev);

	if (ev._major != CORBA_NO_EXCEPTION)
		g_warning ("Cannot save.");

	CORBA_exception_free (&ev);
}


/* Common file selection widget.  We make sure there is only one open at a
   given time.  */

enum _FileSelectionOperation {
	OP_NONE,
	OP_SAVE_THROUGH_PERSIST_STREAM,
	OP_LOAD_THROUGH_PERSIST_STREAM,
	OP_SAVE_THROUGH_PERSIST_FILE,
	OP_LOAD_THROUGH_PERSIST_FILE
};
typedef enum _FileSelectionOperation FileSelectionOperation;

struct _FileSelectionInfo {
	BonoboWidget *control;
	GtkWidget *app;
	GtkWidget *widget;

	FileSelectionOperation operation;
};
typedef struct _FileSelectionInfo FileSelectionInfo;

static FileSelectionInfo file_selection_info = {
	NULL,
	NULL,
	NULL,
	OP_NONE
};

static void
file_selection_destroy_cb (GtkWidget *widget,
			   gpointer data)
{
	file_selection_info.widget = NULL;
}

static void
file_selection_ok_cb (GtkWidget *widget,
		      gpointer data)
{
	CORBA_Object interface;
	BonoboObjectClient *object_client;
	const gchar *interface_name;

	object_client = bonobo_widget_get_server (file_selection_info.control);

	if (file_selection_info.operation == OP_SAVE_THROUGH_PERSIST_FILE
	    || file_selection_info.operation == OP_LOAD_THROUGH_PERSIST_FILE)
		interface_name = "IDL:Bonobo/PersistFile:1.0";
	else
		interface_name = "IDL:Bonobo/PersistStream:1.0";

	interface = bonobo_object_client_query_interface (object_client, interface_name,
							  NULL);

	if (interface == CORBA_OBJECT_NIL) {
		g_warning ("The Control does not seem to support `%s'.", interface_name);
	} else 	 {
		const gchar *fname;
	
		fname = gtk_file_selection_get_filename
			(GTK_FILE_SELECTION (file_selection_info.widget));

		switch (file_selection_info.operation) {
		case OP_LOAD_THROUGH_PERSIST_STREAM:
			load_through_persist_stream (fname, interface);
			break;
		case OP_SAVE_THROUGH_PERSIST_STREAM:
			save_through_persist_stream (fname, interface);
			break;
		case OP_LOAD_THROUGH_PERSIST_FILE:
			load_through_persist_file (fname, interface);
			break;
		case OP_SAVE_THROUGH_PERSIST_FILE:
			save_through_persist_file (fname, interface);
			break;
		default:
			g_assert_not_reached ();
		}
	} 

	gtk_widget_destroy (file_selection_info.widget);
}


static void
open_or_save_as_dialog (GnomeApp *app,
			FileSelectionOperation op)
{
	BonoboWidget *control;
	GtkWidget *widget;

	control = BONOBO_WIDGET (app->contents);

	if (file_selection_info.widget != NULL) {
		gdk_window_show (GTK_WIDGET (file_selection_info.widget)->window);
		return;
	}

	if (op == OP_LOAD_THROUGH_PERSIST_FILE || op == OP_LOAD_THROUGH_PERSIST_STREAM)
		widget = gtk_file_selection_new (_("Open file..."));
	else
		widget = gtk_file_selection_new (_("Save file as..."));

	gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (app));

	file_selection_info.widget = widget;
	file_selection_info.app = GTK_WIDGET (app);
	file_selection_info.control = control;
	file_selection_info.operation = op;

	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (widget)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (widget));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (file_selection_ok_cb),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (file_selection_info.widget), "destroy",
			    GTK_SIGNAL_FUNC (file_selection_destroy_cb),
			    NULL);

	gtk_widget_show (file_selection_info.widget);
}

/* "Open through persist stream" dialog.  */
static void
open_through_persist_stream_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (GNOME_APP (data), OP_LOAD_THROUGH_PERSIST_STREAM);
}

/* "Save through persist stream" dialog.  */
static void
save_through_persist_stream_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (GNOME_APP (data), OP_SAVE_THROUGH_PERSIST_STREAM);
}

/* "Open through persist file" dialog.  */
static void
open_through_persist_file_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (GNOME_APP (data), OP_LOAD_THROUGH_PERSIST_FILE);
}

/* "Save through persist file" dialog.  */
static void
save_through_persist_file_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (GNOME_APP (data), OP_SAVE_THROUGH_PERSIST_FILE);
}


static void
exit_cb (GtkWidget *widget,
	 gpointer data)
{
	gtk_main_quit ();
}


static GnomeUIInfo file_menu_info[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Open (PersistFile)"), N_("Open using the PersistFile interface"),
				open_through_persist_file_cb, GNOME_STOCK_MENU_OPEN),
	GNOMEUIINFO_ITEM_STOCK (N_("Save as... (PersistFile)"), N_("Save using the PersistFile interface"),
				save_through_persist_file_cb, GNOME_STOCK_MENU_SAVE_AS),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_STOCK (N_("Open (PersistStream)"), N_("Open using the PersistStream interface"),
				open_through_persist_stream_cb, GNOME_STOCK_MENU_OPEN),
	GNOMEUIINFO_ITEM_STOCK (N_("Save as... (PersistStream)"), N_("Save using the PersistStream interface"),
				save_through_persist_stream_cb, GNOME_STOCK_MENU_SAVE_AS),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo menu_info[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu_info),
	GNOMEUIINFO_END
};

#if 0
static GnomeUIInfo toolbar_info[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Cut"), N_("Cut selection to the clipboard"),
				NULL, GNOME_STOCK_PIXMAP_CUT),
	GNOMEUIINFO_ITEM_STOCK (N_("Copy"), N_("Copy selection to the clipboard"),
				NULL, GNOME_STOCK_PIXMAP_COPY),
	GNOMEUIINFO_ITEM_STOCK (N_("Paste"), N_("Paste contents of the clipboard"),
				NULL, GNOME_STOCK_PIXMAP_PASTE),
	GNOMEUIINFO_END
};
#endif


static guint
container_create (void)
{
	GtkWidget *app;
	GtkWidget *control;
	BonoboUIHandler *uih;
	BonoboUIHandlerMenuItem *tree;

	app = gnome_app_new ("test-html-editor-control",
			     "HTML Editor Control Test");
	gtk_window_set_default_size (GTK_WINDOW (app), 500, 440);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	uih = bonobo_ui_handler_new ();
	bonobo_ui_handler_set_app (uih, GNOME_APP (app));

	/* Create the menus/toolbars */
	bonobo_ui_handler_create_menubar (uih);

	tree = bonobo_ui_handler_menu_parse_uiinfo_tree_with_data (menu_info, app);
	bonobo_ui_handler_menu_add_tree (uih, "/", tree);
	bonobo_ui_handler_menu_free_tree (tree);

	control = bonobo_widget_new_control (HTML_EDITOR_CONTROL_ID,
					     bonobo_object_corba_objref (BONOBO_OBJECT (uih)));

	if (control == NULL)
		g_error ("Cannot get `%s'.", HTML_EDITOR_CONTROL_ID);

	gnome_app_set_contents (GNOME_APP (app), control);

	gtk_widget_show_all (app);

	return FALSE;
}

#ifdef USING_OAF

#include <liboaf/liboaf.h>

static void
init_corba (int *argc, char **argv)
{
	gnome_init_with_popt_table ("test-html-editor-control", "1.0", *argc, argv,
				    NULL, 0, NULL);
	oaf_init (*argc, argv);
}

#else  /* USING_OAF */

#include <libgnorba/gnorba.h>

static void
init_corba (int *argc, char **argv)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	gnome_CORBA_init ("test-html-editor-control", "1.0", argc, argv, 0, &ev);

	CORBA_exception_free (&ev);
}

#endif /* USING_OAF */

int
main (int argc, char **argv)
{
	init_corba (&argc, argv);

	if (bonobo_init (CORBA_OBJECT_NIL, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL) == FALSE)
		g_error ("Could not initialize Bonobo\n");

	/* We can't make any CORBA calls unless we're in the main loop.  So we
	   delay creating the container here. */
	gtk_idle_add ((GtkFunction) container_create, NULL);

	bonobo_main ();

	return 0;
}
