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

#include "htmlengine-edit-clueflowstyle.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-insert.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-paste.h"
#include "htmlengine-edit.h"
#include "htmlengine-print.h"

#include "gtkhtml-embedded.h"
#include "gtkhtml-keybinding.h"
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
	OBJECT_REQUESTED,
	CURRENT_PARAGRAPH_STYLE_CHANGED,
	CURRENT_PARAGRAPH_INDENTATION_CHANGED,
	CURRENT_PARAGRAPH_ALIGNMENT_CHANGED,
	INSERTION_FONT_STYLE_CHANGED,
	LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };

/* Values for selection information.  FIXME: what about COMPOUND_STRING and
   TEXT?  */

enum _TargetInfo {
	TARGET_INFO_STRING
};
typedef enum _TargetInfo TargetInfo;


static GtkHTMLParagraphStyle
clueflow_style_to_paragraph_style (HTMLClueFlowStyle style)
{
	switch (style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
		return GTK_HTML_PARAGRAPH_STYLE_NORMAL;
	case HTML_CLUEFLOW_STYLE_H1:
		return GTK_HTML_PARAGRAPH_STYLE_H1;
	case HTML_CLUEFLOW_STYLE_H2:
		return GTK_HTML_PARAGRAPH_STYLE_H2;
	case HTML_CLUEFLOW_STYLE_H3:
		return GTK_HTML_PARAGRAPH_STYLE_H3;
	case HTML_CLUEFLOW_STYLE_H4:
		return GTK_HTML_PARAGRAPH_STYLE_H4;
	case HTML_CLUEFLOW_STYLE_H5:
		return GTK_HTML_PARAGRAPH_STYLE_H5;
	case HTML_CLUEFLOW_STYLE_H6:
		return GTK_HTML_PARAGRAPH_STYLE_H6;
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return GTK_HTML_PARAGRAPH_STYLE_ADDRESS;
	case HTML_CLUEFLOW_STYLE_PRE:
		return GTK_HTML_PARAGRAPH_STYLE_PRE;
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
		return GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED;
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
		return GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN;
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT;
	default:		/* This should not really happen, though.  */
		return GTK_HTML_PARAGRAPH_STYLE_NORMAL;
	}
}

static HTMLClueFlowStyle
paragraph_style_to_clueflow_style (GtkHTMLParagraphStyle style)
{
	switch (style) {
	case GTK_HTML_PARAGRAPH_STYLE_NORMAL:
		return HTML_CLUEFLOW_STYLE_NORMAL;
	case GTK_HTML_PARAGRAPH_STYLE_H1:
		return HTML_CLUEFLOW_STYLE_H1;
	case GTK_HTML_PARAGRAPH_STYLE_H2:
		return HTML_CLUEFLOW_STYLE_H2;
	case GTK_HTML_PARAGRAPH_STYLE_H3:
		return HTML_CLUEFLOW_STYLE_H3;
	case GTK_HTML_PARAGRAPH_STYLE_H4:
		return HTML_CLUEFLOW_STYLE_H4;
	case GTK_HTML_PARAGRAPH_STYLE_H5:
		return HTML_CLUEFLOW_STYLE_H5;
	case GTK_HTML_PARAGRAPH_STYLE_H6:
		return HTML_CLUEFLOW_STYLE_H6;
	case GTK_HTML_PARAGRAPH_STYLE_ADDRESS:
		return HTML_CLUEFLOW_STYLE_ADDRESS;
	case GTK_HTML_PARAGRAPH_STYLE_PRE:
		return HTML_CLUEFLOW_STYLE_PRE;
	case GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED:
		return HTML_CLUEFLOW_STYLE_ITEMDOTTED;
	case GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN:
		return HTML_CLUEFLOW_STYLE_ITEMROMAN;
	case GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT:
		return HTML_CLUEFLOW_STYLE_ITEMDIGIT;
	default:		/* This should not really happen, though.  */
		return HTML_CLUEFLOW_STYLE_NORMAL;
	}
}

static HTMLHAlignType
paragraph_alignment_to_html (GtkHTMLParagraphAlignment alignment)
{
	switch (alignment) {
	case GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT:
		return HTML_HALIGN_LEFT;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT:
		return HTML_HALIGN_RIGHT;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER:
		return HTML_HALIGN_CENTER;
	default:
		return HTML_HALIGN_LEFT;
	}
}

