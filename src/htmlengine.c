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
#include <stdlib.h>
#include <ctype.h>
#include "gtkhtml.h"
#include "htmlbullet.h"
#include "htmlengine.h"
#include "htmlrule.h"
#include "htmlobject.h"
#include "htmlclueh.h"
#include "htmlcluev.h"
#include "htmlcluealigned.h"
#include "htmlvspace.h"
#include "htmlimage.h"
#include "htmllist.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltextmaster.h"
#include "htmltext.h"
#include "htmlhspace.h"
#include "htmlclueflow.h"
#include "htmlcolor.h"
#include "htmlstack.h"
#include "stringtokenizer.h"

guint           html_engine_get_type (void);
static void     html_engine_class_init (HTMLEngineClass *klass);
static void     html_engine_init (HTMLEngine *engine);
static gboolean html_engine_timer_event (HTMLEngine *e);
static gboolean html_engine_update_event (HTMLEngine *e);
static void html_engine_schedule_update (HTMLEngine *e, gboolean clear);
static GtkLayoutClass *parent_class = NULL;
guint html_engine_signals [LAST_SIGNAL] = { 0 };


#define TIMER_INTERVAL 30
#define INDENT_SIZE 30

extern gint defaultFontSizes [7];

enum ID { ID_FONT, ID_B, ID_HEADER, ID_U, ID_UL, ID_TD };

