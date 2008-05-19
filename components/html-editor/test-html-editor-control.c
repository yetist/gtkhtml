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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    Authors: Ettore Perazzoli <ettore@helixcode.com>
*/

#include <config.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-app-helper.h>
#include <bonobo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib.h>
#include <Editor.h>
#include "editor-control-factory.h"

#include "gtkhtml.h"
#include "gtkhtml-private.h"
#include "gtkhtml-properties.h"
#include "htmlsourceview.h"

static GtkWidget *control;
static gint formatHTML = 1;


/* Saving/loading through PersistStream.  */

static void
load_through_persist_stream (const gchar *filename,
			     Bonobo_PersistStream pstream)
{
	BonoboObject *stream = NULL;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	/* FIX2 stream = bonobo_stream_open (BONOBO_IO_DRIVER_FS, filename,
	   Bonobo_Storage_READ, 0); */

	if (stream == NULL) {
		g_warning ("Couldn't load `%s'\n", filename);
	} else {
		Bonobo_Stream corba_stream;

		corba_stream = bonobo_object_corba_objref (stream);
		Bonobo_Stream_truncate (corba_stream, 0, &ev);
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
	BonoboObject *stream = NULL;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	/* FIX2 stream = bonobo_stream_open (BONOBO_IO_DRIVER_FS, filename,
				     Bonobo_Storage_WRITE |
				     Bonobo_Storage_CREATE |
				     Bonobo_Storage_FAILIFEXIST, 0); */

	if (stream == NULL) {
		g_warning ("Couldn't create `%s'\n", filename);
	} else {
		Bonobo_Stream corba_stream;

		corba_stream = bonobo_object_corba_objref (stream);
		Bonobo_PersistStream_save (pstream, corba_stream, "text/html", &ev);
	}

	Bonobo_Unknown_unref (pstream, &ev);
	CORBA_Object_release (pstream, &ev);

	CORBA_exception_free (&ev);
}

static void
save_through_plain_persist_stream (const gchar *filename,
			     Bonobo_PersistStream pstream)
{
	BonoboObject *stream = NULL;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	stream = bonobo_stream_mem_create (NULL, 0, FALSE, TRUE);

	if (stream == NULL) {
		g_warning ("Couldn't create `%s'\n", filename);
	} else {
		Bonobo_Stream corba_stream;
		BonoboStreamMem *stream_mem;
		GByteArray *text;
		int fd;

		corba_stream = bonobo_object_corba_objref (stream);
		Bonobo_Stream_truncate (corba_stream, 0, &ev);
		Bonobo_PersistStream_save (pstream, corba_stream, "text/plain", &ev);

		stream_mem = BONOBO_STREAM_MEM (stream);
		text = g_byte_array_new ();
		g_byte_array_append (text, (guint8 *) stream_mem->buffer, stream_mem->pos);
		bonobo_object_unref (BONOBO_OBJECT (stream));

		fd = creat (filename, 0622);
		write (fd, text->data, text->len);
		close (fd);
		g_byte_array_free (text, TRUE);
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
	OP_SAVE_THROUGH_PLAIN_PERSIST_STREAM,
	OP_LOAD_THROUGH_PERSIST_STREAM,
	OP_SAVE_THROUGH_PERSIST_FILE,
	OP_LOAD_THROUGH_PERSIST_FILE
};
typedef enum _FileSelectionOperation FileSelectionOperation;

static void
view_source_dialog (BonoboWindow *app, char *type, gboolean as_html)
{
	BonoboWidget *control;
	GtkWidget *window;
	GtkWidget *view;
	BonoboControlFrame *cf;

	control = BONOBO_WIDGET (bonobo_window_get_contents (app));

	cf = bonobo_widget_get_control_frame (BONOBO_WIDGET (control));
	bonobo_control_frame_control_activate (cf);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	view = html_source_view_new ();

	gtk_container_add (GTK_CONTAINER (window), view);
	html_source_view_set_source (HTML_SOURCE_VIEW (view), control, type);
	html_source_view_set_mode (HTML_SOURCE_VIEW (view), as_html);
	gtk_widget_show_all (window);

	g_signal_connect_swapped (control, "destroy", G_CALLBACK (gtk_widget_destroy), window);
}

static void
view_html_source_cb (GtkWidget *widget,
		     gpointer data)
{
	view_source_dialog (data, "text/html", FALSE);
}

static void
view_plain_source_cb (GtkWidget *widget,
		      gpointer data)
{
	view_source_dialog (data, "text/plain", FALSE);
}

static void
view_html_source_html_cb (GtkWidget *widget,
			  gpointer data)
{
	view_source_dialog (data, "text/html", TRUE);
}


static void
open_or_save_as_dialog (BonoboWindow *app,
			FileSelectionOperation operation)
{
	CORBA_Object interface;
	const gchar *interface_name;
	CORBA_Environment ev;
	GtkWidget *widget;
	BonoboWidget *control;
	gchar *filename;

	control = BONOBO_WIDGET (bonobo_window_get_contents (app));

	if (operation == OP_LOAD_THROUGH_PERSIST_FILE ||
			operation == OP_LOAD_THROUGH_PERSIST_STREAM)
		widget = gtk_file_chooser_dialog_new (
			_("Open file..."), GTK_WINDOW (app),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_APPLY,
			NULL);
	else
		widget = gtk_file_chooser_dialog_new (
			_("Save file as..."), GTK_WINDOW (app),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_APPLY,
			NULL);

	if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_CANCEL)
		goto exit;

	if (operation == OP_SAVE_THROUGH_PERSIST_FILE ||
			operation == OP_LOAD_THROUGH_PERSIST_FILE)
		interface_name = "IDL:Bonobo/PersistFile:1.0";
	else
		interface_name = "IDL:Bonobo/PersistStream:1.0";

	CORBA_exception_init (&ev);
	interface = Bonobo_Unknown_queryInterface (
		bonobo_widget_get_objref (control), interface_name, &ev);
	CORBA_exception_free (&ev);

	if (interface == CORBA_OBJECT_NIL) {
		g_warning (
			"The Control does not seem to support `%s'.",
			interface_name);
		goto exit;
	}

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));

	switch (operation) {
	case OP_LOAD_THROUGH_PERSIST_STREAM:
		load_through_persist_stream (filename, interface);
		break;
	case OP_SAVE_THROUGH_PERSIST_STREAM:
		save_through_persist_stream (filename, interface);
		break;
	case OP_SAVE_THROUGH_PLAIN_PERSIST_STREAM:
		save_through_plain_persist_stream (filename, interface);
		break;
	case OP_LOAD_THROUGH_PERSIST_FILE:
		load_through_persist_file (filename, interface);
		break;
	case OP_SAVE_THROUGH_PERSIST_FILE:
		save_through_persist_file (filename, interface);
		break;
	default:
		;
		//g_assert_not_reached ();
	}

	g_free (filename);