static GtkHTMLParagraphAlignment
html_alignment_to_paragraph (HTMLHAlignType alignment)
{
	switch (alignment) {
	case HTML_HALIGN_LEFT:
		return GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT;
	case HTML_HALIGN_CENTER:
		return GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER;
	case HTML_HALIGN_RIGHT:
		return GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT;
	default:
		return GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT;
	}
}

static void
update_styles (GtkHTML *html)
{
	GtkHTMLParagraphStyle paragraph_style;
	GtkHTMLParagraphAlignment alignment;
	HTMLClueFlowStyle clueflow_style;
	HTMLEngine *engine;
	guint indentation;

	if (! html_engine_get_editable (html->engine))
		return;

	engine = html->engine;

	clueflow_style = html_engine_get_current_clueflow_style (engine);
	paragraph_style = clueflow_style_to_paragraph_style (clueflow_style);

	if (paragraph_style != html->paragraph_style) {
		html->paragraph_style = paragraph_style;
		gtk_signal_emit (GTK_OBJECT (html), signals[CURRENT_PARAGRAPH_STYLE_CHANGED],
				 paragraph_style);
	}

	indentation = html_engine_get_current_clueflow_indentation (engine);
	if (indentation != html->paragraph_indentation) {
		html->paragraph_style = paragraph_style;
		gtk_signal_emit (GTK_OBJECT (html), signals[CURRENT_PARAGRAPH_STYLE_CHANGED], paragraph_style);
	}

	alignment = html_alignment_to_paragraph (html_engine_get_current_clueflow_alignment (engine));
	if (alignment != html->paragraph_alignment) {
		html->paragraph_alignment = alignment;
		gtk_signal_emit (GTK_OBJECT (html), signals[CURRENT_PARAGRAPH_ALIGNMENT_CHANGED], alignment);
	}

	if (html_engine_update_insertion_font_style (engine))
		gtk_signal_emit (GTK_OBJECT (html), signals[INSERTION_FONT_STYLE_CHANGED], engine->insertion_font_style);
}


/* GTK+ idle loop handler.  */

static gint
idle_handler (gpointer data)
{
	GtkHTML *html;
	HTMLEngine *engine;

	html = GTK_HTML (data);
	engine = html->engine;

	html_engine_make_cursor_visible (engine);

	gtk_adjustment_set_value (GTK_LAYOUT (html)->hadjustment, (gfloat) engine->x_offset);
	gtk_adjustment_set_value (GTK_LAYOUT (html)->vadjustment, (gfloat) engine->y_offset);

	gtk_html_private_calc_scrollbars (html);

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
	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[TITLE_CHANGED], engine->title->str);
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
			      const gchar *url,
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
			 const gchar *url,
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