guint
html_engine_get_type (void)
{
	static guint html_engine_type = 0;

	if (!html_engine_type) {
		static const GtkTypeInfo html_engine_info = {
			"HTMLEngine",
			sizeof (HTMLEngine),
			sizeof (HTMLEngineClass),
			(GtkClassInitFunc) html_engine_class_init,
			(GtkObjectInitFunc) html_engine_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		html_engine_type = gtk_type_unique (GTK_TYPE_OBJECT, &html_engine_info);
	}

	return html_engine_type;
}

static void
html_engine_class_init (HTMLEngineClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	html_engine_signals [TITLE_CHANGED] = 
		gtk_signal_new ("title_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, title_changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, html_engine_signals, LAST_SIGNAL);
}

static void
html_engine_init (HTMLEngine *engine)
{

	engine->ht = html_tokenizer_new ();
	engine->st = string_tokenizer_new ();
	engine->fs = html_font_stack_new ();
	engine->cs = html_color_stack_new ();
	engine->listStack = html_list_stack_new ();
	engine->settings = html_settings_new ();
	engine->painter = html_painter_new ();

	engine->leftBorder = LEFT_BORDER;
	engine->rightBorder = RIGHT_BORDER;
	engine->topBorder = TOP_BORDER;
	engine->bottomBorder = BOTTOM_BORDER;
	
	/* Set up parser functions */
	engine->parseFuncArray[0] = NULL;
	engine->parseFuncArray[1] = html_engine_parse_b;
	engine->parseFuncArray[2] = NULL;
	engine->parseFuncArray[3] = NULL;
	engine->parseFuncArray[4] = NULL;
	engine->parseFuncArray[5] = html_engine_parse_f;
	engine->parseFuncArray[6] = NULL;
	engine->parseFuncArray[7] = html_engine_parse_h;
	engine->parseFuncArray[8] = html_engine_parse_i;
	engine->parseFuncArray[9] = NULL;
	engine->parseFuncArray[10] = NULL;
	engine->parseFuncArray[11] = html_engine_parse_l;
	engine->parseFuncArray[12] = NULL;
	engine->parseFuncArray[13] = NULL;
	engine->parseFuncArray[14] = NULL;
	engine->parseFuncArray[15] = html_engine_parse_p;
	engine->parseFuncArray[16] = NULL;
	engine->parseFuncArray[17] = NULL;
	engine->parseFuncArray[18] = NULL;
	engine->parseFuncArray[19] = html_engine_parse_t;
	engine->parseFuncArray[20] = html_engine_parse_u;
	engine->parseFuncArray[21] = NULL;
	engine->parseFuncArray[22] = NULL;
	engine->parseFuncArray[23] = NULL;
	engine->parseFuncArray[24] = NULL;
	engine->parseFuncArray[25] = NULL;
}

HTMLEngine *
html_engine_new (void)
{
	HTMLEngine *engine;

	engine = gtk_type_new (html_engine_get_type ());

	return engine;
}


#if 0
HTMLEngine *
html_engine_new (void)
{
	HTMLEngine *p;
	
	p = g_new0 (HTMLEngine, 1);
	p->ht = html_tokenizer_new ();
	p->st = string_tokenizer_new ();
	p->fs = html_font_stack_new ();
	p->cs = html_color_stack_new ();
	p->listStack = html_list_stack_new ();
	p->settings = html_settings_new ();
	p->painter = html_painter_new ();

	p->leftBorder = LEFT_BORDER;
	p->rightBorder = RIGHT_BORDER;
	p->topBorder = TOP_BORDER;
	p->bottomBorder = BOTTOM_BORDER;
	
	/* Set up parser functions */
	p->parseFuncArray[0] = NULL;
	p->parseFuncArray[1] = html_engine_parse_b;
	p->parseFuncArray[2] = NULL;
	p->parseFuncArray[3] = NULL;
	p->parseFuncArray[4] = NULL;
	p->parseFuncArray[5] = html_engine_parse_f;
	p->parseFuncArray[6] = NULL;
	p->parseFuncArray[7] = html_engine_parse_h;
	p->parseFuncArray[8] = html_engine_parse_i;
	p->parseFuncArray[9] = NULL;
	p->parseFuncArray[10] = NULL;
	p->parseFuncArray[11] = html_engine_parse_l;
	p->parseFuncArray[12] = NULL;
	p->parseFuncArray[13] = NULL;
	p->parseFuncArray[14] = NULL;
	p->parseFuncArray[15] = html_engine_parse_p;
	p->parseFuncArray[16] = NULL;
	p->parseFuncArray[17] = NULL;
	p->parseFuncArray[18] = NULL;
	p->parseFuncArray[19] = html_engine_parse_t;
	p->parseFuncArray[20] = html_engine_parse_u;
	p->parseFuncArray[21] = NULL;
	p->parseFuncArray[22] = NULL;
	p->parseFuncArray[23] = NULL;
	p->parseFuncArray[24] = NULL;
	p->parseFuncArray[25] = NULL;

	return p;
}
#endif

void
html_engine_calc_absolute_pos (HTMLEngine *e)
{
	if (e->clue)
		e->clue->calc_absolute_pos (e->clue, 0, 0);
}

void
html_engine_draw_background (HTMLEngine *e, gint xval, gint yval, gint x, gint y, gint w, gint h)
{
	gint xoff = 0;
	gint yoff = 0;
	gint pw, ph, yp, xp;
	gint xOrigin, yOrigin;

	/* FIXME: Should check if background should be drawn */
	
	xoff = xval;
	yoff = yval;
	xval = e->x_offset;
	yval = e->y_offset;

	pw = e->bgPixmap->art_pixbuf->width;
	ph = e->bgPixmap->art_pixbuf->height;

	xOrigin = x / pw*pw - xval % pw;
	yOrigin = y / ph*ph - yval % ph;
	
	for (yp = yOrigin; yp < y + h; yp += ph) {
		for (xp = xOrigin; xp < x + w; xp += pw) {
			html_painter_draw_pixmap (e->painter, 
						  xp - xoff, 
						  yp - yoff, e->bgPixmap);
		}
	}
}

void
html_engine_stop_parser (HTMLEngine *e)
{
	if (!e->parsing)
		return;

	if (e->timerId != 0)
		gtk_timeout_remove (e->timerId);
	
	e->parsing = FALSE;
}

void
html_engine_begin (HTMLEngine *p, gchar *url)
{
	html_tokenizer_begin (p->ht);
	
	html_engine_free_block (p); /* Clear the block stack */

	if (url != 0) {
		p->actualURL = g_strdup (url);
		p->baseURL = g_dirname (url);
		g_print ("baseURL: %s\n", p->baseURL);
		g_print ("actualURL: %s\n", p->actualURL);
	}

	html_engine_stop_parser (p);
	p->writing = TRUE;
}

void
html_engine_write (HTMLEngine *e, gchar *buffer)
{
	if (buffer == 0)
		return;

	html_tokenizer_write (e->ht, buffer);
	
	if (e->parsing && e->timerId == 0) {
		e->timerId = gtk_timeout_add (TIMER_INTERVAL,
					      (GtkFunction) html_engine_timer_event,
					      e);
	}
}

static void
html_engine_schedule_update (HTMLEngine *e, gboolean clear)
{
	if (clear)
		e->bDrawBackground = TRUE;

	if (!e->updateTimer) {
		e->bDrawBackground = clear;
		gtk_timeout_add (100, html_engine_update_event, e);
	}

}

static gboolean
html_engine_update_event (HTMLEngine *e)
{
	html_engine_draw (e, 0, 0, e->width, e->height);

	return FALSE;
}

static gboolean
html_engine_timer_event (HTMLEngine *e)
{
	static const gchar *end[] = { "</body>", 0};
	HTMLFont *oldFont;
	gint lastHeight;

	/* Killing timer */
	gtk_timeout_remove (e->timerId);
	e->timerId = 0;
	
	/* Has more tokens? */
	if (!html_tokenizer_has_more_tokens (e->ht) && e->writing)
		return FALSE;

	g_print ("has more tokens!\n");
	
	/* Storing font info */
	oldFont = e->painter->font;

	/* Setting font */
	html_painter_set_font (e->painter, html_font_stack_top (e->fs));

	/* Getting height */
	lastHeight = html_engine_get_doc_height (e);

	e->parseCount = e->granularity;

	/* Parsing body height */
	if (html_engine_parse_body (e, e->clue, end, TRUE))
	  	html_engine_stop_parser (e);

	g_print ("calcing size in htmlengine\n");
	html_engine_calc_size (e);
	html_engine_calc_absolute_pos (e);

	/* Restoring font */
	html_painter_set_font (e->painter, oldFont);

	html_painter_set_background_color (e->painter, e->settings->bgcolor);
	gdk_window_clear (e->painter->window);
	html_engine_draw (e, 0, 0, e->width, e->height);
	
	/* If the visible rectangle was not filled before the parsing and
	   if we have something to display in the visible area now then repaint. */
	if (lastHeight - e->y_offset < e->height * 2 && html_engine_get_doc_height (e) - e->y_offset > 0)
		html_engine_schedule_update (e, FALSE);

	if (!e->parsing) {
		/* Parsing done */
		
		/* Is y_offset too big? */
		if (html_engine_get_doc_height (e) - e->y_offset < e->height) {
			e->y_offset = html_engine_get_doc_height (e) - e->height;
			if (e->y_offset < 0)
				e->y_offset = 0;
		}
		
		/* Adjust the scrollbars */
		gtk_html_calc_scrollbars (e->widget);

	}
	else {
		e->timerId = gtk_timeout_add (TIMER_INTERVAL,
					      (GtkFunction)html_engine_timer_event,
					      e);
	}
		
	return TRUE;
}

void
html_engine_end (HTMLEngine *e)
{
	e->writing = FALSE;

	html_tokenizer_end (e->ht);

}

void
html_engine_draw (HTMLEngine *e, gint x, gint y, gint width, gint height)
{
	gint tx, ty;

	tx = -e->x_offset + e->leftBorder;
	ty = -e->y_offset + e->topBorder;

	/*
	if (e->bgPixmap)
		html_engine_draw_background (e, e->x_offset, e->y_offset, 
		0, 0, e->width, e->height);*/

	if (e->clue)
		e->clue->draw (e->clue, e->painter,
			       x - e->x_offset,
			       y + e->y_offset - e->topBorder,
			       width,
			       height,
			       tx, ty);
}

gint
html_engine_get_doc_height (HTMLEngine *p)
{
	gint height;

	if (p->clue) {
		height = p->clue->ascent;
		height += p->clue->descent;
		height += p->topBorder;
		height += p->bottomBorder;

		return height;
	}
	
	return 0;
}

void
html_engine_calc_size (HTMLEngine *p)
{
	gint max_width, min_width;

	if (p->clue == 0)
		return;

	p->clue->reset (p->clue);

	max_width = p->width - p->leftBorder - p->rightBorder;

	/* Set the clue size */
	p->clue->width = p->width - p->leftBorder - p->rightBorder;

	min_width = p->clue->calc_min_width (p->clue);

	if (min_width > max_width) {
		max_width = min_width;
	}

	g_print ("set max width\n");
	p->clue->set_max_width (p->clue, max_width);
	g_print ("calc size\n");
	p->clue->calc_size (p->clue, NULL);
	p->clue->x = 0;
	p->clue->y = p->clue->ascent;


}

void
html_engine_new_flow (HTMLEngine *p, HTMLObject *clue)
{
	
	/* FIXME: If inpre */
	p->flow = html_clueflow_new (0, 0, clue->max_width, 100);

	HTML_CLUEFLOW (p->flow)->indent = HTML_CLUEFLOW (clue)->indent;
	HTML_CLUE (p->flow)->halign = p->divAlign;
	html_clue_append (clue, p->flow);
}

void
html_engine_parse (HTMLEngine *p)
{
	HTMLFont *f;
	GdkColor *c;

	html_engine_stop_parser (p);

	if (p->clue) {
		p->clue->destroy (p->clue);
	}
	
	p->flow = 0;

	p->clue = html_cluev_new (0, 0, p->width - p->leftBorder - p->rightBorder, 100);
	HTML_CLUE (p->clue)->valign = Top;
	HTML_CLUE (p->clue)->halign = Left;
	
	/* Initialize the font stack with the default font */
	p->italic = FALSE;
	p->underline = FALSE;
	p->weight = Normal;
	p->fontsize = p->settings->fontBaseSize;

	f = html_font_new ("times", p->settings->fontBaseSize, defaultFontSizes, p->weight, p->italic, p->underline);

	html_font_stack_push (p->fs, f);

	/* FIXME: is this nice to do? */
	c = g_new0 (GdkColor, 1);
	gdk_color_black (gdk_window_get_colormap (p->painter->window), c);
	html_color_stack_push (p->cs, c);
	f->textColor = html_color_stack_top (p->cs);

	
	p->parsing = TRUE;
	p->vspace_inserted = TRUE;

	p->timerId = gtk_timeout_add (TIMER_INTERVAL,
				      (GtkFunction) html_engine_timer_event,
				      p);
}

gboolean
html_engine_insert_vspace (HTMLEngine *e, HTMLObject *clue, gboolean vspace_inserted)
{
	HTMLObject *f, *t;

	if (!vspace_inserted) {
		f = html_clueflow_new (0, 0, clue->max_width, 100);
		html_clue_append (clue, f);

		/* FIXME: correct font size */
		t = html_vspace_new (defaultFontSizes[e->settings->fontBaseSize], CNone);
		html_clue_append (f, t);
		
		e->flow = NULL;
	}
	
	return TRUE;
}

void
html_engine_push_block (HTMLEngine *e, gint id, gint level, HTMLBlockFunc exitFunc, 
			gint miscData1, gint miscData2)
{
	HTMLStackElement *elem;

	elem = html_stack_element_new (id, level, exitFunc, miscData1, miscData2, e->blockStack);
	e->blockStack = elem;
}

void
html_engine_free_block (HTMLEngine *e)
{
	HTMLStackElement *elem = e->blockStack;
	while (elem != 0) {
		HTMLStackElement *tmp = elem;
		elem = elem->next;
		/* Fixme: destroy? */
		g_free (tmp);
	}
	e->blockStack = 0;
}

void
html_engine_pop_block (HTMLEngine *e, gint id, HTMLObject *clue)
{
	HTMLStackElement *elem, *tmp;
	gint maxLevel;

	elem = e->blockStack;
	maxLevel = 0;

	while ((elem != 0) && (elem->id != id)) {
		if (maxLevel < elem->level) {
			maxLevel = elem->level;
		}
		elem = elem->next;
	}
	if (elem == 0)
		return;
	if (maxLevel > elem->level)
		return;
	
	elem = e->blockStack;
	
	while (elem) {
		tmp = elem;
		if (elem->exitFunc != 0)
			(*(elem->exitFunc))(e, clue, elem);
		if (elem->id == id) {
			e->blockStack = elem->next;
			elem = 0;
		}
		else {
			elem = elem->next;
		}
		/* FIXME: Something else? */
		g_free (tmp);
	}
				
}

void
html_engine_select_font (HTMLEngine *e)
{
	HTMLFont *f;

	if (e->fontsize <0)
		e->fontsize = 0;
	else if (e->fontsize >= MAXFONTSIZES)
		e->fontsize = MAXFONTSIZES - 1;
	
	f = html_font_new (html_font_stack_top (e->fs)->family, e->fontsize, defaultFontSizes,
			   e->weight, e->italic, e->underline);
	f->textColor = html_color_stack_top (e->cs);

	html_font_stack_push (e->fs, f);
	html_painter_set_font (e->painter, f);
}

void
html_engine_pop_color (HTMLEngine *e)
{
	html_color_stack_pop (e->cs);

	/* FIXME: Check for empty */
}

void
html_engine_pop_font (HTMLEngine *e)
{
	html_font_stack_pop (e->fs);
	
	/* FIXME: Check for empty */
	html_painter_set_font (e->painter, html_font_stack_top (e->fs));

	e->fontsize = html_font_stack_top (e->fs)->size;
	e->weight = html_font_stack_top (e->fs)->weight;
	e->italic = html_font_stack_top (e->fs)->italic;
	e->underline = html_font_stack_top (e->fs)->underline;
}

void
html_engine_insert_text (HTMLEngine *e, gchar *str, HTMLFont *f)
{
	enum {unknown, fixed, variable} textType = unknown;
	int i = 0;
	HTMLObject *obj;
	gchar *remainingStr = 0;
	gboolean insertSpace = FALSE;
	gboolean insertNBSP = FALSE;
	gboolean insertBlock = FALSE;
	
	for (;;) {
		if (str[i] == 0x20) {
			/* Normal space */
			if (textType == fixed) {
				/* We have a normal space in a block of fixed text.
				   We need to split the text and insert a separate normal space. */
				str[i] = 0x00; /* End of string */
				remainingStr = &(str [i+1]);
				insertBlock = TRUE;
				insertSpace = TRUE;
			}
			else {
				/* We have a normal space, if this is the first
				   character, we insert a normal space and continue */
				if (i == 0) {
					if (str [i+1] == 0x00) {
						str++;
						remainingStr = 0;
					}
					else {
						str [i] = 0x00; /* End of string */
						remainingStr = str+1;
					}
					insertBlock = TRUE;
					insertSpace = TRUE;
				}
				else if (str [i+1] == 0x00) {
					str[i] = 0x00;
					remainingStr = 0;
					insertBlock = TRUE;
					insertSpace = TRUE;
				}
				else {
					textType = variable;
				}
			}
		}
		else if (str[i] == 0x00) {
			/* End of string */
			insertBlock = TRUE;
			remainingStr = 0;
		}
		if (insertBlock) {
			if (*str) {
				if (textType == variable) {
					obj = html_text_master_new (str, f, e->painter);
					html_clue_append (e->flow, obj);
				}
				else {
					obj = html_text_new (str, f, e->painter);
					html_clue_append (e->flow, obj);
				}
			}
			if (insertSpace) {
				obj = html_hspace_new (f, e->painter, FALSE);
				html_clue_append (e->flow, obj);
			}
			else if (insertNBSP) {
				g_print ("insertNBSP\n");
			}
		
			str = remainingStr;

			if ((str == 0) || (*str == 0x00))
				return; /* Finished */
			i = 0;
			textType = unknown;
			insertBlock = FALSE;
			insertSpace = FALSE;
			insertNBSP = FALSE;

		}
		else {
			i++;
		}
	}
}

HTMLFont *
html_engine_get_current_font (HTMLEngine *p)
{
	return html_font_stack_top (p->fs);
}

void
html_engine_set_named_color (HTMLEngine *p, GdkColor *c, gchar *name)
{
	gdk_color_parse (name, c);
	gdk_colormap_alloc_color (gdk_window_get_colormap (p->painter->window),
			    c, FALSE, TRUE);

}

gchar *
html_engine_parse_body (HTMLEngine *p, HTMLObject *clue, const gchar *end[], gboolean toplevel)
{
	gchar *str;

	while (html_tokenizer_has_more_tokens (p->ht) && p->parsing) {
		str = html_tokenizer_next_token (p->ht);

		if (*str == '\0')
			continue;

		if (*str != TAG_ESCAPE) {

			if (p->inTitle) {
				g_string_append (p->title, str);
			}
			else {
				p->vspace_inserted = FALSE;
				
				if (!p->flow)
					html_engine_new_flow (p, clue);
				
				html_engine_insert_text (p, str, html_engine_get_current_font (p));
			}
		}
		else {
			gint i  = 0;
			str++;
			
			while (end [i] != 0) {
				if (strncasecmp (str, end[i], strlen(end[i])) == 0) {
					return str;
				}
				i++;
			}

			html_engine_parse_one_token (p, clue, str);
		}
	}

	if (!html_tokenizer_has_more_tokens (p->ht) && toplevel && 
	    !p->writing)
		html_engine_stop_parser (p);

	return 0;
}

void
html_engine_parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (*str == '<') {
		gint indx;
		
		str++;
		
		if (*str == '/')
			indx = *(str + 1) - 'a';
		else
			indx = *str - 'a';

		if (indx >= 0 && indx < 26) {
			/* FIXME: This should be removed */
			if (p->parseFuncArray[indx]) {
				(*(p->parseFuncArray[indx]))(p, clue, str);
			}
			else {
			  /*				g_print ("Unsupported tag %s\n", str);*/
			}
		}

	}
}

void
html_engine_parse_b (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "body", 4) == 0) {
		GdkColor *bgcolor = g_new0 (GdkColor, 1);
		gboolean bgColorSet = FALSE;

		if (e->bodyParsed)
			return;

		e->bodyParsed = TRUE;
		
		string_tokenizer_tokenize (e->st, str + 5, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			if (strncasecmp (token, "bgcolor=", 8) == 0) {
				html_engine_set_named_color (e, bgcolor, token + 8);
				g_print ("bgcolor is set\n");
				bgColorSet = TRUE;
			}
			else if (strncasecmp (token, "background=", 11) == 0) {
				gchar *filename = g_strdup_printf ("%s/%s", e->baseURL, token + 11);
				g_print ("should load: %s\n", filename);
				e->bgPixmap = gdk_pixbuf_new_from_file (filename);
				g_free (filename);
				
				e->bgPixmapSet = (e->bgPixmap != 0);
			}
		}
		
		if (!bgColorSet) {
			/* FIXME: Do this in a better way */
		}
		else {
			e->settings->bgcolor = bgcolor;
			html_painter_set_background_color (e->painter, e->settings->bgcolor);			
		}
	}
	else if (strncmp (str, "br", 2) == 0) {
		ClearType clear = CNone;

		if (!e->flow)
			html_engine_new_flow (e, clue);

		html_clue_append (e->flow, html_vspace_new (14, clear));

		e->vspace_inserted = FALSE;
	}
	else if (strncmp (str, "b", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			e->weight = Bold;
			html_engine_select_font (e);
			html_engine_push_block (e, ID_B, 1, 
						html_engine_block_end_font,
						0, 0);
		}
	}
	else if (strncmp (str, "/b", 2) == 0) {
		html_engine_pop_block (e, ID_B, clue);
	}
}

