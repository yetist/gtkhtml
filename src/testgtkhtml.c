/*
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
#include <gnome.h>
#include "config.h"
#include "debug.h"
#include "gtkhtml.h"
#include "htmlengine.h"

#ifdef HAVE_LIBWWW

#undef PACKAGE
#undef VERSION

#include <WWWLib.h>
#include <WWWStream.h>
#include <WWWInit.h>
#undef PACKAGE
#undef VERSION

#include "config.h"

#include <glibwww/glibwww.h>
#endif

typedef struct {
  FILE *fil;
  GtkHTMLStreamHandle handle;
} FileInProgress;

static void exit_cb (GtkWidget *widget, gpointer data);
static void test_cb (GtkWidget *widget, gpointer data);
static void slow_cb (GtkWidget *widget, gpointer data);
static void stop_cb (GtkWidget *widget, gpointer data);
static void dump_cb (GtkWidget *widget, gpointer data);
static void redraw_cb (GtkWidget *widget, gpointer data);
static void resize_cb (GtkWidget *widget, gpointer data);
static void title_changed_cb (HTMLEngine *engine, gpointer data);
static gboolean load_timer_event (FileInProgress *fip);
static void url_requested (GtkHTML *html, const char *url, GtkHTMLStreamHandle handle, gpointer data);
static void entry_goto_url(GtkWidget *widget, gpointer data);
static void goto_url(const char *url);

#ifdef HAVE_LIBWWW
static int netin_stream_put_character (HTStream * me, char c);
static int netin_stream_put_string (HTStream * me, const char * s);;
static int netin_stream_write (HTStream * me, const char * s, int l);
static int netin_stream_flush (HTStream * me);
static int netin_stream_free (HTStream * me);
static int netin_stream_abort (HTStream * me, HTList * e);
static HTResponse *netin_request_response(HTRequest *request);
static HTStream *netin_stream_new (GtkHTMLStreamHandle handle, HTRequest *request);
#endif

GtkWidget *area, *box, *button;
GtkHTML *html;
GtkWidget *animator, *entry;

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
	{ GNOME_APP_UI_ITEM, "Test 8", "Run test 8 (local test)",
	  test_cb, GINT_TO_POINTER (8), NULL, 0, 0, 0, 0},
	GNOMEUIINFO_END
};

static GnomeUIInfo debug_menu[] = {
	{ GNOME_APP_UI_ITEM, "Dump Object tree", "Dump Object tree to stdout",
	  dump_cb, NULL, NULL, 0, 0, 0, 0},
	GNOMEUIINFO_TOGGLEITEM("Slow loading", "Load documents slowly", slow_cb, NULL),
	{ GNOME_APP_UI_ITEM, "Force resize", "Force a resize event",
	  resize_cb, NULL, NULL, 0, 0, 0 },
	{ GNOME_APP_UI_ITEM, "Force repaint", "Force a repaint event",
	  redraw_cb, NULL, NULL, 0, 0, 0 },
	GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
	GNOMEUIINFO_MENU_FILE_TREE (file_menu),
	GNOMEUIINFO_SUBTREE (("_Tests"), test_menu),
	GNOMEUIINFO_SUBTREE (("_Debug"), debug_menu),
	GNOMEUIINFO_END
};

static void
create_toolbars (GtkWidget *app)
{
	GtkWidget *dock;
	GtkWidget *hbox, *hbox2;
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *toolbar;
	char *imgloc;

	dock = gnome_dock_item_new ("testgtkhtml-toolbar1",
				    (GNOME_DOCK_ITEM_BEH_EXCLUSIVE));
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (dock), hbox);
	gtk_container_border_width (GTK_CONTAINER (dock), 2);
	
	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				   GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
			       GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar),
				       GTK_RELIEF_NONE);
	gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);

	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Move back",
				 "Back",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_BACK),
				 NULL, NULL);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Move forward",
				 "Forward",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_FORWARD),
				 NULL, NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Stop loading",
				 "Stop",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_STOP),
				 stop_cb,
				 NULL);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Reload page",
				 "Reload",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_REFRESH),
				 NULL, NULL);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL,
				 "Home page",
				 "Home",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_HOME),
				 NULL, NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
				 NULL, 
				 "Search bar",
				 "Search",
				 gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_SEARCH),
				 NULL, NULL);
	animator = gnome_animator_new_with_size (32, 32);

	if (g_file_exists("32.png"))
	  imgloc = "32.png";
	else if (g_file_exists(SRCDIR "/32.png"))
	  imgloc = SRCDIR "/32.png";
	else
	  imgloc = "32.png";
	gnome_animator_append_frames_from_file_at_size (GNOME_ANIMATOR (animator),
							imgloc,
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
	gtk_signal_connect(GTK_OBJECT(entry), "activate", GTK_SIGNAL_FUNC(entry_goto_url), NULL);
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
resize_cb (GtkWidget *widget, gpointer data)
{
	g_print ("forcing resize\n");
	html_engine_calc_size (html->engine);
}

static void
redraw_cb (GtkWidget *widget, gpointer data)
{
	g_print ("forcing redraw\n");
	gtk_widget_draw (GTK_WIDGET (html), NULL);
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
entry_goto_url(GtkWidget *widget, gpointer data)
{
  goto_url(gtk_entry_get_text(GTK_ENTRY(widget)));
}


#ifdef HAVE_LIBWWW

static void
www_incoming(const char *url, const char *buffer, int size, int status, GtkHTMLStreamHandle handle)
{
  gtk_html_write(html, handle, buffer, size);
  gtk_html_end(html, handle, GTK_HTML_STREAM_OK);
}
#endif

static void
stop_cb (GtkWidget *widget, gpointer data)
{
}

static void
load_done(GtkHTML *html)
{
  gnome_animator_stop (GNOME_ANIMATOR (animator));
  gnome_animator_goto_frame (GNOME_ANIMATOR (animator), 1);
  gtk_entry_set_text(GTK_ENTRY(entry), html->engine->baseURL);
}

static void
fip_destroy(gpointer data)
{
  FileInProgress *fip = data;

  gtk_html_end(html, fip->handle, GTK_HTML_STREAM_OK);
  fclose(fip->fil);
  g_free(fip);
}

static gboolean
load_timer_event (FileInProgress *fip)
{
	gchar buffer[32768];

	g_print ("LOAD\n");

        if (!feof (fip->fil)) {
		fgets (buffer, 32768, fip->fil);
		gtk_html_write (html, fip->handle, buffer, strlen(buffer));
		return TRUE;
	} else {
		load_timer = -1;
		return FALSE;
	}
}

#ifdef HAVE_LIBWWW
/* Lame hack */
struct _HTStream {
  const HTStreamClass *	isa;
  GtkHTMLStreamHandle handle;
  HTRequest *request;
  HTResponse *response;
  long read_len;
};

