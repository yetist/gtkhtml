/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtkhtml.h"
#include "gtkhtml-private.h"
#include "gtkhtml-stream.h"
#include "htmlcolorset.h"
#include "htmlgdkpainter.h"
#include "htmlprinter.h"
#include "htmliframe.h"
#include "htmlengine.h"
#include "htmlengine-search.h"
#include "htmlengine-save.h"
#include "htmlsearch.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmltokenizer.h"
#include "htmlembedded.h"

HTMLIFrameClass html_iframe_class;
static HTMLEmbeddedClass *parent_class = NULL;

static void
iframe_set_base (GtkHTML *html,
                 const gchar *url,
                 gpointer data)
{
	gchar *new_url = gtk_html_get_url_base_relative (html, url);

	gtk_html_set_base (html, new_url);
	g_free (new_url);
}

static void
iframe_url_requested (GtkHTML *html,
                      const gchar *url,
                      GtkHTMLStream *handle,
                      gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED (iframe)->parent);

	if (!html->engine->stopped)
		g_signal_emit_by_name (parent->engine, "url_requested", url, handle);
}

static void
iframe_size_changed (GtkHTML *html,
                     gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED (iframe)->parent);

	html_engine_schedule_update (parent->engine);
}

static gboolean
iframe_object_requested (GtkHTML *html,
                         GtkHTMLEmbedded *eb,
                         gpointer data)
{
	HTMLIFrame *iframe = HTML_IFRAME (data);
	GtkHTML *parent = GTK_HTML (HTML_EMBEDDED (iframe)->parent);
	gboolean ret_val;

	ret_val = FALSE;
	g_signal_emit_by_name (parent, "object_requested", eb, &ret_val);
	return ret_val;
}

static void
iframe_set_gdk_painter (HTMLIFrame *iframe,
                        HTMLPainter *painter)
{
	if (painter)
		g_object_ref (G_OBJECT (painter));

	if (iframe->gdk_painter)
		g_object_unref (G_OBJECT (iframe->gdk_painter));

	iframe->gdk_painter = painter;
}

HTMLObject *
html_iframe_new (GtkWidget *parent,
                 gchar *src,
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
html_iframe_grab_cursor (GtkWidget *iframe,
                         GdkEvent *event)
{
	/* Keep the focus! Fight the power */
	return TRUE;
}

static gint
calc_min_width (HTMLObject *o,
                HTMLPainter *painter)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (o);
	if (iframe->width < 0)
		return html_engine_calc_min_width (GTK_HTML (HTML_IFRAME (o)->html)->engine);
	else
		return iframe->width;
}

static void
set_max_width (HTMLObject *o,
               HTMLPainter *painter,
               gint max_width)
{
	HTMLEngine *e = GTK_HTML (HTML_IFRAME (o)->html)->engine;

	if (o->max_width != max_width) {
		o->max_width = max_width;
		html_object_set_max_width (e->clue, e->painter, max_width - (html_engine_get_left_border (e) + html_engine_get_right_border (e)));
	}
}

static void
reset (HTMLObject *o)
{
	HTMLIFrame *iframe;

	(* HTML_OBJECT_CLASS (parent_class)->reset) (o);
	iframe = HTML_IFRAME (o);
	html_object_reset (GTK_HTML (iframe->html)->engine->clue);
}

/* FIXME rodo - draw + set_painter is not much clean now, needs refactoring */

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x,
      gint y,
      gint width,
      gint height,
      gint tx,
      gint ty)
{
	HTMLIFrame   *iframe  = HTML_IFRAME (o);
	HTMLEngine   *e       = GTK_HTML (iframe->html)->engine;
	GdkRectangle paint;

	if (G_OBJECT_TYPE (e->painter) == HTML_TYPE_PRINTER) {
		gint pixel_size = html_painter_get_pixel_size (e->painter);

		if (!html_object_intersect (o, &paint, x, y, width, height))
			return;

		html_object_draw (e->clue, e->painter,
				  x, y,
				  width - pixel_size * (html_engine_get_left_border (e) + html_engine_get_right_border (e)),
				  height - pixel_size * (html_engine_get_top_border (e) + html_engine_get_bottom_border (e)),
				  tx + pixel_size * html_engine_get_left_border (e), ty + pixel_size * html_engine_get_top_border (e));
	} else
		(*HTML_OBJECT_CLASS (parent_class)->draw) (o, p, x, y, width, height, tx, ty);
}

