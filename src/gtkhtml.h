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

#ifndef _GTKHTML_H_
#define _GTKHTML_H_


typedef struct _GtkHTML	GtkHTML;
typedef struct _GtkHTMLClass	GtkHTMLClass;

#include <sys/types.h>
#include <gtk/gtk.h>
#include <libgnomeprint/gnome-print.h>


#define GTK_TYPE_HTML                  (gtk_html_get_type ())
#define GTK_HTML(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_HTML, GtkHTML))
#define GTK_HTML_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_HTML, GtkHTMLClass))
#define GTK_IS_HTML(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_HTML))
#define GTK_IS_HTML_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HTML))

#define GTK_HTML_PROPERTY(w,p)         (GTK_HTML_CLASS (GTK_OBJECT (w)->klass)->properties-> ## p)


typedef struct _GtkHTMLStream GtkHTMLStream;

enum _GtkHTMLStreamStatus {
	GTK_HTML_STREAM_OK,
	GTK_HTML_STREAM_ERROR
};
typedef enum _GtkHTMLStreamStatus GtkHTMLStreamStatus;

enum _GtkHTMLParagraphStyle {
	GTK_HTML_PARAGRAPH_STYLE_NORMAL,
	GTK_HTML_PARAGRAPH_STYLE_H1,
	GTK_HTML_PARAGRAPH_STYLE_H2,
	GTK_HTML_PARAGRAPH_STYLE_H3,
	GTK_HTML_PARAGRAPH_STYLE_H4,
	GTK_HTML_PARAGRAPH_STYLE_H5,
	GTK_HTML_PARAGRAPH_STYLE_H6,
	GTK_HTML_PARAGRAPH_STYLE_ADDRESS,
	GTK_HTML_PARAGRAPH_STYLE_PRE,
	GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED,
	GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN,
	GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT,
};
typedef enum _GtkHTMLParagraphStyle GtkHTMLParagraphStyle;

enum _GtkHTMLParagraphAlignment {
	GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT,
	GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT,
	GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER
};
typedef enum _GtkHTMLParagraphAlignment GtkHTMLParagraphAlignment;

enum _GtkHTMLCursorSkipType {
	GTK_HTML_CURSOR_SKIP_ONE,
	GTK_HTML_CURSOR_SKIP_WORD,
	GTK_HTML_CURSOR_SKIP_PAGE,
	GTK_HTML_CURSOR_SKIP_ALL,
};
typedef enum _GtkHTMLCursorSkipType GtkHTMLCursorSkipType;

enum _GtkHTMLCommandType {
	GTK_HTML_COMMAND_UNDO,
	GTK_HTML_COMMAND_REDO,
	GTK_HTML_COMMAND_COPY,
	GTK_HTML_COMMAND_CUT,
	GTK_HTML_COMMAND_PASTE,

	GTK_HTML_COMMAND_CUT_LINE,

	GTK_HTML_COMMAND_INSERT_PARAGRAPH,
	GTK_HTML_COMMAND_INSERT_RULE,
	GTK_HTML_COMMAND_INSERT_RULE_PARAM,
	GTK_HTML_COMMAND_INSERT_IMAGE_PARAM,

	GTK_HTML_COMMAND_MAKE_LINK,
	GTK_HTML_COMMAND_REMOVE_LINK,

	GTK_HTML_COMMAND_DELETE,
	GTK_HTML_COMMAND_DELETE_BACK,

	GTK_HTML_COMMAND_SET_MARK,
	GTK_HTML_COMMAND_DISABLE_SELECTION,

	GTK_HTML_COMMAND_TOGGLE_BOLD,
	GTK_HTML_COMMAND_TOGGLE_ITALIC,
	GTK_HTML_COMMAND_TOGGLE_UNDERLINE,
	GTK_HTML_COMMAND_TOGGLE_STRIKEOUT,

	GTK_HTML_COMMAND_SIZE_MINUS_2,
	GTK_HTML_COMMAND_SIZE_MINUS_1,
	GTK_HTML_COMMAND_SIZE_PLUS_0,
	GTK_HTML_COMMAND_SIZE_PLUS_1,
	GTK_HTML_COMMAND_SIZE_PLUS_2,
	GTK_HTML_COMMAND_SIZE_PLUS_3,
	GTK_HTML_COMMAND_SIZE_PLUS_4,