static int
netin_stream_put_character (HTStream * me, char c)
{
  return netin_stream_write(me, &c, 1);
}

static int
netin_stream_put_string (HTStream * me, const char * s)
{
  return netin_stream_write(me, s, strlen(s));
}

static gint
do_request_delete(HTRequest *req)
{
  HTRequest_delete(req);

  return FALSE;
}

static int
netin_stream_write (HTStream * me, const char * s, int l)
{
  int reqlen;

  g_print("netin_stream_write(%p, %d bytes)\n", me->handle, l);

  if(!me->response)
    me->response = netin_request_response(me->request);

  me->read_len += l;

  gtk_html_write(html, me->handle, s, l);

  reqlen = HTResponse_length(me->response);
  g_print("reqlen on %p is %d (so far %d)\n", me->handle, reqlen, me->read_len);
  if((reqlen > 0) && (me->read_len >= reqlen))
    gtk_idle_add(do_request_delete, me->request);

  return HT_OK;
}

static int
netin_stream_flush (HTStream * me)
{
  return HT_OK;
}

static int
netin_stream_free (HTStream * me)
{
  g_print("netin_stream_free on %p\n", me->handle);
  gtk_html_end(html, me->handle, GTK_HTML_STREAM_OK);
  g_free(me);

  return HT_OK;
}

static int
netin_stream_abort (HTStream * me, HTList * e)
{
  return HT_ERROR;
}

static const HTStreamClass netin_stream_class =
{		
    "netin_stream",
    netin_stream_flush,
    netin_stream_free,
    netin_stream_abort,
    netin_stream_put_character,
    netin_stream_put_string,
    netin_stream_write
}; 

static HTResponse *
netin_request_response(HTRequest *request)
{
  HTResponse *retval;

  while(!(retval = HTRequest_response(request)))
    g_main_iteration(TRUE);

  return retval;
}

static HTStream *
netin_stream_new (GtkHTMLStreamHandle handle, HTRequest *request)
{
  HTStream *retval;

  retval = g_new0(HTStream, 1);

  retval->isa = &netin_stream_class;
  retval->handle = handle;
  retval->request = request;

  return retval;
}
#endif

