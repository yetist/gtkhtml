/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *            (C) 1997 Torben Weis (weis@kde.org)
 *            (C) 1999 Anders Carlsson (andersca@gnu.org)
 *            (C) 2000 Helix Code, Inc., Radek Doulik (rodo@helixcode.com)
 *            (C) 2001 Ximian, Inc.
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

/* The HTML Tokenizer */
#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "htmltokenizer.h"
#include "htmlentity.h"

enum {
	HTML_TOKENIZER_BEGIN_SIGNAL,
	HTML_TOKENIZER_END_SIGNAL,
	HTML_TOKENIZER_CHANGECONTENT_SIGNAL,
	HTML_TOKENIZER_CHANGEENGINE_SIGNAL,
	HTML_TOKENIZER_LAST_SIGNAL
};

static guint html_tokenizer_signals[HTML_TOKENIZER_LAST_SIGNAL] = { 0 };

#define TOKEN_BUFFER_SIZE (1 << 10)

#define dt(x)

typedef struct _HTMLBlockingToken HTMLBlockingToken;
typedef struct _HTMLTokenBuffer   HTMLTokenBuffer;
typedef	enum { Table }            HTMLTokenType;

struct _HTMLTokenBuffer {
	gint size;
	gint used;
	gchar * data;
};

struct _HTMLTokenizerPrivate {

	/* token buffers list */
	GList *token_buffers;

	/* current read_buf position in list */
	GList *read_cur;

	/* current read buffer */
	HTMLTokenBuffer * read_buf;
	HTMLTokenBuffer * write_buf;

	/* position in the read_buf */
	gint read_pos;

	/* non-blocking and blocking unreaded tokens in tokenizer */
	gint tokens_num;
	gint blocking_tokens_num;

	gchar *dest;
	gchar *buffer;
	gint size;

	gboolean skipLF; /* Skip the LF par of a CRLF sequence */

	gboolean tag; /* Are we in an html tag? */
	gboolean tquote; /* Are we in quotes in an html tag? */
	gboolean startTag;
	gboolean comment; /* Are we in a comment block? */
	gboolean title; /* Are we in a <title> block? */
	gboolean style; /* Are we in a <style> block? */
	gboolean script; /* Are we in a <script> block? */
	gboolean textarea; /* Are we in a <textarea> block? */
	gint     pre; /* Are we in a <pre> block? */
	gboolean select; /* Are we in a <select> block? */
	gboolean extension; /* Are we in an <!-- +GtkHTML: sequence? */

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

	gchar searchBuffer[20];
	gint searchCount;
	gint searchGtkHTMLCount;
	gint searchExtensionEndCount;

	gchar *scriptCode;
	gint scriptCodeSize;
	gint scriptCodeMaxSize;

	GList *blocking; /* Blocking tokens */

	const gchar *searchFor;

	gboolean enableconvert;

	gchar * content_type;
	/*convert*/
	GIConv iconv_cd;

};

static const gchar *commentStart = "<!--";
static const gchar *scriptEnd = "</script>";
static const gchar *styleEnd = "</style>";
static const gchar *gtkhtmlStart = "+gtkhtml:";

enum quoteEnum {
	NO_QUOTE = 0,
	SINGLE_QUOTE,
	DOUBLE_QUOTE
};

/* private tokenizer functions */
static void           html_tokenizer_reset        (HTMLTokenizer *t);
static void           html_tokenizer_add_pending  (HTMLTokenizer *t);
static void           html_tokenizer_append_token (HTMLTokenizer *t,
						   const gchar *string,
						   gint len);
static void           html_tokenizer_append_token_buffer (HTMLTokenizer *t,
							  gint min_size);

/* default implementations of tokenization functions */
static void     html_tokenizer_finalize             (GObject *);
static void     html_tokenizer_real_change          (HTMLTokenizer *, const gchar *content_type);
static void     html_tokenizer_real_begin           (HTMLTokenizer *, const gchar *content_type);
static void     html_tokenizer_real_engine_type (HTMLTokenizer *t, gboolean engine_type);
static void     html_tokenizer_real_write           (HTMLTokenizer *, const gchar *str, gsize size);
static void     html_tokenizer_real_end             (HTMLTokenizer *);
static const gchar *
				html_tokenizer_real_get_content_type (HTMLTokenizer *);
static gboolean
				html_tokenizer_real_get_engine_type (HTMLTokenizer *);
static gchar   *html_tokenizer_real_peek_token      (HTMLTokenizer *);
static gchar   *html_tokenizer_real_next_token      (HTMLTokenizer *);
static gboolean html_tokenizer_real_has_more_tokens (HTMLTokenizer *);
static gchar   *html_tokenizer_converted_token (HTMLTokenizer *t,const gchar * token);

static HTMLTokenizer *html_tokenizer_real_clone     (HTMLTokenizer *);

/* blocking tokens */
static const gchar       *html_tokenizer_blocking_get_name   (HTMLTokenizer  *t);
static void               html_tokenizer_blocking_pop        (HTMLTokenizer  *t);
static void               html_tokenizer_blocking_push       (HTMLTokenizer  *t,
							      HTMLTokenType   tt);
