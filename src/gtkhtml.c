/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
    Copyright 1999, Helix Code, Inc.

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
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "htmlpainter.h"
#include "htmlengine-cursor.h"
#include "gtkhtml-private.h"


guint           gtk_html_get_type (void);
static void     gtk_html_class_init (GtkHTMLClass *klass);
static void     gtk_html_init (GtkHTML* html);

static void     gtk_html_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint     gtk_html_expose        (GtkWidget *widget, GdkEventExpose *event);
static void     gtk_html_realize       (GtkWidget *widget);
static void     gtk_html_unrealize     (GtkWidget *widget);
static void     gtk_html_draw          (GtkWidget *widget, GdkRectangle *area);
void            gtk_html_calc_scrollbars (GtkHTML *html);
static void     gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data);
static void     gtk_html_horizontal_scroll (GtkAdjustment *adjustment, gpointer data);
static gint	gtk_html_motion_notify_event (GtkWidget *widget, GdkEventMotion *event);
static gint     gtk_html_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gint     gtk_html_button_release_event (GtkWidget *widget, GdkEventButton *event);
static void     gtk_html_set_adjustments    (GtkLayout     *layout,
					     GtkAdjustment *hadj,
					     GtkAdjustment *vadj);
static void     gtk_html_destroy            (GtkObject     *object);


static GtkLayoutClass *parent_class = NULL;

enum {
	TITLE_CHANGED,
	URL_REQUESTED,
	LOAD_DONE,
	LINK_CLICKED,
	SET_BASE,
	SET_BASE_TARGET,
	ON_URL,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };


/* HTMLEngine callbacks.  */

static void
html_engine_title_changed_cb (HTMLEngine *engine, gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[TITLE_CHANGED]);
}

static void
html_engine_set_base_cb (HTMLEngine *engine, const gchar *base, gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[SET_BASE], base);
}

static void
html_engine_set_base_target_cb (HTMLEngine *engine, const gchar *base_target, gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[SET_BASE_TARGET], base_target);
}

static void
html_engine_load_done_cb (HTMLEngine *engine, gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[LOAD_DONE]);
}

static void
html_engine_url_requested_cb (GtkHTML *html,
			      const char *url,
			      GtkHTMLStreamHandle handle,
			      gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[URL_REQUESTED], url, handle);
}


static void
destroy (GtkObject *object)
{
	GtkHTML *html;

	html = GTK_HTML (object);

	g_free (html->pointer_url);

	gdk_cursor_destroy (html->hand_cursor);
	gdk_cursor_destroy (html->arrow_cursor);
}

static gint
key_press_event (GtkWidget *widget,
		 GdkEventKey *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	gboolean retval;

	puts (__FUNCTION__);

	html = GTK_HTML (widget);
	engine = html->engine;

	if (! engine->show_cursor) {
		/* FIXME handle differently in this case */
		return FALSE;
	}

	switch (event->keyval) {
	case GDK_Right:
		html_engine_cursor_move (engine, HTML_ENGINE_CURSOR_RIGHT, 1);
		retval = TRUE;
		break;
	case GDK_Left:
		html_engine_cursor_move (engine, HTML_ENGINE_CURSOR_LEFT, 1);
		retval = TRUE;
		break;
	default:
		retval = FALSE;
	}

	return retval;
}


guint
gtk_html_get_type (void)
{
	static guint html_type = 0;

	if (!html_type) {
		static const GtkTypeInfo html_info = {
			"GtkHTML",
			sizeof (GtkHTML),
			sizeof (GtkHTMLClass),
			(GtkClassInitFunc) gtk_html_class_init,
			(GtkObjectInitFunc) gtk_html_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		html_type = gtk_type_unique (GTK_TYPE_LAYOUT, &html_info);
	}

	return html_type;
}

static void
gtk_html_class_init (GtkHTMLClass *klass)
{
	GtkHTMLClass *html_class;
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;
	GtkLayoutClass *layout_class;
	
	html_class = (GtkHTMLClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	object_class = (GtkObjectClass *)klass;
	layout_class = (GtkLayoutClass *)klass;
	
	object_class->destroy = destroy;

	parent_class = gtk_type_class (GTK_TYPE_LAYOUT);

	signals [TITLE_CHANGED] = 
	  gtk_signal_new ("title_changed",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, title_changed),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
	signals [URL_REQUESTED] =
	  gtk_signal_new ("url_requested",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, url_requested),
			  gtk_marshal_NONE__POINTER_POINTER,
			  GTK_TYPE_NONE, 2,
			  GTK_TYPE_STRING,
			  GTK_TYPE_POINTER);
	signals [LOAD_DONE] = 
	  gtk_signal_new ("load_done",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, load_done),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
	signals [LINK_CLICKED] =
	  gtk_signal_new ("link_clicked",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, link_clicked),
			  gtk_marshal_NONE__STRING,
			  GTK_TYPE_NONE, 1,
			  GTK_TYPE_STRING);
	signals [SET_BASE] =
		gtk_signal_new ("set_base",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, set_base),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
	signals [SET_BASE_TARGET] =
		gtk_signal_new ("set_base_target",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, set_base_target),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
	
	signals [ON_URL] =
		gtk_signal_new ("on_url",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, on_url),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
	
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = gtk_html_destroy;
	
	widget_class->realize = gtk_html_realize;
	widget_class->unrealize = gtk_html_unrealize;
	widget_class->draw = gtk_html_draw;
	widget_class->key_press_event = key_press_event;
	widget_class->expose_event  = gtk_html_expose;
	widget_class->size_allocate = gtk_html_size_allocate;
	widget_class->motion_notify_event = gtk_html_motion_notify_event;
	widget_class->button_press_event = gtk_html_button_press_event;
	widget_class->button_release_event = gtk_html_button_release_event;

	layout_class->set_scroll_adjustments = gtk_html_set_adjustments;
}

