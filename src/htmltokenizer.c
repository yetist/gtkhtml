/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
	      (C) 1999 Anders Carlsson (andersca@gnu.org)

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

/* The HTML Tokenizer */
#include <ctype.h>
#include <gnome.h>
#include "htmltokenizer.h"
#include "htmlentity.h"

#define TOKEN_BUFFER_SIZE (32*1024) - 1

static const gchar *commentStart = "<!--";
static const gchar *scriptEnd = "</script>";
static const gchar *styleEnd = "</style>";

enum quoteEnum {
	NO_QUOTE = 0,
	SINGLE_QUOTE,
	DOUBLE_QUOTE
};

HTMLTokenizer *
html_tokenizer_new (void)
{
	HTMLTokenizer *t;
	
	t = g_new0 (HTMLTokenizer, 1);

	return t;
}

void
html_tokenizer_destroy (HTMLTokenizer *tokenizer)
{
	g_return_if_fail (tokenizer != NULL);

	html_tokenizer_reset (tokenizer);
	g_free (tokenizer);
}

gchar *
html_tokenizer_next_token (HTMLTokenizer *t)
{
	gchar *buf;

	if (!t->curr)
		return NULL;
	
	buf = t->curr;
	t->curr += strlen (t->curr) + 1;
	
	if ((t->curr != t->next) && (*(t->curr) == '\0')) {
		html_tokenizer_next_token_buffer (t);
	}
	
	return buf;
	
}

void
html_tokenizer_next_token_buffer (HTMLTokenizer *t)
{
	t->tokenBufferCurrIndex++;
	
	g_print ("next token buffer\n");

	if (t->tokenBufferCurrIndex < g_list_length (t->tokenBufferList)) {
		t->curr = ((GList *)g_list_nth (t->tokenBufferList,
						g_list_length (t->tokenBufferList) - t->tokenBufferCurrIndex - 1))->data;
		
	}
	else
		g_error ("Error in html_tokenizer_next_token_buffer");
}

gboolean
html_tokenizer_has_more_tokens (HTMLTokenizer *t)
{
	if (!html_blocking_token_is_empty (t) &&
	    (html_blocking_token_get_first (t))->tok == t->curr)
		return FALSE;

	return ((t->curr != 0 ) && (t->curr != t->next));
}

void
html_tokenizer_reset (HTMLTokenizer *t)
{
	GList *buffers;
	
	/*
	 * Free buffers here
	 */
	for (buffers = t->tokenBufferList; buffers; buffers = buffers->next)
		g_free (buffers->data);
	g_list_free (t->tokenBufferList);
	t->tokenBufferList = NULL;	


	t->last = t->next = t->curr = 0;
	t->tokenBufferSizeRemaining = 0;

	if (t->buffer)
		g_free (t->buffer);
	t->buffer = 0;

	if (t->scriptCode)
		g_free (t->scriptCode);
	t->scriptCode = 0;
}

void
html_tokenizer_begin (HTMLTokenizer *t)
{
	html_tokenizer_reset (t);

	t->buffer = g_malloc (1024);
	t->dest = t->buffer;
	t->size = 1000;

	t->dest = t->buffer;
	t->tag = FALSE;
	t->pending = NonePending;
	t->discard = NoneDiscard;
	t->pre = FALSE;
	t->prePos = 0;
	t->script = FALSE;
	t->style = FALSE;
	t->skipLF = FALSE;
	t->select = FALSE;
	t->comment = FALSE;
	t->textarea = FALSE;
	t->startTag = FALSE;
	t->tquote = NO_QUOTE;
	t->searchCount = 0;
	t->title = FALSE;
	t->charEntity = FALSE;

}

void
html_tokenizer_end (HTMLTokenizer *t)
{
	if (t->buffer == 0)
		return;

	if (t->dest > t->buffer) {
		*(t->dest) = 0;
		html_tokenizer_append_token (t, t->buffer, t->dest - t->buffer);
	}

	g_free (t->buffer);	

	t->buffer = 0;

	/* FIXME: Delete blocking tokens */
	t->blocking = NULL;
}

