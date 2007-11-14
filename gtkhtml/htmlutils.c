/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 2007 Novell, Inc.

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <glib.h>
#include <string.h>

#include "htmlutils.h"

/**
 * html_utils_maybe_escape_amp
 * When find '&' in url which is not part of "&amp;", then will escape it
 * to "&amp;". Always returns new pointer on url.
 * @param url URL to try to escape ampersands in.
 * @return New allocated memory with escaped ampersands. Should be freed with g_free.
 **/
gchar *
html_utils_maybe_escape_amp (const gchar *url)
{
	gchar *buff;
	int amps, i, j;

	if (!url)
		return NULL;

	amps = 0;
	for (i = 0; url [i]; i++) {
		if (url [i] == '&' && strncmp (url + i, "&amp;", 5) )
			amps++;
	}

	if (!amps)
		return g_strdup (url);

	buff = g_malloc (sizeof (gchar) * (1 + i + 4 * amps));

	for (i = 0, j = 0; url [i]; i++, j++) {
		buff [j] = url [i];

		if (url [i] == '&' && strncmp (url + i, "&amp;", 5) ) {
			buff [j + 1] = 0;
			strcat (buff + j, "amp;");
			j += 4;
		}
	}

	buff [j] = 0;

	return buff;
}

/**
 * html_utils_maybe_unescape_amp
 * This will unescape "&amp;" to "&" if necessary. Always returns new pointer on url, so free it with g_free.
 * @param url URL where to unescape "&amp;" sequences.
 * @return Newly allocated memory with unescaped ampersands. Should be freed with g_free.
 **/
gchar *
html_utils_maybe_unescape_amp (const gchar *url)
{
	gchar *buff;
	int i, j, amps;

	if (!url)
		return NULL;

	amps = 0;
	for (i = 0; url [i]; i++) {
		if (url [i] == '&' && strncmp (url + i, "&amp;", 5) == 0)
			amps++;
	}

	buff = g_strdup (url);

	if (!amps)
		return buff;

	for (i = 0, j = 0; url [i]; i++, j++) {
		buff [j] = url [i];

		if (url [i] == '&' && strncmp (url + i, "&amp;", 5) == 0)
			i += 4;
	}
	buff [j] = 0;

	return buff;
}
