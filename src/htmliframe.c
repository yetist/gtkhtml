
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

#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtkhtml.h"
#include "gtkhtml-private.h"
#include "htmlcolorset.h"
#include "htmlgdkpainter.h"
#include "htmliframe.h"
#include "htmlengine-search.h"
#include "htmlsearch.h"
#include "htmlselection.h"
#include "htmlsettings.h"

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
iframe_size_changed (GtkHTML *html, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);
	
	html_engine_schedule_update (parent->engine);
}

static gboolean
iframe_object_requested (GtkHTML *html, GtkHTMLEmbedded *eb, gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED(iframe)->parent);
	gboolean ret_val;

	ret_val = FALSE;
	gtk_signal_emit_by_name (GTK_OBJECT (parent), "object_requested", eb, &ret_val);
	return ret_val;
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
draw_background (HTMLObject *self,
		 HTMLPainter *p,
		 gint x, gint y,
		 gint width, gint height,
		 gint tx, gint ty)
{
}

/* FIXME rodo - draw + set_painter is not much clean now, needs refactoring */

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
	HTMLEmbedded *element = HTML_EMBEDDED(o);
	HTMLIFrame *iframe;
	ArtIRect paint;
	gint new_x, new_y;

	html_object_calc_intersection (o, &paint, x, y, width, height);
	if (art_irect_empty (&paint))
		return;

	iframe = HTML_IFRAME (o);
	if (GTK_OBJECT_TYPE (GTK_HTML (iframe->html)->engine->painter) != HTML_TYPE_GDK_PAINTER)
		html_object_draw (GTK_HTML (iframe->html)->engine->clue, GTK_HTML (iframe->html)->engine->painter,
				  x, y, width, height, tx, ty);

	if (element->widget) {
		new_x = GTK_LAYOUT (element->parent)->hadjustment->value + o->x + tx;
		new_y = GTK_LAYOUT (element->parent)->vadjustment->value + o->y + ty - o->ascent;

		if(new_x != element->abs_x || new_y != element->abs_y) {
			printf ("moveto: %d,%d\n", new_x, new_y);
			gtk_layout_move(GTK_LAYOUT(element->parent), element->widget,
					new_x, new_y);
		}
		element->abs_x = new_x;
		element->abs_y = new_y;
	}
}

static void
set_painter (HTMLObject *o, HTMLPainter *painter, gint max_width)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (o);
	if (GTK_OBJECT_TYPE (GTK_HTML (iframe->html)->engine->painter) == HTML_TYPE_GDK_PAINTER)
		iframe->gdk_painter = GTK_HTML (iframe->html)->engine->painter;
	html_engine_set_painter (GTK_HTML (iframe->html)->engine,
				 GTK_OBJECT_TYPE (painter) == HTML_TYPE_GDK_PAINTER ? iframe->gdk_painter : painter,
				 max_width);
}

static void
forall (HTMLObject *self,
	HTMLEngine *e,
	HTMLObjectForallFunc func,
	gpointer data)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (self);
	(* func) (self, html_object_get_engine (self, e), data);
	html_object_forall (GTK_HTML (iframe->html)->engine->clue, html_object_get_engine (self, e), func, data);
}

static gint
check_page_split (HTMLObject *self, gint y)
{
	return html_object_check_page_split (GTK_HTML (HTML_IFRAME (self)->html)->engine->clue, y);
}

static gboolean
calc_size (HTMLObject *o,
	   HTMLPainter *painter)
{
	HTMLIFrame *iframe;
	HTMLEngine *e;
	gint width, height;
	gint old_width, old_ascent, old_descent;
	
	old_width = o->width;
	old_ascent = o->ascent;
	old_descent = o->descent;

	iframe = HTML_IFRAME (o);
	e      = GTK_HTML (iframe->html)->engine;

	if ((iframe->width < 0) && (iframe->height < 0)) {
		e->width = o->max_width;
		html_engine_calc_size (e);

		height = html_engine_get_doc_height (e);
		width = html_engine_get_doc_width (e);

		gtk_widget_set_usize (iframe->scroll, width, height);
		gtk_widget_queue_resize (iframe->scroll);
		
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iframe->scroll),
						GTK_POLICY_NEVER,
						GTK_POLICY_NEVER);

		o->width = width;
		o->ascent = height;
		o->descent = 0;
	} else
		return (* HTML_OBJECT_CLASS (parent_class)->calc_size) 
			(o, painter);

	if (o->descent != old_descent
	    || o->ascent != old_ascent
	    || o->width != old_width)
		return TRUE;

	return FALSE;
}

static gboolean
search (HTMLObject *self, HTMLSearch *info)
{
	HTMLEngine *e = GTK_HTML (HTML_IFRAME (self)->html)->engine;

	/* printf ("search\n"); */

	/* search_next? */
	if (info->stack && HTML_OBJECT (info->stack->data) == e->clue) {
		/* printf ("next\n"); */
		info->engine = GTK_HTML (GTK_HTML (HTML_IFRAME (self)->html)->iframe_parent)->engine;
		html_search_pop (info);
		html_engine_unselect_all (e);
		return html_search_next_parent (info);
	}

	info->engine = e;
	html_search_push (info, e->clue);
	if (html_object_search (e->clue, info))
		return TRUE;
	html_search_pop (info);

	info->engine = GTK_HTML (GTK_HTML (HTML_IFRAME (self)->html)->iframe_parent)->engine;
	/* printf ("FALSE\n"); */

	return FALSE;
}

static HTMLObject *
head (HTMLObject *self)
{
	return GTK_HTML (HTML_IFRAME (self)->html)->engine->clue;
}