exit:
	gtk_widget_destroy (widget);
}

/* "Open through persist stream" dialog.  */
static void
open_through_persist_stream_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (BONOBO_WINDOW (data), OP_LOAD_THROUGH_PERSIST_STREAM);
}

/* "Save through persist stream" dialog.  */
static void
save_through_persist_stream_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (BONOBO_WINDOW (data), OP_SAVE_THROUGH_PERSIST_STREAM);
}

/* "Save through persist stream" dialog.  */
static void
save_through_plain_persist_stream_cb (GtkWidget *widget,
				gpointer data)
{
	open_or_save_as_dialog (BONOBO_WINDOW (data), OP_SAVE_THROUGH_PLAIN_PERSIST_STREAM);
}

/* "Open through persist file" dialog.  */
static void
open_through_persist_file_cb (GtkWidget *widget,
			      gpointer data)
{
	open_or_save_as_dialog (BONOBO_WINDOW (data), OP_LOAD_THROUGH_PERSIST_FILE);
}

/* "Save through persist file" dialog.  */
static void
save_through_persist_file_cb (GtkWidget *widget,
			      gpointer data)
{
	open_or_save_as_dialog (BONOBO_WINDOW (data), OP_SAVE_THROUGH_PERSIST_FILE);
}


static void
exit_cb (GtkWidget *widget,
	 gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (data));
	bonobo_main_quit ();
}