void
html_engine_parse_f (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "font", 4) == 0) {
		GdkColor *color = g_new0 (GdkColor, 1);
		gint newSize = (html_engine_get_current_font (p))->size;

		color = gdk_color_copy (html_color_stack_top (p->cs));

		string_tokenizer_tokenize (p->st, str + 5, " >");

		while (string_tokenizer_has_more_tokens (p->st)) {
			gchar *token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "size=", 5) == 0) {
				gint num = atoi (token + 5);
				if (*(token + 5) == '+' || *(token + 5) == '-')
					newSize = p->settings->fontBaseSize + num;
				else
					newSize = num;
			}
			else if (strncasecmp (token, "color=", 6) == 0) {
				/* FIXME: Have a way to override colors? */
				html_engine_set_named_color (p, color, token + 6);
			}
		}
		p->fontsize = newSize;
		html_color_stack_push (p->cs, color);
		html_engine_select_font (p);
		html_engine_push_block  (p, ID_FONT, 1, html_engine_block_end_color_font, 0, 0);
	}
	else if (strncmp (str, "/font", 5) == 0) {
		html_engine_pop_block (p, ID_FONT, clue);
	}
	
}

void
html_engine_parse_h (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (*(str) == 'h' &&
	    (*(str + 1) == '1')) {
		HAlignType align = p->divAlign;
		p->vspace_inserted = html_engine_insert_vspace (p, clue, p->vspace_inserted);
		
		/* Start a new flow box */
		if (!p->flow)
			html_engine_new_flow (p, clue);
		HTML_CLUE (p->flow)->halign = align;

		switch (str[1]) {
		case '1':
			p->weight = Bold;
			p->fontsize += 3;
			html_engine_select_font (p);
			break;
		}
		html_engine_push_block (p, ID_HEADER, 2, html_engine_block_end_font, TRUE, 0);
	} else if (*(str) == '/' && *(str + 1) == 'h' &&
		   (*(str + 2) == '1')) {
		html_engine_pop_block (p, ID_HEADER, clue);
	}
	else if (strncmp (str, "hr", 2) == 0) {
		gint size = 1;
		gint length = clue->max_width;
		gint percent = 100;
		HAlignType align = p->divAlign;
		HAlignType oldAlign = p->divAlign;
		gboolean shade = TRUE;

		if (p->flow)
			oldAlign = align = HTML_CLUE (p->flow)->halign;

		string_tokenizer_tokenize (p->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			gchar *token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "align=", 6) == 0) {
				if (strcasecmp (token + 6, "left") == 0)
					align = Left;
				else if (strcasecmp (token + 6, "right") == 0)
					align = Right;
				else if (strcasecmp (token + 6, "center") == 0)
					align = HCenter;
			}
			else if (strncasecmp (token, "size=", 5) == 0) {
				size = atoi (token + 5);
			}
			else if (strncasecmp (token, "width=", 6) == 0) {
				if (strchr (token + 6, '%'))
					percent = atoi (token + 6);
				else if (isdigit (*(token + 6))) {
					length = atoi (token + 6);
					percent = 0;
				}
			}
			else if (strncasecmp (token, "noshade", 6) == 0) {
				shade = FALSE;
			}
		}

		html_engine_new_flow (p, clue);
		html_clue_append (p->flow, html_rule_new (length, percent, size, shade));
		HTML_CLUE (p->flow)->halign = align;
		p->flow = 0;
		p->vspace_inserted = FALSE;
		
	}
}