static void
url_requested (GtkHTML *html, const char *url, GtkHTMLStreamHandle handle, gpointer data)
{
	FILE *fil;
	gchar buffer[32768];

#ifdef HAVE_LIBWWW
	HTRequest *newreq;
	BOOL status;

	if(!strstr(url, "://")) {
	  if(g_file_exists(url))
	    {
	      char *cwd;

	      cwd = g_get_current_dir();
	      g_snprintf(buffer, sizeof(buffer), "file://%s%s%s",
			 (*url != '/')?cwd:"",
			 (*url != '/')?"/":"",
			 url);
	      g_free(cwd);
	    }
	  else
	    g_snprintf(buffer, sizeof(buffer), "http://%s", url);

	  url = buffer;

	}

	newreq = HTRequest_new();
	g_assert(newreq);

	HTRequest_setOutputFormat(newreq, WWW_SOURCE);
	{
	  HTStream *newstream = netin_stream_new(handle, newreq);
	  status = HTLoadToStream(url, newstream, newreq);
	  g_message("Loading URL %s to stream %p (status %d)", url, handle, status);
	}

	return;
#endif

	fil = fopen (url, "r");

	if(!fil)
	  {
	    gtk_html_end (html, handle, GTK_HTML_STREAM_ERROR);
	    return;
	  }

	if (slow_loading)
	  {
	    FileInProgress *fip;
	    
	    fip = g_new(FileInProgress, 1);
	    fip->fil = fil;
	    fip->handle = handle;
	    load_timer = gtk_timeout_add_full (10,
					       (GtkFunction) load_timer_event,
					       NULL, fip,
					       fip_destroy);
	    return;
	  }

	while (!feof (fil))
	  {
	    int nread;

	    nread = fread(buffer, 1, sizeof(buffer), fil);
	    gtk_html_write (html, handle, buffer, nread);
	  }

	gtk_html_end (html, handle, GTK_HTML_STREAM_OK);
	fclose (fil);
}

static void
goto_url(const char *url)
{
        gchar buffer[32768];

	if(!strstr(url, "://")) {
	  if(g_file_exists(url))
	    {
	      char *cwd;

	      cwd = g_get_current_dir();
	      g_snprintf(buffer, sizeof(buffer), "file://%s%s%s",
			 (*url != '/')?cwd:"",
			 (*url != '/')?"/":"",
			 url);
	      g_free(cwd);
	    }
	  else
	    g_snprintf(buffer, sizeof(buffer), "http://%s", url);

	  url = buffer;

	}

	gnome_animator_start (GNOME_ANIMATOR (animator));
	gtk_html_begin (html, url);
	gtk_html_parse (html);
}

static void
test_cb (GtkWidget *widget, gpointer data)
{
	gchar *filename;

	/* Don't start loading if another file is already being loaded. */
	if (load_timer != -1)
		return;

	filename = g_strdup_printf ("tests/test%d.html", GPOINTER_TO_INT (data));
	goto_url(filename);
	g_free(filename);
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
	GtkWidget *hbox, *vscrollbar;
	
	gnome_init (PACKAGE, VERSION,
		    argc, argv);
#ifdef HAVE_LIBWWW
	glibwww_init(PACKAGE, VERSION);
	glibwww_register_gnome_dialogs();
#endif

	gdk_rgb_init ();
	
	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());
	
	app = gnome_app_new ("testgtkhtml", "GtkHTML: testbed application");

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    GTK_SIGNAL_FUNC (exit_cb), NULL);

	create_toolbars (app);
	bar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (app), bar);
	gnome_app_create_menus (GNOME_APP (app), main_menu);
	gnome_app_install_menu_hints (GNOME_APP (app), main_menu);

	box = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), box);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start_defaults (GTK_BOX (box), GTK_WIDGET (hbox));

	html = GTK_HTML (gtk_html_new (NULL, NULL));
	gtk_signal_connect (GTK_OBJECT (html), "title_changed",
			    GTK_SIGNAL_FUNC (title_changed_cb), (gpointer)app);
	gtk_signal_connect (GTK_OBJECT (html), "url_requested",
			    GTK_SIGNAL_FUNC (url_requested), (gpointer)app);
	gtk_signal_connect (GTK_OBJECT (html), "load_done",
			    GTK_SIGNAL_FUNC (load_done), (gpointer)app);

	gtk_box_pack_start_defaults (GTK_BOX (hbox), GTK_WIDGET (html));
	vscrollbar = gtk_vscrollbar_new (GTK_LAYOUT (html)->vadjustment);
	gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, TRUE, 0);
	
	gtk_widget_realize (GTK_WIDGET (html));

	gtk_window_set_default_size (GTK_WINDOW (app), 500, 400);

	gtk_widget_show_all (app);

	if (argc > 1)
	  goto_url(argv[1]);

	gtk_main ();

	return 0;
}