static BonoboUIVerb verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("OpenFile",   open_through_persist_file_cb),
	BONOBO_UI_UNSAFE_VERB ("SaveFile",   save_through_persist_file_cb),
	BONOBO_UI_UNSAFE_VERB ("OpenStream", open_through_persist_stream_cb),
	BONOBO_UI_UNSAFE_VERB ("SaveStream", save_through_persist_stream_cb),
	BONOBO_UI_UNSAFE_VERB ("SavePlainStream", save_through_plain_persist_stream_cb),
	BONOBO_UI_UNSAFE_VERB ("ViewHTMLSource", view_html_source_cb),
	BONOBO_UI_UNSAFE_VERB ("ViewHTMLSourceHTML", view_html_source_html_cb),
	BONOBO_UI_UNSAFE_VERB ("ViewPlainSource", view_plain_source_cb),
	BONOBO_UI_UNSAFE_VERB ("FileExit", exit_cb),

	BONOBO_UI_VERB_END
};

static void
menu_format_html_cb (BonoboUIComponent           *component,
		     const char                  *path,
		     Bonobo_UIComponent_EventType type,
		     const char                  *state,
		     gpointer                     user_data)

{
	if (type != Bonobo_UIComponent_STATE_CHANGED)
		return;
	formatHTML = *state == '0' ? 0 : 1;

	bonobo_widget_set_property (BONOBO_WIDGET (user_data), "FormatHTML", TC_CORBA_boolean, formatHTML, NULL);
	bonobo_widget_set_property (BONOBO_WIDGET (user_data), "HTMLTitle", TC_CORBA_string, "testing", NULL);
}

/* A dirty, non-translatable hack */
static char ui [] =
"<Root>"
"	<commands>"
"	        <cmd name=\"FileExit\" _label=\"Exit\" _tip=\"Exit the program\""
"	         pixtype=\"stock\" pixname=\"Exit\" accel=\"*Control*q\"/>"
"	        <cmd name=\"FormatHTML\" _label=\"HTML mode\" type=\"toggle\" _tip=\"HTML Format switch\"/>"
"	</commands>"
"	<menu>"
"		<submenu name=\"File\" _label=\"_File\">"
"			<menuitem name=\"OpenFile\" verb=\"\" _label=\"Open (PersistFile)\" _tip=\"Open using the PersistFile interface\""
"			pixtype=\"stock\" pixname=\"Open\"/>"
"			<menuitem name=\"SaveFile\" verb=\"\" _label=\"Save (PersistFile)\" _tip=\"Save using the PersistFile interface\""
"			pixtype=\"stock\" pixname=\"Save\"/>"
"			<separator/>"
"			<menuitem name=\"OpenStream\" verb=\"\" _label=\"_Open (PersistStream)\" _tip=\"Open using the PersistStream interface\""
"			pixtype=\"stock\" pixname=\"Open\"/>"
"			<menuitem name=\"SaveStream\" verb=\"\" _label=\"_Save (PersistStream)\" _tip=\"Save using the PersistStream interface\""
"			pixtype=\"stock\" pixname=\"Save\"/>"
"			<menuitem name=\"SavePlainStream\" verb=\"\" _label=\"Save _plain(PersistStream)\" _tip=\"Save using the PersistStream interface\""
"			pixtype=\"stock\" pixname=\"Save\"/>"
"			<separator/>"
"                       <menuitem name=\"ViewHTMLSource\" verb=\"\" _label=\"View HTML Source\" _tip=\"View the html source of the current document\"/>"
"                       <menuitem name=\"ViewHTMLSourceHTML\" verb=\"\" _label=\"View HTML Output\" _tip=\"View the html source of the current document as html\"/>"
"                       <menuitem name=\"ViewPlainSource\" verb=\"\" _label=\"View PLAIN Source\" _tip=\"View the plain text source of the current document\"/>"
"			<separator/>"
"			<menuitem name=\"FileExit\" verb=\"\" _label=\"E_xit\"/>"
"		</submenu>"
"		<placeholder name=\"Component\"/>"
"		<submenu name=\"Format\" _label=\"For_mat\">"
"			<menuitem name=\"FormatHTML\" verb=\"\"/>"
"                       <separator/>"
"                       <placeholder name=\"FormatParagraph\"/>"
"               </submenu>"
"	</menu>"
"	<dockitem name=\"Toolbar\" behavior=\"exclusive\">"
"	</dockitem>"
"</Root>";