static void               html_tokenizer_tokenize_one_char   (HTMLTokenizer  *t,
							      const gchar  **src);
static void				  add_char (HTMLTokenizer *t, gchar c);

gboolean				  is_need_convert (const gchar * token);

gchar *					  html_tokenizer_convert_entity (gchar * token);

G_DEFINE_TYPE_WITH_PRIVATE (HTMLTokenizer, html_tokenizer, G_TYPE_OBJECT);

static void
html_tokenizer_class_init (HTMLTokenizerClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	html_tokenizer_signals[HTML_TOKENIZER_CHANGECONTENT_SIGNAL] =
		g_signal_new ("change",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, change),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	html_tokenizer_signals[HTML_TOKENIZER_CHANGEENGINE_SIGNAL] =
		g_signal_new ("engine",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, engine),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	html_tokenizer_signals[HTML_TOKENIZER_BEGIN_SIGNAL] =
		g_signal_new ("begin",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, begin),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1, G_TYPE_POINTER);

	html_tokenizer_signals[HTML_TOKENIZER_END_SIGNAL] =
		g_signal_new ("end",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HTMLTokenizerClass, end),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	object_class->finalize = html_tokenizer_finalize;

	klass->change     = html_tokenizer_real_change;
	klass->engine     = html_tokenizer_real_engine_type;
	klass->begin      = html_tokenizer_real_begin;
	klass->end        = html_tokenizer_real_end;

	klass->write      = html_tokenizer_real_write;
	klass->peek_token = html_tokenizer_real_peek_token;
	klass->next_token = html_tokenizer_real_next_token;
	klass->get_content_type = html_tokenizer_real_get_content_type;
	klass->get_engine_type = html_tokenizer_real_get_engine_type;
	klass->has_more   = html_tokenizer_real_has_more_tokens;
	klass->clone      = html_tokenizer_real_clone;
}

static void
html_tokenizer_init (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;

	p = html_tokenizer_get_instance_private (t);

	p->token_buffers = NULL;
	p->read_cur  = NULL;
	p->read_buf  = NULL;
	p->write_buf = NULL;
	p->read_pos  = 0;

	p->dest = NULL;
	p->buffer = NULL;
	p->size = 0;

	p->skipLF = FALSE;
	p->tag = FALSE;
	p->tquote = FALSE;
	p->startTag = FALSE;
	p->comment = FALSE;
	p->title = FALSE;
	p->style = FALSE;
	p->script = FALSE;
	p->textarea = FALSE;
	p->pre = 0;
	p->select = FALSE;
	p->extension = FALSE;

	p->discard = NoneDiscard;
	p->pending = NonePending;

	memset (p->searchBuffer, 0, sizeof (p->searchBuffer));
	p->searchCount = 0;
	p->searchGtkHTMLCount = 0;

	p->scriptCode = NULL;
	p->scriptCodeSize = 0;
	p->scriptCodeMaxSize = 0;

	p->blocking = NULL;

	p->searchFor = NULL;

	/* Use old logic and not convert charset */
	p->enableconvert = FALSE;

	p->content_type = g_strdup ("html/text; charset=utf-8");
}

static void
html_tokenizer_finalize (GObject *obj)
{
	HTMLTokenizerPrivate *priv;
	HTMLTokenizer *t = HTML_TOKENIZER (obj);

	html_tokenizer_reset (t);
	priv = html_tokenizer_get_instance_private (t);

	if (is_valid_g_iconv (priv->iconv_cd))
		g_iconv_close (priv->iconv_cd);

	if (priv->content_type)
		g_free (priv->content_type);

    G_OBJECT_CLASS (html_tokenizer_parent_class)->finalize (obj);
}

static HTMLTokenBuffer *
html_token_buffer_new (gint size)
{
	HTMLTokenBuffer *nb = g_new (HTMLTokenBuffer, 1);

	nb->data = g_new (gchar, size);
	nb->size = size;
	nb->used = 0;

	return nb;
}

static void
html_token_buffer_destroy (HTMLTokenBuffer *tb)
{
	g_free (tb->data);
	g_free (tb);
}

static gboolean
html_token_buffer_append_token (HTMLTokenBuffer *buf,
                                const gchar *token,
                                gint len)
{

	/* check if we have enough free space */
	if (len + 1 > buf->size - buf->used) {
		return FALSE;
	}

	/* copy token and terminate with zero */
	strncpy (buf->data + buf->used, token, len);
	buf->used += len;
	buf->data[buf->used] = 0;
	buf->used++;

	dt (printf ("html_token_buffer_append_token: '%s'\n", buf->data + buf->used - 1 - len));

	return TRUE;
}

HTMLTokenizer *
html_tokenizer_new (void)
{
	return (HTMLTokenizer *) g_object_new (HTML_TYPE_TOKENIZER, NULL);
}

void
html_tokenizer_destroy (HTMLTokenizer *t)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_object_unref (G_OBJECT (t));
}