static void
set_painter (HTMLObject *o,
             HTMLPainter *painter)
{
	HTMLIFrame *iframe;

	iframe = HTML_IFRAME (o);
	if (G_OBJECT_TYPE (GTK_HTML (iframe->html)->engine->painter) != HTML_TYPE_PRINTER) {
		iframe_set_gdk_painter (iframe, GTK_HTML (iframe->html)->engine->painter);
	}

	html_engine_set_painter (GTK_HTML (iframe->html)->engine,
				 G_OBJECT_TYPE (painter) != HTML_TYPE_PRINTER ? iframe->gdk_painter : painter);
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
check_page_split (HTMLObject *self,
                  HTMLPainter *p,
                  gint y)
{
	HTMLEngine *e = GTK_HTML (HTML_IFRAME (self)->html)->engine;
	gint y1, y2, pixel_size = html_painter_get_pixel_size (p);

	y1 = self->y - self->ascent + pixel_size * html_engine_get_top_border (e);
	y2 = self->y + self->descent + pixel_size * html_engine_get_bottom_border (e);

	if (y1 > y)
		return 0;

	if (y >= y1 && y < y2)
		return html_object_check_page_split (e->clue, p, y - y1) + y1;

	return y;
}

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	HTMLIFrame *s = HTML_IFRAME (self);
	HTMLIFrame *d = HTML_IFRAME (dest);

	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);
	/*
	html_iframe_init (d, &html_iframe_class,
			  HTML_EMBEDDED (dest)->parent,
			  s->url,
			  s->width,
			  s->height,
			  s->frameborder);
	*/
	d->scroll = s->scroll;
	d->html = gtk_html_new ();

	d->gdk_painter = NULL;

	d->url = g_strdup (s->url);
	d->width = s->width;
	d->height = s->height;
	d->frameborder = s->frameborder;
}

static HTMLObject *
op_copy (HTMLObject *self,
         HTMLObject *parent,
         HTMLEngine *e,
         GList *from,
         GList *to,
         guint *len)
{
	HTMLObject *dup, *clue;

	dup = html_object_dup (self);
	clue = GTK_HTML (HTML_IFRAME (self)->html)->engine->clue;
	GTK_HTML (HTML_IFRAME (dup)->html)->engine->clue =
		html_object_op_copy (clue, dup, GTK_HTML (HTML_IFRAME (self)->html)->engine,
				     html_object_get_bound_list (clue, from),
				     html_object_get_bound_list (clue, to), len);
	GTK_HTML (HTML_IFRAME (dup)->html)->engine->clue->parent = parent;

	return dup;
}

static HTMLAnchor *
find_anchor (HTMLObject *self,
             const gchar *name,
             gint *x,
             gint *y)
{
	HTMLIFrame *iframe;
	HTMLAnchor *anchor;

	g_return_val_if_fail (HTML_IS_IFRAME (self), NULL);

	iframe = HTML_IFRAME (self);

	if (!iframe || !iframe->html || !GTK_IS_HTML (iframe->html) || !GTK_HTML (iframe->html)->engine || !GTK_HTML (iframe->html)->engine->clue)
		return NULL;

	anchor = html_object_find_anchor (GTK_HTML (iframe->html)->engine->clue, name, x, y);

	if (anchor) {
		*x += self->x;
		*y += self->y - self->ascent;
	}

	return anchor;
}

void
html_iframe_set_margin_width (HTMLIFrame *iframe,
                              gint margin_width)
{
	HTMLEngine *e;

	e = GTK_HTML (iframe->html)->engine;

	e->leftBorder = e->rightBorder = margin_width;
	html_engine_schedule_redraw (e);
}

void
html_iframe_set_margin_height (HTMLIFrame *iframe,
                               gint margin_height)
{
	HTMLEngine *e;

	e = GTK_HTML (iframe->html)->engine;

	e->bottomBorder = e->topBorder = margin_height;
	html_engine_schedule_redraw (e);
}

