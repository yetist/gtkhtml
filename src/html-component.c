/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gnome.h>
#include <bonobo/gnome-bonobo.h>
#include "gtkhtml.h"

/*
 * The Embeddable data.
 *
 * This is where we store the document's abstract data.  Each
 * on-screen representation of the document (a GnomeView) will be
 * based on the data in this structure.
 */
typedef struct {
	GnomeEmbeddable     *embeddable;
	GNOME_Stream         stream;
	/*
	 * The widget used to render the view of the html data.
	 */
	GtkHTML *html;
	int views;
} embeddable_data_t;

/*
 * The per-view data.
 */
typedef struct {
	GnomeView	    *view;
	embeddable_data_t   *bed;
} view_data_t;

/*                                                                              
 * Number of running objects                                                    
 */                                                                             
static int running_objects = 0;                                                 
static GnomeEmbeddableFactory *factory = NULL;                                  
                                                                                 
/*
 * Clean up our supplementary GnomeEmbeddable data structures.
 */
static void
embeddable_destroy_cb (GnomeEmbeddable *embeddable, embeddable_data_t *embeddable_data)
{
	g_message ("embeddable_destroy_cb");

	/* FIXME: free stuff. */
	g_free (embeddable_data); 
                                                                                
	running_objects--;                                                      
	if (running_objects > 0)                                             
		return;                                                         

	if (embeddable_data->stream) {
		CORBA_Environment ev;
		CORBA_exception_init (&ev);
		CORBA_Object_release (embeddable_data->stream, &ev);
		embeddable_data->stream = NULL;
		CORBA_exception_free (&ev);
	}
	
	/*                                                                      
	 * When last object has gone unref the factory & quit.              
	 */                                                                     
	gnome_object_unref (GNOME_OBJECT (factory));                            
	gtk_main_quit ();           
}

/*
 * This callback is invoked when the GnomeEmbeddable object
 * encounters a fatal CORBA exception.
 */
static void
embeddable_system_exception_cb (GnomeEmbeddable *embeddable, CORBA_Object corba_object,
				CORBA_Environment *ev, gpointer data)
{
	gnome_object_destroy (GNOME_OBJECT (embeddable));
}

/*
 * The view encounters a fatal corba exception.
 */
static void
view_system_exception_cb (GnomeView *view, CORBA_Object corba_object,
			  CORBA_Environment *ev, gpointer data)
{
	gnome_object_destroy (GNOME_OBJECT (view));
}

static void 
redraw_view (GnomeView *view, void *data)
{
	view_data_t *view_data;

	view_data = gtk_object_get_data (GTK_OBJECT (view), "view_data");
	gtk_widget_queue_draw (GTK_WIDGET (view_data->bed->html));
}

/*
 * This function updates all of an embeddable's views to reflect the
 * image data stored in the embeddable.
 */
static void
embeddable_update_all_views (embeddable_data_t *embeddable_data)
{
	GnomeEmbeddable *embeddable;

	embeddable = embeddable_data->embeddable;

	gnome_embeddable_foreach_view (embeddable, redraw_view, NULL);
}

static void
html_menu_test_cb (GnomeUIHandler *uih, view_data_t *view_data, gchar *path)
{
	FILE *fil;
	gchar buffer[32768];
	gchar *filename;
	gint test_nr = 0;
	GtkHTMLStreamHandle handle;

	if (strstr (path, "Test 1") != 0)
		test_nr = 1;
	else if (strstr (path, "Test 2") != 0)
		test_nr = 2;
	else if (strstr (path, "Test 3") != 0)
		test_nr = 3;
	else if (strstr (path, "Test 4") != 0)
		test_nr = 4;
	else if (strstr (path, "Test 5") != 0)
		test_nr = 5;
	else if (strstr (path, "Test 6") != 0)
		test_nr = 6;
	else if (strstr (path, "Test 7") != 0)
		test_nr = 7;
	else
		return;

	filename = g_strdup_printf ("tests/test%d.html", test_nr);

	g_print ("Parsing file: %s\n", filename);

	handle = gtk_html_begin (view_data->bed->html, filename);

	fil = fopen (filename, "r");
	if(fil)
	  {
	    while (!feof (fil)) {
	      fgets (buffer, 32768, fil);
	      gtk_html_write (view_data->bed->html, handle, buffer, strlen(buffer));
	    }
	    fclose (fil);
	    gtk_html_parse (view_data->bed->html);
	    gtk_html_end (view_data->bed->html, handle, GTK_HTML_STREAM_OK);
	  }
	else
	  gtk_html_end (view_data->bed->html, handle, GTK_HTML_STREAM_ERROR);

	g_free (filename);
}

