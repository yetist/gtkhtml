/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
   
   Copyright (C) 2000 Helix Code, Inc.

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
#include <gtk/gtk.h>
#include <string.h>
#include "htmliframe.h"

HTMLIFrameClass html_iframe_class;
static HTMLEmbeddedClass *parent_class = NULL;
static gboolean calc_size (HTMLObject *o, HTMLPainter *painter);

static void
iframe_url_requested (GtkHTML *html, const char *url, GtkHTMLStream *handle, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);
	char *new_url = NULL;

	/* FIXME this is not exactly the single safest method of expanding a relative url */
	if (!strstr (url, ":"))
		new_url = g_strconcat (iframe->url, url, NULL);
	
	gtk_signal_emit_by_name (GTK_OBJECT (parent->engine), "url_requested", new_url ? new_url : url,
				 handle);
	
	if (new_url)
		g_free (new_url);
}

static void
iframe_on_url (GtkHTML *html, const gchar *url, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);

	gtk_signal_emit_by_name (GTK_OBJECT (parent), "on_url", url);
}

static void
iframe_link_clicked (GtkHTML *html, const gchar *url, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);

	gtk_signal_emit_by_name (GTK_OBJECT (parent), "link_clicked", url);
}

static void
iframe_size_changed (GtkAdjustment *adj, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);
	
	html_engine_schedule_update (parent->engine);
}
	
HTMLObject *
html_iframe_new (GtkWidget *parent, 
		 char *src, 
		 gint width, 
		 gint height,
		 gboolean border) 
{
	HTMLIFrame *iframe;
	
	iframe = g_new (HTMLIFrame, 1);
	
	html_iframe_init (iframe, 
			  &html_iframe_class, 
			  parent,
			  src,
			  width,
			  height,
			  border);
	
	return HTML_OBJECT (iframe);
}

static gboolean
html_iframe_grab_cursor(GtkWidget *iframe, GdkEvent *event)
{
	/* Keep the focus! Fight the power */
	return TRUE;
}

static gint
calc_min_width (HTMLObject *o,
		HTMLPainter *painter)
{
	HTMLIFrame *iframe = HTML_IFRAME (o);
	GtkHTML *html;
	gint min_width;

	html = GTK_HTML (iframe->html);
	min_width = html_object_calc_min_width (html->engine->clue,
						html->engine->painter);
	html->engine->width = min_width;
	html_engine_calc_size (html->engine);
	min_width = html_engine_get_doc_width (html->engine);
    
	return min_width;
}

static void
set_max_width (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	/* FIXME FIXME amazingly broken to set this */
	o->max_width = max_width;
}

static void
reset (HTMLObject *o)
{
	HTMLIFrame *iframe;

	(* HTML_OBJECT_CLASS (parent_class)->reset) (o);
	iframe = HTML_IFRAME (o);
	html_object_reset (GTK_HTML (iframe->html)->engine->clue);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (o);
	html_object_draw (GTK_HTML (iframe->html)->engine->clue, p, x, y, width, height, tx, ty);
}

static void
set_painter (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	HTMLIFrame *iframe;

	printf ("iframe set painter\n");
	iframe = HTML_IFRAME (o);
	html_engine_set_painter (GTK_HTML (iframe->html)->engine, painter, max_width);
	printf ("iframe set painter end\n");
}

static void
forall (HTMLObject *self,
	HTMLObjectForallFunc func,
	gpointer data)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (self);
	(* func) (self, data);
	html_object_forall (GTK_HTML (iframe->html)->engine->clue, func, data);
}

static gboolean
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
{
	HTMLIFrame *iframe;
	gint width, height;
	gint old_width, old_ascent, old_descent;
	GtkHTML *html;
	
	old_width = o->width;
	old_ascent = o->ascent;
	old_descent = o->descent;

	iframe = HTML_IFRAME (o);
	
	html = GTK_HTML (iframe->html);

	if ((iframe->width < 0) && (iframe->height < 0)) {
		html->engine->width = o->max_width;
		html_engine_calc_size (html->engine);

		height = html_engine_get_doc_height (html->engine);
		width = html_engine_get_doc_width (html->engine);

		gtk_widget_set_usize (iframe->scroll, width, height);
		gtk_widget_queue_resize (iframe->scroll);
		
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iframe->scroll),
						GTK_POLICY_NEVER,
						GTK_POLICY_NEVER);

		o->width = width;
		o->ascent = height;
		o->descent = 0;
	} else {
		return (* HTML_OBJECT_CLASS (parent_class)->calc_size) 
			(o, painter);
	}

	if (o->descent != old_descent
	    || o->ascent != old_ascent
	    || o->width != old_width)
		return TRUE;

	return FALSE;
}