void
html_engine_parse_i (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "img", 3) == 0) {
		gchar *filename = 0;
		HTMLObject *image = 0;
		gchar *token = 0;
		gint width = -1;
		gint height = -1;
		gint percent = 0;
		HAlignType align = None;
		gint border = 0;
		VAlignType valign = VNone;
		
		p->vspace_inserted = FALSE;

		string_tokenizer_tokenize (p->st, str + 4, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "src=", 4) == 0) {
				filename = token + 4;
			}
			else if (strncasecmp (token, "width=", 6) == 0) {
				if (strchr (token + 6, '%'))
					percent = atoi (token + 6);
				else if (isdigit (*(token + 6)))
					width = atoi (token + 6);
			}
			else if (strncasecmp (token, "height=", 7) == 0) {
				height = atoi (token + 7);
			}
			else if (strncasecmp (token, "border=", 7) == 0) {
				border = atoi (token + 7);
			}
			else if (strncasecmp (token, "align=", 6) == 0) {
				if (strcasecmp (token + 6, "left") == 0)
					align = Left;
				else if (strcasecmp (token + 6, "right") == 0)
					align = Right;
				else if (strcasecmp (token + 6, "top") == 0)
					valign = Top;
				else if (strcasecmp (token + 6, "bottom") ==0)
					valign = Bottom;
			}

		}
		if (filename != 0) {
			
			filename = g_strdup_printf ("%s/%s", p->baseURL, filename);
			if (!p->flow)
				html_engine_new_flow (p, clue);

			image = html_image_new (p, filename, clue->max_width, 
						width, height, percent, border);
		}

		if (align == None) {
			if (valign == VNone) {
				html_clue_append (p->flow, image);
			}
		}
		/* We need to put the image in a HTMLClueAligned */
		else {
			HTMLClueAligned *aligned = HTML_CLUEALIGNED (html_cluealigned_new (HTML_CLUE (p->flow), 0, 0, clue->max_width, 100));
			HTML_CLUE (aligned)->halign = align;
			html_clue_append (HTML_OBJECT (aligned),
					  HTML_OBJECT (image));
			html_clue_append (HTML_OBJECT (p->flow),
					  HTML_OBJECT (aligned));
		}
		       
	}
}

