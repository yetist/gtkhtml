/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
   Copyright (C) 1997 Torben Weis (weis@kde.org)
   Copyright (C) 1999, 2000 Helix Code, Inc.
   
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
#ifndef _HTMLTEXT_H_
#define _HTMLTEXT_H_

#include <unicode.h>
#include "htmlobject.h"
#include "htmlfontmanager.h"
#include "htmlcolor.h"
#include "htmlinterval.h"

typedef struct _HTMLText HTMLText;
typedef struct _HTMLTextClass HTMLTextClass;

#define HTML_TEXT(x) ((HTMLText *)(x))
#define HTML_TEXT_CLASS(x) ((HTMLTextClass *)(x))

struct _HTMLText {
	HTMLObject object;
	
	gchar *text;
	guint text_len;

	GtkHTMLFontStyle  font_style;
	HTMLFontFace     *face;
	HTMLColor        *color;

#ifdef GTKHTML_HAVE_PSPELL
	GList *spell_errors;
#endif
};

struct _HTMLTextClass {
	HTMLObjectClass object_class;

        guint        	   (* insert_text)    (HTMLText *text, HTMLEngine *engine,
					       guint offset, const gchar *p, guint len);
        guint        	   (* remove_text)    (HTMLText *text, HTMLEngine *engine,
					       guint offset, guint len);
        void         	   (* queue_draw)     (HTMLText *text, HTMLEngine *engine,
					       guint offset, guint len);
        	      
        HTMLText     	 * (* split)          (HTMLText *text, guint offset);
        HTMLText     	 * (* extract_text)   (HTMLText *text, guint offset, gint len);
        void         	   (* merge)          (HTMLText *text, HTMLText *other, gboolean prepend);
        gboolean     	   (* check_merge)    (HTMLText *self, HTMLText *text);

	GtkHTMLFontStyle   (* get_font_style) (const HTMLText *text);
	HTMLColor        * (* get_color)      (HTMLText *text, HTMLPainter *painter);

	void               (* set_font_style) (HTMLText *text, HTMLEngine *engine, GtkHTMLFontStyle style);
	void               (* set_color)      (HTMLText *text, HTMLEngine *engine, HTMLColor *color);
};


extern HTMLTextClass html_text_class;


void              html_text_type_init       (void);
void              html_text_class_init      (HTMLTextClass    *klass,
					     HTMLType          type,
					     guint             object_size);
void              html_text_init            (HTMLText         *text_object,
					     HTMLTextClass    *klass,
					     const gchar      *text,
					     gint              len,
					     GtkHTMLFontStyle  font_style,
					     HTMLColor        *color);
HTMLObject       *html_text_new             (const gchar      *text,
					     GtkHTMLFontStyle  font_style,
					     HTMLColor        *color);
HTMLObject       *html_text_new_with_len    (const gchar      *text,
					     gint              len,
					     GtkHTMLFontStyle  font_style,
					     HTMLColor        *color);
guint             html_text_insert_text     (HTMLText         *text,
					     HTMLEngine       *engine,
					     guint             offset,
					     const gchar      *p,
					     guint             len);
HTMLText         *html_text_extract_text    (HTMLText         *text,
					     guint             offset,
					     gint              len);
guint             html_text_remove_text     (HTMLText         *text,
					     HTMLEngine       *engine,
					     guint             offset,
					     guint             len);
void              html_text_queue_draw      (HTMLText         *text,
					     HTMLEngine       *engine,
					     guint             offset,
					     guint             len);
HTMLText         *html_text_split           (HTMLText         *text,
					     guint             offset);
void              html_text_merge           (HTMLText         *text,
					     HTMLText         *other,
					     gboolean          prepend);
gboolean          html_text_check_merge     (HTMLText         *self,
					     HTMLText         *text);
GtkHTMLFontStyle  html_text_get_font_style  (const HTMLText   *text);
HTMLColor        *html_text_get_color       (HTMLText         *text,
					     HTMLPainter      *painter);
void              html_text_set_font_style  (HTMLText         *text,
					     HTMLEngine       *engine,
					     GtkHTMLFontStyle  style);
void              html_text_set_color       (HTMLText         *text,
					     HTMLEngine       *engine,
					     HTMLColor        *color);
void              html_text_set_text        (HTMLText         *text,
					     const gchar      *new_text);
void              html_text_set_font_face   (HTMLText         *text,
					     HTMLFontFace     *face);
gint              html_text_get_nb_width    (HTMLText         *text,
					     HTMLPainter      *painter,
					     gboolean          begin);
guint             html_text_get_bytes       (HTMLText         *text);
guint             html_text_get_index       (HTMLText         *text,
					     guint             offset);
unicode_char_t    html_text_get_char        (HTMLText         *text,
					     guint             offset);

#ifdef GTKHTML_HAVE_PSPELL

struct _SpellError {
	guint off;
	guint len;
};
typedef struct _SpellError SpellError;

void  html_text_spell_errors_clear           (HTMLText     *text);
void  html_text_spell_errors_clear_interval  (HTMLText     *text,
					      HTMLInterval *i);
void  html_text_spell_errors_add             (HTMLText     *text,
					      guint         off,
					      guint         len);
#endif

#endif /* _HTMLTEXT_H_ */
