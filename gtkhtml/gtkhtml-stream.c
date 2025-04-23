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

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "gtkhtml-compat.h"
#include "gtkhtml-stream.h"

/**
 * SECTION: gtkhtml-stream
 * @Short_description: data stream for loading HTML contents into GtkHTML widget.
 * @Title: GtkHTML stream
 *
 * GtkHTML stream
 */

GtkHTMLStream *
gtk_html_stream_new (GtkHTML *html,
                     GtkHTMLStreamTypesFunc types_func,
                     GtkHTMLStreamWriteFunc write_func,
                     GtkHTMLStreamCloseFunc close_func,
                     gpointer user_data)
{
	GtkHTMLStream *new_stream;

	new_stream = g_new (GtkHTMLStream, 1);

	new_stream->types_func = types_func;
	new_stream->write_func = write_func;
	new_stream->close_func = close_func;
	new_stream->user_data = user_data;

	return new_stream;
}

void
gtk_html_stream_destroy (GtkHTMLStream *stream)
{
	g_return_if_fail (stream != NULL);

	g_free (stream);
}

/**
 * gtk_html_stream_write:
 * @stream:
 * @buffer:
 * @size:
 *
 * Write data to a GtkHTMLStream.
 *
 */
void
gtk_html_stream_write (GtkHTMLStream *stream,
                       const gchar *buffer,
                       gsize size)
{
	g_return_if_fail (stream != NULL);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (size > 0);

	if (stream->write_func != NULL)
		stream->write_func (stream, buffer, size, stream->user_data);
}

gint
gtk_html_stream_vprintf (GtkHTMLStream *stream,
                         const gchar *format,
                         va_list ap)
{
	gsize len;
	gchar *buf = NULL;
	gchar *mbuf = NULL;
	gchar *result_string = NULL;
	gint rv;
	va_list ap_copy;

	G_VA_COPY (ap_copy, ap);

	result_string = g_strdup_vprintf (format, ap_copy);
	g_return_val_if_fail (result_string != NULL, 0);

	len = strlen (result_string) + 1;
	g_free (result_string);

	if (len < 8192)
		buf = alloca (len);

	if (buf == NULL)
		buf = mbuf = g_malloc (len);

	rv = vsprintf (buf, format, ap);
	gtk_html_stream_write (stream, buf, rv);

	g_free (mbuf);
	return rv;
}

gint
gtk_html_stream_printf (GtkHTMLStream *stream,
                        const gchar *format,
                        ...)
{
	va_list ap;
	gint rv;

	va_start (ap, format);
	rv = gtk_html_stream_vprintf (stream, format, ap);
	va_end (ap);

	return rv;
}

void
gtk_html_stream_close (GtkHTMLStream *stream,
                       GtkHTMLStreamStatus status)
{
	g_return_if_fail (stream != NULL);

	if (stream->close_func != NULL)
		stream->close_func (stream, status, stream->user_data);

	gtk_html_stream_destroy (stream);
}

gchar **
gtk_html_stream_get_types (GtkHTMLStream *stream)
{
	if (stream->types_func != NULL)
		return stream->types_func (stream, stream->user_data);

	return NULL;
}

typedef struct _GtkHTMLLog GtkHTMLLog;
struct _GtkHTMLLog {
	GtkHTMLStream *stream;
	FILE *file;
};

static gchar **
stream_log_types (GtkHTMLStream *stream,
                  gpointer user_data)
{
	GtkHTMLLog *log = user_data;

	return gtk_html_stream_get_types (log->stream);
}

static void
stream_log_write (GtkHTMLStream *stream,
               const gchar *buffer,
               gsize size,
               gpointer user_data)
{
	GtkHTMLLog *log = user_data;
	gint i;

	for (i = 0; i < size; i++)
		fprintf (log->file, "%c", buffer[i]);

	gtk_html_stream_write (log->stream, buffer, size);

}

static void
stream_log_close (GtkHTMLStream *stream,
               GtkHTMLStreamStatus status,
               gpointer user_data)
{
	GtkHTMLLog *log = user_data;

	fclose (log->file);
	gtk_html_stream_close (log->stream, status);

	g_free (log);
}

GtkHTMLStream *
gtk_html_stream_log_new (GtkHTML *html,
                         GtkHTMLStream *stream)
{
	GtkHTMLLog *log;
	GtkHTMLStream *new_stream;
	gchar *fname;
	static gint log_num = 0;

	log = g_new (GtkHTMLLog, 1);
	log->stream = stream;

	fname = g_strdup_printf ("gtkhtml.log.%d.html", log_num);
	log->file = fopen (fname, "w+");
	g_free (fname);

	log_num++;

	new_stream = gtk_html_stream_new (html,
					  stream_log_types,
					  stream_log_write,
					  stream_log_close,
					  log);

	return new_stream;
}

