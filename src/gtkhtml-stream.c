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

#include <config.h>

#include "gtkhtml-stream.h"


GtkHTMLStream *
gtk_html_stream_new (GtkHTML *html,
		     GtkHTMLStreamWriteFunc write_func,
		     GtkHTMLStreamCloseFunc close_func,
		     gpointer user_data)
{
	GtkHTMLStream *new_stream;
	
	new_stream = g_new (GtkHTMLStream, 1);

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

void
gtk_html_stream_write (GtkHTMLStream *stream,
		       const gchar *buffer,
		       size_t size)
{
	g_return_if_fail (stream != NULL);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (size > 0);
	
	if (stream->write_func != NULL)
		stream->write_func (stream, buffer, size, stream->user_data);
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