void 
html_iframe_init (HTMLIFrame *iframe,
		  HTMLIFrameClass *klass,
		  GtkWidget *parent,
		  char *src,
		  gint width,
		  gint height,
		  gboolean border)
{
	HTMLEmbedded *em = HTML_EMBEDDED (iframe);
	GtkWidget *html;
	GtkHTML   *parent_html;
	GtkHTMLStream *handle;
	GtkWidget *scrolled_window;

	g_assert (GTK_IS_HTML (parent));

	html_embedded_init (em, HTML_EMBEDDED_CLASS (klass),
			    parent, NULL, NULL);
	
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	html = gtk_html_new ();
	iframe->html = html;
	gtk_html_set_iframe_parent (GTK_HTML (html), parent);
	gtk_container_add (GTK_CONTAINER (scrolled_window), html);
	gtk_widget_show (html);

	iframe->url = src;
	iframe->width = width;
	iframe->height = height;

	parent_html = GTK_HTML (parent);

	handle = gtk_html_begin (GTK_HTML (html));

	gtk_signal_connect (GTK_OBJECT (html), "url_requested",
			    GTK_SIGNAL_FUNC (iframe_url_requested),
			    (gpointer)iframe);
	gtk_signal_connect (GTK_OBJECT (html), "on_url",
			    GTK_SIGNAL_FUNC (iframe_on_url), 
			    (gpointer)iframe);
	gtk_signal_connect (GTK_OBJECT (html), "link_clicked",
			    GTK_SIGNAL_FUNC (iframe_link_clicked),
			    (gpointer)iframe);	
	gtk_signal_connect (GTK_OBJECT (html), "size_changed",
			    GTK_SIGNAL_FUNC (iframe_size_changed),
			    (gpointer)iframe);	
	/*
	  gtk_signal_connect (GTK_OBJECT (html), "button_press_event",
	  GTK_SIGNAL_FUNC (iframe_button_press_event), iframe);
	*/
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_HTML (html)->engine), 
				 "url_requested", src, handle);

	if ((width > 0) && (height > 0))
		gtk_widget_set_usize (scrolled_window, width, height);

	/* 
	   FIXME: huge hack to get the widget to allocate it's size 
	   before it is actually drawn.  Place it in the gtklayout but offscreen
	   it will be moved to the proper place in the draw routine, but it's size
	   allocation needs to happen now.
	*/
	gtk_widget_show (scrolled_window);
	
	iframe->scroll = scrolled_window;
	html_embedded_set_widget (em, scrolled_window);
	html_embedded_size_recalc(em);

	gtk_signal_connect(GTK_OBJECT(scrolled_window), "button_press_event",
			   GTK_SIGNAL_FUNC(html_iframe_grab_cursor), NULL);

	/* inherit the current colors from our parent */
	html_colorset_set_unchanged (GTK_HTML (html)->engine->defaultSettings->color_set,
				     parent_html->engine->settings->color_set);
	html_colorset_set_unchanged (GTK_HTML (html)->engine->settings->color_set,
				     parent_html->engine->settings->color_set);
	/*
	gtk_signal_connect (GTK_OBJECT (html), "title_changed",
			    GTK_SIGNAL_FUNC (title_changed_cb), (gpointer)app);
	gtk_signal_connect (GTK_OBJECT (html), "set_base",
			    GTK_SIGNAL_FUNC (on_set_base), (gpointer)app);
	gtk_signal_connect (GTK_OBJECT (html), "button_press_event",
			    GTK_SIGNAL_FUNC (on_button_press_event), popup_menu);
	gtk_signal_connect (GTK_OBJECT (html), "redirect",
			    GTK_SIGNAL_FUNC (on_redirect), NULL);
	gtk_signal_connect (GTK_OBJECT (html), "submit",
			    GTK_SIGNAL_FUNC (on_submit), NULL);
	gtk_signal_connect (GTK_OBJECT (html), "object_requested",
			    GTK_SIGNAL_FUNC (object_requested_cmd), NULL);
	*/
}


void
html_iframe_type_init (void)
{
	html_iframe_class_init (&html_iframe_class, HTML_TYPE_IFRAME, sizeof (HTMLIFrame));
}

void
html_iframe_class_init (HTMLIFrameClass *klass,
			HTMLType type,
		        guint size) 
{
	HTMLEmbeddedClass *embedded_class;
	HTMLObjectClass  *object_class;

	g_return_if_fail (klass != NULL);
	
	embedded_class = HTML_EMBEDDED_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_embedded_class_init (embedded_class, type, size);
	parent_class = &html_embedded_class;

	object_class->calc_size = calc_size;
	object_class->calc_min_width = calc_min_width;
	object_class->set_painter = set_painter;
	object_class->reset = reset;
	object_class->draw = draw;
	object_class->set_max_width = set_max_width;
	object_class->forall = forall;
}