static void
gtk_html_init (GtkHTML* html)
{
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_APP_PAINTABLE);

	html->pointer_url = NULL;
	html->hand_cursor = gdk_cursor_new (GDK_HAND2);
	html->arrow_cursor = gdk_cursor_new (GDK_LEFT_PTR);
	html->hadj_connection = 0;
	html->vadj_connection = 0;
}

GtkWidget *
gtk_html_new (void)
{
	GtkHTML *html;

	html = gtk_type_new (gtk_html_get_type ());
	
	html->engine = html_engine_new ();
	html->engine->widget = html; /* FIXME FIXME */

	gtk_signal_connect (GTK_OBJECT (html->engine), "title_changed",
			    GTK_SIGNAL_FUNC (html_engine_title_changed_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "set_base",
			    GTK_SIGNAL_FUNC (html_engine_set_base_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "set_base_target",
			    GTK_SIGNAL_FUNC (html_engine_set_base_target_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "load_done",
			    GTK_SIGNAL_FUNC (html_engine_load_done_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "url_requested",
			    GTK_SIGNAL_FUNC (html_engine_url_requested_cb), html);

	return GTK_WIDGET (html);
}

void
gtk_html_destroy (GtkObject *object)
{
	/* Disconnect our adjustments */
	gtk_html_set_adjustments(GTK_LAYOUT(object), NULL, NULL);

}

void
gtk_html_parse (GtkHTML *html)
{
	html_engine_parse (html->engine);
}

GtkHTMLStreamHandle
gtk_html_begin (GtkHTML *html, const char *url)
{
	GtkHTMLStreamHandle *handle;

	handle = html_engine_begin (html->engine, url);

	return handle;
}

void
gtk_html_write (GtkHTML *html, GtkHTMLStreamHandle handle, const gchar *buffer, size_t size)
{
	gtk_html_stream_write(handle, buffer, size);
}
void
gtk_html_end (GtkHTML *html, GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status)
{
	g_warning (__FUNCTION__);
	gtk_html_stream_end(handle, status);
}

static void
gtk_html_realize (GtkWidget *widget)
{
	GtkHTML *html;

	g_message ("realize");
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_HTML (widget));

	html = GTK_HTML (widget);

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	gdk_window_set_events (html->layout.bin_window,
			       (gdk_window_get_events (html->layout.bin_window)
				| GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK
				| GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK));

	html_settings_set_bgcolor (html->engine->settings, 
				   &widget->style->bg[GTK_STATE_NORMAL]);

	html_painter_realize (html->engine->painter, html->layout.bin_window);

	gdk_window_set_cursor (widget->window, html->arrow_cursor);
}

static void
gtk_html_unrealize (GtkWidget *widget)
{
	GtkHTML *html = GTK_HTML (widget);
	
	html_painter_unrealize (html->engine->painter);

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static gint
gtk_html_expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (GTK_WIDGET_CLASS (parent_class)->expose_event)
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

	html_engine_draw (GTK_HTML (widget)->engine,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);

	return FALSE;
}

static void
gtk_html_draw (GtkWidget *widget, GdkRectangle *area)
{
	GtkHTML *html = GTK_HTML (widget);
	HTMLPainter *painter = html->engine->painter;

	if (GTK_WIDGET_CLASS (parent_class)->draw)
		(* GTK_WIDGET_CLASS (parent_class)->draw) (widget, area);
	
	html_painter_clear (painter);

	html_engine_draw (GTK_HTML (widget)->engine,
			  area->x, area->y,
			  area->width, area->height);
}

static void
gtk_html_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkHTML *html;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_HTML (widget));
	g_return_if_fail (allocation != NULL);
	
	html = GTK_HTML (widget);

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		( *GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	html->engine->width = allocation->width;
	html->engine->height = allocation->height;

	html_engine_calc_size (html->engine);
	
	gtk_html_calc_scrollbars (html);
}

static gint
gtk_html_motion_notify_event (GtkWidget *widget,
			      GdkEventMotion *event)
{
	GtkHTML *html;
	const gchar *url;

	g_return_val_if_fail (widget != NULL, 0);
	g_return_val_if_fail (GTK_IS_HTML (widget), 0);
	g_return_val_if_fail (event != NULL, 0);

	html = GTK_HTML (widget);

	url = html_engine_get_link_at (GTK_HTML (widget)->engine, event->x, event->y);

	if (url == NULL) {
		if (html->pointer_url != NULL) {
			g_free (html->pointer_url);
			html->pointer_url = NULL;
			gtk_signal_emit (GTK_OBJECT (html), signals[ON_URL], NULL);
		}
		gdk_window_set_cursor (widget->window, html->arrow_cursor);
	} else {
		if (html->pointer_url == NULL || strcmp (html->pointer_url, url) != 0) {
			g_free (html->pointer_url);
			html->pointer_url = g_strdup (url);
			gtk_signal_emit (GTK_OBJECT (html), signals[ON_URL], url);
		}
		gdk_window_set_cursor (widget->window, html->hand_cursor);
	}

	return TRUE;
}

static gint
gtk_html_button_press_event (GtkWidget *widget,
			     GdkEventButton *event)
{
	return FALSE;
}

static gint
gtk_html_button_release_event (GtkWidget *widget,
			       GdkEventButton *event)
{
	GtkHTML *html;

	html = GTK_HTML (widget);
	if (event->button == 1 && html->pointer_url != NULL)
		gtk_signal_emit (GTK_OBJECT (widget),
				 signals[LINK_CLICKED],
				 html->pointer_url);

	return TRUE;
}


static void
gtk_html_set_adjustments    (GtkLayout     *layout,
			     GtkAdjustment *hadj,
			     GtkAdjustment *vadj)
{
	GtkHTML *html = GTK_HTML(layout);

	if (html->hadj_connection != 0)
		gtk_signal_disconnect(GTK_OBJECT(layout->hadjustment),
				      html->hadj_connection);

	if (html->vadj_connection != 0)
		gtk_signal_disconnect(GTK_OBJECT(layout->vadjustment),
				      html->vadj_connection);

	if (vadj != NULL)
		html->vadj_connection =
			gtk_signal_connect (GTK_OBJECT (vadj), "value_changed",
					    GTK_SIGNAL_FUNC (gtk_html_vertical_scroll), (gpointer)html);
	else
		html->vadj_connection = 0;
	
	if (hadj != NULL)
		html->hadj_connection =
			gtk_signal_connect (GTK_OBJECT (hadj), "value_changed",
					    GTK_SIGNAL_FUNC (gtk_html_horizontal_scroll), (gpointer)html);
	else
		html->hadj_connection = 0;
	
	if (parent_class->set_scroll_adjustments)
		(* parent_class->set_scroll_adjustments) (layout, hadj, vadj);
}


void
gtk_html_calc_scrollbars (GtkHTML *html)
{
	gint width, height;

	height = html_engine_get_doc_height (html->engine);
	width = html_engine_get_doc_width (html->engine);

	GTK_LAYOUT (html)->vadjustment->lower = 0;
	GTK_LAYOUT (html)->vadjustment->upper = height;
	GTK_LAYOUT (html)->vadjustment->page_size = html->engine->height;
	GTK_LAYOUT (html)->vadjustment->step_increment = 14;
	GTK_LAYOUT (html)->vadjustment->page_increment = html->engine->height;

	GTK_LAYOUT (html)->hadjustment->lower = 0;
	GTK_LAYOUT (html)->hadjustment->upper = width;
	GTK_LAYOUT (html)->hadjustment->page_size = html->engine->width;
	GTK_LAYOUT (html)->hadjustment->step_increment = 14;
	GTK_LAYOUT (html)->hadjustment->page_increment = html->engine->width;

	gtk_layout_set_size (GTK_LAYOUT (html), width, height);
}

static void
gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);

	html->engine->y_offset = (gint)adjustment->value;
}

static void
gtk_html_horizontal_scroll (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);
		
	html->engine->x_offset = (gint)adjustment->value;
}

void
gtk_html_set_base_url (GtkHTML *html, const char *url)
{
	html_engine_set_base_url(html->engine, url);
}