void
html_tokenizer_append_token (HTMLTokenizer *t, const gchar *string, gint len)
{
	if (len < 1)
		return;

	if (len >= t->tokenBufferSizeRemaining) {
		/* Create a new buffer */
		g_print ("Appending token buffer\n");
		html_tokenizer_append_token_buffer (t, len);
	}

	t->last = t->next;
	t->tokenBufferSizeRemaining -= len + 1; /* One for null-termination */

	while (len--) {
		*(t->next)++ = *string++;
	}
	*(t->next)++ = '\0';
		
}

void
html_tokenizer_append_token_buffer (HTMLTokenizer *t, gint min_size)
{
	gint newBufSize = TOKEN_BUFFER_SIZE;
	gchar *newBuffer;

	if (t->next) {
		*(t->next) = '\0';
	}
	
	if (min_size > newBufSize) {
		newBufSize += min_size;
	}
	newBuffer = g_malloc (newBufSize + 1);
	
	g_print ("newBuffer: %p\n", newBuffer);

	t->tokenBufferList = g_list_prepend (t->tokenBufferList, newBuffer);
	t->next = newBuffer;
	t->tokenBufferSizeRemaining = newBufSize;

	if (!t->curr) {
		t->curr = ((GList *)g_list_last (t->tokenBufferList))->data;
		t->tokenBufferCurrIndex = 0;
	}

}

/* EP CHECK: OK.  */
void
html_tokenizer_add_pending (HTMLTokenizer *t)
{
	if (t->tag || t->select) {
		*(t->dest)++ = ' ';
	}
	else if (t->textarea) {
		if (t->pending == LFPending) 
			*(t->dest)++ = '\n';
		else
			*(t->dest)++ = ' ';
	}
	else if (t->pre) {
		gint p, x;
		
		switch (t->pending) {
		case SpacePending:
			/* Insert a non breaking space */
			*(t->dest)++ = 0xa0;
			t->prePos++;
			break;
		case LFPending:;
			if (t->dest > t->buffer) {
				*(t->dest) = 0;
				html_tokenizer_append_token (t, t->buffer, t->dest - t->buffer);
			}
			t->dest = t->buffer;
			*(t->dest) = TAG_ESCAPE;
			*((t->dest) + 1) = '\n';
			*((t->dest) + 2) = 0;
			html_tokenizer_append_token (t, t->buffer, 2);
			t->dest = t->buffer;
			t->prePos = 0;
			break;
		case TabPending:
			p = TAB_SIZE - (t->prePos % TAB_SIZE);
			for (x = 0; x < p; x++) {
				*(t->dest) = ' ';
				t->dest++;
			}
			t->prePos += p;
			break;
		default:
			g_warning ("Unknown pending type: %d\n", (gint) t->pending);
			break;
		}
	}
	else {
		*(t->dest)++ = ' ';
	}
	
	t->pending = NonePending;
}