void
html_engine_parse_l (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "link", 4) == 0) {
	}
	else if (strncmp (str, "li", 2) == 0) {
		/* FIXME: close anchor and fix color*/
		GdkColor fixme;
		HTMLObject *f, *c, *vc;

		ListType listType = Unordered;
		ListNumType listNumType = Numeric;
		gint listLevel = 1;
		gint itemNumber = 1;
		gint indentSize = INDENT_SIZE;

		if (html_list_stack_count (p->listStack) > 0) {
			listType = (html_list_stack_top (p->listStack))->type;
			listNumType = (html_list_stack_top (p->listStack))->numType;
			itemNumber = (html_list_stack_top (p->listStack))->itemNumber;
			listLevel = html_list_stack_count (p->listStack);
			indentSize = p->indent;
		}
		
		f = html_clueflow_new (0, 0, clue->max_width, 100);
		html_clue_append (clue, f);
		c = html_clueh_new (0, 0, clue->max_width);
		HTML_CLUE (c)->valign = Top;
		html_clue_append (f, c);

		/* Fixed width spacer */
		vc = html_cluev_new (0, 0, indentSize, 0);
		HTML_CLUE (vc)->valign = Top;
		html_clue_append (c, vc);

		switch (listType) {
		case Unordered:
			p->flow = html_clueflow_new (0, 0, vc->max_width, 0);
			HTML_CLUE (p->flow)->halign = Right;
			html_clue_append (vc, p->flow);
			html_clue_append (p->flow, html_bullet_new ((html_font_stack_top (p->fs))->pointSize, listLevel, fixme));
			break;
		default:
			break;

		}
		
		vc = html_cluev_new (0, 0, clue->max_width - indentSize, 100);
		html_clue_append (c, vc);
		p->flow = html_clueflow_new (0,0, vc->max_width, 100);
		html_clue_append (vc, p->flow);

	}
}