/*
 * When one of our views is activated, we merge our menus
 * in with our container's menus.
 */
static void
view_create_menus (view_data_t *view_data)
{
	GNOME_UIHandler remote_uih;
	GnomeView *view = view_data->view;
	GnomeUIHandler *uih;

	/*
	 * Grab our GnomeUIHandler object.
	 */
	uih = gnome_view_get_ui_handler (view);

	/*
	 * Get our container's UIHandler server.
	 */
	remote_uih = gnome_view_get_remote_ui_handler (view);

	/*
	 * We have to deal gracefully with containers
	 * which don't have a UIHandler running.
	 */
	if (remote_uih == CORBA_OBJECT_NIL)
		return;

	/*
	 * Give our GnomeUIHandler object a reference to the
	 * container's UIhandler server.
	 */
	gnome_ui_handler_set_container (uih, remote_uih);

	/*
	 * Create our menu entries.
	 */
	gnome_ui_handler_menu_new_subtree (uih, "/Html",
					   N_("Html"),
					   N_("Html test"),
					   1,
					   GNOME_UI_HANDLER_PIXMAP_NONE, NULL,
					   0, (GdkModifierType)0);
	
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 1",
					N_("Test1"), N_("Test1"),
					1, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);

	gnome_ui_handler_menu_new_item (uih, "/Html/Test 2",
					N_("Test2"), N_("Test2"),
					2, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 3",
					N_("Test3"), N_("Test3"),
					3, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 4",
					N_("Test4"), N_("Test4"),
					4, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 5",
					N_("Test5"), N_("Test5"),
					5, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 6",
					N_("Test6"), N_("Test6"),
					6, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
	gnome_ui_handler_menu_new_item (uih, "/Html/Test 7",
					N_("Test7"), N_("Test7"),
					7, 0,
					0, 
					0, (GdkModifierType) 0,
					GTK_SIGNAL_FUNC (html_menu_test_cb), view_data);
}

/*
 * When this view is deactivated, we must remove our menu items.
 */
static void
view_remove_menus (view_data_t *view_data)
{
	GnomeView *view = view_data->view;
	GnomeUIHandler *uih;

	uih = gnome_view_get_ui_handler (view);

	gnome_ui_handler_unset_container (uih);
}

static void
view_activate_cb (GnomeView *view, gboolean activate, view_data_t *view_data)
{
	/*
	 * The ViewFrame has just asked the View (that's us) to be
	 * activated or deactivated.  We must reply to the ViewFrame
	 * and say whether or not we want our activation state to
	 * change.  We are an acquiescent GnomeView, so we just agree
	 * with whatever the ViewFrame told us.  Most components
	 * should behave this way.
	 */
	gnome_view_activate_notify (view, activate);

	/*
	 * If we were just activated, we merge in our menu entries.
	 * If we were just deactivated, we remove them.
	 */
	if (activate)
		view_create_menus (view_data);
	else
		view_remove_menus (view_data);
}

/*
 * This callback is envoked when the view is destroyed.  We use it to
 * free up our ancillary view-centric data structures.
 */
static void
view_destroy_cb (GnomeView *view, view_data_t *view_data)
{
	g_message ("view_destroy_cb");
	
	/* FIXME: free stuff. */
	g_free (view_data);
}

/*
 * This callback is invoked when the container asks a view what size
 * it wants to be.
 */
static void
view_size_query_cb (GnomeView *view, int *desired_width, int *desired_height,
		    view_data_t *view_data)
{
	*desired_width  = view_data->bed->html->engine->width;
	*desired_height = view_data->bed->html->engine->height;
}

/*
 * This function is called when the "Refresh" verb is executed on
 * the component.
 */
static void
view_refresh_cb (GnomeView *view, const char *verb_name, view_data_t *view_data)
{
	embeddable_data_t *embeddable_data;

	embeddable_data = view_data->bed;
	embeddable_update_all_views (embeddable_data);
}

/*
 * This function is invoked whenever the container requests that a new
 * view be created for a given embeddable.  Its job is to construct the
 * new view and return it.
 */