static gchar *
html_tokenizer_real_peek_token (HTMLTokenizer *t)
{
	gchar *token;
	HTMLTokenizerPrivate *p;

	p = html_tokenizer_get_instance_private (t);
	g_assert (p->read_buf);

	if (p->read_buf->used > p->read_pos) {
		token = p->read_buf->data + p->read_pos;
	} else {
		GList *next;
		HTMLTokenBuffer *buffer;

		g_assert (p->read_cur);
		g_assert (p->read_buf);

		/* lookup for next buffer */
		next = p->read_cur->next;
		g_assert (next);

		buffer = (HTMLTokenBuffer *) next->data;

		g_return_val_if_fail (buffer->used != 0, NULL);

		/* finally get first token */
		token = buffer->data;
	}

	return html_tokenizer_converted_token (t,token);
}

/* test iconv for valid*/
gboolean
is_valid_g_iconv (const GIConv iconv_cd)
{
	return iconv_cd != NULL && iconv_cd != (GIConv) - 1;
}

/*Convert only chars when code >127*/
gboolean
is_need_convert (const gchar *token)
{
	gint i = strlen (token);
	for (; i >= 0; i--)
		if (token[i]&128)
			return TRUE;
	return FALSE;
}

/*Convert entity values in already converted to right charset token*/
gchar *
html_tokenizer_convert_entity (gchar *token)
{
	gchar *full_pos;
	gchar *resulted;
	gchar *write_pos;
	gchar *read_pos;

	if (token == NULL)
		return NULL;

	/*stop pointer*/
	full_pos = token + strlen (token);
	resulted = g_new (gchar, strlen (token) + 1);
	write_pos = resulted;
	read_pos = token;
	while (read_pos < full_pos) {
		gsize count_chars = strcspn (read_pos, "&");
		memcpy (write_pos, read_pos, count_chars);
		write_pos += count_chars;
		read_pos += count_chars;
		/*may be end string?*/
		if (read_pos < full_pos)
			if (*read_pos == '&') {
				/*value to add*/
				gunichar value = INVALID_ENTITY_CHARACTER_MARKER;
				/*skip not needed &*/
				read_pos++;
				count_chars = strcspn (read_pos, ";");
				if (count_chars < 14 && count_chars > 1) {
					/*save for recovery*/
					gchar save_gchar = *(read_pos + count_chars);
					*(read_pos + count_chars)=0;
					/* &#******; */
					if (*read_pos == '#') {
						/* &#1234567 */
						if (isdigit (*(read_pos + 1))) {
							value = strtoull (read_pos + 1, NULL, 10);
						/* &#xdd; */
						} else if (*(read_pos + 1) == 'x') {
							value = strtoull (read_pos + 2, NULL, 16);
						}
					} else {
						value = html_entity_parse (read_pos, strlen (read_pos));
					}
					if (*read_pos == '#' || value != INVALID_ENTITY_CHARACTER_MARKER) {
						write_pos += g_unichar_to_utf8 (value, write_pos);
						read_pos += (count_chars + 1);
					} else {
						/*recovery old value - it's not entity*/
						write_pos += g_unichar_to_utf8 ('&', write_pos);
						*(read_pos + count_chars) = save_gchar;
					}
				}
				else
					/*very large string*/
					write_pos += g_unichar_to_utf8 ('&', write_pos);
			}
	}
	*write_pos = 0;
	free (token);

	return resulted;
}

/*convert text to utf8 - allways alloc memmory*/
gchar *
convert_text_encoding (const GIConv iconv_cd,
                       const gchar *token)
{
	gsize currlength;
	gchar *newbuffer;
	gchar *returnbuffer;
	const gchar *current;
	gsize newlength;
	gsize oldlength;

	if (token == NULL)
		return NULL;

	if (is_valid_g_iconv (iconv_cd) && is_need_convert (token)) {
		currlength = strlen (token);
		current = token;
		newlength = currlength * 7 + 1;
		oldlength = newlength;
		newbuffer = g_new (gchar, newlength);
		returnbuffer = newbuffer;

		while (currlength > 0) {
			/*function not change current, but g_iconv use not const source*/
			g_iconv (iconv_cd, (gchar **) &current, &currlength, &newbuffer, &newlength);
			if (currlength > 0) {
				g_warning ("IconvError=%s", current);
				*newbuffer = INVALID_ENTITY_CHARACTER_MARKER;
				newbuffer++;
				current++;
				currlength--;
				newlength--;
			}
		}
		returnbuffer[oldlength - newlength] = '\0';
		returnbuffer = g_realloc (returnbuffer, oldlength - newlength + 1);
		return returnbuffer;
	}
	return g_strdup (token);
}

static gchar *
html_tokenizer_converted_token (HTMLTokenizer *t,
                                const gchar *token)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);
	if (token != NULL) {
		return html_tokenizer_convert_entity (convert_text_encoding (p->iconv_cd, token));
	}

	return NULL;
}

static const gchar *
html_tokenizer_real_get_content_type (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (p->content_type)
		return p->content_type;

	return NULL;
}

static gboolean
html_tokenizer_real_get_engine_type (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	return p->enableconvert;
}

