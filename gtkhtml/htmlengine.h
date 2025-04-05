/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 * Copyright (C) 1997 Torben Weis (weis@kde.org)
 * Copyright (C) 1999, 2000 Helix Code, Inc.
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

#ifndef _HTMLENGINE_H_
#define _HTMLENGINE_H_

#include <gtk/gtk.h>
#include "gtkhtml-types.h"

#include "htmltypes.h"
#include "htmlenums.h"
#include "htmlcursor.h"


#define HTML_TYPE_ENGINE                 (html_engine_get_type ())
G_DECLARE_DERIVABLE_TYPE (HTMLEngine, html_engine, HTML, ENGINE, GObject)


/* FIXME extreme hideous ugliness in the following lines.  */

#define LEFT_BORDER 10
#define RIGHT_BORDER 10
#define TOP_BORDER 10
#define BOTTOM_BORDER 10


/* must be forward referenced *sigh* */
struct _HTMLEmbedded;

struct _HTMLEngineClass {
	GObjectClass parent_class;

	void (* title_changed) (HTMLEngine *engine);
	void (* set_base) (HTMLEngine *engine, const gchar *base);
	void (* set_base_target) (HTMLEngine *engine, const gchar *base_target);
	void (* load_done) (HTMLEngine *engine);
        void (* url_requested) (HTMLEngine *engine, const gchar *url, GtkHTMLStream *handle);
	void (* draw_pending) (HTMLEngine *engine);
        void (* redirect) (HTMLEngine *engine, const gchar *url, gint delay);
        void (* submit) (HTMLEngine *engine, const gchar *method, const gchar *action, const gchar *encoding);
	gboolean (* object_requested) (HTMLEngine *engine, GtkHTMLEmbedded *);
	void (* undo_changed) (HTMLEngine *engine);
};

HTMLEngine *html_engine_new           (GtkWidget *);
void        html_engine_realize       (HTMLEngine *engine,
				       GdkWindow  *window);
void        html_engine_unrealize     (HTMLEngine *engine);

void      html_engine_saved     (HTMLEngine *e);
gboolean  html_engine_is_saved  (HTMLEngine *e);

/* Editability control.  */
void      html_engine_set_editable  (HTMLEngine *e,
				     gboolean    editable);
gboolean  html_engine_get_editable  (HTMLEngine *e);

/* Focus.  */
void  html_engine_set_focus  (HTMLEngine *engine,
			      gboolean    have_focus);

/* Tokenizer. */
void html_engine_set_tokenizer (HTMLEngine *engine,
				HTMLTokenizer *tok);

/* Parsing control.  */
GtkHTMLStream *html_engine_begin            (HTMLEngine  *p,
					     const gchar  *content_type);
void           html_engine_parse            (HTMLEngine  *p);
void           html_engine_stop_parser      (HTMLEngine  *e);
void           html_engine_stop             (HTMLEngine  *e);
void           html_engine_flush            (HTMLEngine  *e);
void           html_engine_set_engine_type   (HTMLEngine *e,
					 gboolean engine_type);
gboolean       html_engine_get_engine_type   (HTMLEngine *e);
void		   html_engine_set_content_type (HTMLEngine *e,
					const gchar * content_type);
const gchar *  html_engine_get_content_type (HTMLEngine *e);

/* Rendering control.  */
gint  html_engine_calc_min_width       (HTMLEngine *e);
gboolean  html_engine_calc_size        (HTMLEngine *e,
					GList     **changed_objs);
gint  html_engine_get_doc_height       (HTMLEngine *p);
gint  html_engine_get_doc_width        (HTMLEngine *e);
gint  html_engine_get_max_width        (HTMLEngine *e);
gint  html_engine_get_max_height       (HTMLEngine *e);
void  html_engine_draw                 (HTMLEngine *e,
					gint        x,
					gint        y,
					gint        width,
					gint        height);
void  html_engine_draw_cb              (HTMLEngine *e,
					cairo_t    *cr);
void  html_engine_draw_background      (HTMLEngine *e,
					gint        x,
					gint        y,
					gint        width,
					gint        height);

/* Scrolling.  */
void      html_engine_schedule_update      (HTMLEngine  *e);
void      html_engine_schedule_redraw      (HTMLEngine  *e);
void      html_engine_block_redraw         (HTMLEngine  *e);
void      html_engine_unblock_redraw       (HTMLEngine  *e);
gboolean  html_engine_make_cursor_visible  (HTMLEngine  *e);
gboolean  html_engine_goto_anchor          (HTMLEngine  *e,
					    const gchar *anchor);

/* Draw/clear queue.  */
void  html_engine_flush_draw_queue  (HTMLEngine *e);
void  html_engine_queue_draw        (HTMLEngine *e,
				     HTMLObject *o);
void  html_engine_queue_clear       (HTMLEngine *e,
				     gint        x,
				     gint        y,
				     guint       width,
				     guint       height);

void  html_engine_set_painter       (HTMLEngine  *e,
				     HTMLPainter *painter);

/* Getting objects through pointer positions.  */
HTMLObject  *html_engine_get_object_at  (HTMLEngine *e,
					 gint        x,
					 gint        y,
					 guint      *offset_return,
					 gboolean    for_cursor);