static void
html_engine_object_requested_cb (HTMLEngine *engine,
		       GtkHTMLEmbedded *eb,
		       gpointer data)
{
	GtkHTML *gtk_html;

	gtk_html = GTK_HTML (data);

	gtk_signal_emit (GTK_OBJECT (gtk_html), signals[OBJECT_REQUESTED], eb);
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
	gdk_cursor_destroy (html->ibeam_cursor);

	connect_adjustments (html, NULL, NULL);

	if (html->idle_handler_id != 0)
		gtk_idle_remove (html->idle_handler_id);

	gtk_object_destroy (GTK_OBJECT (html->engine));

	if (GTK_OBJECT_CLASS (parent_class)->destroy != NULL)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* GtkWidget methods.  */

static gint
key_press_event (GtkWidget *widget,
		 GdkEventKey *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	gboolean retval;
	gboolean do_update_styles;

	html = GTK_HTML (widget);
	engine = html->engine;

	if (! html_engine_get_editable (engine)) {
		/* FIXME handle differently in this case */
		return FALSE;
	}

	retval = gtk_html_handle_key_event (GTK_HTML (widget), event, &do_update_styles);

	if (retval == TRUE) {
		queue_draw (html);
		if (do_update_styles)
			update_styles (html);
	}

	return retval;
}

static void
realize (GtkWidget *widget)
{
	GtkHTML *html;

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

	/* This sets the backing pixmap to None, so that scrolling does not
           erase the newly exposed area, thus making the thing smoother.  */
	gdk_window_set_back_pixmap (html->layout.bin_window, NULL, FALSE);
}

static void
unrealize (GtkWidget *widget)
{
	GtkHTML *html = GTK_HTML (widget);
	
	html_gdk_painter_unrealize (HTML_GDK_PAINTER (html->engine->painter));

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static gint
expose (GtkWidget *widget, GdkEventExpose *event)
{
	html_engine_draw (GTK_HTML (widget)->engine,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);

	if (GTK_WIDGET_CLASS (parent_class)->expose_event)
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

	return TRUE;
}

static void
draw (GtkWidget *widget, GdkRectangle *area)
{
	GtkHTML *html = GTK_HTML (widget);
	HTMLPainter *painter = html->engine->painter;

	html_painter_clear (painter);

	html_engine_draw (GTK_HTML (widget)->engine,
			  area->x, area->y,
			  area->width, area->height);

	if (GTK_WIDGET_CLASS (parent_class)->draw)
		(* GTK_WIDGET_CLASS (parent_class)->draw) (widget, area);
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

	if (html->engine->width != allocation->width
	    || html->engine->height != allocation->height) {
		html->engine->width = allocation->width;
		html->engine->height = allocation->height;

		html_engine_calc_size (html->engine);

		gtk_html_private_calc_scrollbars (html);
	}
}

static gint
motion_notify_event (GtkWidget *widget,
		     GdkEventMotion *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	HTMLObject *obj;
	GdkModifierType mask;
	const gchar *url;
	gint x, y;
	HTMLType type;

	g_return_val_if_fail (widget != NULL, 0);
	g_return_val_if_fail (GTK_IS_HTML (widget), 0);
	g_return_val_if_fail (event != NULL, 0);

	html = GTK_HTML (widget);
	engine = html->engine;

	if (event->is_hint) {
		gdk_window_get_pointer (GTK_LAYOUT (widget)->bin_window, &x, &y, &mask);
	} else {
		x = event->x;
		y = event->y;
	}
		

	obj = html_engine_get_object_at (engine,
					 x + engine->x_offset, y + engine->y_offset,
					 NULL, FALSE);
	if (html->button_pressed) {
		if (obj) {

			type = HTML_OBJECT_TYPE (obj);

			if (type == HTML_TYPE_BUTTON ||
			    type ==  HTML_TYPE_CHECKBOX ||
			    type ==  HTML_TYPE_EMBEDDED ||
			    type ==  HTML_TYPE_HIDDEN ||
			    type ==  HTML_TYPE_IMAGEINPUT ||
			    type ==  HTML_TYPE_RADIO ||
			    type ==  HTML_TYPE_SELECT ||
			    type ==  HTML_TYPE_TEXTAREA ||
			    type ==  HTML_TYPE_TEXTINPUT ) {

				return FALSE;
			}
		}
		html->in_selection = TRUE;
		
		html_engine_select_region (engine,
					   html->selection_x1, html->selection_y1,
					   x + engine->x_offset, y + engine->y_offset,
					   TRUE);
		
		if (html_engine_get_editable (engine))
			html_engine_jump_at (engine,
					     event->x + engine->x_offset,
					     event->y + engine->y_offset);
		
		return TRUE;
	}
	if (obj != NULL)
		url = html_object_get_url (obj);
	else
		url = NULL;

	if (url == NULL) {
		if (html->pointer_url != NULL) {
			g_free (html->pointer_url);
			html->pointer_url = NULL;
			gtk_signal_emit (GTK_OBJECT (html), signals[ON_URL], NULL);
		}

		if (obj != NULL && html_object_is_text (obj))
			gdk_window_set_cursor (widget->window, html->ibeam_cursor);
		else
			gdk_window_set_cursor (widget->window, html->arrow_cursor);
	} else {
		if (html->pointer_url == NULL || strcmp (html->pointer_url, url) != 0) {
			g_free (html->pointer_url);
			html->pointer_url = g_strdup (url);
			gtk_signal_emit (GTK_OBJECT (html), signals[ON_URL], url);
		}

		if (engine->editable)
			gdk_window_set_cursor (widget->window, html->ibeam_cursor);
		else
			gdk_window_set_cursor (widget->window, html->hand_cursor);
	}

	return TRUE;
}

static gint
button_press_event (GtkWidget *widget,
		    GdkEventButton *event)
{
	GtkHTML *html;
	HTMLEngine *engine;
	gint value;

	html = GTK_HTML (widget);
	engine = html->engine;

	gtk_widget_grab_focus (widget);

	if (event->type == GDK_BUTTON_PRESS
	    && (event->button == 4 || event->button == 5)) {
		GtkAdjustment *vadj;

		vadj = GTK_LAYOUT (widget)->vadjustment;

		/* Mouse wheel scroll up.  */
		if (event->button == 4) {
			value = vadj->value - vadj->step_increment * 3;

			if (value < vadj->lower)
				value = vadj->lower;

			gtk_adjustment_set_value (vadj, value);
			return TRUE;
		}

		/* Mouse wheel scroll down.  */
		if (event->button == 5) {
			value = vadj->value + vadj->step_increment * 3;

			if (value > (vadj->upper - vadj->page_size))
				value = vadj->upper - vadj->page_size;

			gtk_adjustment_set_value (vadj, value);
			return TRUE;
		}
	}
		
	if (html_engine_get_editable (engine)) {
		html_engine_jump_at (engine,
				     event->x + engine->x_offset,
				     event->y + engine->y_offset);
		update_styles (html);
	}

	if (html->allow_selection) {
		gtk_grab_add (widget);
		gdk_pointer_grab (widget->window, TRUE,
				  (GDK_BUTTON_RELEASE_MASK
				   | GDK_BUTTON_MOTION_MASK
				   | GDK_POINTER_MOTION_HINT_MASK),
				  NULL, NULL, 0);

		html->selection_x1 = event->x + engine->x_offset;
		html->selection_y1 = event->y + engine->y_offset;
	}

	html->button_pressed = TRUE;

	html_engine_unselect_all (engine, TRUE);

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

	if (html->in_selection) {
		gtk_selection_owner_set (widget, GDK_SELECTION_PRIMARY, event->time);
		html->in_selection = FALSE;
		update_styles (html);
	}

	return TRUE;
}

static gint
focus_in_event (GtkWidget *widget,
		GdkEventFocus *event)
{
	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

	html_engine_set_focus (GTK_HTML (widget)->engine, TRUE);

	return FALSE;
}

static gint
focus_out_event (GtkWidget *widget,
		 GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	html_engine_set_focus (GTK_HTML (widget)->engine, FALSE);

	return FALSE;
}


/* X11 selection support.  */

static void
selection_get (GtkWidget        *widget, 
	       GtkSelectionData *selection_data_ptr,
	       guint             info,
	       guint             time)
{
	GtkHTML *html;
	gchar *selection_string;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_HTML (widget));

	if (info != TARGET_INFO_STRING)
		return;

	html = GTK_HTML (widget);
	selection_string = html_engine_get_selection_string (html->engine);

	if (selection_string != NULL) {
		gtk_selection_data_set (selection_data_ptr,
					GDK_SELECTION_TYPE_STRING, 8,
					selection_string, strlen (selection_string));
		g_free (selection_string);
	}
}

static gint
selection_clear_event (GtkWidget *widget,
		       GdkEventSelection *event)
{
	GtkHTML *html;

	if (! gtk_selection_clear (widget, event))
		return FALSE;

	html = GTK_HTML (widget);

	html_engine_unselect_all (html->engine, TRUE);
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
			  gtk_marshal_NONE__STRING,
			  GTK_TYPE_NONE, 1,
			  GTK_TYPE_STRING);
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

	signals [OBJECT_REQUESTED] =
		gtk_signal_new ("object_requested",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, object_requested),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_OBJECT);
	
	signals [CURRENT_PARAGRAPH_STYLE_CHANGED] =
		gtk_signal_new ("current_paragraph_style_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, current_paragraph_style_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT);

	signals [CURRENT_PARAGRAPH_INDENTATION_CHANGED] =
		gtk_signal_new ("current_paragraph_indentation_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, current_paragraph_indentation_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT);

	signals [CURRENT_PARAGRAPH_ALIGNMENT_CHANGED] =
		gtk_signal_new ("current_paragraph_alignment_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, current_paragraph_alignment_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT);

	signals [INSERTION_FONT_STYLE_CHANGED] =
		gtk_signal_new ("insertion_font_style_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, insertion_font_style_changed),
				gtk_marshal_NONE__INT,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_INT);
	
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
	widget_class->focus_in_event = focus_in_event;
	widget_class->focus_out_event = focus_out_event;
	widget_class->selection_get = selection_get;
	widget_class->selection_clear_event = selection_clear_event;

	layout_class->set_scroll_adjustments = set_adjustments;
}

static void
init (GtkHTML* html)
{
	static const GtkTargetEntry targets[] = {
		{ "STRING", 0, TARGET_INFO_STRING },
	};
	static const gint n_targets = sizeof(targets) / sizeof(targets[0]);

	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_APP_PAINTABLE);

	html->debug = FALSE;
	html->allow_selection = TRUE;

	html->pointer_url = NULL;
	html->hand_cursor = gdk_cursor_new (GDK_HAND2);
	html->arrow_cursor = gdk_cursor_new (GDK_LEFT_PTR);
	html->ibeam_cursor = gdk_cursor_new (GDK_XTERM);
	html->hadj_connection = 0;
	html->vadj_connection = 0;

	html->selection_x1 = 0;
	html->selection_y1 = 0;

	html->in_selection = FALSE;
	html->button_pressed = FALSE;

	html->load_in_progress = TRUE;

	html->idle_handler_id = 0;

	html->paragraph_style = GTK_HTML_PARAGRAPH_STYLE_NORMAL;
	html->paragraph_alignment = GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT;
	html->paragraph_indentation = 0;

	html->insertion_font_style = GTK_HTML_FONT_STYLE_DEFAULT;

	gtk_selection_add_targets (GTK_WIDGET (html),
				   GDK_SELECTION_PRIMARY,
				   targets, n_targets);
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
	GtkWidget *html;

	html = gtk_type_new (gtk_html_get_type ());
	gtk_html_construct (html);
	return html;
}

void
gtk_html_construct (GtkWidget *htmlw)
{
	GtkHTML *html;

	g_return_if_fail (htmlw != NULL);
	g_return_if_fail (GTK_IS_HTML (htmlw));

	html = GTK_HTML (htmlw);

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
	gtk_signal_connect (GTK_OBJECT (html->engine), "object_requested",
			    GTK_SIGNAL_FUNC (html_engine_object_requested_cb), html);
}


void
gtk_html_enable_debug (GtkHTML *html,
		       gboolean debug)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html->debug = debug;
}