static int
app_delete_cb (GtkWidget *widget, GdkEvent *event, gpointer dummy)
{
	gtk_widget_destroy (GTK_WIDGET (widget));
	bonobo_main_quit ();

	return FALSE;
}

static guint
container_create (void)
{
	GtkWidget *win;
	GtkWindow *window;
	BonoboUIComponent *component;
	BonoboUIContainer *container;
	CORBA_Environment ev;
	GNOME_GtkHTML_Editor_Engine engine;

	win = bonobo_window_new ("test-editor",
				 "HTML Editor Control Test");
	window = GTK_WINDOW (win);

	container = bonobo_window_get_ui_container (BONOBO_WINDOW (win));

	g_signal_connect (window, "delete_event", G_CALLBACK (app_delete_cb), NULL);

	gtk_window_set_default_size (window, 600, 440);
	gtk_window_set_resizable (window, TRUE);

	component = bonobo_ui_component_new ("test-editor");
	bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (component));

	bonobo_ui_component_set_container (component, BONOBO_OBJREF (container), NULL);
	bonobo_ui_component_add_verb_list_with_data (component, verbs, win);
	bonobo_ui_component_set_translate (component, "/", ui, NULL);

	control = bonobo_widget_new_control (CONTROL_ID, BONOBO_OBJREF (container));

	if (control == NULL)
		g_error ("Cannot get `%s'.", CONTROL_ID);

	bonobo_widget_set_property (BONOBO_WIDGET (control), "FormatHTML", TC_CORBA_boolean, formatHTML, NULL);
	bonobo_ui_component_set_prop (component, "/commands/FormatHTML", "state", formatHTML ? "1" : "0", NULL);
	bonobo_ui_component_add_listener (component, "FormatHTML", menu_format_html_cb, control);


	bonobo_window_set_contents (BONOBO_WINDOW (win), control);

	gtk_widget_show_all (GTK_WIDGET (window));

	CORBA_exception_init (&ev);
	engine = (GNOME_GtkHTML_Editor_Engine) Bonobo_Unknown_queryInterface
		(bonobo_widget_get_objref (BONOBO_WIDGET (control)), "IDL:GNOME/GtkHTML/Editor/Engine:1.0", &ev);
	GNOME_GtkHTML_Editor_Engine_runCommand (engine, "grab-focus", &ev);
	bonobo_object_release_unref (engine, &ev);
	CORBA_exception_free (&ev);

	bonobo_widget_set_property (BONOBO_WIDGET (control),
				    "MagicSmileys", TC_CORBA_boolean, TRUE,
				    NULL);
	bonobo_widget_set_property (BONOBO_WIDGET (control),
				    "MagicLinks", TC_CORBA_boolean, TRUE,
				    NULL);
	bonobo_widget_set_property (BONOBO_WIDGET (control),
				   "InlineSpelling", TC_CORBA_boolean, TRUE,
				   NULL);


	return FALSE;
}

static gint
load_file (const gchar *fname)
{
	CORBA_Object interface;
	CORBA_Environment ev;

	printf ("loading: %s\n", fname);
	CORBA_exception_init (&ev);
	interface = Bonobo_Unknown_queryInterface (bonobo_widget_get_objref (BONOBO_WIDGET (control)),
						   "IDL:Bonobo/PersistFile:1.0", &ev);
	CORBA_exception_free (&ev);
	load_through_persist_file (fname, interface);

	return FALSE;
}

int
main (int argc, char **argv)
{
	bindtextdomain(GTKHTML_RELEASE_STRING, GNOMELOCALEDIR);
	textdomain(GTKHTML_RELEASE_STRING);

	gnome_program_init("test-editor", VERSION, LIBGNOMEUI_MODULE, argc, argv,
			   GNOME_PROGRAM_STANDARD_PROPERTIES,
			   GNOME_PARAM_HUMAN_READABLE_NAME, _("GtkHTML Editor Test Container"),
			   NULL);

	bonobo_activate ();

	/* We can't make any CORBA calls unless we're in the main loop.  So we
	   delay creating the container here. */
	g_idle_add ((GtkFunction) container_create, NULL);
	if (argc > 1 && *argv [argc - 1] != '-')
		g_idle_add ((GtkFunction) load_file, argv [argc - 1]);

	bonobo_activate ();
	bonobo_main ();

	return bonobo_ui_debug_shutdown ();
}