static gchar *
html_tokenizer_real_next_token (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	gchar *token;

	p = html_tokenizer_get_instance_private (t);
	g_assert (p->read_buf);

	/* token is in current read_buf */
	if (p->read_buf->used > p->read_pos) {
		token = p->read_buf->data + p->read_pos;
		p->read_pos += strlen (token) + 1;
	} else {
		GList *new;

		g_assert (p->read_cur);
		g_assert (p->read_buf);

		/* lookup for next buffer */
		new = p->read_cur->next;
		g_assert (new);

		/* destroy current buffer */
		p->token_buffers = g_list_remove (p->token_buffers, p->read_buf);
		html_token_buffer_destroy (p->read_buf);

		p->read_cur = new;
		p->read_buf = (HTMLTokenBuffer *) new->data;

		g_return_val_if_fail (p->read_buf->used != 0, NULL);

		/* finally get first token */
		token = p->read_buf->data;
		p->read_pos = strlen (token) + 1;
	}

	p->tokens_num--;
	g_assert (p->tokens_num >= 0);

	return html_tokenizer_converted_token (t, token);
}

static gboolean
html_tokenizer_real_has_more_tokens (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);
	return p->tokens_num > 0;
}

static HTMLTokenizer *
html_tokenizer_real_clone (HTMLTokenizer *t)
{
	return html_tokenizer_new ();
}

static void
html_tokenizer_reset (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	GList *cur;

	p = html_tokenizer_get_instance_private (t);
	cur = p->token_buffers;

	/* free remaining token buffers */
	while (cur) {
		g_assert (cur->data);
		html_token_buffer_destroy ((HTMLTokenBuffer *) cur->data);
		cur = cur->next;
	}

	/* reset buffer list */
	g_list_free (p->token_buffers);
	p->token_buffers = p->read_cur = NULL;
	p->read_buf = p->write_buf = NULL;
	p->read_pos = 0;

	/* reset token counters */
	p->tokens_num = p->blocking_tokens_num = 0;

	if (p->buffer)
		g_free (p->buffer);
	p->buffer = NULL;
	p->dest = NULL;
	p->size = 0;

	if (p->scriptCode)
		g_free (p->scriptCode);
	p->scriptCode = NULL;
}

static gboolean
charset_is_utf8 (const gchar *content_type)
{
	return content_type && strstr (content_type, "=utf-8") != NULL;
}

static gboolean
is_text (const gchar *content_type)
{
	return content_type && strstr (content_type, "text/") != NULL;
}

static const gchar *
get_encoding_from_content_type (const gchar *content_type)
{
	gchar * charset;
	if (content_type)
	{
		charset =  g_strrstr (content_type, "charset=");
		if (charset != NULL)
			return charset + strlen ("charset=");
		charset =  g_strrstr (content_type, "encoding=");
		if (charset != NULL)
			return charset + strlen ("encoding=");

	}
	return NULL;
}

GIConv
generate_iconv_from (const gchar *content_type)
{
	if (content_type)
		if (!charset_is_utf8 (content_type))
		{
			const gchar * encoding = get_encoding_from_content_type (content_type);
			if (encoding)
				return g_iconv_open ("utf-8", encoding);
		}
	return NULL;
}

GIConv
generate_iconv_to (const gchar *content_type)
{
	if (content_type)
		if (!charset_is_utf8 (content_type))
		{
			const gchar * encoding = get_encoding_from_content_type (content_type);
			if (encoding)
				return g_iconv_open (encoding, "utf-8");
		}
	return NULL;
}

static void
html_tokenizer_real_engine_type (HTMLTokenizer *t,
                                 gboolean engine_type)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	p->enableconvert = engine_type;
}

static void
html_tokenizer_real_change (HTMLTokenizer *t,
                            const gchar *content_type)
{
	HTMLTokenizerPrivate *p;
	if (!is_text (content_type))
		return;

	p = html_tokenizer_get_instance_private (t);

	if (!p->enableconvert)
		return;

	if (p->content_type)
		g_free (p->content_type);

	p->content_type = g_ascii_strdown (content_type, -1);

	if (is_valid_g_iconv (p->iconv_cd))
		g_iconv_close (p->iconv_cd);

	p->iconv_cd = generate_iconv_from (p->content_type);

#if 0
	if (charset_is_utf8 (p->content_type))
		g_warning ("Trying UTF-8");
	else
		g_warning ("Trying %s",p->content_type);
#endif
}

static void
html_tokenizer_real_begin (HTMLTokenizer *t,
                           const gchar *content_type)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	html_tokenizer_reset (t);

	p->dest = p->buffer;
	p->tag = FALSE;
	p->pending = NonePending;
	p->discard = NoneDiscard;
	p->pre = 0;
	p->script = FALSE;
	p->style = FALSE;
	p->skipLF = FALSE;
	p->select = FALSE;
	p->comment = FALSE;
	p->textarea = FALSE;
	p->startTag = FALSE;
	p->extension = FALSE;
	p->tquote = NO_QUOTE;
	p->searchCount = 0;
	p->searchGtkHTMLCount = 0;
	p->title = FALSE;

	html_tokenizer_real_change (t, content_type);
}

static void
destroy_blocking (gpointer data,
                  gpointer user_data)
{
	g_free (data);
}