	GTK_HTML_COMMAND_SIZE_INCREASE,
	GTK_HTML_COMMAND_SIZE_DECREASE,

	GTK_HTML_COMMAND_ALIGN_LEFT,
	GTK_HTML_COMMAND_ALIGN_CENTER,
	GTK_HTML_COMMAND_ALIGN_RIGHT,

	GTK_HTML_COMMAND_INDENT_INC,
	GTK_HTML_COMMAND_INDENT_DEC,

	GTK_HTML_COMMAND_PARAGRAPH_STYLE_NORMAL,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H1,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H2,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H3,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H4,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H5,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_H6,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_ADDRESS,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_PRE,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDOTTED,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMROMAN,
	GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDIGIT,

	GTK_HTML_COMMAND_MODIFY_SELECTION_UP,
	GTK_HTML_COMMAND_MODIFY_SELECTION_DOWN,
	GTK_HTML_COMMAND_MODIFY_SELECTION_LEFT,
	GTK_HTML_COMMAND_MODIFY_SELECTION_RIGHT,
	GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEUP,
	GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEDOWN,
	GTK_HTML_COMMAND_MODIFY_SELECTION_BOL,
	GTK_HTML_COMMAND_MODIFY_SELECTION_EOL,
	GTK_HTML_COMMAND_MODIFY_SELECTION_BOD,
	GTK_HTML_COMMAND_MODIFY_SELECTION_EOD,
};
typedef enum _GtkHTMLCommandType GtkHTMLCommandType;


#include "gtkhtml-properties.h"
#include "gtkhtml-embedded.h"

/* FIXME we should hide this stuff.  */
#include "htmlengine.h"
#include "htmlengine-save.h"
#include "gtkhtmlfontstyle.h"

typedef HTMLEngineSaveReceiverFn GtkHTMLSaveReceiverFn;

struct _GtkHTML {
	GtkLayout layout;

	HTMLEngine *engine;

	/* The URL of the link over which the pointer currently is.  NULL if
           the pointer is not over a link.  */
	gchar *pointer_url;

	/* The cursors we use within the widget.  */
	GdkCursor *hand_cursor;
	GdkCursor *arrow_cursor;
	GdkCursor *ibeam_cursor;

	gint selection_x1, selection_y1;

	guint in_selection : 1;
	guint button_pressed : 1;
	guint load_in_progress : 1;

	guint debug : 1;
	guint allow_selection : 1;

	guint hadj_connection;
	guint vadj_connection;

	guint idle_handler_id;
	guint scroll_timeout_id;

	GtkHTMLParagraphStyle paragraph_style;
	guint paragraph_indentation;
	GtkHTMLParagraphAlignment paragraph_alignment;

	GtkHTMLFontStyle insertion_font_style;

	gboolean binding_handled;
};

/* must be forward referenced *sigh* */
struct _HTMLEmbedded;

struct _GtkHTMLClass {
	GtkLayoutClass parent_class;
	
        void (* title_changed)   (GtkHTML *html, const gchar *new_title);
        void (* url_requested)   (GtkHTML *html, const gchar *url, GtkHTMLStream *handle);
        void (* load_done)       (GtkHTML *html);
        void (* link_clicked)    (GtkHTML *html, const gchar *url);
	void (* set_base)        (GtkHTML *html, const gchar *base_url);
	void (* set_base_target) (GtkHTML *html, const gchar *base_url);

	void (* on_url)		 (GtkHTML *html, const gchar *url);
	void (* redirect)        (GtkHTML *html, const gchar *url, int delay);
	void (* submit)          (GtkHTML *html, const gchar *method, const gchar *url, const gchar *encoding);
	gboolean (* object_requested)(GtkHTML *html, GtkHTMLEmbedded *);