static GnomeView *
view_factory (GnomeEmbeddable *embeddable,
	      const GNOME_ViewFrame view_frame,
	      embeddable_data_t *embeddable_data)
{
	view_data_t *view_data;
	GnomeUIHandler *uih;
	GnomeView *view;
	GtkWidget *vbox;

	g_message ("view_factory");

	/*
	 * Create the private view data.
	 */
	view_data = g_new0 (view_data_t, 1);
	view_data->bed = embeddable_data;

	if (view_data->bed->views > 0) {
		g_warning ("Doc/View error");
		g_free (view_data);
		return NULL;
	}
	view_data->bed->views++;

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox),
			    GTK_WIDGET (view_data->bed->html),
			    TRUE, TRUE, 0);

	gtk_widget_show_all (vbox);

	/*
	 * Create the GnomeView object.
	 */
	view = gnome_view_new (vbox);
	view_data->view = view;
	gtk_object_set_data (GTK_OBJECT (view), "view_data", view_data);

	/*
	 * Create the GnomeUIHandler for this view.  It will be used
	 * to merge menu and toolbar items when the view is activated.
	 */
	uih = gnome_ui_handler_new ();
	gnome_view_set_ui_handler (view, uih);

	/*
	 * Register a callback to handle the ClearImage verb.
	 */
	gnome_view_register_verb (view, "Refresh",
				  GNOME_VIEW_VERB_FUNC (view_refresh_cb),
				  view_data);

	/*
	 * The "size_query" signal is raised when the container asks
	 * the component what size it wants to be.
	 */
	gtk_signal_connect (GTK_OBJECT (view), "size_query",
			    GTK_SIGNAL_FUNC (view_size_query_cb), view_data);


	/*
	 * When our container wants to activate a given view of this
	 * component, we will get the "view_activate" signal.
	 */
	gtk_signal_connect (GTK_OBJECT (view), "view_activate",
			    GTK_SIGNAL_FUNC (view_activate_cb), view_data);

	/*
	 * The "system_exception" signal is raised when the GnomeView
	 * encounters a fatal CORBA exception.
	 */
	gtk_signal_connect (GTK_OBJECT (view), "system_exception",
			    GTK_SIGNAL_FUNC (view_system_exception_cb), view_data);

	/*
	 * We'll need to be able to cleanup when this view gets
	 * destroyed.
	 */
	gtk_signal_connect (GTK_OBJECT (view), "destroy",
			    GTK_SIGNAL_FUNC (view_destroy_cb), view_data);

	return view;
}

static void 
embeddable_load (embeddable_data_t *bed)
{
	CORBA_long len;
	CORBA_Environment ev;
	GNOME_Stream_iobuf *buffer;
	#define CHUNK      4096

	g_return_if_fail (bed != NULL);
	g_return_if_fail (bed->stream != NULL);

	gtk_html_begin (bed->html, "Unknown name");
	gtk_html_parse (bed->html);
	CORBA_exception_init (&ev);

	do {
		buffer = NULL;
		len = GNOME_Stream_read (bed->stream, CHUNK, &buffer, &ev);
		if (ev._major != CORBA_NO_EXCEPTION) {
			g_warning ("Failed read");
			CORBA_exception_free (&ev);
			return;
		}
		{ /* Ugly hack; we need a better gtkhtml API */
			char *tmp = g_malloc (CHUNK + 1);
			g_return_if_fail (tmp != NULL);
			memcpy (tmp, buffer->_buffer, buffer->_length);
			tmp [CHUNK] = 0;
			gtk_html_write (bed->html, tmp);
			g_free (tmp);
		}

		CORBA_free (buffer);
	} while (len > 0);
	gtk_html_end (bed->html);

	CORBA_exception_free (&ev);
}

/*
 * Loads a HTML from a GNOME_Stream
 */
static void
load_url(GtkHTML *html, const char *url, GtkHTMLStreamHandle handle)
{
  FILE *fil;

  fil = fopen(url, "r");
  if(!fil)
    {
      gtk_html_end(html, handle, GTK_HTML_STREAM_ERROR);
      return;
    }

  while(!feof(fil))
    {
      gchar buffer[32768];
      fgets(buffer, 32768, fil);
      gtk_html_write(html, handle, buffer, strlen(buffer));
    }
  fclose(fil);
  gtk_html_end(html, handle, GTK_HTML_STREAM_OK);
}

static int
load_html_from_stream (GnomePersistStream *ps,
		       GNOME_Stream stream, void *data)
{
	embeddable_data_t *bed = (embeddable_data_t *)data;
	CORBA_Environment ev;

	g_warning ("load_html");

	if (bed->stream) {
		g_warning ("Won't overwrite pre-existing stream: you wierdo");
		return 0;
	}
	CORBA_exception_init (&ev);

	/* We need this for later */
	CORBA_Object_duplicate (stream, &ev);
	g_return_val_if_fail (ev._major == CORBA_NO_EXCEPTION, 0);
	bed->stream = stream;

	embeddable_load (bed);

	CORBA_exception_free (&ev);
	return 0;
}

static int
save_html (GnomePersistStream *ps,
	   GNOME_Stream stream, void *data)
{
  g_warning ("Unimplemented");
  return -1;
}

