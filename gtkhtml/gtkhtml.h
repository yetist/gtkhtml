/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
 *
 *  Copyright 1999, 2000 Helix Code, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

#ifndef _GTKHTML_H_
#define _GTKHTML_H_

#include <sys/types.h>

#include "gtkhtml-types.h"
#include "gtkhtml-enums.h"
#include <gtk/gtk.h>

/* FIXME we should remove html dep */
#include "htmltypes.h"

G_BEGIN_DECLS

#define GTK_TYPE_HTML                  (gtk_html_get_type ())
#define GTK_HTML(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_HTML, GtkHTML))
#define GTK_HTML_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_HTML, GtkHTMLClass))
#define GTK_IS_HTML(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_HTML))
#define GTK_IS_HTML_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HTML))

struct _GtkHTML {
	GtkLayout layout;

	GtkWidget            *iframe_parent;
	HTMLObject           *frame;
	GtkHTMLEditorAPI     *editor_api;
	gpointer              editor_data;
	HTMLEngine           *engine;

	/* The URL of the link over which the pointer currently is.  NULL if
	 * the pointer is not over a link.  */
	gchar *pointer_url;

	/* The cursors we use within the widget.  */
	GdkCursor *hand_cursor;
	GdkCursor *ibeam_cursor;

	gint selection_x1, selection_y1;

	guint in_selection : 1;
	guint in_selection_drag : 1;

	guint debug : 1;
	guint allow_selection : 1;

	guint hadj_connection;
	guint vadj_connection;

	gboolean binding_handled;
	GtkHTMLPrivate *priv;
};

struct _GtkHTMLClass {
	GtkLayoutClass parent_class;

        void (* title_changed)   (GtkHTML *html, const gchar *new_title);
        void (* url_requested)   (GtkHTML *html, const gchar *url, GtkHTMLStream *handle);
        void (* load_done)       (GtkHTML *html);
        void (* link_clicked)    (GtkHTML *html, const gchar *url);
	void (* set_base)        (GtkHTML *html, const gchar *base_url);
	void (* set_base_target) (GtkHTML *html, const gchar *base_url);

	void (* on_url)		 (GtkHTML *html, const gchar *url);
	void (* redirect)        (GtkHTML *html, const gchar *url, gint delay);
	void (* submit)          (GtkHTML *html, const gchar *method, const gchar *url, const gchar *encoding);
	gboolean (* object_requested)(GtkHTML *html, GtkHTMLEmbedded *);

	void (* current_paragraph_style_changed) (GtkHTML *html, GtkHTMLParagraphStyle new_style);
	void (* current_paragraph_alignment_changed) (GtkHTML *html, GtkHTMLParagraphAlignment new_alignment);
	void (* current_paragraph_indentation_changed) (GtkHTML *html, guint new_indentation);
	void (* insertion_font_style_changed) (GtkHTML *html, GtkHTMLFontStyle style);
	void (* insertion_color_changed) (GtkHTML *html, GdkColor *color);

        void (* size_changed)       (GtkHTML *html);
	void (* iframe_created)     (GtkHTML *html, GtkHTML *iframe);

	/* keybindings signals */
	void     (* scroll)               (GtkHTML *html, GtkOrientation orientation, GtkScrollType scroll_type,
					   gfloat position);
	void     (* cursor_move)          (GtkHTML *html, GtkDirectionType dir_type, GtkHTMLCursorSkipType skip);
	gboolean (* command)              (GtkHTML *html, GtkHTMLCommandType com_type);
	void (* cursor_changed)       (GtkHTML *html);
	void (* object_inserted)       (GtkHTML *html, gint pos, gint len);
	void (* object_delete)        (GtkHTML *html, gint pos, gint len);

	/* properties */
	GtkHTMLClassProperties *properties;
	GtkBindingSet *emacs_bindings;
	gboolean use_emacs_bindings;
};

struct _GtkHTMLEditorAPI
{
	/* spell checking methods */
	gboolean  (* check_word)              (GtkHTML *html, const gchar *word, gpointer data);
	void      (* suggestion_request)      (GtkHTML *html, gpointer data);
	void      (* add_to_session)          (GtkHTML *html, const gchar *word, gpointer data);
	void      (* add_to_personal)         (GtkHTML *html, const gchar *word, const gchar *language, gpointer data);

