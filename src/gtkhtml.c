/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright 1999, 2000 Helix Code, Inc.

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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "htmlengine-edit.h"
#include "gtkhtml-private.h"


static GtkLayoutClass *parent_class = NULL;

enum {
	TITLE_CHANGED,
	URL_REQUESTED,
	LOAD_DONE,
	LINK_CLICKED,
	SET_BASE,
	SET_BASE_TARGET,
	ON_URL,
	REDIRECT,
	SUBMIT,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };


/* GTK+ idle loop handler.  */

static gint
idle_handler (gpointer data)
{
	GtkHTML *html;
	HTMLEngine *engine;

	html = GTK_HTML (data);
	engine = html->engine;

	html_engine_make_cursor_visible (engine);
	gtk_adjustment_set_value (GTK_LAYOUT (html)->hadjustment,
				  (gfloat) engine->x_offset);
	gtk_adjustment_set_value (GTK_LAYOUT (html)->vadjustment,
				  (gfloat) engine->y_offset);

	html_engine_flush_draw_queue (engine);

	html->idle_handler_id = 0;
	return FALSE;
}

static void
queue_draw (GtkHTML *html)
{
	if (html->idle_handler_id == 0)
		html->idle_handler_id = gtk_idle_add (idle_handler, html);
}


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
html_engine_url_requested_cb (HTMLEngine *engine,
			      const char *url,
			      GtkHTMLStreamHandle handle,
			      gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[URL_REQUESTED], url, handle);
}

static void
html_engine_draw_pending_cb (HTMLEngine *engine,
			     gpointer data)
{
	GtkHTML *html;

	html = GTK_HTML (data);
	queue_draw (html);
}

static void
html_engine_redirect_cb (HTMLEngine *engine,
			 const char *url,
			 int delay,
			 gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);

	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[REDIRECT], url, delay);
}

static void
html_engine_submit_cb (HTMLEngine *engine,
		       const gchar *method,
		       const gchar *url,
		       const gchar *encoding,
		       gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);

	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[SUBMIT], method, url, encoding);
}


/* GtkAdjustment handling.  */

static void
vertical_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);

	html->engine->y_offset = (gint)adjustment->value;
}

static void
horizontal_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);
		
	html->engine->x_offset = (gint)adjustment->value;
}

static void
connect_adjustments (GtkHTML *html,
		     GtkAdjustment *hadj,
		     GtkAdjustment *vadj)
{
	GtkLayout *layout;

	layout = GTK_LAYOUT (html);

	if (html->hadj_connection != 0)
		gtk_signal_disconnect (GTK_OBJECT(layout->hadjustment),
				       html->hadj_connection);

	if (html->vadj_connection != 0)
		gtk_signal_disconnect (GTK_OBJECT(layout->vadjustment),
				       html->vadj_connection);

	if (vadj != NULL)
		html->vadj_connection =
			gtk_signal_connect (GTK_OBJECT (vadj), "value_changed",
					    GTK_SIGNAL_FUNC (vertical_scroll_cb), (gpointer)html);
	else
		html->vadj_connection = 0;
	
	if (hadj != NULL)
		html->hadj_connection =
			gtk_signal_connect (GTK_OBJECT (hadj), "value_changed",
					    GTK_SIGNAL_FUNC (horizontal_scroll_cb), (gpointer)html);
	else
		html->hadj_connection = 0;
}


/* GtkObject methods.  */