	void (* current_paragraph_style_changed) (GtkHTML *html, GtkHTMLParagraphStyle new_style);
	void (* current_paragraph_alignment_changed) (GtkHTML *html, GtkHTMLParagraphAlignment new_alignment);
	void (* current_paragraph_indentation_changed) (GtkHTML *html, guint new_indentation);
	void (* insertion_font_style_changed) (GtkHTML *html, GtkHTMLFontStyle style);
	void (* insertion_color_changed) (GtkHTML *html, GdkColor *color);

        void (* size_changed)       (GtkHTML *html);

	/* keybindings signals */
	void (* scroll)               (GtkHTML *html, GtkOrientation orientation, GtkScrollType scroll_type, gfloat position);
	void (* cursor_move)          (GtkHTML *html, GtkDirectionType dir_type, GtkHTMLCursorSkipType skip);
	void (* command)              (GtkHTML *html, GtkHTMLCommandType com_type);

	/* properties */
	GtkHTMLClassProperties *properties;
};



gboolean gtkhtmllib_init  (gint argc, gchar **argv);

/* Creation.  */
GtkType    gtk_html_get_type  (void);
GtkWidget *gtk_html_new       (void);
void       gtk_html_construct (GtkWidget *htmlw);

/* Debugging.  */
void  gtk_html_enable_debug  (GtkHTML  *html,
			      gboolean  debug);

/* Behavior.  */
void  gtk_html_allow_selection  (GtkHTML  *html,
				 gboolean  allow);
int   gtk_html_request_paste    (GtkWidget *widget,
				 gint32 time);
/* Loading.  */
GtkHTMLStream *gtk_html_begin       (GtkHTML             *html);
void           gtk_html_write       (GtkHTML             *html,
				     GtkHTMLStream       *handle,
				     const gchar         *buffer,
				     size_t               size);
void           gtk_html_end         (GtkHTML             *html,
				     GtkHTMLStream       *handle,
				     GtkHTMLStreamStatus  status);
void           gtk_html_load_empty  (GtkHTML             *html);

/* Saving.  */
gboolean  gtk_html_save  (GtkHTML               *html,
			  GtkHTMLSaveReceiverFn  receiver,
			  gpointer               data);

gboolean  gtk_html_export  (GtkHTML               *html,
			    const char            *type,
			    GtkHTMLSaveReceiverFn  receiver,
			    gpointer               data);

/* Streams for feeding the widget with extra data (e.g. images) at loading
   time.  */
GtkHTMLStream *gtk_html_stream_ref    (GtkHTMLStream *handle);
void           gtk_html_stream_unref  (GtkHTMLStream *handle);

/* Editable support.  */
void      gtk_html_set_editable  (GtkHTML       *html,
				  gboolean       editable);
gboolean  gtk_html_get_editable  (const GtkHTML *html);

/* Printing support.  */
void  gtk_html_print  (GtkHTML           *html,
		       GnomePrintContext *print_context);

/* Title.  */
const gchar *gtk_html_get_title  (GtkHTML *html);

/* Anchors.  */
gboolean  gtk_html_jump_to_anchor  (GtkHTML *html,
				    const gchar *anchor);


/* Editing functions.  */

GtkHTMLParagraphStyle	   gtk_html_get_paragraph_style          (GtkHTML                   *html);
void  			   gtk_html_set_paragraph_style          (GtkHTML                   *html,
								  GtkHTMLParagraphStyle      style);
void  			   gtk_html_indent                       (GtkHTML                   *html,
								  gint                       delta);
void  			   gtk_html_set_font_style               (GtkHTML                   *html,
								  GtkHTMLFontStyle           and_mask,
								  GtkHTMLFontStyle           or_mask);
GtkHTMLParagraphAlignment  gtk_html_get_paragraph_alignment      (GtkHTML                   *html);
void  			   gtk_html_set_paragraph_alignment      (GtkHTML                   *html,
								  GtkHTMLParagraphAlignment  alignment);

void  gtk_html_cut    (GtkHTML *html);
void  gtk_html_copy   (GtkHTML *html);
void  gtk_html_paste  (GtkHTML *html);

void  gtk_html_undo  (GtkHTML *html);
void  gtk_html_redo  (GtkHTML *html);

/* misc utils */

void  gtk_html_set_default_background_color (GtkHTML *html, GdkColor *c);

#endif /* _GTKHTML_H_ */