	/* unhandled commands */
	gboolean  (* command)                 (GtkHTML *html, GtkHTMLCommandType com_type, gpointer data);

	GValue  * (* event)                   (GtkHTML *html, GtkHTMLEditorEventType event_type, GValue *args, gpointer data);

	/* input line */
	GtkWidget * (* create_input_line)     (GtkHTML *html, gpointer data);

	/* spell checking methods */
	void      (* set_language)            (GtkHTML *html, const gchar *language, gpointer data);
};

/* Creation.  */
GType                    gtk_html_get_type                      (void);
void                       gtk_html_construct                     (GtkHTML                   *html);
GtkWidget                 *gtk_html_new                           (void);
void                       gtk_html_set_editor_api                (GtkHTML                   *html,
								   GtkHTMLEditorAPI          *api,
								   gpointer                   data);

/* parent iframe setting */
gint                       gtk_html_set_iframe_parent             (GtkHTML                   *html,
								   GtkWidget                 *parent,
								   HTMLObject                *frame);
GtkHTML                   *gtk_html_get_top_html                  (GtkHTML                   *html);

/* Debugging.  */
void                       gtk_html_enable_debug                  (GtkHTML                   *html,
								   gboolean                   debug);

/* Behavior.  */
void                       gtk_html_allow_selection               (GtkHTML                   *html,
								   gboolean                   allow);
void                       gtk_html_select_word                   (GtkHTML                   *html);
void                       gtk_html_select_line                   (GtkHTML                   *html);
void                       gtk_html_select_paragraph              (GtkHTML                   *html);
void                       gtk_html_select_paragraph_extended     (GtkHTML                   *html);
void                       gtk_html_select_all                    (GtkHTML                   *html);
void                       gtk_html_unselect_all                  (GtkHTML                   *html);
/* Loading.  */
GtkHTMLStream             *gtk_html_begin                         (GtkHTML                   *html);
GtkHTMLStream             *gtk_html_begin_full                    (GtkHTML                   *html,
								   gchar                      *target_frame,
								   const gchar                *content_type,
								   GtkHTMLBeginFlags         flags);

void                       gtk_html_write                         (GtkHTML                   *html,
								   GtkHTMLStream             *handle,
								   const gchar               *buffer,
								   gsize                     size);
void                       gtk_html_end                           (GtkHTML                   *html,
								   GtkHTMLStream             *handle,
								   GtkHTMLStreamStatus        status);
void                       gtk_html_flush                         (GtkHTML                   *html);
void                       gtk_html_stop                          (GtkHTML                   *html);
void                       gtk_html_load_from_string              (GtkHTML                   *html,
								   const gchar               *str,
								   gint                       len);

/* Saving.  */
gboolean                   gtk_html_export                        (GtkHTML                   *html,
								   const gchar                *type,
								   GtkHTMLSaveReceiverFn      receiver,
								   gpointer                   data);
gchar *                     gtk_html_get_selection_html            (GtkHTML                   *html,
								   gint                       *len);
gchar *                     gtk_html_get_selection_plain_text      (GtkHTML                   *html,
								   gint                       *len);

/* Editable support.  */
void                       gtk_html_set_editable                  (GtkHTML                   *html,
								   gboolean                   editable);
gboolean                   gtk_html_get_editable                  (const GtkHTML             *html);
void                       gtk_html_set_inline_spelling           (GtkHTML                   *html,
								   gboolean                   inline_spell);
gboolean                   gtk_html_get_inline_spelling           (const GtkHTML             *html);
void                       gtk_html_set_magic_links               (GtkHTML                   *html,
								   gboolean                   magic_links);
gboolean                   gtk_html_get_magic_links               (const GtkHTML             *html);
void                       gtk_html_set_magic_smileys             (GtkHTML                   *html,
								   gboolean                   magic_smileys);
gboolean                   gtk_html_get_magic_smileys             (const GtkHTML             *html);

