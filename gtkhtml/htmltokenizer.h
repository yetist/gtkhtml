/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 2000 Helix Code, Inc.
   Copyright (C) 2001 Ximian, Inc.

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

#ifndef _HTMLTOKENIZER_H_
#define _HTMLTOKENIZER_H_

#include <glib-object.h>
#include "htmltypes.h"

#define TAG_ESCAPE 13
#define TAB_SIZE 8

#define HTML_TYPE_TOKENIZER        (html_tokenizer_get_type ())
#define HTML_TOKENIZER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), HTML_TYPE_TOKENIZER, HTMLTokenizer))
#define HTML_TOKENIZER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST ((k), HTML_TYPE_TOKENIZER, HTMLTokenizerClass))
#define HTML_IS_TOKENIZER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), HTML_TYPE_TOKENIZER))
#define HTML_IS_TOKENIZER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), HTML_TYPE_TOKENIZER))

struct _HTMLTokenizerPrivate;

struct _HTMLTokenizer {
	GObject parent;
	struct _HTMLTokenizerPrivate *priv;
};

struct _HTMLTokenizerClass {
	GObjectClass parent_class;

	/* signals */
	void     (*begin)           (HTMLTokenizer *, const gchar *content_type);
	void     (*change)          (HTMLTokenizer *, const gchar *content_type);
	void     (*engine)          (HTMLTokenizer *, gboolean enginetype);
	void     (*end)             (HTMLTokenizer *);

	/* virtual functions */
	void           (*write)      (HTMLTokenizer *, const gchar *string, gsize size);
	gchar         *(*peek_token) (HTMLTokenizer *);
	gchar         *(*next_token) (HTMLTokenizer *);
	const gchar   *(*get_content_type) (HTMLTokenizer *);
	gboolean       (*get_engine_type) (HTMLTokenizer *);
	gboolean       (*has_more)   (HTMLTokenizer *);

	HTMLTokenizer *(*clone)      (HTMLTokenizer *);
};

GType          html_tokenizer_get_type        (void);

HTMLTokenizer *html_tokenizer_new             (void);
void           html_tokenizer_destroy         (HTMLTokenizer *tokenizer);

void           html_tokenizer_begin           (HTMLTokenizer *t,
					       const gchar *content_type);

const gchar *  html_tokenizer_get_content_type(HTMLTokenizer *t);
void           html_tokenizer_change_content_type
					      (HTMLTokenizer *t,
					       const gchar *content_type);

void	       html_tokenizer_set_engine_type (HTMLTokenizer *t,
						   gboolean enginetype);
gboolean       html_tokenizer_get_engine_type (HTMLTokenizer *t);

void           html_tokenizer_write           (HTMLTokenizer *t,
					       const gchar *string,
					       gsize size);
void           html_tokenizer_end             (HTMLTokenizer *t);
gchar *        html_tokenizer_peek_token      (HTMLTokenizer *t);
gchar *        html_tokenizer_next_token      (HTMLTokenizer *t);
gboolean       html_tokenizer_has_more_tokens (HTMLTokenizer *t);

HTMLTokenizer *html_tokenizer_clone           (HTMLTokenizer *t);

/*for convert input code page to -->utf */
GIConv     generate_iconv_from (const gchar * content_type);
/*for convert resulted query to needed encoding <--utf*/
GIConv     generate_iconv_to (const gchar * content_type);
/*convert test to needed encoding*/
gchar *     convert_text_encoding (const GIConv iconv_cd, const gchar * token);
/*validate result g_iconv_open*/
gboolean   is_valid_g_iconv (const GIConv iconv_cd);
#endif /* _HTMLTOKENIZER_H_ */