void
html_tokenizer_write (HTMLTokenizer *t, const gchar *string, size_t size)
{
	const gchar *src = string;
	gchar *p, *ptr;
	gchar *srcPtr = 0;

	while ((src - string) < size) {
		/* Check if the buffer is too big */
		if ((t->dest - t->buffer) > t->size) {
			gchar *newbuf = g_malloc (t->size + 1024 + 20);
			memcpy (newbuf, t->buffer, 
				t->dest - t->buffer + 1);
			t->dest = newbuf + (t->dest - t->buffer);
			g_free (t->buffer);
			t->buffer = newbuf;
			t->size += 1024;
		}
		
		if (t->skipLF && (*src != '\n')) {
			t->skipLF = FALSE;
		}

		if (t->skipLF) {
			src++;
		}
		else if (t->comment) {
			/* Look for "-->" */
			if (*src == '-') {
				if (t->searchCount < 2)
					t->searchCount++;
					
			}
			else if (t->searchCount == 2 && (*src == '>')) {
				/* We've got a "-->" sequence */
				t->comment = FALSE;
			}
			else {
				t->searchCount = 0;
			}
			src++;
		}

		/* We are inside a <script> or a <style> tag. Look for ending
		   tag which is </script> or </style> */
		else if (t->script || t->style) {
			/* Allocate memory to store the script or style */
			if (t->scriptCodeSize + 11 > t->scriptCodeMaxSize) {
				gchar *newbuf = g_malloc (t->scriptCodeSize + 1024);
				memcpy (newbuf, t->scriptCode, t->scriptCodeSize);
				g_free (t->scriptCode);
				t->scriptCode = newbuf;
				t->scriptCodeMaxSize += 1024;
			}
			
			if ((*src == '>' ) && ( t->searchFor [t->searchCount] == '>')) {
				src++;
				t->scriptCode [t->scriptCodeSize] = 0;
				t->scriptCode [t->scriptCodeSize + 1] = 0;
				if (t->script) {
					t->script = FALSE;
				}
				else {
					t->style = FALSE;
				}
				g_free (t->scriptCode);
				t->scriptCode = NULL;
			}
			/* Check if a </script> tag is on its way */
			else if (t->searchCount > 0) {
				if (tolower (*src) == t->searchFor [t->searchCount]) {
					t->searchBuffer [t->searchCount] = *src;
					t->searchCount++;
					src++;
				}
				else {
					t->searchBuffer [t->searchCount] = 0;
					p = t->searchBuffer;
					while (*p)
						t->scriptCode [t->scriptCodeSize++] = *p++;
					t->scriptCode [t->scriptCodeSize] = *src++;
					t->searchCount = 0;
				}
			}
			else if (*src == '<') {
				t->searchCount = 1;
				t->searchBuffer [0] = '<';
				src++;
			}
			else
				t->scriptCode [t->scriptCodeSize] = *src++;
		}
		else if (t->charEntity) {
			gulong entityValue = 0;

			/* See http://www.mozilla.org/newlayout/testcases/layout/entities.html for a complete entity list,
			   ftp://ftp.unicode.org/Public/MAPPINGS/ISO8859/8859-1.TXT
			   (or 'man iso_8859_1') for the character encodings. */


			t->searchBuffer [t->searchCount + 1] = *src;
			t->searchBuffer [t->searchCount + 2] = '\0';
			
			/* Check for &#0000 sequence */
			if (t->searchBuffer[2] == '#') {
				if ((t->searchCount > 1) &&
				    (!isdigit (*src)) &&
				    (t->searchBuffer[3] != 'x')) {
					/* &#123 */
					t->searchBuffer [t->searchCount + 1] = '\0';
					entityValue = strtoul (&(t->searchBuffer [3]),
							       NULL, 10);
					t->charEntity = FALSE;
				}
				if ((t->searchCount > 1) &&
				    (!isalnum (*src)) && 
				    (t->searchBuffer[3] == 'x')) {
					/* &x12AB */
					t->searchBuffer [t->searchCount + 1] = '\0';
					entityValue = strtoul (&(t->searchBuffer [4]),
							       NULL, 16);
					t->charEntity = FALSE;
				}
			}
			else {
				/* Check for &abc12 sequence */
				if (!isalnum (*src)) {
					t->charEntity = FALSE;
					if ((t->searchBuffer [t->searchCount + 1] == ';') ||
					    (!t->tag)) {
						char *ename = t->searchBuffer + 2;
						
						t->searchBuffer [t->searchCount + 1] = '\0'; /* FIXME sucks */
						entityValue = html_entity_parse (ename, 0);
					}
				}
				
			}
			
			switch (entityValue) {
			case 139:
				entityValue = 60;
				break;
			case 145:
				entityValue = 96;
				break;
			case 146:
				entityValue = 39;
				break;

			case 147:
				//strcpy (t->searchBuffer+2, "ldquo");
				//t->searchCount = 6;
				//break;
			case 148:
				//strcpy(t->searchBuffer+2, "rdquo");
				//searchCount = 6;
				entityValue = 34;
				break;
			case 150:
				//strcpy(t->searchBuffer+2, "ndash");
				//t->searchCount = 6;
				//break;
			case 151:
				//strcpy (t->searchBuffer+2, "mdash");
				//t->searchCount = 6;
				entityValue = 45;
				break;
			case 152:
				entityValue = 126;
				break;
			case 155:
				entityValue = 62;
				break;
			case 133:
				strcpy (t->searchBuffer+2, "hellip");
				t->searchCount = 7;
				break;
			case 149:
				strcpy (t->searchBuffer+2, "bull");
				t->searchCount = 5;
				break;
			case 153:
				strcpy (t->searchBuffer+2, "trade");
				t->searchCount = 6;
				break;
			default:;
			}
			
			if (t->searchCount > 8) {
				/* Ignore this sequence since it's too long */
				t->charEntity = FALSE;
				memcpy (t->dest, t->searchBuffer + 1, t->searchCount);
				t->dest += t->searchCount;
				
				if (t->pre)
					t->prePos += t->searchCount;
			}
			else if (t->charEntity) {
				/* Keep searching for end of character entity */
				t->searchCount++;
				src++;
			}
			else {
				if(entityValue) {
					/* Insert plain ASCII */
					*(t->dest)++ = (gchar) entityValue;
					if (t->pre)
						t->prePos++;
					if (*src == ';')
						src++;
				}
				/* FIXME: Get entities */
				else if (!entityValue) {
					/* Ignore the sequence, just add it as plaintext */
					memcpy (t->dest, t->searchBuffer + 1, t->searchCount);
					t->dest += t->searchCount;
					if (t->pre)
						t->prePos += t->searchCount;

				}
#if 0
				else if (!t->tag && !t->textarea && !t->select && !t->title) {
					/* Add current token first */
					if (t->dest > t->buffer) {
						*(t->dest) = 0;
						html_tokenizer_append_token (t, t->buffer,
									     t->dest - t->buffer);
						t->dest = t->buffer;
					}

					/* Add token with the amp sequence for further conversion */
					html_tokenizer_append_token (t, t->searchBuffer, t->searchCount + 1);
					t->dest = t->buffer;
					if (t->pre)
						t->prePos++;
					if (*src == ';')
						src++;
				}
#endif
				t->searchCount = 0;
			}
		}
		else if (t->startTag) {
			t->startTag = FALSE;
			if (*src == '/') {
				if (t->pending == LFPending) {
					t->pending = NonePending;
				}
			}
			else if (((*src >= 'a') && (*src <= 'z'))
							 || ((*src >= 'A') && (*src <= 'Z'))) {
				/* Start of a start tag */
			}
			else if (*src == '!') {
				/* <!-- comment --> */
			}
			else if (*src == '?') {
				/* <? meta ?> */
			}
			else {
				/* Invalid tag, just add it */
				if (t->pending)
					html_tokenizer_add_pending (t);
				*(t->dest) = '<';
				t->dest++;
				*(t->dest)++ = *src++;
				continue;
			}
			
			if (t->pending)
				html_tokenizer_add_pending (t);

			if (t->dest > t->buffer) {
				*(t->dest) = 0;
				html_tokenizer_append_token (t, t->buffer, t->dest - t->buffer);
				t->dest = t->buffer;
			}
			*(t->dest) = TAG_ESCAPE;
			t->dest++;
			*(t->dest) = '<';
			t->dest++;
			t->tag = TRUE;
			t->searchCount = 1; /* Look for <!-- to start comment */
		}
		else if (*src == '&') {
			src++;
			
			t->discard = NoneDiscard;
			
			if (t->pending)
				html_tokenizer_add_pending (t);
			t->charEntity = TRUE;
			t->searchBuffer[0] = TAG_ESCAPE;
			t->searchBuffer[1] = '&';
			t->searchCount = 1;
		}
		else if (*src == '<' && !t->tag) {
			src++;
			t->startTag = TRUE;
			t->discard = NoneDiscard;
			
		}
		else if (*src == '>' && t->tag && !t->tquote) {

			t->searchCount = 0; /* Stop looking for <!-- sequence */
			
			*(t->dest) = '>';
			*(t->dest+1) = 0;
			
			/* Make the tag lower case */
			ptr = t->buffer + 2;
			if (*ptr == '/') {
				/* End tag */
				t->discard = NoneDiscard;
			}
			else {
				/* Start tag */
				/* Ignore CRLFs after a start tag */
				t->discard = LFDiscard;
			}
			while (*ptr && *ptr !=' ') {
				*ptr = tolower (*ptr);
				ptr++;
			}
			html_tokenizer_append_token (t, t->buffer, t->dest - t->buffer + 1);
			t->dest = t->buffer;
			
			t->tag = FALSE;
			t->pending = NonePending;
			src++;
			
			if (strncmp (t->buffer + 2, "pre", 3) == 0) {
				t->prePos = 0;
				t->pre = TRUE;
			}
			else if (strncmp (t->buffer + 2, "/pre", 4) == 0) {
				t->pre = FALSE;
			}
			else if (strncmp (t->buffer + 2, "textarea", 8) == 0) {
				t->textarea = TRUE;
			}
			else if (strncmp (t->buffer + 2, "/textarea", 9) == 0) {
				t->textarea = FALSE;
			}
			else if (strncmp (t->buffer + 2, "title", 5) == 0) {
				t->title = TRUE;
			}
			else if (strncmp (t->buffer + 2, "/title", 6) == 0) {
				t->title = FALSE;
			}
			else if (strncmp (t->buffer + 2, "script", 6) == 0) {
				t->script = TRUE;
				t->searchCount = 0;
				t->searchFor = scriptEnd;
				t->scriptCode = g_malloc (1024);
				t->scriptCodeSize = 0;
				t->scriptCodeMaxSize = 1024;
			}
			else if (strncmp (t->buffer + 2, "style", 5) == 0) {
				t->style = TRUE;
				t->searchCount = 0;
				t->searchFor = styleEnd;
				t->scriptCode = g_malloc (1024);
				t->scriptCodeSize = 0;
				t->scriptCodeMaxSize = 1024;
			}
			else if (strncmp (t->buffer + 2, "select", 6) == 0) {
				t->select = TRUE;
			}
			else if (strncmp (t->buffer + 2, "/select", 7) == 0) {
				t->select = FALSE;
			}
			else if (strncmp (t->buffer + 2, "frameset", 8) == 0) {
				g_warning ("<frameset> tag not supported");
			}
			else if (strncmp (t->buffer + 2, "cell", 4) == 0) {
				g_warning ("<cell> tag not supported");
			}
			else if (strncmp (t->buffer + 2, "table", 5) == 0) {
				html_blocking_token_append (t, html_blocking_token_new (Table, t->last));
			}
			else {
				if (!html_blocking_token_is_empty (t) &&
				    strncmp (t->buffer + 1, html_blocking_token_get_token_name (html_blocking_token_get_last (t)), 
					     strlen (html_blocking_token_get_token_name (html_blocking_token_get_last (t)))) == 0) {
					html_blocking_token_remove_last (t);
				}
			}
		}
		else if ((*src == '\n') || (*src == '\r')) {
			if (t->tquote) {
				if (t->discard == NoneDiscard)
					t->pending = SpacePending;
			}
			else if (t->tag) {
				t->searchCount = 0; /* Stop looking for <!-- sequence */
				if (t->discard == NoneDiscard)
					t->pending = SpacePending; /* Treat LFs inside tags as spaces */
			}
			else if (t->pre || t->textarea) {
				if (t->discard == LFDiscard) {
					/* Ignore this LF */
					t->discard = NoneDiscard; /*  We have discarded 1 LF */
				} else {
					/* Process this LF */
					if (t->pending)
						html_tokenizer_add_pending (t);
					t->pending = LFPending;
				}
			}
			else {
				if (t->discard == LFDiscard) {
					/* Ignore this LF */
					t->discard = NoneDiscard; /* We have discarded 1 LF */
				} else {
					/* Process this LF */
					if (t->pending == NonePending)
						t->pending = LFPending;
				}
			}
			/* Check for MS-DOS CRLF sequence */
			if (*src == '\r') {
				t->skipLF = TRUE;
			}
			src++;
		}
		else if ((*src == ' ') || (*src == '\t')) {
			if (t->tquote) {
				if (t->discard == NoneDiscard)
					t->pending = SpacePending;
			}
			else if (t->tag) {
				t->searchCount = 0; /* Stop looking for <!-- sequence */
				if (t->discard == NoneDiscard)
					t->pending = SpacePending;
			}
			else if (t->pre || t->textarea) {
				if (t->pending)
					html_tokenizer_add_pending (t);
				if (*src == ' ')
					t->pending = SpacePending;
				else
					t->pending = TabPending;
			}
			else {
				t->pending = SpacePending;
			}
			src++;
		}
		else if (*src == '\"' || *src == '\'') {
			/* We treat ' and " the same in tags */
			t->discard = NoneDiscard;
			if (t->tag) {
				t->searchCount = 0; /* Stop looking for <!-- sequence */
				if (((t->tquote == SINGLE_QUOTE) && (*src == '\"')) ||
				    ((t->tquote == DOUBLE_QUOTE) && (*src == '\''))) {
					*(t->dest)++ = *src;
				}
				else if (*(t->dest-1) == '=' && !t->tquote) {
					t->discard = SpaceDiscard;
					t->pending = NonePending;
					
					if (*src == '\"')
						t->tquote = DOUBLE_QUOTE;
					else
						t->tquote = SINGLE_QUOTE;
					*(t->dest)++ = *src;
				}
				else if (t->tquote) {
					t->tquote = NO_QUOTE;
					*(t->dest)++ = *src;
					t->pending = SpacePending;
				}
				else {
					/* Ignore stray "\'" */
				}
				src++;
			}
			else {
				if (t->pending)
					html_tokenizer_add_pending (t);
				if (t->pre)
					t->prePos++;
				
				*(t->dest)++ = *src++;
			}
		}
		else if (*src == '=') {
			src++;
			t->discard = NoneDiscard;
			if (t->tag) {
				t->searchCount = 0; /* Stop looking for <!-- sequence */
				*(t->dest)++ = '=';
				if (!t->tquote) {
					t->pending = NonePending;
					t->discard = SpaceDiscard;
				}
			}
			else {
				if (t->pending)
					html_tokenizer_add_pending (t);
				if (t->pre)
					t->prePos++;

				*(t->dest)++ = '=';
			}
		}
		else {
			t->discard = NoneDiscard;
			if (t->pending)
				html_tokenizer_add_pending (t);
			
			if (t->tag) {
				if (t->searchCount > 0) {
					if (*src == commentStart[t->searchCount]) {
						t->searchCount++;
						if (t->searchCount == 4) {
							/* Found <!-- sequence */
							t->comment = TRUE;
							t->dest = t->buffer;
							t->tag = FALSE;
							t->searchCount = 0;
							continue;
						}
					}
					else {
						t->searchCount = 0; /* Stop lookinf for <!-- sequence */
					}
				}
			}
			else if (t->pre) {
				t->prePos++;
			}
			*(t->dest)++ = *src++;
		}
	}
	
	if (srcPtr)
		g_free (srcPtr);

}

