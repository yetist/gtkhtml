/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#include <string.h>
#include <glib.h>
#include "htmltokenizer.h"
#include "stringtokenizer.h"

enum quoteEnum { NO_QUOTE=0, SINGLE_QUOTE, DOUBLE_QUOTE };

StringTokenizer *
string_tokenizer_new (void)
{
	StringTokenizer *s;
	
	s = g_new0 (StringTokenizer, 1);

	return s;
}

gboolean
string_tokenizer_has_more_tokens (StringTokenizer *t)
{
	return (t->pos != 0);
}

gchar *
string_tokenizer_next_token (StringTokenizer *t)
{
	gchar *ret;

	if (t->pos == 0)
		return 0;
	
	ret = t->pos;
	t->pos += strlen (ret) + 1;
	if (t->pos >= t->end)
		t->pos = 0;
	
	return ret;
}

void
string_tokenizer_tokenize (StringTokenizer *t, const gchar *str, gchar *separators)
{
	gint strLength, quoted;
	const gchar *src, *x;

	if (*str == '\0') {
		t->pos = 0;
		return;
	}
	
	strLength = strlen (str) + 1;
	
	if (t->bufLen < strLength) {
		g_free (t->buffer);
		t->buffer = g_malloc (strLength);
		t->bufLen = strLength;
	}

	src = str;
	t->end = t->buffer;

	quoted = NO_QUOTE;
	
	for (; *src != '\0'; src++) {
		x = strchr (separators, *src);
		if ((*src == '\"') && !quoted)
			quoted = DOUBLE_QUOTE;
		else if ((*src == '\'') && !quoted)
			quoted = SINGLE_QUOTE;
		else if (((*src == '\"') && (quoted == DOUBLE_QUOTE)) ||
			 ((*src == '\'') && (quoted == SINGLE_QUOTE)))
			quoted = NO_QUOTE;
		else if (x && !quoted)
			*(t->end)++ = 0;
		else
			*(t->end)++ = *src;
	}

	*(t->end) = 0;
	
	if (t->end - t->buffer <= 1)
		t->pos = 0; /* No tokens */
	else
		t->pos = t->buffer;
}
