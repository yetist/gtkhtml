/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

/* This file implements the Bonobo::PersistStream interface for the GtkHTML
   component.  */

#include <config.h>

#include <gnome.h>
#include <bonobo.h>

#include "gtkhtml.h"

#include "persist-stream-impl.h"


/* Loading.  */

#define READ_CHUNK_SIZE 4096

static gint
load (BonoboPersistStream *ps,
      Bonobo_Stream stream,
      gpointer data)
{
	GtkHTML *html;
	CORBA_Environment ev;
	CORBA_long bytes_read;
	Bonobo_Stream_iobuf *buffer;
	GtkHTMLStreamHandle handle;

	CORBA_exception_init (&ev);

	html = GTK_HTML (data);

	/* FIXME the GtkHTML API is broken.  */
	handle = gtk_html_begin (html, "");

	do {
		bytes_read = Bonobo_Stream_read (stream, READ_CHUNK_SIZE, &buffer, &ev);
		if (ev._major != CORBA_NO_EXCEPTION) {
			bytes_read = -1;
			break;
		}

		gtk_html_write (html, handle, buffer->_buffer, buffer->_length);

		CORBA_free (buffer);
	} while (bytes_read > 0);

	if (bytes_read < 0)
		gtk_html_end (html, handle, GTK_HTML_STREAM_ERROR);
	else
		gtk_html_end (html, handle, GTK_HTML_STREAM_OK);

	CORBA_exception_free (&ev);

	if (bytes_read < 0)
		return -1;

	return 0;
}


/* Saving.  */

struct _SaveState {
	Bonobo_Stream stream;
	CORBA_Environment ev;
};
typedef struct _SaveState SaveState;

static gboolean
save_receiver (const HTMLEngine *engine,
	       const gchar *data,
	       guint length,
	       gpointer user_data)
{
	Bonobo_Stream_iobuf buffer;
	CORBA_long bytes_written;
	SaveState *state;

	state = (SaveState *) user_data;
	if (state->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;

	buffer._maximum = length;
	buffer._length = length;
	buffer._buffer = (CORBA_char *) data; /* Should be safe.  */

	bytes_written = Bonobo_Stream_write (state->stream, &buffer, &state->ev);

	if (bytes_written != length || state->ev._major != CORBA_NO_EXCEPTION)
		return FALSE;

	return TRUE;
}

static gint
save (BonoboPersistStream *ps,
      Bonobo_Stream stream,
      gpointer data)
{
	GtkHTML *html;
	SaveState save_state;
	gboolean success;

	html = GTK_HTML (data);

	CORBA_exception_init (&save_state.ev);
	save_state.stream = CORBA_Object_duplicate (stream, &save_state.ev);
	if (save_state.ev._major != CORBA_NO_EXCEPTION) {
		CORBA_exception_free (&save_state.ev);
		return -1;
	}

	success = gtk_html_save (html, save_receiver, &save_state);
	if (save_state.ev._major != CORBA_NO_EXCEPTION)
		success = FALSE;

	CORBA_Object_release (save_state.stream, &save_state.ev);
	CORBA_exception_free (&save_state.ev);

	if (success)
		return 0;
	else
		return -1;
}


BonoboPersistStream *
persist_stream_impl_new (GtkHTML *html)
{
	return bonobo_persist_stream_new (load, save, html);
}