void
gtk_html_allow_selection (GtkHTML *html,
			  gboolean allow)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html->allow_selection = allow;
}


GtkHTMLStreamHandle
gtk_html_begin (GtkHTML *html, const gchar *url)
{
	GtkHTMLStreamHandle *handle;

	handle = html_engine_begin (html->engine, url);
	if (handle == NULL)
		return NULL;

	html_engine_parse (html->engine);

	html->load_in_progress = TRUE;

	return handle;
}

void
gtk_html_write (GtkHTML *html,
		GtkHTMLStreamHandle handle,
		const gchar *buffer,
		size_t size)
{
	gtk_html_stream_write (handle, buffer, size);
}

void
gtk_html_end (GtkHTML *html,
	      GtkHTMLStreamHandle handle,
	      GtkHTMLStreamStatus status)
{
	gtk_html_stream_end (handle, status);

	html->load_in_progress = FALSE;
}


const gchar *
gtk_html_get_title (GtkHTML *html)
{
	g_return_val_if_fail (html != NULL, NULL);
	g_return_val_if_fail (GTK_IS_HTML (html), NULL);

	if (html->engine->title == NULL)
		return NULL;

	return html->engine->title->str;
}


gboolean
gtk_html_jump_to_anchor (GtkHTML *html,
			 const gchar *anchor)
{
	g_return_val_if_fail (html != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_HTML (html), FALSE);

	return html_engine_goto_anchor (html->engine, anchor);
}