/* Caret Mode */
void                       gtk_html_set_caret_mode                (GtkHTML                   *html,
								   gboolean                   caret_mode);
gboolean                   gtk_html_get_caret_mode                (const GtkHTML             *html);

void			   gtk_html_set_caret_first_focus_anchor  (GtkHTML		     *html,
								   const gchar                *name);

/* Animated Images */
void                       gtk_html_set_animate                   (GtkHTML                   *html,
								   gboolean                   animate);
gboolean                   gtk_html_get_animate                   (const GtkHTML             *html);

/* Printing support.  */
void			   gtk_html_print_page_with_header_footer (GtkHTML		     *html,
								   GtkPrintContext	     *context,
								   gdouble		      header_height,
								   gdouble		      footer_height,
								   GtkHTMLPrintCallback       header_print,
								   GtkHTMLPrintCallback       footer_print,
								   gpointer		      user_data);
void			   gtk_html_print_page			   (GtkHTML		      *html,
								    GtkPrintContext	      *context);
gint			   gtk_html_print_page_get_pages_num	  (GtkHTML		      *html,
								   GtkPrintContext            *context,
								   gdouble		       header_height,
								   gdouble                     footer_height);
GtkPrintOperationResult	   gtk_html_print_operation_run		  (GtkHTML                    *html,
								   GtkPrintOperation          *operation,
								   GtkPrintOperationAction     action,
								   GtkWindow                  *parent,
								   GtkHTMLPrintCalcHeight      calc_header_height,
								   GtkHTMLPrintCalcHeight      calc_footer_height,
								   GtkHTMLPrintDrawFunc        draw_header,
								   GtkHTMLPrintDrawFunc        draw_footer,
								   gpointer                    user_data,
								   GError                    **error);

/* Title.  */
const gchar               *gtk_html_get_title                     (GtkHTML                   *html);
void                       gtk_html_set_title                     (GtkHTML                   *html, const gchar *title);

/* Anchors.  */
gboolean                   gtk_html_jump_to_anchor                (GtkHTML                   *html,
								   const gchar               *anchor);


/* Editing functions.  */
GtkHTMLParagraphStyle      gtk_html_get_paragraph_style           (GtkHTML                   *html);
void                       gtk_html_set_paragraph_style           (GtkHTML                   *html,
								   GtkHTMLParagraphStyle      style);
void                       gtk_html_set_indent                    (GtkHTML                   *html,
								   GByteArray                *levels);
void                       gtk_html_indent_push_level             (GtkHTML                   *html,
								   HTMLListType               level_type);
void                       gtk_html_indent_pop_level              (GtkHTML                   *html);
guint                      gtk_html_get_paragraph_indentation     (GtkHTML                   *html);
void                       gtk_html_set_font_style                (GtkHTML                   *html,
								   GtkHTMLFontStyle           and_mask,
								   GtkHTMLFontStyle           or_mask);
void                       gtk_html_set_color                     (GtkHTML                   *html,
								   HTMLColor                 *color);
void                       gtk_html_toggle_font_style             (GtkHTML                   *html,
								   GtkHTMLFontStyle           style);
GtkHTMLParagraphAlignment  gtk_html_get_paragraph_alignment       (GtkHTML                   *html);
void                       gtk_html_set_paragraph_alignment       (GtkHTML                   *html,
								   GtkHTMLParagraphAlignment  alignment);
void                       gtk_html_cut                           (GtkHTML                   *html);
void                       gtk_html_copy                          (GtkHTML                   *html);
void                       gtk_html_paste                         (GtkHTML                   *html,
								   gboolean                   as_cite);
void                       gtk_html_undo                          (GtkHTML                   *html);
void                       gtk_html_redo                          (GtkHTML                   *html);
void                       gtk_html_insert_html                   (GtkHTML                   *html,
								   const gchar               *html_src);
void                       gtk_html_insert_gtk_html               (GtkHTML                   *html,
								   GtkHTML                   *to_be_destroyed);
void                       gtk_html_append_html                   (GtkHTML                   *html,
								   const gchar               *html_src);