/*
 * When a container asks our EmbeddableFactory for a new paint
 * component, this function is called.  It creates the new
 * GnomeEmbeddable object and returns it.
 */
static GnomeObject *
embeddable_factory (GnomeEmbeddableFactory *this,
		    void *data)
{
	GnomeEmbeddable    *embeddable;
	GnomePersistStream *stream;
	embeddable_data_t  *embeddable_data;

	g_message ("embeddable_factory");

	/*
	 * Create a data structure in which we can store
	 * Embeddable-object-specific data about this document.
	 */
	embeddable_data = g_new0 (embeddable_data_t, 1);
	if (embeddable_data == NULL) {
		g_warning ("Failed to allocate data");
		return NULL;
	}

	/*
	 * Create the GnomeEmbeddable object.
	 */
	embeddable = gnome_embeddable_new (GNOME_VIEW_FACTORY (view_factory),
					   embeddable_data);

	if (embeddable == NULL) {
		g_free (embeddable_data);
		g_warning ("Failed to create embeddable");
		return NULL;
	}

	/*
	 * Interface GNOME::PersistStream 
	 */
	embeddable_data->embeddable = embeddable;
	embeddable_data->stream = NULL;
	embeddable_data->html = GTK_HTML (gtk_html_new (NULL, NULL));
	gtk_signal_connect(GTK_OBJECT(embeddable_data->html), "url_requested",
			   load_url, embeddable_data);
	embeddable_data->views = 0;
	stream = gnome_persist_stream_new ("embeddable:html-component",
					   load_html_from_stream,
					   save_html,
					   embeddable_data);
	if (stream == NULL) {
		gtk_object_unref (GTK_OBJECT (embeddable));
		g_free (embeddable_data);
		g_warning ("Failed to bind persist stream interface");
		return NULL;
	}
	/*
	 * Bind the interfaces
	 */
	gnome_object_add_interface (GNOME_OBJECT (embeddable),
				    GNOME_OBJECT (stream));

	running_objects++;      

	/*
	 * Add some verbs to the embeddable.
	 *
	 * Verbs are simple non-paramameterized actions which the
	 * component can perform.  The GnomeEmbeddable must maintain a
	 * list of the verbs which a component supports, and the
	 * component author must register callbacks for each of his
	 * verbs with the GnomeView.
	 *
	 * The container application will then have the programmatic
	 * ability to execute the verbs on the component.  It will
	 * also provide a simple mechanism whereby the user can
	 * right-click on the component to create a popup menu
	 * listing the available verbs.
	 *
	 * We provide one simple verb whose job it is to clear the
	 * window.
	 */
	gnome_embeddable_add_verb (embeddable,
				   "Refresh",
				   _("_Refresh"),
				   _("Reload the page"));

	/*
	 * If the Embeddable encounters a fatal CORBA exception, it
	 * will emit a "system_exception" signal, notifying us that
	 * the object is defunct.  Our callback --
	 * embeddable_system_exception_cb() -- destroys the defunct
	 * GnomeEmbeddable object.
	 */
	gtk_signal_connect (GTK_OBJECT (embeddable), "system_exception",
			    GTK_SIGNAL_FUNC (embeddable_system_exception_cb),
			    embeddable_data);

	/*
	 * Catch the destroy signal so that we can free up resources.
	 * When an Embeddable is destroyed, its views will
	 * automatically be destroyed.
	 */
	gtk_signal_connect (GTK_OBJECT (embeddable), "destroy",
			    GTK_SIGNAL_FUNC (embeddable_destroy_cb),
			    embeddable_data);

	g_warning ("embeddable created OK");

	return GNOME_OBJECT (embeddable);
}

static GnomeEmbeddableFactory *
init_simple_paint_factory (void)
{
	return gnome_embeddable_factory_new (
		"embeddable-factory:html-component",
		embeddable_factory, NULL);
}

static void
init_server_factory (int argc, char **argv)
{
	CORBA_Environment ev;
	CORBA_ORB orb;

	CORBA_exception_init (&ev);

	gnome_CORBA_init_with_popt_table (
		"html-component", VERSION,
		&argc, argv, NULL, 0, NULL, GNORBA_INIT_SERVER_FUNC, &ev);

	CORBA_exception_free (&ev);

	orb = gnome_CORBA_ORB ();
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Could not initialize Bonobo!"));
}
 
int
main (int argc, char **argv)
{

	/*
	 * Setup the factory.
	 */
	init_server_factory (argc, argv);
	factory = init_simple_paint_factory ();

	gdk_rgb_init ();
	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());

	/*
	 * Start processing.
	 */
	bonobo_main ();

	return 0;
}
