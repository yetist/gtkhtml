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


GtkType		gtk_html_get_type	(void);
GtkWidget*	gtk_html_new		(void);
void            gtk_html_parse          (GtkHTML *html);
void            gtk_html_begin          (GtkHTML *html, gchar *url);
void            gtk_html_write          (GtkHTML *html, gchar *buffer);
void            gtk_html_end            (GtkHTML *html);

#endif /* _GTKHTML_H_ */