void
html_iframe_set_scrolling (HTMLIFrame *iframe,
                           GtkPolicyType scroll)
{
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (iframe->scroll),
					scroll, scroll);
}

static gboolean
html_iframe_real_calc_size (HTMLObject *o,
                            HTMLPainter *painter,
                            GList **changed_objs)
{
	HTMLIFrame *iframe;
	HTMLEngine *e;
	gint old_width, old_ascent, old_descent;

	old_width = o->width;
	old_ascent = o->ascent;
	old_descent = o->descent;

	iframe = HTML_IFRAME (o);
	e      = GTK_HTML (iframe->html)->engine;

	if (HTML_EMBEDDED (o)->widget == NULL)
		return TRUE;

	if ((iframe->width < 0) && (iframe->height < 0)) {
		if (e->clue) {
			html_engine_calc_size (e, changed_objs);
			e->width = html_engine_get_doc_width (e);
			e->height = html_engine_get_doc_height (e);
		}
		html_iframe_set_scrolling (iframe, GTK_POLICY_NEVER);

		o->width = e->width;
		o->ascent = e->height;
		o->descent = 0;

		if (G_OBJECT_TYPE (painter) == HTML_TYPE_PRINTER) {
			o->ascent += html_painter_get_pixel_size (painter) * (html_engine_get_top_border (e) + html_engine_get_bottom_border (e));
		}
	} else
		return (* HTML_OBJECT_CLASS (parent_class)->calc_size) (o, painter, changed_objs);

	if (o->descent != old_descent
	    || o->ascent != old_ascent
	    || o->width != old_width)
		return TRUE;

	return FALSE;
}

static gboolean
search (HTMLObject *self,
        HTMLSearch *info)
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
get_engine (HTMLObject *self,
            HTMLEngine *e)
{
	return GTK_HTML (HTML_IFRAME (self)->html)->engine;
}