gboolean
gtk_html_save (GtkHTML *html,
	       GtkHTMLSaveReceiverFn receiver,
	       gpointer data)
{
	g_return_val_if_fail (html != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_HTML (html), FALSE);
	g_return_val_if_fail (receiver != NULL, FALSE);

	return html_engine_save (html->engine, receiver, data);
}


void
gtk_html_private_calc_scrollbars (GtkHTML *html)
{
	gint width, height;

	height = html_engine_get_doc_height (html->engine);
	width = html_engine_get_doc_width (html->engine);

	GTK_LAYOUT (html)->vadjustment->lower = 0;
	GTK_LAYOUT (html)->vadjustment->page_size = html->engine->height;
	GTK_LAYOUT (html)->vadjustment->step_increment = 14; /* FIXME */
	GTK_LAYOUT (html)->vadjustment->page_increment = html->engine->height;

	GTK_LAYOUT (html)->hadjustment->lower = 0;
	GTK_LAYOUT (html)->hadjustment->page_size = html->engine->width;
	GTK_LAYOUT (html)->hadjustment->step_increment = 14; /* FIXME */
	GTK_LAYOUT (html)->hadjustment->page_increment = html->engine->width;

	if (width != GTK_LAYOUT (html)->width || height != GTK_LAYOUT (html)->height)
		gtk_layout_set_size (GTK_LAYOUT (html), width, height);
}


