#define _EBROWSER_TEST_C_

/*
 * Copyright 2000 Helix Code, Inc.
 *
 * Author: Lauris Kaplinski  <lauris@helixcode.com>
 *
 * License: GPL
 */

#include <gdk/gdkkeysyms.h>
#include "ebrowser-widget.h"
#include "test.h"

GtkWidget * browser;

static gint
delete_window (GtkWidget * widget, GdkEventAny * event, gpointer data)
{
	gtk_main_quit ();

	return FALSE;
}

static void
submit (GtkWidget * widget, gpointer data)
{
	GtkEntry * entry;

	entry = GTK_ENTRY (data);

	gtk_object_set (GTK_OBJECT (browser), "url", gtk_entry_get_text (entry), NULL);
}

static void
set_proxy (GtkWidget * widget, gpointer data)
{
	GtkEntry * entry;

	entry = GTK_ENTRY (data);

	gtk_object_set (GTK_OBJECT (browser), "http_proxy", gtk_entry_get_text (entry), NULL);
}

static void
uri_set (EBrowser * ebr, const gchar * uri, gpointer data)
{
	GtkEntry * entry;

	entry = GTK_ENTRY (data);

	if (uri) {
		gtk_entry_set_text (entry, uri);
	} else {
		gtk_entry_set_text (entry, "");
	}
}

int
main (int argc, char ** argv)
{
	GtkWidget * window, * vb, * hb, * urientry, * entry, * w;

	gtk_init (&argc, &argv);

	gdk_rgb_init ();
	gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
	gtk_widget_set_default_visual (gdk_rgb_get_visual ());

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Test EBrowser");
	gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			    GTK_SIGNAL_FUNC (delete_window), NULL);

	vb = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vb);
	gtk_widget_show (vb);

	/* Create URL entry */

	hb = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);
	gtk_widget_show (hb);

	w = gtk_label_new ("Url:");
	gtk_box_pack_start (GTK_BOX (hb), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	
	urientry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hb), urientry, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (urientry), "activate",
			    GTK_SIGNAL_FUNC (submit), urientry);
	gtk_widget_show (urientry);

	w = gtk_button_new_with_label ("Submit");
	gtk_box_pack_start (GTK_BOX (hb), w, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (submit), urientry);
	gtk_widget_show (w);

	/* Create proxy entry */

	hb = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);
	gtk_widget_show (hb);

	w = gtk_label_new ("Proxy:");
	gtk_box_pack_start (GTK_BOX (hb), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	
	entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hb), entry, TRUE, TRUE, 0);
	gtk_widget_show (entry);

	w = gtk_button_new_with_label ("Set proxy");
	gtk_box_pack_start (GTK_BOX (hb), w, FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (w), "clicked",
			    GTK_SIGNAL_FUNC (set_proxy), entry);
	gtk_widget_show (w);

	w = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vb), w, TRUE, TRUE, 0);
	gtk_widget_show (w);
	
	browser = ebrowser_new ();
	gtk_container_add (GTK_CONTAINER (w), browser);
	gtk_object_set (GTK_OBJECT (browser),
			"follow_links", TRUE,
			"allow_submit", TRUE, NULL);
	gtk_signal_connect (GTK_OBJECT (browser), "url_set",
			    GTK_SIGNAL_FUNC (uri_set), urientry);
	gtk_widget_show (browser);

	gtk_widget_show (window);

	gtk_main ();

	return 0;
}