static void
html_tokenizer_real_end (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (p->buffer == 0)
		return;

	if (p->dest > p->buffer) {
		html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
	}

	g_free (p->buffer);

	p->buffer = NULL;
	p->dest = NULL;
	p->size = 0;

	if (p->blocking) {
		g_list_foreach (p->blocking, destroy_blocking, NULL);
		p->tokens_num += p->blocking_tokens_num;
		p->blocking_tokens_num = 0;
	}
	p->blocking = NULL;
}

static void
html_tokenizer_append_token (HTMLTokenizer *t,
                             const gchar *string,
                             gint len)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (len < 1)
		return;

	/* allocate first buffer */
	if (p->write_buf == NULL)
		html_tokenizer_append_token_buffer (t, len);

	/* try append token to current buffer, if not successful, create append new token buffer */
	if (!html_token_buffer_append_token (p->write_buf, string, len)) {
		html_tokenizer_append_token_buffer (t, len + 1);
		/* now it must pass as we have enough space */
		g_assert (html_token_buffer_append_token (p->write_buf, string, len));
	}

	if (p->blocking) {
		p->blocking_tokens_num++;
	} else {
		p->tokens_num++;
	}
}

static void
add_byte (HTMLTokenizer *t,
          const gchar **c)
{
	add_char (t,**c);
	(*c) ++;
}

static void
add_char (HTMLTokenizer *t,
          gchar c)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);
	if (c != '\0')
	{
		*(p->dest) = c;
		p->dest++;
		*(p->dest) = 0;
	}
}

static void
html_tokenizer_append_token_buffer (HTMLTokenizer *t,
                                    gint min_size)
{
	HTMLTokenizerPrivate *p;
	HTMLTokenBuffer *nb;

	gint size = TOKEN_BUFFER_SIZE;

	if (min_size > size)
		size = min_size + (min_size >> 2);

	p = html_tokenizer_get_instance_private (t);
	/* create new buffer and add it to list */
	nb = html_token_buffer_new (size);
	p->token_buffers = g_list_append (p->token_buffers, nb);

	/* this one is now write_buf */
	p->write_buf = nb;

	/* if we don't have read_buf already set it to this one */
	if (p->read_buf == NULL) {
		p->read_buf = nb;
		p->read_cur = p->token_buffers;
	}
}

/* EP CHECK: OK.  */
static void
html_tokenizer_add_pending (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (p->tag || p->select) {
		add_char (t, ' ');
	}
	else if (p->textarea) {
		if (p->pending == LFPending)
			add_char (t, '\n');
		else
			add_char (t, ' ');
	}
	else if (p->pre) {
		switch (p->pending) {
		case SpacePending:
			add_char (t, ' ');
			break;
		case LFPending:
			if (p->dest > p->buffer) {
				html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
			}
			p->dest = p->buffer;
			add_char (t, TAG_ESCAPE);
			add_char (t, '\n');
			html_tokenizer_append_token (t, p->buffer, 2);
			p->dest = p->buffer;
			break;
		case TabPending:
			add_char (t, '\t');
			break;
		default:
			g_warning ("Unknown pending type: %d\n", (gint) p->pending);
			break;
		}
	}
	else {
		add_char (t, ' ');
	}

	p->pending = NonePending;
}

static void
prepare_enough_space (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if ((p->dest - p->buffer + 32) > p->size) {
		guint off = p->dest - p->buffer;

		p->size  += (p->size >> 2) + 32;
		p->buffer = g_realloc (p->buffer, p->size);
		p->dest   = p->buffer + off;
	}
}

static void
in_comment (HTMLTokenizer *t,
            const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (**src == '-') {		     /* Look for "-->" */
		if (p->searchCount < 2)
			p->searchCount++;
	} else if (p->searchCount == 2 && (**src == '>')) {
		p->comment = FALSE;          /* We've got a "-->" sequence */
	} else if (tolower (**src) == gtkhtmlStart[p->searchGtkHTMLCount]) {
		if (p->searchGtkHTMLCount == 8) {
			p->extension    = TRUE;
			p->comment = FALSE;
			p->searchCount = 0;
			p->searchExtensionEndCount = 0;
			p->searchGtkHTMLCount = 0;
		} else
			p->searchGtkHTMLCount++;
	} else {
		p->searchGtkHTMLCount = 0;
		if (p->searchCount < 2)
			p->searchCount = 0;
	}

	(*src)++;
}

static inline void
extension_one_char (HTMLTokenizer *t,
                    const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	p->extension = FALSE;
	html_tokenizer_tokenize_one_char (t, src);
	p->extension = TRUE;
}

static void
in_extension (HTMLTokenizer *t,
              const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	/* check for "-->" */
	if (!p->tquote && **src == '-') {
		if (p->searchExtensionEndCount < 2)
			p->searchExtensionEndCount++;
		(*src) ++;
	} else if (!p->tquote && p->searchExtensionEndCount == 2 && **src == '>') {
		p->extension = FALSE;
		(*src) ++;
	} else {
		if (p->searchExtensionEndCount > 0) {
			if (p->extension) {
				const gchar *c = "-->";

				while (p->searchExtensionEndCount) {
					extension_one_char (t, &c);
					p->searchExtensionEndCount--;
				}
			}
		}
		extension_one_char (t, src);
	}
}

