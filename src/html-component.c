#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gnome.h>
#include <bonobo/gnome-bonobo.h>
#include "gtkhtml.h"
#include "htmlengine.h"
#include "htmltokenizer.h"

/*
 * The Embeddable data.
 *
 * This is where we store the document's abstract data.  Each
 * on-screen representation of the document (a GnomeView) will be
 * based on the data in this structure.
 */
typedef struct {
	GnomeEmbeddable     *embeddable;

	/*
	 * Ok, the internal representation should go here. I haven't figured
	 * that part out yet.
	 */
/*	HTMLEngine          *engine;*/
} embeddable_data_t;

/*
 * The per-view data.
 */
typedef struct {
	GnomeView	    *view;
	embeddable_data_t   *embeddable_data;

	/*
	 * The widget used to render this view of the image data.
	 */
	GtkLayout *layout;

	GtkWidget *vscrollbar;
	GtkObject *vsbadjust;

	gboolean displayVScroll;

	HTMLEngine *engine;
} view_data_t;

/*                                                                              
 * Number of running objects                                                    
 */                                                                             
static int running_objects = 0;                                                 
static GnomeEmbeddableFactory *factory = NULL;                                  
                                                                                 
static void calc_scrollbars (view_data_t *view_data);
static void vertical_scroll (GtkAdjustment *adjustment, gpointer data);
void update_view (GnomeView *view);

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

/*
 * The job of this function is to update the view from the
 * embeddable's representation of the image data.
 */
static void
view_update (view_data_t *view_data)
{
	html_painter_set_background_color (view_data->engine->painter,
					   view_data->engine->settings->bgcolor);
	gdk_window_clear (view_data->engine->painter->window);

	html_engine_draw (view_data->engine, 0, 0, view_data->engine->width, view_data->engine->height);
}


void 
update_view (GnomeView *view)
{
	view_data_t *view_data;

	view_data = gtk_object_get_data (GTK_OBJECT (view), "view_data");
	view_update (view_data);
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

	gnome_embeddable_foreach_view (embeddable, update_view, NULL);
}

static void
html_menu_test_cb (GnomeUIHandler *uih, view_data_t *view_data, gchar *path)
{
	FILE *fil;
	gchar buffer[32768];
	gchar *filename;
	gint test_nr = 0;

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

	html_engine_begin (view_data->engine, filename);
	html_engine_parse (view_data->engine);

	fil = fopen (filename, "r");
	while (!feof (fil)) {
		fgets (buffer, 32768, fil);
		html_engine_write (view_data->engine, buffer);
	}

	html_engine_end (view_data->engine);
	fclose (fil);
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
	
	/*FIXME: free stuff. */
	g_free (view_data);
}

/*
 * Perform the equivalence of gtk_html_realize ().
 */
static void
view_realize_cb (GtkWidget *widget, view_data_t *view_data)
{
	GtkLayout *layout;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_LAYOUT (widget));

	layout = GTK_LAYOUT (widget);

	gdk_window_set_events (layout->bin_window,
			       (gdk_window_get_events (layout->bin_window)
				| GDK_EXPOSURE_MASK));

	/* Create our painter dc */
	view_data->engine->painter->gc = gdk_gc_new (layout->bin_window);

	/* Set our painter window */
	view_data->engine->painter->window = layout->bin_window;

	view_data->displayVScroll = FALSE;
	gtk_widget_hide (view_data->vscrollbar);
	GTK_ADJUSTMENT (view_data->vsbadjust)->step_increment = 12;

	/* Just test the compontent in simplest possible way. */