static void
destroy (GtkObject *object)
{
	GtkHTML *html;

	html = GTK_HTML (object);

	g_free (html->pointer_url);

	gdk_cursor_destroy (html->hand_cursor);
	gdk_cursor_destroy (html->arrow_cursor);

	connect_adjustments (html, NULL, NULL);

	if (html->idle_handler_id != 0)
		gtk_idle_remove (html->idle_handler_id);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* GtkWidget methods.  */

static gint
key_press_event (GtkWidget *widget,
		 GdkEventKey *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	gboolean retval;

	html = GTK_HTML (widget);
	engine = html->engine;

	if (! engine->editable) {
		/* FIXME handle differently in this case */
		return FALSE;
	}

	switch (event->keyval) {
	case GDK_Right:
		html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_RIGHT, 1);
		retval = TRUE;
		break;
	case GDK_Left:
		html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_LEFT, 1);
		retval = TRUE;
		break;
	case GDK_Up:
		html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_UP, 1);
		retval = TRUE;
		break;
	case GDK_Down:
		html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_DOWN, 1);
		retval = TRUE;
		break;
	case GDK_Delete:
	case GDK_KP_Delete:
		html_engine_delete (engine, 1);
		retval = TRUE;
		break;
	case GDK_Return:
		html_engine_insert_para (engine, TRUE);
		retval = TRUE;
		break;
	case GDK_BackSpace:
		if (html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_LEFT, 1) == 1)
			html_engine_delete (engine, 1);
		retval = TRUE;
		break;
	/* The following cases are for keys that we don't want to map yet, but
           have an annoying default behavior if not handled. */
	case GDK_Tab:
		retval = TRUE;
		break;
	default:
		if (event->length == 0) {
			retval = FALSE;
		} else {
			guint n;

			n = html_engine_insert (engine, event->string, event->length);
			html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_RIGHT, n);
			retval = TRUE;
		}
	}

	if (retval == TRUE)
		queue_draw (html);

	return retval;
}

static void
realize (GtkWidget *widget)
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
				| GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
				| GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK));

	/* FIXME */
	html_settings_set_bgcolor (html->engine->settings, 
				   &widget->style->bg[GTK_STATE_NORMAL]);

	html_engine_realize (html->engine, html->layout.bin_window);

	gdk_window_set_cursor (widget->window, html->arrow_cursor);
}

static void
unrealize (GtkWidget *widget)
{
	GtkHTML *html = GTK_HTML (widget);
	
	html_painter_unrealize (html->engine->painter);

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static gint
expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (GTK_WIDGET_CLASS (parent_class)->expose_event)
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

	html_engine_draw (GTK_HTML (widget)->engine,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);

	return FALSE;
}

static void
draw (GtkWidget *widget, GdkRectangle *area)
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
size_allocate (GtkWidget *widget, GtkAllocation *allocation)
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
motion_notify_event (GtkWidget *widget,
		     GdkEventMotion *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	const gchar *url;

	g_return_val_if_fail (widget != NULL, 0);
	g_return_val_if_fail (GTK_IS_HTML (widget), 0);
	g_return_val_if_fail (event != NULL, 0);

	html = GTK_HTML (widget);
	engine = html->engine;

	if (html->button_pressed) {
		html->in_selection = TRUE;

		html->selection_x2 = event->x + engine->x_offset;
		html->selection_y2 = event->y + engine->y_offset;

		html_engine_select_region (engine,
					   html->selection_x1, html->selection_y1,
					   html->selection_x2, html->selection_y2);

		gtk_widget_queue_draw (widget);
	} else {
		url = html_engine_get_link_at (engine,
					       event->x + engine->x_offset,
					       event->y + engine->y_offset);

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
	}

	return TRUE;
}

static gint
button_press_event (GtkWidget *widget,
		    GdkEventButton *event)
{
	GtkHTML *html;
	HTMLEngine *engine;

	html = GTK_HTML (widget);
	engine = html->engine;

	if (engine->editable)
		html_engine_jump_at (engine,
				     event->x + engine->x_offset,
				     event->y + engine->y_offset);

	gtk_grab_add (widget);
	gdk_pointer_grab (widget->window, TRUE,
			  GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
			  NULL, NULL, 0);

	html->selection_x1 = event->x + engine->x_offset;
	html->selection_y1 = event->y + engine->y_offset;

	html->button_pressed = TRUE;

	html_engine_unselect_all (engine);

	return TRUE;
}