void
html_engine_parse_p (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (*(str) == 'p' && ( *(str + 1) == ' ' || *(str + 1) == '>')) {
		HAlignType align = p->divAlign;
		gchar *token;

		/* FIXME: CloseAnchor */
		p->vspace_inserted = html_engine_insert_vspace (p, clue, p->vspace_inserted);
		
		string_tokenizer_tokenize (p->st, (gchar *)(str + 2), " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "align=", 6) == 0) {
				if (strcasecmp (token + 6, "center") == 0)
					align = HCenter;
				else if (strcasecmp (token + 6, "right") == 0)
					align = Right;
				else if (strcasecmp (token + 6, "left") == 0)
					align = Left;
			}
			
		}
		if (align != p->divAlign) {
			if (p->flow == 0) {
				html_engine_new_flow (p, clue);
				HTML_CLUE (p->flow)->halign = align;
			}
		}
	}
	else if (*(str) == '/' && *(str + 1) == 'p' && 
		 (*(str + 2) == ' ' || *(str + 2) == '>')) {
		/* FIXME: CloseAnchor */
		p->vspace_inserted = html_engine_insert_vspace (p, clue, p->vspace_inserted);
	}
}

void
html_engine_parse_t (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "table", 5) == 0) {
		/* FIXME: Close anchor */
		if (!p->vspace_inserted || !p->flow)
			html_engine_new_flow (p, clue);

		html_engine_parse_table (p, p->flow, clue->max_width, str + 6);

		p->flow = 0;
	}
	else if (strncmp (str, "title", 5) == 0) {
		p->inTitle = TRUE;
		p->title = g_string_new ("");
	}
	else if (strncmp (str, "/title", 6) == 0) {
		p->inTitle = FALSE;

		gtk_signal_emit_by_name (GTK_OBJECT (p), "title_changed");
	}
}

void
html_engine_parse_u (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "ul", 2) == 0) {
		ListType type = Unordered;
		return;
		/* FIXME: should close anchor */

		if (html_list_stack_is_empty (p->listStack)) {
			p->vspace_inserted = html_engine_insert_vspace (p, clue, p->vspace_inserted);
			html_engine_push_block (p, ID_UL, 2, html_engine_block_end_list, p->indent, TRUE);
		}
		else {
			html_engine_push_block (p, ID_UL, 2, html_engine_block_end_list, p->indent, FALSE);
		}

		string_tokenizer_tokenize (p->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			gchar *token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "plain", 5) == 0)
				type = UnorderedPlain;
		}
		
		html_list_stack_push (p->listStack, html_list_new (type, Numeric));
		p->indent += INDENT_SIZE;
		p->flow = 0;
	}
	else if (strncmp (str, "/ul", 3) == 0) {
		html_engine_pop_block (p, ID_UL, clue);
	}
	else if (strncmp (str, "u", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			p->underline = TRUE;
			html_engine_select_font (p);
			html_engine_push_block (p, ID_U, 1, html_engine_block_end_font, FALSE, FALSE);
		}
	}
	else if (strncmp (str, "/u", 2) == 0) {
		html_engine_pop_block (p, ID_U, clue);
	}
}

