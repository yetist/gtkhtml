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


typedef gpointer GtkHTMLStreamHandle;
typedef enum {
	GTK_HTML_STREAM_OK,
	GTK_HTML_STREAM_ERROR
} GtkHTMLStreamStatus;

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

	gint selection_x1, selection_y1;

	gboolean in_selection : 1;
	gboolean button_pressed : 1;
	gboolean editable : 1;
	gboolean load_in_progress : 1;

	gboolean debug : 1;
	gboolean allow_selection : 1;

	guint hadj_connection;
	guint vadj_connection;

	guint idle_handler_id;

	GtkHTMLParagraphStyle paragraph_style;
	guint paragraph_indentation;
	GtkHTMLParagraphAlignment paragraph_alignment;

	GtkHTMLFontStyle insertion_font_style;
};

/* must be forward referenced *sigh* */
struct _HTMLEmbedded;

struct _GtkHTMLClass {
	GtkLayoutClass parent_class;
	
        void (* title_changed)   (GtkHTML *html);
        void (* url_requested)   (GtkHTML *html, const char *url, GtkHTMLStreamHandle handle);
        void (* load_done)       (GtkHTML *html);
        void (* link_clicked)    (GtkHTML *html, const char *url);
	void (* set_base)        (GtkHTML *html, const char *base_url);
	void (* set_base_target) (GtkHTML *html, const char *base_url);

	void (* on_url)		 (GtkHTML *html, const gchar *url);
	void (* redirect)        (GtkHTML *html, const gchar *url, int delay);
	void (* submit)          (GtkHTML *html, const gchar *method, const gchar *url, const gchar *encoding);
	void (* object_requested)(GtkHTML *html, GtkHTMLEmbedded *);

	void (* current_paragraph_style_changed) (GtkHTML *html, GtkHTMLParagraphStyle new_style);
	void (* current_paragraph_alignment_changed) (GtkHTML *html, GtkHTMLParagraphAlignment new_alignment);
	void (* current_paragraph_indentation_changed) (GtkHTML *html, guint new_indentation);
	void (* insertion_font_style_changed) (GtkHTML *html, GtkHTMLFontStyle style);
};


/* Creation.  */
GtkType    gtk_html_get_type  (void);
GtkWidget *gtk_html_new       (void);

/* Debugging.  */
void  gtk_html_enable_debug  (GtkHTML  *html,
			      gboolean  debug);

/* Behavior.  */
void  gtk_html_allow_selection  (GtkHTML  *html,
				 gboolean  allow);

/* Loading.  */
GtkHTMLStreamHandle  gtk_html_begin  (GtkHTML             *html,
				      const char          *url);
void                 gtk_html_write  (GtkHTML             *html,
				      GtkHTMLStreamHandle  handle,
				      const char          *buffer,
				      size_t               size);
void                 gtk_html_end    (GtkHTML             *html,
				      GtkHTMLStreamHandle  handle,
				      GtkHTMLStreamStatus  status);

/* Saving.  */
gboolean  gtk_html_save  (GtkHTML               *html,
			  GtkHTMLSaveReceiverFn  receiver,
			  gpointer               data);

/* Streams for feeding the widget with extra data (e.g. images) at loading
   time.  */
GtkHTMLStreamHandle  gtk_html_stream_ref    (GtkHTMLStreamHandle handle);
void                 gtk_html_stream_unref  (GtkHTMLStreamHandle handle);

/* Editable support.  */
void      gtk_html_set_editable  (GtkHTML       *html,
				  gboolean       editable);
gboolean  gtk_html_get_editable  (const GtkHTML *html);

/* Printing support.  */
void  gtk_html_print  (GtkHTML           *html,
		       GnomePrintContext *print_context);

/* DEPRECATED.  We'll keep it around for a while just to prevent code from
   being broken.  */
void  gtk_html_parse  (GtkHTML *html);

/* FIXME?  Deprecated? */
void  gtk_html_calc_scrollbars  (GtkHTML *html);

void  gtk_html_load_empty  (GtkHTML *html);


/* Editing functions.  */

void  gtk_html_set_paragraph_style  (GtkHTML                   *html,
				     GtkHTMLParagraphStyle      style);
void  gtk_html_indent               (GtkHTML                   *html,
				     gint                       delta);
void  gtk_html_set_font_style       (GtkHTML                   *html,
				     GtkHTMLFontStyle           and_mask,
				     GtkHTMLFontStyle           or_mask);
void  gtk_html_align_paragraph      (GtkHTML                   *html,
				     GtkHTMLParagraphAlignment  alignment);

void  gtk_html_cut    (GtkHTML *html);
void  gtk_html_copy   (GtkHTML *html);
void  gtk_html_paste  (GtkHTML *html);

void  gtk_html_undo  (GtkHTML *html);
void  gtk_html_redo  (GtkHTML *html);

#endif /* _GTKHTML_H_ */