/* misc utils */
gpointer                   gtk_html_get_object_by_id              (GtkHTML                   *html,
								   const gchar               *id);
const gchar *               gtk_html_get_object_id_at              (GtkHTML                   *html,
								   gint                        x,
								   gint                        y);
gboolean                   gtk_html_command                       (GtkHTML                   *html,
								   const gchar               *command_name);
gboolean                   gtk_html_edit_make_cursor_visible      (GtkHTML                   *html);

void                       gtk_html_set_magnification             (GtkHTML                   *html,
								   gdouble                    magnification);
void                       gtk_html_zoom_in                       (GtkHTML                   *html);
void                       gtk_html_zoom_out                      (GtkHTML                   *html);
void                       gtk_html_zoom_reset                    (GtkHTML                   *html);
void                       gtk_html_update_styles                 (GtkHTML                   *html);
void                       gtk_html_set_allow_frameset            (GtkHTML                   *html,
								   gboolean                   allow);
gboolean                   gtk_html_get_allow_frameset            (GtkHTML                   *html);
void                       gtk_html_set_base                      (GtkHTML                   *html,
								   const gchar                *url);
const gchar                *gtk_html_get_base                      (GtkHTML                   *html);
gchar                      *gtk_html_get_url_base_relative         (GtkHTML                   *html,
								   const gchar                *url);
gchar                      *gtk_html_get_url_object_relative       (GtkHTML                   *html,
								   HTMLObject                *o,
								   const gchar                *url);
void                       gtk_html_images_ref                    (GtkHTML                   *html);
void                       gtk_html_images_unref                  (GtkHTML                   *html);
void                       gtk_html_image_ref                     (GtkHTML                   *html,
								   const gchar               *url);
void                       gtk_html_image_unref                   (GtkHTML                   *html,
								   const gchar               *url);
void                       gtk_html_image_preload                 (GtkHTML                   *html,
								   const gchar               *url);
void                       gtk_html_set_blocking                  (GtkHTML                   *html,
								   gboolean                   block);
void                       gtk_html_set_images_blocking           (GtkHTML                   *html,
								   gboolean                   block);
gboolean                   gtk_html_has_undo                      (GtkHTML                   *html);
void                       gtk_html_drop_undo                     (GtkHTML                   *html);
gchar *                     gtk_html_get_url_at                    (GtkHTML                   *html,
								   gint                        x,
								   gint                        y);
gchar *                     gtk_html_get_cursor_url                (GtkHTML                   *html);

gchar *                     gtk_html_get_image_src_at              (GtkHTML                   *html,
								   gint                        x,
								   gint                        y);
gchar *                     gtk_html_get_cursor_image_src          (GtkHTML                   *html);
GdkPixbufAnimation *        gtk_html_get_image_at		   (GtkHTML                   *html,
								    gint		      x,
								    gint		      y);

void                       gtk_html_set_tokenizer                 (GtkHTML                   *html,
								   HTMLTokenizer             *tokenizer);

gboolean		   gtk_html_get_cursor_pos		  (GtkHTML *html, gint *position, gint *offset);

/* DEPRECATED */
#if 1
gboolean                   gtk_html_build_with_gconf              (void);
const gchar *               gtk_html_get_default_content_type              (GtkHTML                   *html);
void                       gtk_html_set_default_content_type              (GtkHTML                   *html,
								   const gchar                     *content_type);
void			   gtk_html_set_default_engine		  (GtkHTML *html,
								   gboolean enginetype);
gboolean		   gtk_html_get_default_engine		  (GtkHTML *html);
GtkWidget                 *gtk_html_new_from_string               (const gchar               *Astr,
								   gint                       len);
void                       gtk_html_load_empty                    (GtkHTML                   *html);
gboolean                   gtk_html_save                          (GtkHTML                   *html,
								   GtkHTMLSaveReceiverFn      receiver,
								   gpointer                   data);
GtkHTMLStream             *gtk_html_begin_content                 (GtkHTML                   *html,
								   const gchar               *content_type);
void                       gtk_html_drag_dest_set                 (GtkHTML                   *html);

#endif

G_END_DECLS

#endif /* _GTKHTML_H_ */