static gint
button_release_event (GtkWidget *widget,
		      GdkEventButton *event)
{
	GtkHTML *html;

	html = GTK_HTML (widget);

	gtk_grab_remove (widget);
	gdk_pointer_ungrab (0);

	if (event->button == 1 && html->pointer_url != NULL && ! html->in_selection)
		gtk_signal_emit (GTK_OBJECT (widget), signals[LINK_CLICKED], html->pointer_url);

	html->button_pressed = FALSE;
	html->in_selection = FALSE;

	return TRUE;
}

static void
set_adjustments (GtkLayout     *layout,
		 GtkAdjustment *hadj,
		 GtkAdjustment *vadj)
{
	GtkHTML *html = GTK_HTML(layout);

	connect_adjustments (html, hadj, vadj);
	
	if (parent_class->set_scroll_adjustments)
		(* parent_class->set_scroll_adjustments) (layout, hadj, vadj);
}


/* Initialization.  */

static void
class_init (GtkHTMLClass *klass)
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
	
	signals [REDIRECT] =
		gtk_signal_new ("redirect",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, redirect),
				gtk_marshal_NONE__POINTER_INT,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_STRING,
				GTK_TYPE_INT);
	
	signals [SUBMIT] =
		gtk_signal_new ("submit",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, submit),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_STRING,
				GTK_TYPE_STRING,
				GTK_TYPE_STRING);
	
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = destroy;
	
	widget_class->realize = realize;
	widget_class->unrealize = unrealize;
	widget_class->draw = draw;
	widget_class->key_press_event = key_press_event;
	widget_class->expose_event  = expose;
	widget_class->size_allocate = size_allocate;
	widget_class->motion_notify_event = motion_notify_event;
	widget_class->button_press_event = button_press_event;
	widget_class->button_release_event = button_release_event;

	layout_class->set_scroll_adjustments = set_adjustments;
}

static void
init (GtkHTML* html)
{
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_APP_PAINTABLE);

	html->pointer_url = NULL;
	html->hand_cursor = gdk_cursor_new (GDK_HAND2);
	html->arrow_cursor = gdk_cursor_new (GDK_LEFT_PTR);
	html->hadj_connection = 0;
	html->vadj_connection = 0;

	html->selection_x1 = 0;
	html->selection_y1 = 0;
	html->selection_x2 = 0;
	html->selection_y2 = 0;

	html->in_selection = FALSE;
	html->button_pressed = FALSE;

	html->idle_handler_id = 0;
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
			(GtkClassInitFunc) class_init,
			(GtkObjectInitFunc) init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		html_type = gtk_type_unique (GTK_TYPE_LAYOUT, &html_info);
	}

	return html_type;
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
	gtk_signal_connect (GTK_OBJECT (html->engine), "draw_pending",
			    GTK_SIGNAL_FUNC (html_engine_draw_pending_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "redirect",
			    GTK_SIGNAL_FUNC (html_engine_redirect_cb), html);
	gtk_signal_connect (GTK_OBJECT (html->engine), "submit",
			    GTK_SIGNAL_FUNC (html_engine_submit_cb), html);

	return GTK_WIDGET (html);
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


void
gtk_html_calc_scrollbars (GtkHTML *html)
{
	gint width, height;

	height = html_engine_get_doc_height (html->engine);
	width = html_engine_get_doc_width (html->engine);

	GTK_LAYOUT (html)->vadjustment->lower = 0;
	GTK_LAYOUT (html)->vadjustment->upper = height;
	GTK_LAYOUT (html)->vadjustment->page_size = html->engine->height;
	GTK_LAYOUT (html)->vadjustment->step_increment = 14; /* FIXME */
	GTK_LAYOUT (html)->vadjustment->page_increment = html->engine->height;

	GTK_LAYOUT (html)->hadjustment->lower = 0;
	GTK_LAYOUT (html)->hadjustment->upper = width;
	GTK_LAYOUT (html)->hadjustment->page_size = html->engine->width;
	GTK_LAYOUT (html)->hadjustment->step_increment = 14; /* FIXME */
	GTK_LAYOUT (html)->hadjustment->page_increment = html->engine->width;

	gtk_layout_set_size (GTK_LAYOUT (html), width, height);
}
