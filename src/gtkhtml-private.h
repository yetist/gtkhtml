#ifndef GTKHTML_PRIVATE_H
#define GTKHTML_PRIVATE_H 1

#include "gtkhtml.h"

typedef void (*GtkHTMLStreamEndFunc)(GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, gpointer user_data);
typedef void (*GtkHTMLStreamWriteFunc)(GtkHTMLStreamHandle handle, const guchar *buffer, size_t size, gpointer user_data);

typedef struct {
  GtkHTMLStreamWriteFunc write_callback;
  GtkHTMLStreamEndFunc end_callback;
  gpointer user_data;
} GtkHTMLStream;

GtkHTMLStreamHandle gtk_html_stream_new(GtkHTML *html, const char *url,
					GtkHTMLStreamWriteFunc write_callback,
					GtkHTMLStreamEndFunc end_callback,
					gpointer user_data);
void gtk_html_stream_write(GtkHTMLStreamHandle handle,
			   const gchar *buffer,
			   size_t size);
void gtk_html_stream_end(GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status);

enum {
	TITLE_CHANGED,
	URL_REQUESTED,
	LOAD_DONE,
	LINK_FOLLOWED,
	LAST_SIGNAL
};
extern guint html_signals [LAST_SIGNAL];

#endif