HTMLBlockingToken *
html_blocking_token_new (TokenType ttype, TokenPtr tok)
{
	HTMLBlockingToken *t = g_new0 (HTMLBlockingToken, 1);
	t->ttype = ttype;
	t->tok = tok;

	return t;
}

gchar *
html_blocking_token_get_token_name (HTMLBlockingToken *token)
{
	switch (token->ttype) {
	case Table:
		return "</table";
	}
	
	return "";
}

void
html_blocking_token_append (HTMLTokenizer *t, HTMLBlockingToken *bt)
{
	t->blocking = g_list_prepend (t->blocking, (gpointer) bt);
}

void
html_blocking_token_remove_last (HTMLTokenizer *t)
{
	t->blocking = g_list_remove (t->blocking, (g_list_first (t->blocking))->data);
}

gboolean
html_blocking_token_is_empty (HTMLTokenizer *t)
{
	return (g_list_length (t->blocking) == 0);
}

HTMLBlockingToken *
html_blocking_token_get_last (HTMLTokenizer *t)
{
	return ((HTMLBlockingToken *)(g_list_first (t->blocking))->data);
}

HTMLBlockingToken *
html_blocking_token_get_first (HTMLTokenizer *t)
{
	return ((HTMLBlockingToken *)(g_list_last (t->blocking))->data);
}