static HTMLObject *
check_point (HTMLObject *self,
             HTMLPainter *painter,
             gint x,
             gint y,
             guint *offset_return,
             gboolean for_cursor)
{
	HTMLEngine *e = GTK_HTML (HTML_IFRAME (self)->html)->engine;

	if (x < self->x || x >= self->x + self->width
	    || y >= self->y + self->descent || y < self->y - self->ascent)
		return NULL;

	x -= self->x - e->x_offset;
	y -= self->y - self->ascent - e->y_offset;

	return html_engine_get_object_at (e, x, y, offset_return, for_cursor);
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

static void
reparent (HTMLEmbedded *emb,
          GtkWidget *html)
{
	HTMLIFrame *iframe = HTML_IFRAME (emb);

	gtk_html_set_iframe_parent (GTK_HTML (iframe->html),
				    html,
				    GTK_HTML (iframe->html)->frame);
	(* HTML_EMBEDDED_CLASS (parent_class)->reparent) (emb, html);
}

/* static gboolean
select_range (HTMLObject *self,
	 *    HTMLEngine *engine,
	 *    guint start,
	 *    gint length,
	 *    gboolean queue_draw)
{
	return html_object_select_range (GTK_HTML (HTML_IFRAME (self)->html)->engine->clue,
					 GTK_HTML (HTML_IFRAME (self)->html)->engine,
					 start, length, queue_draw);
} */

static gboolean
save (HTMLObject *s,
      HTMLEngineSaveState *state)
{
	HTMLIFrame *iframe = HTML_IFRAME (s);
	HTMLEngineSaveState *buffer;
	HTMLEngine *e;

	e = GTK_HTML (iframe->html)->engine;

	/*
	 * FIXME: we should actually save the iframe definition if inline_frames is not
	 * set, but that is a feature and not critical for release.  We should also probably
	 * wrap the body in a <table> tag with a bg etc.
	 */
	if (state->inline_frames && e->clue) {
		buffer = html_engine_save_buffer_new (e, state->inline_frames);
		html_object_save (e->clue, buffer);

		if (state->error ||
		    !html_engine_save_output_buffer (state,
						     (gchar *) html_engine_save_buffer_peek_text (buffer),
						     html_engine_save_buffer_peek_text_bytes (buffer))) {
			html_engine_save_buffer_free (buffer, TRUE);
			return FALSE;
		}
		html_engine_save_buffer_free (buffer, TRUE);
	} else {
		if (!html_engine_save_delims_and_vals (state, "<IFRAME SRC=\"", iframe->url, "\"", NULL))
			 return FALSE;

		 if (iframe->width >= 0)
			 if (!html_engine_save_output_string (state, " WIDTH=\"%d\"", iframe->width))
				 return FALSE;

		 if (iframe->width >= 0)
			 if (!html_engine_save_output_string (state, " WIDTH=\"%d\"", iframe->width))
				 return FALSE;

		 if (e->topBorder != TOP_BORDER || e->bottomBorder != BOTTOM_BORDER)
			 if (!html_engine_save_output_string (state, " MARGINHEIGHT=\"%d\"", e->topBorder))
				 return FALSE;

		 if (e->leftBorder != LEFT_BORDER || e->rightBorder != RIGHT_BORDER)
			 if (!html_engine_save_output_string (state, " MARGINWIDTH=\"%d\"", e->leftBorder))
				 return FALSE;

		 if (!html_engine_save_output_string (state, " FRAMEBORDER=\"%d\"", iframe->frameborder))
			 return FALSE;

		 if (!html_engine_save_output_string (state, "></IFRAME>"))
		     return FALSE;
	}
	return TRUE;
}

static gboolean
save_plain (HTMLObject *s,
            HTMLEngineSaveState *state,
            gint requested_width)
{
	HTMLIFrame *iframe = HTML_IFRAME (s);
	HTMLEngineSaveState *buffer;
	HTMLEngine *e;

	e = GTK_HTML (iframe->html)->engine;

	if (state->inline_frames && e->clue) {
		buffer = html_engine_save_buffer_new (e, state->inline_frames);
		html_object_save_plain (e->clue, buffer, requested_width);
		if (state->error ||
		    !html_engine_save_output_buffer (state,
						     (gchar *) html_engine_save_buffer_peek_text (buffer),
						     html_engine_save_buffer_peek_text_bytes (buffer))) {
			html_engine_save_buffer_free (buffer, TRUE);
			return FALSE;
		}
		html_engine_save_buffer_free (buffer, TRUE);
	}

	return TRUE;
}

static void
destroy (HTMLObject *o)
{
	HTMLIFrame *iframe = HTML_IFRAME (o);

	iframe_set_gdk_painter (iframe, NULL);

	g_free (iframe->url);

	if (iframe->html) {
		g_signal_handlers_disconnect_matched (iframe->html, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, o);
		iframe->html = NULL;
	}

	HTML_OBJECT_CLASS (parent_class)->destroy (o);
}

void
html_iframe_init (HTMLIFrame *iframe,
                  HTMLIFrameClass *klass,
                  GtkWidget *parent,
                  gchar *src,
                  gint width,
                  gint height,
                  gboolean border)
{
	HTMLEmbedded *em = HTML_EMBEDDED (iframe);
	HTMLTokenizer *new_tokenizer;
	GtkWidget *new_widget;
	GtkHTML   *new_html;
	GtkHTML   *parent_html;
	GtkWidget *scrolled_window;
	gint depth;

	g_assert (GTK_IS_HTML (parent));
	parent_html = GTK_HTML (parent);

	html_embedded_init (em, HTML_EMBEDDED_CLASS (klass),
			    parent, NULL, NULL);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     border ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
	/*
	 * FIXME
	 * are we missing:
	 * gtk_widget_show (scrolled_window); here?
	 */

	iframe->scroll = scrolled_window;
	html_iframe_set_scrolling (iframe, GTK_POLICY_AUTOMATIC);

	new_widget = gtk_html_new ();
	new_html = GTK_HTML (new_widget);
	new_html->engine->cursor_hide_count = 0;

	new_tokenizer = html_tokenizer_clone (parent_html->engine->ht);

	html_engine_set_tokenizer (new_html->engine, new_tokenizer);
	g_object_unref (G_OBJECT (new_tokenizer));
	new_tokenizer = NULL;

	gtk_html_set_default_content_type (new_html,
					   gtk_html_get_default_content_type (parent_html));

	gtk_html_set_default_engine (new_html,
					   gtk_html_get_default_engine (parent_html));

	iframe->html = new_widget;
	iframe->url = g_strdup (src);
	iframe->width = width;
	iframe->height = height;
	iframe->gdk_painter = NULL;
	iframe->frameborder = border;
	gtk_html_set_base (new_html, src);
	depth = gtk_html_set_iframe_parent (new_html, parent, HTML_OBJECT (iframe));
	gtk_container_add (GTK_CONTAINER (scrolled_window), new_widget);
	gtk_widget_show (new_widget);

	g_signal_connect (new_html, "url_requested", G_CALLBACK (iframe_url_requested), iframe);

	if (depth < 10) {
		if (parent_html->engine->stopped) {
			gtk_html_stop (new_html);
			gtk_html_load_empty (new_html);
		} else {
			GtkHTMLStream *handle;

			handle = gtk_html_begin (new_html);
			g_signal_emit_by_name (parent_html->engine, "url_requested", src, handle);
		}
	} else
		gtk_html_load_empty (new_html);

	new_html->engine->clue->parent = HTML_OBJECT (iframe);

#if 0
	/* NOTE: because of peculiarities of the frame/gtkhtml relationship
	 * on_url and link_clicked are emitted from the toplevel widget not
	 * proxied like url_requested is.
	 */
	g_signal_connect (new_html, "on_url", G_CALLBACK (iframe_on_url), iframe);
	g_signal_connect (new_html, "link_clicked", G_CALLBACK (iframe_link_clicked), iframe);
#endif
	g_signal_connect (new_html, "size_changed", G_CALLBACK (iframe_size_changed), iframe);
	g_signal_connect (new_html, "set_base", G_CALLBACK (iframe_set_base), iframe);
	g_signal_connect (new_html, "object_requested", G_CALLBACK (iframe_object_requested), iframe);

	/*
	  g_signal_connect (html, "button_press_event", G_CALLBACK (iframe_button_press_event), iframe);
	*/

	gtk_widget_set_size_request (scrolled_window, width, height);

	gtk_widget_show (scrolled_window);

	html_embedded_set_widget (em, scrolled_window);

	g_signal_connect (scrolled_window, "button_press_event", G_CALLBACK (html_iframe_grab_cursor), NULL);

	/* inherit the current colors from our parent */
	html_colorset_set_unchanged (new_html->engine->defaultSettings->color_set,
				     parent_html->engine->settings->color_set);
	html_colorset_set_unchanged (new_html->engine->settings->color_set,
				     parent_html->engine->settings->color_set);
	html_painter_set_focus (new_html->engine->painter, parent_html->engine->have_focus);
	/*
	g_signal_connect (html, "title_changed", G_CALLBACK (title_changed_cb), (gpointer) app);
	g_signal_connect (html, "button_press_event", G_CALLBACK (on_button_press_event), popup_menu);
	g_signal_connect (html, "redirect", G_CALLBACK (on_redirect), NULL);
	g_signal_connect (html, "object_requested", G_CALLBACK (object_requested_cmd), NULL);
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

	object_class->destroy                 = destroy;
	object_class->save                    = save;
	object_class->save_plain              = save_plain;
	object_class->calc_size               = html_iframe_real_calc_size;
	object_class->calc_min_width          = calc_min_width;
	object_class->set_painter             = set_painter;
	object_class->reset                   = reset;
	object_class->draw                    = draw;
	object_class->copy                    = copy;
	object_class->op_copy                 = op_copy;
	object_class->set_max_width           = set_max_width;
	object_class->forall                  = forall;
	object_class->check_page_split        = check_page_split;
	object_class->search                  = search;
	object_class->head                    = head;
	object_class->tail                    = tail;
	object_class->get_engine              = get_engine;
	object_class->check_point             = check_point;
	object_class->is_container            = is_container;
	object_class->append_selection_string = append_selection_string;
	object_class->find_anchor             = find_anchor;

	embedded_class->reparent = reparent;
}
