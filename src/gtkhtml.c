/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*  This library is free software; you can redistribute it and/or
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
#include <gtk/gtk.h>
#include "htmlpainter.h"
#include "gtkhtml-private.h"

guint           gtk_html_get_type (void);
static void     gtk_html_class_init (GtkHTMLClass *klass);
static void     gtk_html_init (GtkHTML* html);

static void     gtk_html_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint     gtk_html_expose        (GtkWidget *widget, GdkEventExpose *event);
static void     gtk_html_realize       (GtkWidget *widget);
static void     gtk_html_unrealize     (GtkWidget *widget);
static void     gtk_html_draw          (GtkWidget *widget, GdkRectangle *area);
void            gtk_html_calc_scrollbars (GtkHTML *htm);
static void     gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data);

static GtkLayoutClass *parent_class = NULL;

guint html_signals [LAST_SIGNAL] = { 0 };

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

	html_class = (GtkHTMLClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	object_class = (GtkObjectClass *)klass;

	parent_class = gtk_type_class (GTK_TYPE_LAYOUT);

	html_signals [TITLE_CHANGED] = 
	  gtk_signal_new ("title_changed",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, title_changed),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
	html_signals [URL_REQUESTED] =
	  gtk_signal_new ("url_requested",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, url_requested),
			  gtk_marshal_NONE__POINTER_POINTER,
			  GTK_TYPE_NONE, 2,
			  GTK_TYPE_STRING,
			  GTK_TYPE_POINTER);
	html_signals [LOAD_DONE] = 
	  gtk_signal_new ("load_done",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, load_done),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
	html_signals [LINK_FOLLOWED] =
	  gtk_signal_new ("link_followed",
			  GTK_RUN_FIRST,
			  object_class->type,
			  GTK_SIGNAL_OFFSET (GtkHTMLClass, link_followed),
			  gtk_marshal_NONE__STRING,
			  GTK_TYPE_NONE, 1,
			  GTK_TYPE_STRING);
	html_signals [SET_BASE] =
		gtk_signal_new ("set_base",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, set_base),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
	html_signals [SET_BASE_TARGET] =
		gtk_signal_new ("set_base_target",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, set_base_target),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);
				
	
	gtk_object_class_add_signals (object_class, html_signals, LAST_SIGNAL);

	widget_class->realize = gtk_html_realize;
	widget_class->unrealize = gtk_html_unrealize;
	widget_class->draw = gtk_html_draw;
	widget_class->expose_event  = gtk_html_expose;
	widget_class->size_allocate = gtk_html_size_allocate;
}

static void
gtk_html_init (GtkHTML* html)
{
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_APP_PAINTABLE);
	
}

GtkWidget *
gtk_html_new (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
	GtkHTML *html;

	html = gtk_type_new (gtk_html_get_type ());

	if (vadjustment == NULL)
		vadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

	gtk_signal_connect (GTK_OBJECT (vadjustment), "value_changed",
			    GTK_SIGNAL_FUNC (gtk_html_vertical_scroll), (gpointer)html);
			    
	gtk_layout_set_hadjustment (GTK_LAYOUT (html), hadjustment);
	gtk_layout_set_vadjustment (GTK_LAYOUT (html), vadjustment);
	
	html->engine = html_engine_new ();
	html->engine->widget = html;

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
	return html_engine_begin (html->engine, url);
}

void
gtk_html_write (GtkHTML *html, GtkHTMLStreamHandle handle, const gchar *buffer, size_t size)
{
	gtk_html_stream_write(handle, buffer, size);
}

void
gtk_html_end (GtkHTML *html, GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status)
{
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
	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	gdk_window_set_events (html->layout.bin_window,
			       (gdk_window_get_events (html->layout.bin_window)
				| GDK_EXPOSURE_MASK));

	html_settings_set_bgcolor (html->engine->settings, 
				   &widget->style->bg[GTK_STATE_NORMAL]);

	html_painter_realize (html->engine->painter, html->layout.bin_window);
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
gtk_html_set_geometry (GtkWidget *widget, gint x, gint y, gint width, gint height)
{
	GtkAllocation allocation;

	allocation.x = x;
	allocation.y = y;
	allocation.width = width;
	allocation.height = height;

	gtk_widget_size_allocate (widget, &allocation);
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
	html_engine_calc_absolute_pos (html->engine);

	gtk_html_calc_scrollbars (html);

}


void
gtk_html_calc_scrollbars (GtkHTML *html)
{
	int size;

	size = html_engine_get_doc_height (html->engine);

	GTK_LAYOUT (html)->vadjustment->lower = 0;
	GTK_LAYOUT (html)->vadjustment->upper = size;
	GTK_LAYOUT (html)->vadjustment->page_size = html->engine->height;
	GTK_LAYOUT (html)->vadjustment->step_increment = 14;
	GTK_LAYOUT (html)->vadjustment->page_increment = html->engine->height;
	
	gtk_adjustment_changed (GTK_LAYOUT (html)->vadjustment);

}

static void
gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);

	html->engine->y_offset = (gint)adjustment->value;
/*	gtk_widget_draw (GTK_WIDGET (html), NULL); */
}