static void
in_script_or_style (HTMLTokenizer *t,
                    const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	/* Allocate memory to store the script or style */
	if (p->scriptCodeSize + 11 > p->scriptCodeMaxSize)
		p->scriptCode = g_realloc (p->scriptCode, p->scriptCodeMaxSize += 1024);

	if ((**src == '>') && (p->searchFor[p->searchCount] == '>')) {
		(*src)++;
		p->scriptCode[p->scriptCodeSize] = 0;
		p->scriptCode[p->scriptCodeSize + 1] = 0;
		if (p->script) {
			p->script = FALSE;
		}
		else {
			p->style = FALSE;
		}
		g_free (p->scriptCode);
		p->scriptCode = NULL;
	}
	/* Check if a </script> tag is on its way */
	else if (p->searchCount > 0) {
		gboolean put_to_script = FALSE;
		if (tolower (**src) == p->searchFor[p->searchCount]) {
			p->searchBuffer[p->searchCount] = **src;
			p->searchCount++;
			(*src)++;
		}
		else if (p->searchFor[p->searchCount] == '>') {
			/* There can be any number of white-space characters between
			 * tag name and closing '>' so try to move through them, if possible */

			const gchar **p = src;
			while (isspace (**p))
				(*p)++;

			if (**p == '>')
				*src = *p;
			else
				put_to_script = TRUE;
		}
		else
			put_to_script = TRUE;

		if (put_to_script) {
			gchar *c;

			p->searchBuffer[p->searchCount] = 0;
			c = p->searchBuffer;
			while (*c)
				p->scriptCode[p->scriptCodeSize++] = *c++;
			p->scriptCode[p->scriptCodeSize] = **src; (*src)++;
			p->searchCount = 0;
		}
	}
	else if (**src == '<') {
		p->searchCount = 1;
		p->searchBuffer[0] = '<';
		(*src)++;
	}
	else {
		p->scriptCode[p->scriptCodeSize] = **src;
		(*src)++;
	}
}

static void
in_tag (HTMLTokenizer *t,
        const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	p->startTag = FALSE;
	if (**src == '/') {
		if (p->pending == LFPending  && !p->pre) {
			p->pending = NonePending;
		}
	}
	else if (((**src >= 'a') && (**src <= 'z'))
		 || ((**src >= 'A') && (**src <= 'Z'))) {
				/* Start of a start tag */
	}
	else if (**src == '!') {
				/* <!-- comment --> */
	}
	else if (**src == '?') {
				/* <? meta ?> */
	}
	else {
				/* Invalid tag, just add it */
		if (p->pending)
			html_tokenizer_add_pending (t);
		add_char (t, '<');
		add_byte (t, src);
		return;
	}

	if (p->pending)
		html_tokenizer_add_pending (t);

	if (p->dest > p->buffer) {
		html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
		p->dest = p->buffer;
	}
	add_char (t, TAG_ESCAPE);
	add_char (t, '<');
	p->tag = TRUE;
	p->searchCount = 1; /* Look for <!-- to start comment */
}

static void
start_tag (HTMLTokenizer *t,
           const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);
	(*src)++;
	p->startTag = TRUE;
	p->discard  = NoneDiscard;
}

static void
end_tag (HTMLTokenizer *t,
         const gchar **src)
{
	HTMLTokenizerPrivate *p;
	gchar *ptr;

	p = html_tokenizer_get_instance_private (t);
	p->searchCount = 0; /* Stop looking for <!-- sequence */

	add_char (t, '>');

	/* Make the tag lower case */
	ptr = p->buffer + 2;
	if (p->pre || *ptr == '/') {
		/* End tag */
		p->discard = NoneDiscard;
	}
	else {
		/* Start tag */
		/* Ignore CRLFs after a start tag */
		p->discard = LFDiscard;
	}

	while (*ptr && *ptr !=' ') {
		*ptr = tolower (*ptr);
		ptr++;
	}
	html_tokenizer_append_token (t, p->buffer, p->dest - p->buffer);
	p->dest = p->buffer;

	p->tag = FALSE;
	p->pending = NonePending;
	(*src)++;

	if (strncmp (p->buffer + 2, "pre", 3) == 0) {
		p->pre++;
	}
	else if (strncmp (p->buffer + 2, "/pre", 4) == 0) {
		p->pre--;
	}
	else if (strncmp (p->buffer + 2, "textarea", 8) == 0) {
		p->textarea = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/textarea", 9) == 0) {
		p->textarea = FALSE;
	}
	else if (strncmp (p->buffer + 2, "title", 5) == 0) {
		p->title = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/title", 6) == 0) {
		p->title = FALSE;
	}
	else if (strncmp (p->buffer + 2, "script", 6) == 0) {
		p->script = TRUE;
		p->searchCount = 0;
		p->searchFor = scriptEnd;
		p->scriptCode = g_malloc (1024);
		p->scriptCodeSize = 0;
		p->scriptCodeMaxSize = 1024;
	}
	else if (strncmp (p->buffer + 2, "style", 5) == 0) {
		p->style = TRUE;
		p->searchCount = 0;
		p->searchFor = styleEnd;
		p->scriptCode = g_malloc (1024);
		p->scriptCodeSize = 0;
		p->scriptCodeMaxSize = 1024;
	}
	else if (strncmp (p->buffer + 2, "select", 6) == 0) {
		p->select = TRUE;
	}
	else if (strncmp (p->buffer + 2, "/select", 7) == 0) {
		p->select = FALSE;
	}
	else if (strncmp (p->buffer + 2, "tablesdkl", 9) == 0) {
		html_tokenizer_blocking_push (t, Table);
	}
	else {
		if (p->blocking) {
			const gchar *bn = html_tokenizer_blocking_get_name (t);

			if (strncmp (p->buffer + 1, bn, strlen (bn)) == 0) {
				html_tokenizer_blocking_pop (t);
			}
		}
	}
}

