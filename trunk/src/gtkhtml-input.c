/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright 1999, 2000 Helix Code, Inc.
    Authors:             Radek Doulik (rodo@helixcode.com)
    
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

#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "gtkhtml-input.h"

struct _GtkHTMLInputLine {
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *frame;
	GtkWidget *ebox;
	GtkHTML   *html;
	GtkSignalFunc key_press;
	GtkSignalFunc finish;
	gpointer data;

	gboolean   activated;
	gboolean   allocated;
	gboolean   landed;
	gint       width;
	guint      changed;
	guint      activate;
};

#define HEIGHT(w) GTK_WIDGET (w)->allocation.height

static void
size_allocate (GtkWidget *w, GtkAllocation *allocation, GtkHTMLInputLine *il)
{
	if (!il->landed) {
		il->landed = TRUE;
		/* printf ("size_allocate %d %d\n", allocation->width, allocation->height); */
		gtk_widget_set_usize (w, GTK_WIDGET (il->html)->allocation.width, -1);
		gtk_layout_move (GTK_LAYOUT (il->html), il->ebox, 0, HEIGHT (il->html) - HEIGHT (il->ebox));
	}
}

static void
get_xy (GtkHTMLInputLine *il, gint *x, gint *y)
{
	*x = GTK_LAYOUT (il->html)->xoffset;
	*y = GTK_WIDGET (il->html)->allocation.height - GTK_WIDGET (il->ebox)->allocation.height
		+ GTK_LAYOUT (il->html)->yoffset;
}

static void
html_size_allocate (GtkWidget *w, GtkAllocation *allocation, GtkHTMLInputLine *il)
{
	/* printf ("html_size_allocate\n"); */
	if (il->activated && !il->allocated) {
		gint x, y;
		il->landed = TRUE;
		il->allocated = TRUE;
		get_xy (il, &x, &y);
		/* printf ("x: %d %d\n", il->ebox->allocation.x, x);
		   printf ("y: %d %d\n", il->ebox->allocation.y, y); */
		if (il->ebox->allocation.y != y || il->ebox->allocation.x != x)
			gtk_layout_move (GTK_LAYOUT (w), il->ebox, x, y);
		/* printf ("w: %d %d\n", il->ebox->allocation.width, allocation->width); */
		if (il->ebox->allocation.width != allocation->width)
			gtk_widget_set_usize (il->ebox, allocation->width, -1);
	} else
		il->allocated = FALSE;
}

static gboolean
set_html_focus (GtkHTMLInputLine *il)
{
	GtkWidget *window;

	if ((window = gtk_widget_get_ancestor (GTK_WIDGET (il->html), gtk_window_get_type ())))
		gtk_window_set_focus (GTK_WINDOW (window), GTK_WIDGET (il->html));

	return FALSE;
}

static void
value_changed (GtkAdjustment *adjustment, GtkHTMLInputLine *il)
{
	gint x, y;
	get_xy (il, &x, &y);
	/* printf ("value_changed %d %d\n", x, y); */
	gtk_layout_move (GTK_LAYOUT (il->html), il->ebox, x, y);
}

static gint
key_press_event (GtkWidget *widget, GdkEventKey *event, GtkHTMLInputLine *il)
{
	gint retval = FALSE;

	/* printf ("key press\n"); */

	if (!event->state && event->keyval == GDK_Escape) {
		gtk_html_input_line_deactivate (il);
		set_html_focus (il);
		retval = TRUE;
	} else if (il->key_press)
		retval = (*(gint (*)(GtkWidget *, GdkEventKey *, gpointer)) il->key_press) (widget, event, il->data);

	return retval;
}

static void
focus_out (GtkWidget *w, GdkEventFocus *event, GtkHTMLInputLine *il)
{
	if (il->activated) {
		gtk_html_input_line_deactivate (il);
		gtk_idle_add ((GtkFunction) set_html_focus, il);
	}
}

GtkHTMLInputLine *
gtk_html_input_line_new (GtkHTML *html)
{
	GtkHTMLInputLine * il = g_new (GtkHTMLInputLine, 1);
	GtkWidget *hbox;

	il->entry     = gtk_entry_new ();
	il->label     = gtk_label_new (NULL);
	il->frame     = gtk_frame_new (NULL);
	il->html      = html;
	il->activated = FALSE;
	il->allocated = FALSE;
	il->landed    = TRUE;
	il->changed   = 0;
	il->activate  = 0;
	il->ebox = gtk_event_box_new ();

	gtk_signal_connect (GTK_OBJECT (il->ebox), "size_allocate", size_allocate, il);
	gtk_signal_connect (GTK_OBJECT (il->html),  "size_allocate", html_size_allocate, il);
	gtk_signal_connect (GTK_OBJECT (il->entry), "key_press_event", GTK_SIGNAL_FUNC (key_press_event), il);
	gtk_signal_connect (GTK_OBJECT (il->entry), "focus_out_event", focus_out, il);

	gtk_widget_add_events (il->entry, GDK_FOCUS_CHANGE);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), il->label, FALSE, FALSE, 3);
	gtk_box_pack_start_defaults (GTK_BOX (hbox), il->entry);
	gtk_frame_set_shadow_type (GTK_FRAME (il->frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (il->frame), hbox);
	gtk_container_add (GTK_CONTAINER (il->ebox), il->frame);
	gtk_widget_ref (il->ebox);
	gtk_widget_show_all (il->ebox);

	return il;
}

void
gtk_html_input_line_destroy (GtkHTMLInputLine *il)
{
	if (il->activated)
		gtk_html_input_line_deactivate (il);
	gtk_widget_unref (il->ebox);
	g_free (il);
}

void
gtk_html_input_line_activate (GtkHTMLInputLine *il, const gchar *label,
			      GtkSignalFunc changed, GtkSignalFunc activate,
			      GtkSignalFunc key_press, GtkSignalFunc finish,
			      gpointer data)
{
	if (il->activated)
		gtk_html_input_line_deactivate (il);

	if (changed)
		il->changed  = gtk_signal_connect (GTK_OBJECT (il->entry), "changed",  changed, data);
	if (activate)
		il->activate = gtk_signal_connect (GTK_OBJECT (il->entry), "activate", activate, data);

	gtk_signal_connect (GTK_OBJECT (GTK_LAYOUT (il->html)->hadjustment),  "value_changed", value_changed, il);
	gtk_signal_connect (GTK_OBJECT (GTK_LAYOUT (il->html)->vadjustment),  "value_changed", value_changed, il);
	gtk_label_set_text (GTK_LABEL (il->label), label);
	gtk_layout_put (GTK_LAYOUT (il->html), il->ebox, 0, 0);
	gtk_widget_grab_focus (il->entry);

	il->activated = TRUE;
	il->landed    = FALSE;
	il->key_press = key_press;
	il->finish    = finish;
	il->data      = data;
}

void
gtk_html_input_line_deactivate (GtkHTMLInputLine *il)
{
	g_return_if_fail (il->activated);

	il->activated = FALSE;
	(*(void (*) (gpointer)) il->finish) (il->data);

	if (il->changed) {
		gtk_signal_disconnect (GTK_OBJECT (il->entry), il->changed);
		il->changed = 0;
	}
	if (il->activate) {
		gtk_signal_disconnect (GTK_OBJECT (il->entry), il->activate);
		il->activate = 0;
	}
	gtk_grab_remove (il->entry);
	gtk_container_remove (GTK_CONTAINER (il->html), il->ebox);
}