#if 0
	html_engine_begin (view_data->engine, "tests/test1.html");
	html_engine_parse (view_data->engine);
	html_engine_write (view_data->engine, 
			"<html>"
			"  <body background=test.jpg>"
			"    <p align=center>"
			"      Det här är tänkt att vara ett väldigt långt och blandat dokument som ska göra det enkelt att se alla typer av kombinationer"
			"      <b>fetstil</b>"
			"      <font size=5>Annorlunda storlekar</font> och <font color=blue>färger.</font>"
			"    </p>"
			"    <h1>Här kommer en hr</h1>"
			"    <hr>"
			"    <p align='center'>Lite text efter</p>"
			"    <p>Här ska jag skriva en jätttejättelång text för att äntligen börja på scrollningen och se om det fungerar, för det måste det ju naturligtvis göra om man ska börja lägga in så att den kan bli en WIDGET!! och det vill man ju att det ska vara så att man kan lägga in en fil som heter testgtkhtml.c i CVS sedan, för då ska den vara textbädd för gtkhtml-widgeten och sen ska jag även lägga in bättre stöd i HTMLObject, ta bort de dumma referenserna till Tex curr och head och tail. Men nu tror jag att jag har skrivit tillräckligt mycket så det så!"
			"  </body>"
			"</html>");

	html_engine_end (view_data->engine);
#endif
	view_update (view_data);
}

/* 
 *
 * When a part of the window is exposed, we must redraw it.
 */
static void
view_expose_cb (GtkWidget *drawing_area, GdkEventExpose *event, view_data_t *view_data)
{
	if (view_data->engine->settings->bgcolor)
		html_painter_set_background_color (view_data->engine->painter,
						   view_data->engine->settings->bgcolor);
	
	html_engine_draw (view_data->engine,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);
}

static void
set_geometry (GtkWidget *widget, gint x, gint y, gint width, gint height)
{
	GtkAllocation allocation;

	allocation.x = x;
	allocation.y = y;
	allocation.width = width;
	allocation.height = height;

	gtk_widget_size_allocate (widget, &allocation);
}

/*
 * This callback is invoked when the container asks a view what size
 * it wants to be.
 */
static void
view_size_query_cb (GnomeView *view, int *desired_width, int *desired_height,
		    view_data_t *view_data)
{
	*desired_width  = view_data->engine->width;
	*desired_height = view_data->engine->height;
}

/*
 * This callback will be invoked when the container assigns us a size.
 */
static void
view_size_allocate_cb (GtkWidget *drawing_area, GtkAllocation *allocation,
		       view_data_t *view_data)
{
	view_data->engine->width = allocation->width;
	view_data->engine->height = allocation->height;

	html_engine_calc_size (view_data->engine);

	calc_scrollbars (view_data);

	view_update (view_data);
}

static void
calc_scrollbars (view_data_t *view_data)
{
	if ((html_engine_get_doc_height (view_data->engine) > view_data->engine->height)) {
		view_data->displayVScroll = TRUE;
	}
	else {
		view_data->displayVScroll = FALSE;
	}
	
	if (view_data->displayVScroll) {
		GTK_ADJUSTMENT (view_data->vsbadjust)->lower = 0;
		GTK_ADJUSTMENT (view_data->vsbadjust)->upper = html_engine_get_doc_height (view_data->engine);
		GTK_ADJUSTMENT (view_data->vsbadjust)->page_size = view_data->engine->height;
	}
	
	if (!view_data->displayVScroll) {
		gtk_widget_hide (view_data->vscrollbar);
		GTK_ADJUSTMENT (view_data->vsbadjust)->upper = GTK_ADJUSTMENT (view_data->vsbadjust)->page_size = 0;
	}
	else {
		set_geometry (view_data->vscrollbar,
				    GTK_WIDGET (view_data->layout)->allocation.width - view_data->vscrollbar->allocation.width, 
				    0,
				    view_data->vscrollbar->allocation.width,
				       GTK_WIDGET (view_data->layout)->allocation.height);
		gtk_widget_show (view_data->vscrollbar);
	}
}

static void
vertical_scroll (GtkAdjustment *adjustment, gpointer data)
{
	view_data_t *view_data = (view_data_t *) data;

	view_data->engine->y_offset = (gint)adjustment->value;
	view_update (view_data);
}

