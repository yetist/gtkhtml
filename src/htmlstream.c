#include "gtkhtml-private.h"

GtkHTMLStreamHandle
gtk_html_stream_new(GtkHTML *html, const char *url,
		    GtkHTMLStreamWriteFunc write_callback,
		    GtkHTMLStreamEndFunc end_callback,
		    gpointer user_data)
{
  GtkHTMLStream *new_stream;

  new_stream = g_new(GtkHTMLStream, 1);
  new_stream->write_callback = write_callback;
  new_stream->end_callback = end_callback;
  new_stream->user_data = user_data;

  gtk_signal_emit(GTK_OBJECT(html), html_signals[URL_REQUESTED], url, new_stream);

  return new_stream;
}

void
gtk_html_stream_write(GtkHTMLStreamHandle handle,
		      const gchar *buffer,
		      size_t size)
{
  GtkHTMLStream *stream = handle;

  if(stream->write_callback)
    stream->write_callback(handle, buffer, size, stream->user_data);
}

void
gtk_html_stream_end(GtkHTMLStreamHandle handle,
		    GtkHTMLStreamStatus status)
{
  GtkHTMLStream *stream = handle;

  if(stream->end_callback)
    stream->end_callback(handle, status, stream->user_data);

  g_free(stream);
}