HTMLPoint   *html_engine_get_point_at   (HTMLEngine *e,
					 gint        x,
					 gint        y,
					 gboolean    for_cursor);
const gchar *html_engine_get_link_at    (HTMLEngine *e,
					 gint        x,
					 gint        y);

/* Form support.  */
void  html_engine_form_submitted  (HTMLEngine  *engine,
				   const gchar *method,
				   const gchar *action,
				   const gchar *encoding);

/* Misc.  (FIXME: Should die?) */
gchar *html_engine_canonicalize_url  (HTMLEngine *e,
				      const gchar *in_url);

/* Cursor */
HTMLCursor *html_engine_get_cursor        (HTMLEngine *e);
void        html_engine_normalize_cursor  (HTMLEngine *e);

/* Freezing/thawing.  */
gboolean  html_engine_frozen  (HTMLEngine *engine);
void      html_engine_freeze  (HTMLEngine *engine);
void      html_engine_thaw    (HTMLEngine *engine);

/* Creating an empty document.  */
void      html_engine_load_empty                (HTMLEngine *engine);

/* Search & Replace */

void      html_engine_replace                   (HTMLEngine *e,
						 const gchar *text,
						 const gchar *rep_text,
						 gboolean case_sensitive,
						 gboolean forward,
						 gboolean regular,
						 void (*ask)(HTMLEngine *, gpointer), gpointer ask_data);
gboolean  html_engine_replace_do                (HTMLEngine *e, HTMLReplaceQueryAnswer answer);
gint      html_engine_replaced                  (void);

/* Magic links */
void      html_engine_init_magic_links          (void);

/* spell checking */
void      html_engine_spell_check              (HTMLEngine  *e);
void      html_engine_clear_spell_check        (HTMLEngine  *e);
gchar    *html_engine_get_spell_word           (HTMLEngine  *e);
gboolean  html_engine_spell_word_is_valid      (HTMLEngine  *e);
void      html_engine_replace_spell_word_with  (HTMLEngine  *e,
						const gchar *word);
void      html_engine_set_language             (HTMLEngine  *e,
						const gchar *language);
const gchar *html_engine_get_language          (HTMLEngine *e);

/* view size - for image size specified in percent */
gint  html_engine_get_view_width   (HTMLEngine *e);
gint  html_engine_get_view_height  (HTMLEngine *e);

/* id support */
void        html_engine_add_object_with_id  (HTMLEngine  *e,
					     const gchar *id,
					     HTMLObject  *obj);
HTMLObject *html_engine_get_object_by_id    (HTMLEngine  *e,
					     const gchar *id);

HTMLEngine *html_engine_get_top_html_engine (HTMLEngine *e);
void        html_engine_thaw_idle_flush     (HTMLEngine *e);

/* class data */
const gchar *html_engine_get_class_data        (HTMLEngine  *e,
						const gchar *class_name,
						const gchar *key);
GHashTable  *html_engine_get_class_table       (HTMLEngine  *e,
						const gchar *class_name);
void         html_engine_set_class_data        (HTMLEngine  *e,
						const gchar *class_name,
						const gchar *key,
						const gchar *value);
void         html_engine_clear_class_data      (HTMLEngine  *e,
						const gchar *class_name,
						const gchar *key);
void         html_engine_clear_all_class_data  (HTMLEngine  *e);
gboolean  html_engine_intersection  (HTMLEngine *e,
				     gint       *x1,
				     gint       *y1,
				     gint       *x2,
				     gint       *y2);
void  html_engine_add_expose  (HTMLEngine *e,
			       gint        x,
			       gint        y,
			       gint        width,
			       gint        height,
			       gboolean    expose);
void html_engine_redraw_selection (HTMLEngine *e);

gboolean    html_engine_focus              (HTMLEngine       *e,
					    GtkDirectionType  dir);
HTMLObject *html_engine_get_focus_object   (HTMLEngine       *e,
					    gint             *offset);
void        html_engine_set_focus_object   (HTMLEngine       *e,
					    HTMLObject       *o,
					    gint              offset);
void        html_engine_update_focus_if_necessary (HTMLEngine	*e,
						   HTMLObject	*o,
						   gint		offset);

HTMLMap *html_engine_get_map  (HTMLEngine  *e,
			       const gchar *name);

gboolean html_engine_selection_contains_object_type (HTMLEngine *e,
						     HTMLType obj_type);
gboolean html_engine_selection_contains_link        (HTMLEngine *e);

gint  html_engine_get_left_border    (HTMLEngine *e);
gint  html_engine_get_right_border   (HTMLEngine *e);
gint  html_engine_get_top_border     (HTMLEngine *e);
gint  html_engine_get_bottom_border  (HTMLEngine *e);

HTMLImageFactory *html_engine_get_image_factory (HTMLEngine *e);
void html_engine_opened_streams_increment (HTMLEngine *e);
void html_engine_opened_streams_decrement (HTMLEngine *e);
void html_engine_opened_streams_set (HTMLEngine *e, gint value);

void html_engine_refresh_fonts (HTMLEngine *e);

void html_engine_emit_undo_changed (HTMLEngine *e);

#endif /* _HTMLENGINE_H_ */
