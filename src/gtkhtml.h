/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
    Copyright 1999, Helix Code, Inc.
    
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

#define GTK_TYPE_HTML                  (gtk_html_get_type ())
#define GTK_HTML(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_HTML, GtkHTML))
#define GTK_HTML_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_HTML, GtkHTMLClass))
#define GTK_IS_HTML(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_HTML))
#define GTK_IS_HTML_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HTML))

typedef gpointer GtkHTMLStreamHandle;
typedef enum { GTK_HTML_STREAM_OK, GTK_HTML_STREAM_ERROR } GtkHTMLStreamStatus;

#include "htmlengine.h"

struct _GtkHTML {
	GtkLayout layout;

	HTMLEngine *engine;

	/* The URL of the link over which the pointer currently is.  NULL if
           the pointer is not over a link.  */
	gchar *pointer_url;

	/* The cursors we use within the widget.  */
	GdkCursor *hand_cursor;
	GdkCursor *arrow_cursor;
};

struct _GtkHTMLClass {
	GtkLayoutClass parent_class;
	
        void (* title_changed)   (GtkHTML *html);
        void (* url_requested)   (GtkHTML *html, const char *url, GtkHTMLStreamHandle handle);
        void (* load_done)       (GtkHTML *html);
        void (* link_followed)   (GtkHTML *html, const char *url);
	void (* set_base)        (GtkHTML *html, const char *base_url);
	void (* set_base_target) (GtkHTML *html, const char *base_url);

	void (* on_url)		 (GtkHTML *html, const gchar *url);
};

GtkType		gtk_html_get_type	 (void);
GtkWidget*	gtk_html_new		 (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);
void            gtk_html_parse           (GtkHTML *html);
GtkHTMLStreamHandle gtk_html_begin       (GtkHTML *html, const char *url);
void            gtk_html_write           (GtkHTML *html, GtkHTMLStreamHandle handle, const char *buffer, size_t size);
void            gtk_html_end             (GtkHTML *html, GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status);
void            gtk_html_calc_scrollbars (GtkHTML *html);
void	        gtk_html_set_base_url    (GtkHTML *html, const char *url);

#endif /* _GTKHTML_H_ */
