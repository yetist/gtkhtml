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
#ifndef _HTMLTOKENIZER_H_
#define _HTMLTOKENIZER_H_

#include <glib.h>

#define TAG_ESCAPE 13
#define TAB_SIZE 8

typedef gchar * TokenPtr;

struct _HTMLTokenizer {
	gchar *dest;
	gchar *buffer;
	gint size;

	/* Token list */
	GList *tokenBufferList;
	TokenPtr last; /* Last token appended */
	TokenPtr next; /* Token written next */
	TokenPtr curr;
	
	gint tokenBufferCurrIndex; 
	gint tokenBufferSizeRemaining; /* The size remaining in the buffer */
	
	gboolean skipLF; /* Skip the LF par of a CRLF sequence */

	gboolean tag; /* Are we in an html tag? */
	gboolean tquote; /* Are we in quotes in an html tag? */
	gboolean startTag;
	gboolean comment; /* Are we in a comment block? */
	gboolean title; /* Are we in a <title> block? */
	gboolean style; /* Are we in a <style> block? */
	gboolean script; /* Are we in a <script> block? */
	gboolean textarea; /* Are we in a <textarea> block? */
	gboolean pre; /* Are we in a <pre> block? */
	gboolean select; /* Are we in a <select> block? */
	gboolean charEntity; /* Are we in an &... sequence? */

	gint prePos;
	
	enum {
		NoneDiscard = 0,
		SpaceDiscard,
		LFDiscard
	} discard;

	enum {
		NonePending = 0,
		SpacePending,
		LFPending,
		TabPending
	} pending;


	gchar searchBuffer[10];
	gint searchCount;

	gchar *scriptCode;
	gint scriptCodeSize;
	gint scriptCodeMaxSize;

	GList *blocking; /* Blocking tokens */

	const gchar *searchFor;
};

typedef	enum { Table } TokenType;

struct _HTMLBlockingToken {
	TokenType ttype;
	TokenPtr tok;
};

typedef struct  _HTMLTokenizer HTMLTokenizer;
typedef struct  _HTMLBlockingToken HTMLBlockingToken;

HTMLTokenizer *html_tokenizer_new          (void);
void           html_tokenizer_destroy      (HTMLTokenizer *tokenizer);
void           html_tokenizer_reset        (HTMLTokenizer *t);
void           html_tokenizer_begin        (HTMLTokenizer *t);
void           html_tokenizer_add_pending  (HTMLTokenizer *t);
void           html_tokenizer_append_token (HTMLTokenizer *t,
					    const gchar *string, gint len);
void           html_tokenizer_write        (HTMLTokenizer *t, const gchar *string, size_t size);
gchar *        html_tokenizer_next_token   (HTMLTokenizer *t);
void           html_tokenizer_end          (HTMLTokenizer *t);

void           html_tokenizer_next_token_buffer   (HTMLTokenizer *t);
void           html_tokenizer_append_token_buffer (HTMLTokenizer *t,
						   gint min_size);
gboolean       html_tokenizer_has_more_tokens     (HTMLTokenizer *t);


HTMLBlockingToken *html_blocking_token_new            (TokenType ttype,
						       TokenPtr tok);
gchar             *html_blocking_token_get_token_name (HTMLBlockingToken *token);
HTMLBlockingToken *html_blocking_token_get_last       (HTMLTokenizer *t);
gboolean           html_blocking_token_is_empty       (HTMLTokenizer *t);
HTMLBlockingToken *html_blocking_token_get_first      (HTMLTokenizer *t);

void               html_blocking_token_remove_last    (HTMLTokenizer *t);
void               html_blocking_token_append         (HTMLTokenizer *t,
						       HTMLBlockingToken *bt);


#endif /* _HTMLTOKENIZER_H_ */