void
gtk_html_set_editable (GtkHTML *html,
		       gboolean editable)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_set_editable (html->engine, editable);
}

gboolean
gtk_html_get_editable  (const GtkHTML *html)
{
	g_return_val_if_fail (html != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_HTML (html), FALSE);

	return html_engine_get_editable (html->engine);
}

void
gtk_html_load_empty (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_load_empty (html->engine);
}


/* Printing.  */

void
gtk_html_print (GtkHTML *html,
		GnomePrintContext *print_context)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_print (html->engine, print_context);
}


/* Editing.  */

void
gtk_html_set_paragraph_style (GtkHTML *html,
			      GtkHTMLParagraphStyle style)
{
	HTMLClueFlowStyle current_style;
	HTMLClueFlowStyle clueflow_style;

	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	/* FIXME precondition: check if it's a valid style.  */

	clueflow_style = paragraph_style_to_clueflow_style (style);

	current_style = html_engine_get_current_clueflow_style (html->engine);
	if (current_style == clueflow_style)
		return;

	if (! html_engine_set_clueflow_style (html->engine, clueflow_style, 0, 0,
					      HTML_ENGINE_SET_CLUEFLOW_STYLE))
		return;

	html->paragraph_style = style;

	gtk_signal_emit (GTK_OBJECT (html), signals[CURRENT_PARAGRAPH_STYLE_CHANGED],
			 style);
}

void
gtk_html_indent (GtkHTML *html,
		 gint delta)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_set_clueflow_style (html->engine, 0, 0, delta,
					HTML_ENGINE_SET_CLUEFLOW_INDENTATION);

	update_styles (html);
}

void
gtk_html_set_font_style (GtkHTML *html,
			 GtkHTMLFontStyle and_mask,
			 GtkHTMLFontStyle or_mask)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_set_font_style (html->engine, and_mask, or_mask);
}

void
gtk_html_align_paragraph (GtkHTML *html,
			  GtkHTMLParagraphAlignment alignment)
{
	HTMLHAlignType align;

	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	align = paragraph_alignment_to_html (alignment);

	html_engine_set_clueflow_style (html->engine, 0, align, 0,
					HTML_ENGINE_SET_CLUEFLOW_ALIGNMENT);
}


/* Clipboard operations.  */

void
gtk_html_cut (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_cut (html->engine);
}

void
gtk_html_copy (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_copy (html->engine);
}

void
gtk_html_paste (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_paste (html->engine);
}


/* Undo/redo.  */

void
gtk_html_undo (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_undo (html->engine);
}

void
gtk_html_redo (GtkHTML *html)
{
	g_return_if_fail (html != NULL);
	g_return_if_fail (GTK_IS_HTML (html));

	html_engine_redo (html->engine);
}

