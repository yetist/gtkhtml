/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#ifndef _HTMLENGINE_H_
#define _HTMLENGINE_H_

typedef struct _HTMLEngine HTMLEngine;
typedef struct _HTMLEngineClass HTMLEngineClass;

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gtkhtml.h"
#include "htmltokenizer.h"
#include "htmlclue.h"
#include "htmlfont.h"
#include "htmlstack.h"
#include "htmlsettings.h"
#include "htmlpainter.h"
#include "htmlurl.h"
#include "stringtokenizer.h"

#define TYPE_HTML_ENGINE                 (html_engine_get_type ())
#define HTML_ENGINE(obj)                  (GTK_CHECK_CAST ((obj), TYPE_HTML_ENGINE, HTMLEngine))
#define HTML_ENGINE_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), TYPE_HTML_ENGINE, HTMLEngineClass))
#define IS_HTML_ENGINE(obj)              (GTK_CHECK_TYPE ((obj), TYPE_HTML_ENGINE))
#define IS_HTML_ENGINE_CLASS(klass)      (GTK_CHECK_CLASS_TYPE ((klass), TYPE_HTML_ENGINE))

#define LEFT_BORDER 10
#define RIGHT_BORDER 20
#define TOP_BORDER 10
#define BOTTOM_BORDER 10

enum _HTMLGlossaryEntry {
	HTML_GLOSSARY_DL = 1,
	HTML_GLOSSARY_DD = 2
};
typedef enum _HTMLGlossaryEntry HTMLGlossaryEntry;

typedef struct _HTMLBlockStackElement HTMLBlockStackElement;

struct _HTMLEngine {
	GtkObject parent;

	HTMLURL *actualURL;

	gboolean show_cursor;

	gboolean parsing;
	HTMLTokenizer *ht;
	StringTokenizer *st;
	HTMLObject *clue;
	
	HTMLObject *flow;

	gint leftBorder;
	gint rightBorder;
	gint topBorder;
	gint bottomBorder;

	/* Size of current indenting */
	gint indent;

	/* For the widget */
	gint width;
	gint height;

	gboolean vspace_inserted;
	gboolean bodyParsed;

	HTMLHAlignType divAlign;

	/* Number of tokens parsed in the current time-slice */
	gint parseCount;
	gint granularity;

	/* Offsets */
	gint x_offset, y_offset;

	gboolean inTitle;
	gboolean inPre;

	gboolean bold;
	gboolean italic;
	gboolean underline;
 
	gint fontsize;
	
	HTMLStack *fs;		/* Font stack, elements are HTMLFonts.  */
	HTMLStack *cs;		/* Color stack, elements are GdkColors.  */

	HTMLURL *url;
	gchar *target;

	HTMLPainter *painter;
	HTMLBlockStackElement *blockStack;

	HTMLSettings *settings;
	HTMLSettings *defaultSettings;

	/* timer id to schedule paint events */
	guint updateTimer;

	/* timer id for parsing routine */
	guint timerId;

	/* Should the background be painted? */
	gboolean bDrawBackground;

	GString *title;

	gboolean writing;

	/* The background pixmap, an HTMLImagePointer */
        gpointer bgPixmapPtr;
  
	/* The background color */
	GdkColor bgColor;

	/* Stack of lists currently active */
	HTMLStack *listStack;

	/* the widget, used for signal emission*/
	GtkHTML *widget;

        gpointer image_factory;

	HTMLStack *glossaryStack; /* HTMLGlossaryEntry */

	/*
	 * This list holds strings which are displayed in the view,
	 * but are not actually contained in the HTML source.
	 * e.g. The numbers in an ordered list.
	 */
	GList *tempStrings;
};

struct _HTMLEngineClass {
	GtkObjectClass parent_class;
	
	void (* title_changed) (HTMLEngine *engine);
	void (* set_base) (HTMLEngine *engine, const gchar *base);
	void (* set_base_target) (HTMLEngine *engine, const gchar *base_target);
	void (* load_done) (HTMLEngine *engine);
        void (* url_requested)   (GtkHTML *html, const char *url, GtkHTMLStreamHandle handle);
};


HTMLEngine *html_engine_new (void);
GtkHTMLStreamHandle html_engine_begin (HTMLEngine *p, const char *url);
void        html_engine_schedule_update (HTMLEngine *p);
gchar      *html_engine_parse_body (HTMLEngine *p, HTMLObject *clue, const gchar *end[], gboolean toplevel);
void        html_engine_parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse (HTMLEngine *p);
HTMLFont *  html_engine_get_current_font (HTMLEngine *p);
void        html_engine_select_font (HTMLEngine *e);
void        html_engine_pop_font (HTMLEngine *e);
void        html_engine_insert_text (HTMLEngine *e, gchar *str, HTMLFont *f);
void        html_engine_calc_size (HTMLEngine *p);
void        html_engine_new_flow (HTMLEngine *p, HTMLObject *clue);
void        html_engine_draw (HTMLEngine *e, gint x, gint y, gint width, gint height);
gboolean    html_engine_insert_vspace (HTMLEngine *e, HTMLObject *clue, gboolean vspace_inserted);
void        html_engine_pop_color (HTMLEngine *e);
gboolean    html_engine_set_named_color (HTMLEngine *p, GdkColor *c, const gchar *name);
void        html_painter_set_background_color (HTMLPainter *painter, GdkColor *color);
gint        html_engine_get_doc_height (HTMLEngine *p);
void        html_engine_stop_parser (HTMLEngine *e);
void        html_engine_calc_absolute_pos (HTMLEngine *e);
gchar      *html_engine_canonicalize_url (HTMLEngine *e, const char *in_url);
const gchar *html_engine_get_link_at (HTMLEngine *e, gint x, gint y);
void	    html_engine_show_cursor (HTMLEngine *e, gboolean show);

#endif /* _HTMLENGINE_H_ */