static void
in_crlf (HTMLTokenizer *t,
         const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	if (p->tquote) {
		if (p->discard == NoneDiscard)
			p->pending = SpacePending;
	}
	else if (p->tag) {
		p->searchCount = 0; /* Stop looking for <!-- sequence */
		if (p->discard == NoneDiscard)
			p->pending = SpacePending; /* Treat LFs inside tags as spaces */
	}
	else if (p->pre || p->textarea) {
		if (p->discard == LFDiscard) {
			/* Ignore this LF */
			p->discard = NoneDiscard; /*  We have discarded 1 LF */
		} else {
			/* Process this LF */
			if (p->pending)
				html_tokenizer_add_pending (t);
			p->pending = LFPending;
		}
	}
	else {
		if (p->discard == LFDiscard) {
			/* Ignore this LF */
			p->discard = NoneDiscard; /* We have discarded 1 LF */
		} else {
			/* Process this LF */
			if (p->pending == NonePending)
				p->pending = LFPending;
		}
	}
	/* Check for MS-DOS CRLF sequence */
	if (**src == '\r') {
		p->skipLF = TRUE;
	}
	(*src)++;
}

static void
in_space_or_tab (HTMLTokenizer *t,
                 const gchar **src)
{
	HTMLTokenizerPrivate *priv;
	priv = html_tokenizer_get_instance_private (t);

	if (priv->tquote) {
		if (priv->discard == NoneDiscard)
			priv->pending = SpacePending;
	}
	else if (priv->tag) {
		priv->searchCount = 0; /* Stop looking for <!-- sequence */
		if (priv->discard == NoneDiscard)
			priv->pending = SpacePending;
	}
	else if (priv->pre || priv->textarea) {
		if (priv->pending)
			html_tokenizer_add_pending (t);
		if (**src == ' ')
			priv->pending = SpacePending;
		else
			priv->pending = TabPending;
	}
	else {
		priv->pending = SpacePending;
	}
	(*src)++;
}

static void
in_quoted (HTMLTokenizer *t,
           const gchar **src)
{
	HTMLTokenizerPrivate *priv;
	priv = html_tokenizer_get_instance_private (t);
	/* We treat ' and " the same in tags " */
	priv->discard = NoneDiscard;
	if (priv->tag) {
		priv->searchCount = 0; /* Stop looking for <!-- sequence */
		if ((priv->tquote == SINGLE_QUOTE && **src == '\"') /* match " */
		    || (priv->tquote == DOUBLE_QUOTE && **src == '\'')) {
			add_char (t, **src);
			(*src)++;
		} else if (*(priv->dest - 1) == '=' && !priv->tquote) {
			priv->discard = SpaceDiscard;
			priv->pending = NonePending;

			if (**src == '\"') /* match " */
				priv->tquote = DOUBLE_QUOTE;
			else
				priv->tquote = SINGLE_QUOTE;
			add_char (t, **src);
			(*src)++;
		}
		else if (priv->tquote) {
			priv->tquote = NO_QUOTE;
			add_byte (t, src);
			priv->pending = SpacePending;
		}
		else {
			/* Ignore stray "\'" */
			(*src)++;
		}
	}
	else {
		if (priv->pending)
			html_tokenizer_add_pending (t);

		add_byte (t, src);
	}
}

static void
in_assignment (HTMLTokenizer *t,
               const gchar **src)
{
	HTMLTokenizerPrivate *priv;
	priv = html_tokenizer_get_instance_private (t);
	priv->discard = NoneDiscard;
	if (priv->tag) {
		priv->searchCount = 0; /* Stop looking for <!-- sequence */
		add_char (t, '=');
		if (!priv->tquote) {
			priv->pending = NonePending;
			priv->discard = SpaceDiscard;
		}
	}
	else {
		if (priv->pending)
			html_tokenizer_add_pending (t);

		add_char (t, '=');
	}
	(*src)++;
}

inline static void
in_plain (HTMLTokenizer *t,
          const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	p->discard = NoneDiscard;
	if (p->pending)
		html_tokenizer_add_pending (t);

	if (p->tag) {
		if (p->searchCount > 0) {
			if (**src == commentStart[p->searchCount]) {
				p->searchCount++;
				if (p->searchCount == 4) {
					/* Found <!-- sequence */
					p->comment = TRUE;
					p->dest = p->buffer;
					p->tag = FALSE;
					p->searchCount = 0;
					return;
				}
			}
			else {
				p->searchCount = 0; /* Stop lookinf for <!-- sequence */
			}
		}
	}

	add_byte (t, src);
}

