/*
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

#include <gtk/gtk.h>
#include "htmlengine.h"

#define GTK_TYPE_HTML                  (gtk_html_get_type ())
#define GTK_HTML(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_HTML, GtkHTML))
#define GTK_HTML_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_HTML, GtkHTMLClass))
#define GTK_IS_HTML(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_HTML))
#define GTK_IS_HTML_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HTML))


struct _GtkHTML
{
	GtkLayout layout;

	GtkWidget *vscrollbar;
	GtkObject *vsbadjust;

	gboolean displayVScroll;

	HTMLEngine *engine;
  
};

struct _GtkHTMLClass
{
	GtkLayoutClass parent_class;
	
	void (*title_changed) (GtkHTML *html);
};

/* FIXME: can these be moved elsewhere? */
enum {
	TITLE_CHANGED,
	LAST_SIGNAL
};


GtkType		gtk_html_get_type	 (void);
GtkWidget*	gtk_html_new		 (void);
void            gtk_html_parse           (GtkHTML *html);
void            gtk_html_begin           (GtkHTML *html, gchar *url);
void            gtk_html_write           (GtkHTML *html, gchar *buffer);
void            gtk_html_end             (GtkHTML *html);
void            gtk_html_calc_scrollbars (GtkHTML *html);

#endif /* _GTKHTML_H_ */
