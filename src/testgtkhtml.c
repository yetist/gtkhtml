#include <gnome.h>
#include "config.h"
#include "debug.h"
#include "gtkhtml.h"
#include "htmlengine.h"

static void exit_cb (GtkWidget *widget, gpointer data);
static void test_cb (GtkWidget *widget, gpointer data);
static void slow_cb (GtkWidget *widget, gpointer data);
static void dump_cb (GtkWidget *widget, gpointer data);
static void title_changed_cb (HTMLEngine *engine, gpointer data);
static gboolean load_timer_event (FILE *fil);

GtkWidget *area, *box, *button;
GtkHTML *html;
GtkWidget *animator;

static gint load_timer  = -1;
static gboolean slow_loading = FALSE;

static GnomeUIInfo file_menu[] = {
	GNOMEUIINFO_MENU_EXIT_ITEM (exit_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo test_menu[] = {
	{ GNOME_APP_UI_ITEM, "Test 1", "Run test 1",
	  test_cb, GINT_TO_POINTER (1), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 2", "Run test 2",
	  test_cb, GINT_TO_POINTER (2), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 3", "Run test 3",
	  test_cb, GINT_TO_POINTER (3), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 4", "Run test 4",
	  test_cb, GINT_TO_POINTER (4), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 5", "Run test 5",
	  test_cb, GINT_TO_POINTER (5), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 6", "Run test 6",
	  test_cb, GINT_TO_POINTER (6), NULL, 0, 0, 0, 0},
	{ GNOME_APP_UI_ITEM, "Test 7", "Run test 7 (FreshMeat)",
	  test_cb, GINT_TO_POINTER (7), NULL, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo debug_menu[] = {
	{ GNOME_APP_UI_ITEM, "Dump Object tree", "Dump Object tree to stdout",
	  dump_cb, NULL, NULL, 0, 0, 0, 0},
	GNOMEUIINFO_TOGGLEITEM("Slow loading", "Load documents slowly", slow_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_SUBTREE (("File"), file_menu),
	GNOMEUIINFO_SUBTREE (("Tests"), test_menu),
	GNOMEUIINFO_SUBTREE (("Debug"), debug_menu),
	GNOMEUIINFO_END
};

static void
create_toolbars (GtkWidget *app) {
	GtkWidget *dock;
	GtkWidget *entry;
	GtkWidget *hbox, *hbox2;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *toolbar;

	dock = gnome_dock_item_new ("testgtkhtml-toolbar1",
				    (GNOME_DOCK_ITEM_BEH_EXCLUSIVE));
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (dock), hbox);
	gtk_container_border_width (GTK_CONTAINER (dock), 2);
	
	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				   GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
			       GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar),
				       GTK_RELIEF_NONE);
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);

	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Move back",
				 "Back",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_BACK),
				 NULL, NULL);
	animator = gnome_animator_new_with_size (32, 32);
	gnome_animator_append_frames_from_file_at_size (GNOME_ANIMATOR (animator),
							"32.png",
							0, 0,
							25,
							32, 
							32, 32);
	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), animator);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
	gnome_animator_set_loop_type (GNOME_ANIMATOR (animator), 
				      GNOME_ANIMATOR_LOOP_RESTART);
	gtk_widget_show_all (dock);
	gnome_dock_add_item (GNOME_DOCK (GNOME_APP (app)->dock),
			     GNOME_DOCK_ITEM (dock), GNOME_DOCK_TOP, 1, 0, 0, FALSE);

	/* Create the location bar */
	dock = gnome_dock_item_new ("testgtkhtml-toolbar2",
				    (GNOME_DOCK_ITEM_BEH_EXCLUSIVE));
	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (dock), hbox);
	gtk_container_border_width (GTK_CONTAINER (dock), 2);
	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox2), gtk_label_new ("Goto"), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox2), gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_REDO), FALSE, FALSE, 0);
	button = gtk_button_new ();
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (button), hbox2);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox),
			    gtk_label_new ("Location:"), FALSE, FALSE, 0);
	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox),
			    entry, TRUE, TRUE, 0);
	gnome_dock_add_item (GNOME_DOCK (GNOME_APP (app)->dock),
			     GNOME_DOCK_ITEM (dock), GNOME_DOCK_TOP, 2, 0, 0, FALSE);

}

static void
dump_cb (GtkWidget *widget, gpointer data)
{
	g_print ("Object Tree\n");
	g_print ("-----------\n");

	debug_dump_tree (html->engine->clue, 0);
}

static void
slow_cb (GtkWidget *widget, gpointer data)
{
	slow_loading = !slow_loading;
}


static void
title_changed_cb (HTMLEngine *engine, gpointer data)
{
	g_print ("title_changed_cb\n");

	gtk_window_set_title (GTK_WINDOW (data), 
			      g_strdup_printf ("GtkHTML: %s\n", 
					       engine->title->str));
}

static void
test_cb (GtkWidget *widget, gpointer data)
{
	FILE *fil;
	gchar buffer[32768];
	gchar *filename;

	/* Don't start loading if another file is already being loaded. */
	if (load_timer != -1)
		return;

	filename = g_strdup_printf ("tests/test%d.html", GPOINTER_TO_INT (data));
	gnome_animator_start (GNOME_ANIMATOR (animator));
	gtk_html_begin (html, filename);
	gtk_html_parse (html);

	fil = fopen (filename, "r");

	if (slow_loading) {
		load_timer = gtk_timeout_add (
			300,
			(GtkFunction) load_timer_event,
			fil);
		return;
	}

	while (!feof (fil)) {
		fgets (buffer, 32768, fil);
		gtk_html_write (html, buffer);
	}

	gnome_animator_stop (GNOME_ANIMATOR (animator));
	gnome_animator_goto_frame (GNOME_ANIMATOR (animator), 1);

	gtk_html_end (html);
	fclose (fil);
}

static gboolean
load_timer_event (FILE *fil)
{
	gchar buffer[32768];

	g_print ("LOAD\n");

        if (!feof (fil)) {
		fgets (buffer, 32768, fil);
		gtk_html_write (html, buffer);
		return TRUE;
	} else {
		gnome_animator_stop (GNOME_ANIMATOR (animator));
		gnome_animator_goto_frame (GNOME_ANIMATOR (animator), 1);
		gtk_html_end (html);
		fclose (fil);
		load_timer = -1;
		return FALSE;
	}
}

static void
exit_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

gint
main (gint argc, gchar *argv[])
{
	GtkWidget *app, *bar;

	gnome_init (PACKAGE, VERSION,
		    argc, argv);

	gdk_rgb_init ();
	
	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());
	
	app = gnome_app_new ("testgtkhtml", "GtkHTML: testbed application");
	create_toolbars (app);
	bar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (app), bar);
	gnome_app_create_menus (GNOME_APP (app), main_menu);
	gnome_app_install_menu_hints (GNOME_APP (app), main_menu);

	box = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), box);

	html = GTK_HTML (gtk_html_new ());
	gtk_signal_connect (GTK_OBJECT (html->engine), "title_changed",
			    GTK_SIGNAL_FUNC (title_changed_cb), (gpointer)app);

	gtk_box_pack_start_defaults (GTK_BOX (box), GTK_WIDGET (html));
	
	gtk_widget_realize (GTK_WIDGET (html));

	gtk_window_set_default_size (GTK_WINDOW (app), 500, 400);

	gtk_widget_show_all (app);


	gtk_main ();

	return 0;
}