static void
html_tokenizer_tokenize_one_char (HTMLTokenizer *t,
                                  const gchar **src)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	prepare_enough_space (t);

	if (p->skipLF && **src != '\n')
		p->skipLF = FALSE;

	if (p->skipLF)
		(*src) ++;
	else if (p->comment)
		in_comment (t, src);
	else if (p->extension)
		in_extension (t, src);
	else if (p->script || p->style)
		in_script_or_style (t, src);
	else if (p->startTag)
		in_tag (t, src);
	else if (**src == '<' && !p->tag)
		start_tag (t, src);
	else if (**src == '>' && p->tag && !p->tquote)
		end_tag (t, src);
	else if ((**src == '\n') || (**src == '\r'))
		in_crlf (t, src);
	else if ((**src == ' ') || (**src == '\t'))
		in_space_or_tab (t, src);
	else if (**src == '\"' || **src == '\'') /* match " ' */
		in_quoted (t, src);
	else if (**src == '=')
		in_assignment (t, src);
	else
		in_plain (t, src);
}

static void
html_tokenizer_real_write (HTMLTokenizer *t,
                           const gchar *string,
                           gsize size)
{
	const gchar *src = string;

	while ((src - string) < size)
		html_tokenizer_tokenize_one_char (t, &src);
}

static const gchar *
html_tokenizer_blocking_get_name (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *priv;
	priv = html_tokenizer_get_instance_private (t);
	switch (GPOINTER_TO_INT (priv->blocking->data)) {
	case Table:
		return "</tabledkdk";
	}

	return "";
}

static void
html_tokenizer_blocking_push (HTMLTokenizer *t,
                              HTMLTokenType tt)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	/* block tokenizer - we must block last token in buffers as it was already added */
	if (!p->blocking) {
		p->tokens_num--;
		p->blocking_tokens_num++;
	}
	p->blocking = g_list_prepend (p->blocking, GINT_TO_POINTER (tt));
}

static void
html_tokenizer_blocking_pop (HTMLTokenizer *t)
{
	HTMLTokenizerPrivate *p;
	p = html_tokenizer_get_instance_private (t);

	p->blocking = g_list_remove (p->blocking, p->blocking->data);

	/* unblock tokenizer */
	if (!p->blocking) {
		p->tokens_num += p->blocking_tokens_num;
		p->blocking_tokens_num = 0;
	}
}

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

void
html_tokenizer_begin (HTMLTokenizer *t,
                      const gchar *content_type)
{

	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals[HTML_TOKENIZER_BEGIN_SIGNAL], 0, content_type);
}

void
html_tokenizer_set_engine_type (HTMLTokenizer *t,
                                gboolean engine_type)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals[HTML_TOKENIZER_CHANGEENGINE_SIGNAL], 0, engine_type);
}

void
html_tokenizer_change_content_type (HTMLTokenizer *t,const gchar *content_type)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals[HTML_TOKENIZER_CHANGECONTENT_SIGNAL], 0, content_type);
}

void
html_tokenizer_end (HTMLTokenizer *t)
{
	g_return_if_fail (t && HTML_IS_TOKENIZER (t));

	g_signal_emit (t, html_tokenizer_signals[HTML_TOKENIZER_END_SIGNAL], 0);
}

void
html_tokenizer_write (HTMLTokenizer *t,
                      const gchar *str,
                      gsize size)
{
	HTMLTokenizerClass *klass;

	g_return_if_fail (t && HTML_IS_TOKENIZER (t));
	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->write)
		klass->write (t, str, size);
	else
		g_warning ("No write method defined.");
}

gchar *
html_tokenizer_peek_token (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->peek_token)
		return klass->peek_token (t);

	g_warning ("No peek_token method defined.");
	return NULL;

}

const gchar *
html_tokenizer_get_content_type (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->get_content_type)
		return  klass->get_content_type (t);

	g_warning ("No get_content_type method defined.");
	return NULL;

}

gboolean
html_tokenizer_get_engine_type (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t),FALSE);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->get_engine_type)
		return  klass->get_engine_type (t);

	g_warning ("No get_engine_type method defined.");
	return FALSE;
}

gchar *
html_tokenizer_next_token (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->next_token)
		return klass->next_token (t);

	g_warning ("No next_token method defined.");
	return NULL;
}

gboolean
html_tokenizer_has_more_tokens (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	g_return_val_if_fail (t && HTML_IS_TOKENIZER (t), FALSE);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->has_more) {
		return klass->has_more (t);
	}

	g_warning ("No has_more method defined.");
	return FALSE;

}

HTMLTokenizer *
html_tokenizer_clone (HTMLTokenizer *t)
{
	HTMLTokenizerClass *klass;

	if (t == NULL)
		return NULL;
	g_return_val_if_fail (HTML_IS_TOKENIZER (t), NULL);

	klass = HTML_TOKENIZER_CLASS (G_OBJECT_GET_CLASS (t));

	if (klass->clone)
		return klass->clone (t);

	g_warning ("No clone method defined.");
	return NULL;
}