const gchar *
html_engine_parse_table (HTMLEngine *e, HTMLObject *clue, gint max_width,
			 const gchar *attr)
{
	static const gchar *endthtd[] = { "</th", "</td", "</tr", "<th", "<td", "<tr", "</table", "</body", 0 };
	static const gchar *endall[] = { "</caption>", "</table", "<tr", "<td", "<th", "</th", "</td", "</tr","</body", 0 };
	HTMLTable *table;
	const gchar *str = 0;
	gint width = 0;
	gint percent = 0;
	gint padding = 1;
	gint spacing = 2;
	gint border = 1;
	gchar has_cell = 0;
	gboolean done = FALSE;
	gboolean tableTag = TRUE;
	gboolean firstRow = TRUE;
	gboolean noCell = TRUE;
	gboolean tableEntry;
	VAlignType rowvalign = VNone;
	HAlignType rowhalign = None;
	HAlignType align = None;
	HTMLClueV *caption = 0;
	HTMLTableCell *tmpCell = 0;
	VAlignType capAlign = Bottom;
	HAlignType olddivalign = e->divAlign;
	HTMLClue *oldflow = HTML_CLUE (e->flow);
	gint oldindent = e->indent;
	/* FIXME: Colors */

	gint rowSpan;
	gint colSpan;
	gint cellwidth;
	gint cellpercent;
	gboolean fixedWidth;
	VAlignType valign;
	HTMLTableCell *cell;

	g_print ("start parse\n");
	string_tokenizer_tokenize (e->st, attr, " >");
	while (string_tokenizer_has_more_tokens (e->st)) {
		const gchar *token = string_tokenizer_next_token (e->st);
		if (strncasecmp (token, "cellpadding=", 12) == 0) {
			padding = atoi (token + 12);
		}
		else if (strncasecmp (token, "cellspacing=", 12) == 0) {
			spacing = atoi (token + 12);
		}
		else if (strncasecmp (token, "border", 6) == 0) {
			if (*(token + 6) == '=')
				border = atoi (token + 7);
			else
				border = 1;
		}
		else if (strncasecmp (token, "width=", 6) == 0) {
			if (strchr (token + 6, '%'))
				percent = atoi (token + 6);
			else if (strchr (token + 6, '*')) {
				/* Ignore */
			}
			else if (isdigit (*(token + 6)))
				width = atoi (token + 6);
		}
		else if (strncasecmp (token, "align=", 6) == 0) {
			if (strcasecmp (token + 6, "left") == 0)
				align = Left;
			else if (strcasecmp (token + 6, "right") == 0)
				align = Right;
		}
		else if (strncasecmp (token, "bgcolor=", 8) == 0) {
			/* FIXME: table color */
		}
	}

	table = HTML_TABLE (html_table_new (0, 0, max_width, width, 
					    percent, padding,
					    spacing, border));
	e->indent = 0;
	while (!done && html_tokenizer_has_more_tokens (e->ht)) {
		str = html_tokenizer_next_token (e->ht);
		
		/* Every tag starts with an escape character */
		if (str[0] == TAG_ESCAPE) {
			str++;

			tableTag = TRUE;

			for (;;) {
				if (strncmp (str, "</table", 7) == 0) {
					/* FIXME: close anchor */
					done = TRUE;
					break;
				}
				/* FIXME: <caption> support */
				if (strncmp (str, "<tr", 3) == 0) {
					if (!firstRow)
						html_table_end_row (table);
					html_table_start_row (table);
					firstRow = FALSE;
					rowvalign = VNone;
					rowhalign = None;

					string_tokenizer_tokenize (e->st, str + 4, " >");
					while (string_tokenizer_has_more_tokens (e->st)) {
						const gchar *token = string_tokenizer_next_token (e->st);
						if (strncasecmp (token, "valign=", 7) == 0) {
							if (strncasecmp (token + 7, "top", 3) == 0)
								rowvalign = Top;
							else if (strncasecmp (token + 7, "bottom", 6) == 0)
								rowvalign = Bottom;
							else
								rowvalign = VCenter;
						}
						else if (strncasecmp (token, "align", 6) == 0) {
							if (strcasecmp (token + 6, "left") == 0)
								rowhalign = Left;
							else if (strcasecmp (token + 6, "right") == 0)
								rowhalign = Right;
							else if (strcasecmp (token + 6, "center") == 0)
								rowhalign = HCenter;
						}
						else if (strncasecmp (token, "bgcolor=", 8) == 0) {
							/* FIXME: Color support */
						}
					}
					break;
				}

				/* Check for <td> and <th> */
				tableEntry = *str == '<' && *(str + 1) == 't' &&
					(*(str + 2) == 'd' || *(str + 2) == 'h');
				if (tableEntry || noCell) {
					gboolean heading = FALSE;
					noCell = FALSE;

					if (*(str + 2) == 'h') {
						g_print ("<th>");
						heading = TRUE;
					}
					/* <tr> may not be specified for the first row */
					if (firstRow) {
						/* Bad HTML: No <tr> tag present */
						html_table_start_row (table);
						firstRow = FALSE;
					}

					rowSpan = 1;
					colSpan = 1;
					cellwidth = clue->max_width;
					cellpercent = -1;
					fixedWidth = FALSE;
					/* FIXME: Color */
					valign = (rowvalign == VNone ?
						  VCenter : rowvalign);

					if (heading)
						e->divAlign = (rowhalign == None ? 
							       HCenter: rowhalign);
					else
						e->divAlign = (rowhalign == None ?
							       Left: rowhalign);

					if (tableEntry) {
						string_tokenizer_tokenize (e->st, str + 4, " >");
						while (string_tokenizer_has_more_tokens (e->st)) {
							const gchar *token = string_tokenizer_next_token (e->st);
							if (strncasecmp (token, "rowspan=", 8) == 0) {
								rowSpan = atoi (token + 8);
								if (rowSpan < 1)
									rowSpan = 1;
							}
							else if (strncasecmp (token, "colspan=", 8) == 0) {
								colSpan = atoi (token + 8);
								if (colSpan < 1)
									colSpan = 1;
							}
							else if (strncasecmp (token, "valign=", 7) == 0) {
								if (strncasecmp (token + 7, "top", 3) == 0)
									valign = Top;
								else if (strncasecmp (token + 7, "bottom", 6) == 0)
									valign = Bottom;
								else 
									valign = VCenter;
							}
							else if (strncasecmp (token, "align=", 6) == 0) {
								if (strcasecmp (token + 6, "center") == 0)
									e->divAlign = HCenter;
								else if (strcasecmp (token + 6, "right") == 0)
									e->divAlign = Right;
								else if (strcasecmp (token + 6, "left") == 0)
									e->divAlign = Left;
							}
							else if (strncasecmp (token, "width=", 6) == 0) {
								if (strchr (token + 6, '%')) {
									g_print ("percent!\n");
									cellpercent = atoi (token + 6);
								}
								else if (strchr (token + 6, '*')) {
									/* ignore */
								}
								else if (isdigit (*(token + 6))) {
									cellwidth = atoi (token + 6);
									cellpercent = 0;
									fixedWidth = TRUE;
								}
							}
							else if (strncasecmp (token, "bgcolor=", 8) == 0) {
								/* FIXME: COLOR! */
							}
						}
					}

					cell = HTML_TABLE_CELL (html_table_cell_new (0, 0, cellwidth,
								    cellpercent,
								    rowSpan, colSpan,
								    padding));
					/* FIXME: Color */
					HTML_CLUE (cell)->valign = valign;
					if (fixedWidth)
						HTML_OBJECT (cell)->flags |= FixedWidth;
					html_table_add_cell (table, cell);
					has_cell = 1;
					e->flow = 0;
					if (heading) {
						/* FIXME: do heading stuff */
					}
					else if (!tableEntry) {
						/* Put all the junk between <table>
						   and the first table tag into one row */
						html_engine_push_block (e, ID_TD, 3, NULL, 0, 0);
						str = html_engine_parse_body (e, HTML_OBJECT (cell), endall, FALSE);
						html_engine_pop_block (e, ID_TD, HTML_OBJECT (cell));
						html_table_end_row (table);
						html_table_start_row (table);
						
					}
					else {
						/* Ignore <p> and such at the beginning */
						e->vspace_inserted = TRUE;
						html_engine_push_block (e, ID_TD, 3, NULL, 0, 0);
						str = html_engine_parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
						html_engine_pop_block (e, ID_TD, HTML_OBJECT (cell));
					}

					if (str == 0) {
						/* Close table description in case of
						   a malformed table before returning! */
						if (!firstRow)
							html_table_end_row (table);
						html_table_end_table (table);
						/* FIXME: Destroy? */
						g_free (table);
						e->divAlign = olddivalign;
						e->flow = HTML_OBJECT (oldflow);
						/* FIXME: Destroy? */
						g_free (tmpCell);
						return 0;
					}

					if ((strncmp (str, "</td", 4) == 0) ||
					    (strncmp (str, "</th", 4) == 0)) {
						/* HTML ok! */
						break; /* Get next token from 'ht' */
					}
					else {
						/* Bad HTML */
						continue;
					}
				}

				/* Unknown or unhandled table-tag: ignore */
				break;
				
			}
		}
	}
		
	/* Did we catch any illegal HTML */
	if (tmpCell) {
		if (!has_cell) {
			if (firstRow) {
				html_table_start_row (table);
				firstRow = FALSE;
			}
			html_table_add_cell (table, tmpCell);
			has_cell = 1;
		}
		else
				/* FIXME: destroy? */
			g_free (tmpCell);
	}
	
	if (has_cell) {
		/* The ending "</table>" might be missing, so
		   we close the table here ... */
		if (!firstRow)
			html_table_end_row (table);
		html_table_end_table (table);
		if (align != Left && align != Right) {
			html_clue_append (clue, HTML_OBJECT (table));
		}
		else {
			g_print ("Aligned!!!!!\n");
				/* FIXME: Support for aligned tables */
		}
	}
	else {
		/* Remove tables that do not contain any cells */
		/* FIXME: destroy? */
		g_free (table);
	}
	
	e->indent = oldindent;
	e->divAlign = olddivalign;
	e->flow = HTML_OBJECT (oldflow);

	g_print ("Returning: %s\n", str);
	return str;
}


void
html_engine_block_end_font (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem)
{
	html_engine_pop_font (e);
	if (elem->miscData1) {
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
	}
}

void
html_engine_block_end_color_font (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem)
{
	html_engine_pop_color (e);

	html_engine_pop_font (e);
	
}

void
html_engine_block_end_list (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem)
{
	if (elem->miscData2) {
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
	}
	
	/* FIXME: Sane check */

	e->indent = elem->miscData1;
	e->flow = 0;
}