/*
 * This function is called when the "ClearImage" verb is executed on
 * the component.
 */
static void
view_clear_image_cb (GnomeView *view, const char *verb_name, view_data_t *view_data)
{
	embeddable_data_t *embeddable_data;

	embeddable_data = view_data->embeddable_data;

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
	view_data->embeddable_data = embeddable_data;

	/*
	 * Perform the equivalent to gtk_html_init ().
	 */
	view_data->layout = GTK_LAYOUT (gtk_layout_new (NULL, NULL));
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (view_data->layout), GTK_CAN_FOCUS);
	gtk_layout_set_hadjustment (view_data->layout, NULL);
	gtk_layout_set_vadjustment (view_data->layout, NULL);

	/* Create vertical scrollbar */
	view_data->vsbadjust = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	view_data->vscrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (view_data->vsbadjust));
	gtk_layout_put (view_data->layout, view_data->vscrollbar, 0, 0);
	gtk_signal_connect (GTK_OBJECT (view_data->vsbadjust), "value_changed",
			    GTK_SIGNAL_FUNC (vertical_scroll), view_data);

	view_data->engine = html_engine_new ();

	gtk_signal_connect (GTK_OBJECT (view_data->layout), "realize",
			    GTK_SIGNAL_FUNC (view_realize_cb), view_data);

	gtk_signal_connect (GTK_OBJECT (view_data->layout), "expose_event",
			    GTK_SIGNAL_FUNC (view_expose_cb), view_data);
	 
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox),
			    GTK_WIDGET (view_data->layout),
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
	gnome_view_register_verb (view, "ClearImage",
				  GNOME_VIEW_VERB_FUNC (view_clear_image_cb),
				  view_data);

	/*
	 * The "size_query" signal is raised when the container asks
	 * the component what size it wants to be.
	 */
	gtk_signal_connect (GTK_OBJECT (view), "size_query",
			    GTK_SIGNAL_FUNC (view_size_query_cb), view_data);

	/*
	 * When the container assigns us a size, we will get a
	 * "size_allocate" signal raised on our top-level widget.
	 */
	gtk_signal_connect (GTK_OBJECT (view_data->layout), "size_allocate",
			    GTK_SIGNAL_FUNC (view_size_allocate_cb), view_data);

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

/*
 * When a container asks our EmbeddableFactory for a new paint
 * component, this function is called.  It creates the new
 * GnomeEmbeddable object and returns it.
 */
static GnomeObject *
embeddable_factory (GnomeEmbeddableFactory *this,
		    void *data)
{
	GnomeEmbeddable *embeddable;
	embeddable_data_t *embeddable_data;

	g_message ("embeddable_factory");

	/*
	 * Create a data structure in which we can store
	 * Embeddable-object-specific data about this document.
	 */
	embeddable_data = g_new0 (embeddable_data_t, 1);
	if (embeddable_data == NULL)
		return NULL;

	/*
	 * The embeddable must maintain an internal representation of
	 * the data for its document.
	 */
/*	embeddable_data->engine = html_engine_new ();*/

	/*
	 * Create the GnomeEmbeddable object.
	 */
	embeddable = gnome_embeddable_new (GNOME_VIEW_FACTORY (view_factory),
					   embeddable_data);

	if (embeddable == NULL) {
		g_free (embeddable_data);
		return NULL;
	}

	running_objects++;      
	embeddable_data->embeddable = embeddable;

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
				   "ClearImage",
				   _("_Clear Image"),
				   _("Clear the image to black"));

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

	return GNOME_OBJECT (embeddable);
}

static GnomeEmbeddableFactory *
init_simple_paint_factory (void)
{
	/*
	 * This will create a factory server for our simple paint
	 * component.  When a container wants to create a paint
	 * component, it will ask the factory to create one, and the
	 * factory will invoke our embeddable_factory() function.
	 */
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