static HTMLObject *
tail (HTMLObject *self)
{
	return GTK_HTML (HTML_IFRAME (self)->html)->engine->clue;
}

static HTMLEngine *
get_engine (HTMLObject *self, HTMLEngine *e)
{
	return GTK_HTML (HTML_IFRAME (self)->html)->engine;
}

static HTMLObject*
check_point (HTMLObject *self,
	     HTMLPainter *painter,
	     gint x, gint y,
	     guint *offset_return,
	     gboolean for_cursor)
{
	HTMLEngine *e = GTK_HTML (HTML_IFRAME (self)->html)->engine;

	if (x < self->x || x >= self->x + self->width
	    || y >= self->y + self->descent || y < self->y - self->ascent)
		return NULL;

	x -= self->x + e->leftBorder;
	y -= self->y - self->ascent + e->topBorder;

	if (for_cursor && (x < 0 || y < e->clue->y - e->clue->ascent))
		return html_object_check_point (e->clue, e->painter, 0, e->clue->y - e->clue->ascent,
						offset_return, for_cursor);

	if (for_cursor && (x > e->clue->width - 1 || y > e->clue->y + e->clue->descent - 1))
		return html_object_check_point (e->clue, e->painter, e->clue->width - 1, e->clue->y + e->clue->descent - 1,
						offset_return, for_cursor);

	return html_object_check_point (e->clue, e->painter, x, y, offset_return, for_cursor);
}

static gboolean
is_container (HTMLObject *self)
{
	return TRUE;
}

static void
append_selection_string (HTMLObject *self,
			 GString *buffer)
{
	html_object_append_selection_string (GTK_HTML (HTML_IFRAME (self)->html)->engine->clue, buffer);
}

/* static gboolean
select_range (HTMLObject *self,
	      HTMLEngine *engine,
	      guint start,
	      gint length,
	      gboolean queue_draw)
{
	return html_object_select_range (GTK_HTML (HTML_IFRAME (self)->html)->engine->clue,
					 GTK_HTML (HTML_IFRAME (self)->html)->engine,
					 start, length, queue_draw);
} */

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
	GtkWidget *new_widget;
	GtkHTML   *new_html;
	GtkHTML   *parent_html;
	GtkHTMLStream *handle;
	GtkWidget *scrolled_window;

	g_assert (GTK_IS_HTML (parent));
	parent_html = GTK_HTML (parent);

	html_embedded_init (em, HTML_EMBEDDED_CLASS (klass),
			    parent, NULL, NULL);
	
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	new_widget = gtk_html_new ();
	new_html = GTK_HTML (new_widget);

	gtk_html_set_default_content_type (new_html,
					   parent_html->priv->content_type);
	iframe->html = new_widget;
	gtk_html_set_iframe_parent (new_html, parent);
	gtk_container_add (GTK_CONTAINER (scrolled_window), new_widget);
	gtk_widget_show (new_widget);

	iframe->url = src;
	iframe->width = width;
	iframe->height = height;

	handle = gtk_html_begin (new_html);
	new_html->engine->clue->parent = HTML_OBJECT (iframe);

	gtk_signal_connect (GTK_OBJECT (new_html), "url_requested",
			    GTK_SIGNAL_FUNC (iframe_url_requested),
			    (gpointer)iframe);
	gtk_signal_connect (GTK_OBJECT (new_html), "on_url",
			    GTK_SIGNAL_FUNC (iframe_on_url), 
			    (gpointer)iframe);
	gtk_signal_connect (GTK_OBJECT (new_html), "link_clicked",
			    GTK_SIGNAL_FUNC (iframe_link_clicked),
			    (gpointer)iframe);	
	gtk_signal_connect (GTK_OBJECT (new_html), "size_changed",
			    GTK_SIGNAL_FUNC (iframe_size_changed),
			    (gpointer)iframe);	
	gtk_signal_connect (GTK_OBJECT (new_html), "object_requested",
			    GTK_SIGNAL_FUNC (iframe_object_requested),
			    (gpointer)iframe);	
	/*
	  gtk_signal_connect (GTK_OBJECT (html), "button_press_event",
	  GTK_SIGNAL_FUNC (iframe_button_press_event), iframe);
	*/
	gtk_signal_emit_by_name (GTK_OBJECT (new_html->engine), 
				 "url_requested", src, handle);

	if ((width > 0) && (height > 0))
		gtk_widget_set_usize (scrolled_window, width, height);

	gtk_widget_show (scrolled_window);	
	iframe->scroll = scrolled_window;

	html_embedded_set_widget (em, scrolled_window);
	html_embedded_size_recalc (em);

	gtk_signal_connect(GTK_OBJECT (scrolled_window), "button_press_event",
			   GTK_SIGNAL_FUNC (html_iframe_grab_cursor), NULL);

	/* inherit the current colors from our parent */
	html_colorset_set_unchanged (new_html->engine->defaultSettings->color_set,
				     parent_html->engine->settings->color_set);
	html_colorset_set_unchanged (new_html->engine->settings->color_set,
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

	object_class->calc_size               = calc_size;
	object_class->calc_min_width          = calc_min_width;
	object_class->set_painter             = set_painter;
	object_class->reset                   = reset;
	object_class->draw                    = draw;
	object_class->set_max_width           = set_max_width;
	object_class->forall                  = forall;
	object_class->check_page_split        = check_page_split;
	object_class->search                  = search;
	object_class->head                    = head;
	object_class->tail                    = tail;
	object_class->get_engine              = get_engine;
	object_class->check_point             = check_point;
	object_class->is_container            = is_container;
	object_class->draw_background         = draw_background;
	object_class->append_selection_string = append_selection_string;
}
