/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) 1999 Anders Carlsson (andersca@gnu.org)
    Copyright (C) 1999 Helix Code, Inc.

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

/* FIXME check all the `push_block()'s and `set_font()'s.  */
/* FIXME there are many functions that should be static instead of being
   exported.  I will slowly do this as time permits.  -- Ettore  */
/* Some stuff is not re-initialized properly when clearing and reloading stuff
   from scratch -- Ettore */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "gtkhtml-private.h"
#include "htmlanchor.h"
#include "htmlbullet.h"
#include "htmlengine.h"
#include "htmlrule.h"
#include "htmlobject.h"
#include "htmlclueh.h"
#include "htmlcluev.h"
#include "htmlcluealigned.h"
#include "htmlvspace.h"
#include "htmlimage.h"
#include "htmllinktext.h"
#include "htmllinktextmaster.h"
#include "htmllist.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltextmaster.h"
#include "htmltext.h"
#include "htmlhspace.h"
#include "htmlclueflow.h"
#include "htmlstack.h"
#include "stringtokenizer.h"


static void     html_engine_class_init (HTMLEngineClass *klass);
static void     html_engine_init (HTMLEngine *engine);
static gboolean html_engine_timer_event (HTMLEngine *e);
static gboolean html_engine_update_event (HTMLEngine *e);
static void html_engine_write (GtkHTMLStreamHandle handle, gchar *buffer, size_t size, HTMLEngine *e);
static void html_engine_end (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, HTMLEngine *e);

static void parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str);
static HTMLURL *parse_href (HTMLEngine *e, const gchar *s);


static GtkLayoutClass *parent_class = NULL;

enum {
	SET_BASE_TARGET,
	SET_BASE,
	LOAD_DONE,
	TITLE_CHANGED,
	URL_REQUESTED,
	DRAW_PENDING,
	REDIRECT,
	LAST_SIGNAL
};
	
static guint signals [LAST_SIGNAL] = { 0 };

#define TIMER_INTERVAL 30
#define INDENT_SIZE 30

enum ID {
	ID_ADDRESS, ID_B, ID_BIG, ID_BLOCKQUOTE, ID_CAPTION, ID_CITE, ID_CODE,
	ID_DIR, ID_DIV, ID_EM, ID_FONT, ID_HEADER, ID_I, ID_KBD, ID_OL, ID_PRE,
	ID_SMALL, ID_U, ID_UL, ID_TD, ID_TH, ID_TT, ID_VAR
};


/* Utility functions.  */

static void
close_anchor (HTMLEngine *e)
{
	if (e->url != NULL) {
		html_engine_pop_color (e);
		html_engine_pop_font (e);
		html_url_destroy (e->url);
	}

	/* FIXME */
	e->url = NULL;

	g_free (e->target);
	e->target = NULL;
}

static void
select_font_relative (HTMLEngine *e,
		      gint _relative_font_size)
{
	HTMLFont *top;
	HTMLFont *f;
	gint fontsize;

	e->fontsize = e->settings->fontBaseSize + _relative_font_size;

	top = html_stack_top (e->fs);
	if ( top == NULL) {
		fontsize = e->settings->fontBaseSize;
		return;
	}

	if ( e->fontsize < 0 )
		e->fontsize = 0;
	else if ( e->fontsize >= HTML_NUM_FONT_SIZES )
		e->fontsize = HTML_NUM_FONT_SIZES - 1;

	f = html_font_new ( top->family, e->fontsize,
			    e->settings->fontSizes,
			    e->bold, e->italic, e->underline);
	html_font_set_color (f, html_stack_top (e->cs));

	html_stack_push (e->fs, f);
	html_painter_set_font (e->painter, f);
}

static void
select_font_full (HTMLEngine *e,
		  const gchar *_fontfamily,
		  guint _fontsize,
		  guint _bold,
		  gboolean _italic,
		  gboolean _underline)
{

	HTMLFont *f;

	if ( _fontsize < 0 )
		_fontsize = 0;
	else if ( _fontsize >= HTML_NUM_FONT_SIZES )
		_fontsize = HTML_NUM_FONT_SIZES - 1;

	f = html_font_new ( _fontfamily, _fontsize, e->settings->fontSizes,
			    _bold, _italic, _underline);

	html_font_set_color (f, html_stack_top (e->cs));

	html_stack_push (e->fs, f);
	html_painter_set_font (e->painter, f);
}

/* FIXME this implementation is a bit lame.  :-) */
static gchar *
to_roman ( gint number, gboolean upper )
{
	GString *roman;
	char ldigits[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm' };
	char udigits[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M' };
	char *digits = upper ? udigits : ldigits;
	int i, d = 0;
	gchar *s;

	roman = g_string_new (NULL);

	do {   
		int num = number % 10;

		if ( num % 5 < 4 )
			for ( i = num % 5; i > 0; i-- )
				g_string_insert_c( roman, 0, digits[ d ] );

		if ( num >= 4 && num <= 8)
			g_string_insert_c( roman, 0, digits[ d+1 ] );

		if ( num == 9 )
			g_string_insert_c( roman, 0, digits[ d+2 ] );

		if ( num % 5 == 4 )
			g_string_insert_c( roman, 0, digits[ d ] );

		number /= 10;
		d += 2;
	} while ( number );

	/* WARNING: Unlike the KDE implementation, this one appends the dot and the
	   space as well, to save extra copying and allocation. */
	g_string_append (roman, ". ");

	s = roman->str;
	g_string_free (roman, FALSE);

	return s;
}

static void
close_flow (HTMLEngine *e)
{
	HTMLObject *prev;

	if (e->flow == NULL)
		return;

	/* The following kludgy code makes sure there is always an hspace at
           the end of a clueflow, for editing.  */

	prev = HTML_CLUE (e->flow)->tail;

	/* FIXME: is_a */
	if (HTML_OBJECT_TYPE (prev) != HTML_TYPE_HSPACE) {
		HTMLObject *hspace;

		hspace = html_hspace_new (html_engine_get_current_font (e), TRUE);
		html_clue_append (HTML_CLUE (e->flow), hspace);
	}

	e->flow = NULL;
}


/* Block stack.  */

typedef void (*BlockFunc)(HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *el);

struct _HTMLBlockStackElement {
	BlockFunc exitFunc;

	gint id;
	gint level;
	gint miscData1;
	gint miscData2;
	HTMLBlockStackElement *next;
};

static HTMLBlockStackElement *
block_stack_element_new (gint id, gint level, BlockFunc exitFunc, 
			 gint miscData1, gint miscData2, HTMLBlockStackElement *next)
{
	HTMLBlockStackElement *se;

	se = g_new0 (HTMLBlockStackElement, 1);
	se->id = id;
	se->level = level;
	se->miscData1 = miscData1;
	se->miscData2 = miscData2;
	se->next = next;
	se->exitFunc = exitFunc;
	return se;
}

static void
block_stack_element_free (HTMLBlockStackElement *elem)
{
	g_free (elem);
}

static void
push_block (HTMLEngine *e, gint id, gint level,
	    BlockFunc exitFunc, gint miscData1, gint miscData2)
{
	HTMLBlockStackElement *elem;

	elem = block_stack_element_new (id, level, exitFunc, miscData1, miscData2, e->blockStack);
	e->blockStack = elem;
}

static void
free_block (HTMLEngine *e)
{
	HTMLBlockStackElement *elem = e->blockStack;

	while (elem != 0) {
		HTMLBlockStackElement *tmp = elem;

		elem = elem->next;
		block_stack_element_free (tmp);
	}
	e->blockStack = 0;
}

static void
pop_block (HTMLEngine *e, gint id, HTMLObject *clue)
{
	HTMLBlockStackElement *elem, *tmp;
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

		block_stack_element_free (tmp);
	}
				
}


/* The following are callbacks that are called at the end of a block.  */

static void
block_end_font (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	html_engine_pop_font (e);

	if (elem->miscData1)
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
}

static void
block_end_pre ( HTMLEngine *e, HTMLObject *_clue, HTMLBlockStackElement *elem)
{
	/* We add a hidden space to get the height
	   If there is no flow box, we add one.  */

	if (!e->flow)
		html_engine_new_flow (e, _clue );

	close_flow (e);

	html_engine_pop_font (e);

	e->vspace_inserted = html_engine_insert_vspace(e, _clue, e->vspace_inserted );
	e->inPre = FALSE;
}

static void
block_end_color_font (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	html_engine_pop_color (e);

	html_engine_pop_font (e);
}

static void
block_end_list (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	if (elem->miscData2) {
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
	}

	html_list_destroy (html_stack_pop (e->listStack));

	close_flow (e);
	
	e->indent = elem->miscData1;
}

static void
block_end_indent (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	close_flow (e);

	e->indent = elem->miscData1;
}

static void
block_end_div (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	e->divAlign =  (HTMLHAlignType) elem->miscData1;
	e->flow = 0;
}


static gchar *
parse_body (HTMLEngine *p, HTMLObject *clue, const gchar *end[], gboolean toplevel)
{
	gchar *str;

	while (html_tokenizer_has_more_tokens (p->ht) && p->parsing) {
		str = html_tokenizer_next_token (p->ht);

		if (*str == '\0')
			continue;

		if ( *str == ' ' && *(str+1) == '\0' ) {
#if 0 /* FIXME!!! */
			/* if in* is set this text belongs in a form element */
			if (p->inOption || p->inTextArea)
				formText += " ";
			else
				;
#endif
			if (p->inTitle) {
				g_string_append (p->title, " ");
			} else if (p->flow != NULL) {
				html_engine_insert_text
					(p, " ",
					 html_engine_get_current_font (p));
			}
		} else if (*str != TAG_ESCAPE) {
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
			
			/* The tag used for line break when we are in <pre>...</pre> */
			if (*str == '\n') {
				if (!p->flow)
					html_engine_new_flow (p, clue);

				/* Add a hidden space to get the line-height right */
				html_clue_append (HTML_CLUE (p->flow),
						  html_hspace_new (html_engine_get_current_font (p), TRUE));
				p->vspace_inserted = TRUE;
				
				html_engine_new_flow (p, clue);
			}
			parse_one_token (p, clue, str);
		}
	}

	if (!html_tokenizer_has_more_tokens (p->ht) && toplevel && 
	    !p->writing)
		html_engine_stop_parser (p);

	return 0;
}

/* EP CHECK: finished except for the settings stuff (see `FIXME').  */
static const gchar *
parse_table (HTMLEngine *e, HTMLObject *clue, gint max_width,
	     const gchar *attr)
{
	static const gchar *endthtd[] = { "</th", "</td", "</tr", "<th", "<td", "<tr", "</table", "</body", 0 };
	static const char *endcap[] = { "</caption>", "</table>", "<tr", "<td", "<th", "</body", 0 };    
	static const gchar *endall[] = { "</caption>", "</table", "<tr", "<td", "<th", "</th", "</td", "</tr","</body", 0 };
	HTMLTable *table;
	const gchar *str = 0;
	gint width = 0;
	gint percent = 0;
	gint padding = 1;
	gint spacing = 2;
	gint border = 0;
	gchar has_cell = 0;
	gboolean done = FALSE;
	gboolean tableTag = TRUE;
	gboolean firstRow = TRUE;
	gboolean noCell = TRUE;
	gboolean tableEntry;
	HTMLVAlignType rowvalign = HTML_VALIGN_NONE;
	HTMLHAlignType rowhalign = HTML_HALIGN_NONE;
	HTMLHAlignType align = HTML_HALIGN_NONE;
	HTMLClueV *caption = 0;
	HTMLVAlignType capAlign = HTML_VALIGN_BOTTOM;
	HTMLHAlignType olddivalign = e->divAlign;
	HTMLClue *oldflow = HTML_CLUE (e->flow);
	gint oldindent = e->indent;
	GdkColor tableColor, rowColor, bgColor;
	gboolean have_tableColor, have_rowColor, have_bgColor;
	gboolean have_tablePixmap, have_rowPixmap, have_bgPixmap;
	gint rowSpan;
	gint colSpan;
	gint cellwidth;
	gint cellpercent;
	gboolean fixedWidth;
	HTMLVAlignType valign;
	HTMLTableCell *cell;
	gpointer tablePixmapPtr = NULL;
	gpointer rowPixmapPtr = NULL;
	gpointer bgPixmapPtr = NULL;

	have_tablePixmap = FALSE;
	have_rowPixmap = FALSE;
	have_bgPixmap = FALSE;

	have_tableColor = FALSE;
	have_rowColor = FALSE;
	have_bgColor = FALSE;

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
				align = HTML_HALIGN_LEFT;
			else if (strcasecmp (token + 6, "right") == 0)
				align = HTML_HALIGN_RIGHT;
		}
		else if (strncasecmp (token, "bgcolor=", 8) == 0
			 && !e->defaultSettings->forceDefault) {
			if (html_engine_set_named_color (e, &tableColor, token + 8)) {
				rowColor = tableColor;
				have_rowColor = have_tableColor = TRUE;
			}
		}
		else if (strncasecmp (token, "background=", 11) == 0
			 && !e->defaultSettings->forceDefault) {
			
			HTMLURL *bgurl;
			gchar *string_url;
			
			if((bgurl = parse_href (e, token + 11))) {
				string_url = html_url_to_string (bgurl);
			
				tablePixmapPtr = html_image_factory_register(e->image_factory, NULL, string_url);
				if(tablePixmapPtr) {
					rowPixmapPtr = tablePixmapPtr;
					have_tablePixmap = have_rowPixmap = TRUE;
				}
				g_free(string_url);
				html_url_destroy(bgurl);			
			}
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
					close_anchor (e);
					done = TRUE;
					break;
				}

				if ( strncmp( str, "<caption", 8 ) == 0 ) {
					string_tokenizer_tokenize( e->st, str + 9, " >" );
					while ( string_tokenizer_has_more_tokens (e->st) ) {
						const char* token = string_tokenizer_next_token(e->st);
						if ( strncasecmp( token, "align=", 6 ) == 0) {
							if ( strncasecmp( token+6, "top", 3 ) == 0)
								capAlign = HTML_VALIGN_TOP;
						}
					}

					caption = HTML_CLUEV (html_cluev_new
							      ( 0, 0,
								HTML_OBJECT (clue)->max_width,
								100 ));

					e->divAlign = HTML_HALIGN_CENTER;
					e->flow = 0;

					push_block (e, ID_CAPTION, 3, NULL, 0, 0);
					str = parse_body ( e, HTML_OBJECT (caption), endcap, FALSE );
					pop_block (e, ID_CAPTION, HTML_OBJECT (caption) );

					table->caption = caption;
					table->capAlign = capAlign;

					e->flow = 0;

					if ( str == 0 ) { 
						/* CC: Close table description in case of a malformed
						   table before returning! */
						if ( !firstRow )
							html_table_end_row (table);
						html_table_end_table (table); 
						html_object_destroy (HTML_OBJECT (table));
						e->divAlign = olddivalign;
						e->flow = HTML_OBJECT (oldflow);
						return 0;
					}

					if (strncmp( str, "</caption", 9) == 0 ) {
						// HTML Ok!
						break; // Get next token from 'ht'
					}
					else {
						// Bad HTML
						// caption ended with </table> <td> <tr> or <th>
						continue; // parse the returned tag
					}
				}

				if (strncmp (str, "<tr", 3) == 0) {
					if (!firstRow)
						html_table_end_row (table);
					html_table_start_row (table);
					firstRow = FALSE;
					rowvalign = HTML_VALIGN_NONE;
					rowhalign = HTML_HALIGN_NONE;

					if (have_tableColor) {
						rowColor = tableColor;
						have_rowColor = TRUE;
					} else {
						have_rowColor = FALSE;
					}

					if (have_tablePixmap) {
						rowPixmapPtr = tablePixmapPtr;
						have_rowPixmap = TRUE;
					} else {
						have_rowPixmap = FALSE;
					}

					string_tokenizer_tokenize (e->st, str + 4, " >");
					while (string_tokenizer_has_more_tokens (e->st)) {
						const gchar *token = string_tokenizer_next_token (e->st);
						if (strncasecmp (token, "valign=", 7) == 0) {
							if (strncasecmp (token + 7, "top", 3) == 0)
								rowvalign = HTML_VALIGN_TOP;
							else if (strncasecmp (token + 7, "bottom", 6) == 0)
								rowvalign = HTML_VALIGN_BOTTOM;
							else
								rowvalign = HTML_VALIGN_CENTER;
						}
						else if (strncasecmp (token, "align", 6) == 0) {
							if (strcasecmp (token + 6, "left") == 0)
								rowhalign = HTML_HALIGN_LEFT;
							else if (strcasecmp (token + 6, "right") == 0)
								rowhalign = HTML_HALIGN_RIGHT;
							else if (strcasecmp (token + 6, "center") == 0)
								rowhalign = HTML_HALIGN_CENTER;
						}
						else if (strncasecmp (token, "bgcolor=", 8) == 0) {
							have_rowColor
								= html_engine_set_named_color (e, &rowColor,
											       token + 8);
						}
						else if (strncasecmp (token, "background=", 11) == 0
							 && !e->defaultSettings->forceDefault) {
							
							HTMLURL *bgurl;
							gchar *string_url;
							
							if((bgurl = parse_href (e, token + 11))) {
								string_url = html_url_to_string (bgurl);
								
								rowPixmapPtr = html_image_factory_register(e->image_factory, NULL, string_url);								
								if(rowPixmapPtr) {
									have_rowPixmap = TRUE;
								}
								g_free(string_url);
								html_url_destroy(bgurl);
							}			
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

					if (have_rowColor) {
						bgColor = rowColor;
						have_bgColor = TRUE;
					} else {
						have_bgColor = FALSE;
					}

					if (have_rowPixmap) {
						bgPixmapPtr = rowPixmapPtr;
						have_bgPixmap = TRUE;
					} else {
						have_bgPixmap = FALSE;
					}

					valign = (rowvalign == HTML_VALIGN_NONE ?
						  HTML_VALIGN_CENTER : rowvalign);

					if (heading)
						e->divAlign = (rowhalign == HTML_HALIGN_NONE ? 
							       HTML_HALIGN_CENTER : rowhalign);
					else
						e->divAlign = (rowhalign == HTML_HALIGN_NONE ?
							       HTML_HALIGN_LEFT : rowhalign);

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
									valign = HTML_VALIGN_TOP;
								else if (strncasecmp (token + 7, "bottom", 6) == 0)
									valign = HTML_VALIGN_BOTTOM;
								else 
									valign = HTML_VALIGN_CENTER;
							}
							else if (strncasecmp (token, "align=", 6) == 0) {
								if (strcasecmp (token + 6, "center") == 0)
									e->divAlign = HTML_HALIGN_CENTER;
								else if (strcasecmp (token + 6, "right") == 0)
									e->divAlign = HTML_HALIGN_RIGHT;
								else if (strcasecmp (token + 6, "left") == 0)
									e->divAlign = HTML_HALIGN_LEFT;
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
							else if (strncasecmp (token, "bgcolor=", 8) == 0
								 && !e->defaultSettings->forceDefault) {
								have_bgColor
									= html_engine_set_named_color (e, &bgColor,
												       token + 8);
							}
							else if (strncasecmp (token, "background=", 11) == 0
								 && !e->defaultSettings->forceDefault) {
								
								HTMLURL *bgurl;
								gchar *string_url;
								
								if((bgurl = parse_href (e, token + 11))) {
									string_url = html_url_to_string (bgurl);
									
									bgPixmapPtr = html_image_factory_register(e->image_factory, NULL, string_url);								
									if(bgPixmapPtr)
										have_bgPixmap = TRUE;
									
									g_free(string_url);
									html_url_destroy(bgurl);
								}

							}
						}
					}

					cell = HTML_TABLE_CELL (html_table_cell_new (0, 0, cellwidth,
										     cellpercent,
										     rowSpan, colSpan,
										     padding));
					html_object_set_bg_color (HTML_OBJECT (cell),
								  have_bgColor ? &bgColor : NULL);

					if(have_bgPixmap)
						html_table_cell_set_bg_pixmap(cell, bgPixmapPtr);


					HTML_CLUE (cell)->valign = valign;
					if (fixedWidth)
						HTML_OBJECT (cell)->flags |= HTML_OBJECT_FLAG_FIXEDWIDTH;
					html_table_add_cell (table, cell);
					has_cell = 1;
					e->flow = 0;
					if (heading) {
						e->bold = TRUE;
						html_engine_select_font (e);
						push_block (e, ID_TH, 3,
							    block_end_font,
							    FALSE, 0);
						str = parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
						pop_block (e, ID_TH, HTML_OBJECT (cell));
					}
					if (!tableEntry) {
						/* Put all the junk between <table>
						   and the first table tag into one row */
						push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endall, FALSE);
						pop_block (e, ID_TD, HTML_OBJECT (cell));
						html_table_end_row (table);
						html_table_start_row (table);
						
					}
					else {
						/* Ignore <p> and such at the beginning */
						e->vspace_inserted = TRUE;
						push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
						pop_block (e, ID_TD, HTML_OBJECT (cell));
					}

					if (str == 0) {
						/* Close table description in case of
						   a malformed table before returning! */
						if (!firstRow)
							html_table_end_row (table);
						html_table_end_table (table);
						html_object_destroy (HTML_OBJECT (table));
						e->divAlign = olddivalign;
						e->flow = HTML_OBJECT (oldflow);
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
		
	if (has_cell) {
		/* The ending "</table>" might be missing, so we close the table
		   here...  */
		if (!firstRow)
			html_table_end_row (table);
		html_table_end_table (table);
		if (align != HTML_HALIGN_LEFT && align != HTML_HALIGN_RIGHT) {
			html_clue_append (HTML_CLUE (clue), HTML_OBJECT (table));
		} else {
			HTMLClueAligned *aligned;

			aligned = HTML_CLUEALIGNED (html_cluealigned_new (NULL,
									  0, 0,
									  clue->max_width,
									  100));
			HTML_CLUE (aligned)->halign = align;
			html_clue_append (HTML_CLUE (aligned), HTML_OBJECT (table));
			html_clue_append (HTML_CLUE (clue), HTML_OBJECT (aligned));
		}
	} else {
		/* Last resort: remove tables that do not contain any cells */
		html_object_destroy (HTML_OBJECT (table));
	}
	
	e->indent = oldindent;
	e->divAlign = olddivalign;
	e->flow = HTML_OBJECT (oldflow);

	g_print ("Returning: %s\n", str);
	return str;
}

static HTMLURL *
parse_href (HTMLEngine *e,
	    const gchar *s)
{
	HTMLURL *retval;
	HTMLURL *tmp;

	if(s == NULL || *s == 0)
		return NULL;

	if (s[0] == '#') {
		tmp = html_url_dup (e->actualURL, HTML_URL_DUP_NOREFERENCE);
		html_url_set_reference (tmp, s + 1);
		return tmp;
	}

	tmp = html_url_new (s);
	if (html_url_get_protocol (tmp) == NULL) {
		if (s[0] == '/') {
			if (s[1] == '/') {
				gchar *t;

				/* Double slash at the beginning.  */

				/* FIXME?  This is a bit sucky.  */
				t = g_strconcat (html_url_get_protocol (e->actualURL),
						 ":", s, NULL);
				retval = html_url_new (t);
				g_free (t);
				html_url_destroy (tmp);
			} else {
				/* Single slash at the beginning.  */

				retval = html_url_dup (e->actualURL,
						       HTML_URL_DUP_NOPATH);
				html_url_set_path (retval, s);
				html_url_destroy (tmp);
			}
		} else {
			retval = html_url_append_path (e->actualURL, s);
			html_url_destroy (tmp);
		}
	} else {
		retval = tmp;
	}

	return retval;
}


/*
  <a               </a>
  <address>        </address>
  <area            </area>
*/
static void
parse_a (HTMLEngine *e, HTMLObject *_clue, const gchar *str)
{
#if 0							/* FIXME TODO */
	if ( strncmp( str, "area", 4 ) == 0 ) {
		GString *href = g_string_new (NULL);
		GString *coords = g_string_new (NULL);
		GString *atarget = g_string_new (baseTarget->str);

		if ( mapList.isEmpty() )
			return;

		string_tokenizer_tokenize (e->st, str + 5, " >");

		HTMLArea::Shape shape = HTMLArea::Rect;

		while ( stringTok->hasMoreTokens() )
		{
			const char* p = stringTok->nextToken();

			if ( strncasecmp( p, "shape=", 6 ) == 0 ) {
				if ( strncasecmp( p+6, "rect", 4 ) == 0 )
					shape = HTMLArea::Rect;
				else if ( strncasecmp( p+6, "poly", 4 ) == 0 )
					shape = HTMLArea::Poly;
				else if ( strncasecmp( p+6, "circle", 6 ) == 0 )
					shape = HTMLArea::Circle;
			} else

				if ( strncasecmp( p, "href=", 5 ) == 0 ) {
					p += 5;
					if ( *p == '#' ) { /* FIXME TODO */
						g_warning ("#references are not implemented yet.");
						KURL u( actualURL );
						u.setReference( p + 1 );
						href = u.url();
					}
					else 
					{
						KURL u( baseURL, p );
						href = u.url();
					}
				}
				else if ( strncasecmp( p, "target=", 7 ) == 0 )
				{
					atarget = p+7;
				}
				else if ( strncasecmp( p, "coords=", 7 ) == 0 )
				{
					coords = p+7;
				}
		}

		if ( !coords.isEmpty() && !href.isEmpty() )
		{
			HTMLArea *area = 0;

			switch ( shape )
			{
			case HTMLArea::Rect:
			{
				int x1, y1, x2, y2;
				sscanf( coords, "%d,%d,%d,%d", &x1, &y1, &x2, &y2 );
				QRect rect( x1, y1, x2-x1, y2-y1 );
				area = new HTMLArea( rect, href, atarget );
				debugM( "Area Rect %d, %d, %d, %d\n", x1, y1, x2, y2 );
			}
			break;

			case HTMLArea::Circle:
			{
				int xc, yc, rc;
				sscanf( coords, "%d,%d,%d", &xc, &yc, &rc );
				area = new HTMLArea( xc, yc, rc, href, atarget );
				debugM( "Area Circle %d, %d, %d\n", xc, yc, rc );
			}
			break;

			case HTMLArea::Poly:
			{
				debugM( "Area Poly " );
				int count = 0, x, y;
				QPointArray parray;
				const char *ptr = coords;
				while ( ptr )
				{
					x = atoi( ptr );
					ptr = strchr( ptr, ',' );
					if ( ptr )
					{
						y = atoi( ++ptr );
						parray.resize( count + 1 );
						parray.setPoint( count, x, y );
						debugM( "%d, %d  ", x, y );
						count++;
						ptr = strchr( ptr, ',' );
						if ( ptr ) ptr++;
					}
				}
				debugM( "\n" );
				if ( count > 2 )
					area = new HTMLArea( parray, href, atarget );
			}
			break;
			}

			if ( area )
				mapList.getLast()->addArea( area );
		}
	} else
#endif
		if ( strncmp( str, "address", 7) == 0 )
		{
			e->flow = 0;
			e->italic = TRUE;
			e->bold = FALSE;
			html_engine_select_font (e);
			push_block (e, ID_ADDRESS, 2, block_end_font, TRUE, 0);
		}
		else if ( strncmp( str, "/address", 8) == 0 )
		{
			pop_block ( e, ID_ADDRESS, _clue);
		}
		else if ( strncmp( str, "a ", 2 ) == 0 ) {
			HTMLURL *tmpurl = NULL;
			gboolean visited = FALSE;
			gchar *target = NULL;
			const gchar *p;

			close_anchor (e);

			string_tokenizer_tokenize( e->st, str + 2, " >" );

			while ( ( p = string_tokenizer_next_token (e->st) ) != 0 ) {
				if ( strncasecmp( p, "href=", 5 ) == 0 ) {
					if (tmpurl != NULL)
						html_url_destroy (tmpurl);

					p += 5;
					tmpurl = parse_href (e, p);

					/* FIXME visited? */
				} else if ( strncasecmp( p, "name=", 5 ) == 0 ) {
					if (e->flow == 0 )
						html_clue_append (HTML_CLUE (_clue),
								  html_anchor_new (p + 5));
					else
						html_clue_append (HTML_CLUE (e->flow),
								  html_anchor_new (p+5));
				} else if ( strncasecmp( p, "target=", 7 ) == 0 ) {
					target = g_strdup (p + 7);
#if 0							/* FIXME TODO */
					parsedTargets.append( target );
#endif
				}
			}

#if 0
			if ( !target
			     && e->baseTarget != NULL
			     && e->baseTarget[0] != '\0' ) {
				target = g_strdup (e->baseTarget);
				/*  parsedTargets.append( target ); FIXME TODO */
			}
#endif

			if (tmpurl != NULL) {
				e->vspace_inserted = FALSE;

				if ( visited )
					html_stack_push (e->cs, gdk_color_copy (&e->settings->vLinkColor));
				else
					html_stack_push (e->cs, gdk_color_copy (&e->settings->linkColor));

				if (e->settings->underlineLinks)
					e->underline = TRUE;

				html_engine_select_font (e);

				if (e->url != NULL)
					html_url_destroy (e->url);
				e->url = tmpurl;

				{
					gchar *url_string;

					url_string = html_url_to_string (e->url);
					g_print ("HREF: %s\n", url_string);
					g_free (url_string);
				}

#if 0							/* FIXME TODO */
				url = new char [ tmpurl.length() + 1 ];
				strcpy( url, tmpurl.data() );
				parsedURLs.append( url );
#endif
			}
		} else if ( strncmp( str, "/a", 2 ) == 0 ) {
			close_anchor (e);
		}
}


/*
  <b>              </b>
  <base
  <basefont                        unimplemented
  <big>            </big>
  <blockquote>     </blockquote>
  <body
  <br
*/
/* EP CHECK All done except for the color specifications in the `<body>'
   tag.  */
static void
parse_b (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "basefont", 8) == 0) {
	}
	else if ( strncmp(str, "base", 4 ) == 0 ) {
		string_tokenizer_tokenize( e->st, str + 5, " >" );
		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token = string_tokenizer_next_token(e->st);
			if ( strncasecmp( token, "target=", 7 ) == 0 ) {
				gtk_signal_emit (GTK_OBJECT (e), signals[SET_BASE_TARGET], token + 7);
			} else if ( strncasecmp( token, "href=", 5 ) == 0 ) {

				html_engine_set_base_url(e, token + 5);
			}
		}
	}
	else if ( strncmp(str, "big", 3 ) == 0 ) {
		select_font_relative (e, +2);
		push_block (e, ID_BIG, 1, block_end_font, 0, 0);
	} else if ( strncmp(str, "/big", 4 ) == 0 ) {
		pop_block (e, ID_BIG, clue);
	} else if ( strncmp(str, "blockquote", 10 ) == 0 ) {
		push_block (e, ID_BLOCKQUOTE, 2,
			    block_end_indent, e->indent, 0);
		close_flow (e);
		e->indent += INDENT_SIZE;
	} else if ( strncmp(str, "/blockquote", 11 ) == 0 ) {
		pop_block (e, ID_BLOCKQUOTE, clue);
	} else if (strncmp (str, "body", 4) == 0) {
		GdkColor bgColor;
		gboolean bgColorSet = FALSE;

		if (e->bodyParsed) {
			g_print ("body is parsed\n"); /* FIXME */
			return;
		}

		e->bodyParsed = TRUE;
		
		string_tokenizer_tokenize (e->st, str + 5, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			g_print ("token is: %s\n", token);

			if (strncasecmp (token, "bgcolor=", 8) == 0) {
				g_print ("setting color\n");
				if (html_engine_set_named_color (e, &bgColor, token + 8)) {
					g_print ("bgcolor is set\n");
					bgColorSet = TRUE;
				} else {
					g_print ("`%s' could not be allocated\n", token);
				}
			}
			else if (strncasecmp (token, "background=", 11) == 0
				 && !e->defaultSettings->forceDefault) {

				HTMLURL *bgurl;
				gchar *string_url;

				bgurl = parse_href (e, token + 11);
				string_url = html_url_to_string (bgurl);
				
				e->bgPixmapPtr = html_image_factory_register(e->image_factory, NULL, string_url);

				g_free(string_url);
				html_url_destroy(bgurl);

			} else if ( strncasecmp( token, "text=", 5 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				html_engine_set_named_color (e,
							     &e->settings->fontBaseColor,
							     token+5 );
				gdk_color_free (html_stack_pop (e->cs));
				html_stack_push (e->cs, gdk_color_copy (&e->settings->fontBaseColor));
				html_engine_select_font (e);
			} else if ( strncasecmp( token, "link=", 5 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				html_engine_set_named_color (e,
							     &e->settings->linkColor,
							     token+5 );
			} else if ( strncasecmp( token, "vlink=", 6 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				html_engine_set_named_color (e,
							     &e->settings->vLinkColor,
							     token+5 );
			}
		}
#if 0
		if ( !bgColorSet || defaultSettings->forceDefault )
			{
				QPalette pal = palette().copy();
				QColorGroup cg = pal.normal();
				QColorGroup newGroup( cg.foreground(), defaultSettings->bgColor,
						      cg.light(), cg.dark(), cg.mid(), cg.text(),
						      defaultSettings->bgColor );
				pal.setNormal( newGroup );
				setPalette( pal );

				// simply testing if QColor == QColor fails!?, so we must compare
				// each RGB
				if ( defaultSettings->bgColor.red() != settings->bgColor.red() ||
				     defaultSettings->bgColor.green() != settings->bgColor.green() ||
				     defaultSettings->bgColor.blue() != settings->bgColor.blue() ||
				     bDrawBackground )
					{
						settings->bgColor = defaultSettings->bgColor;
						setBackgroundColor( settings->bgColor );
					}
			}
		else
			{
				QPalette pal = palette().copy();
				QColorGroup cg = pal.normal();
				QColorGroup newGroup( cg.foreground(), settings->bgColor,
						      cg.light(), cg.dark(), cg.mid(), cg.text(), settings->bgColor );
				pal.setNormal( newGroup );
				setPalette( pal );

				if ( settings->bgColor.red() != bgColor.red() ||
				     settings->bgColor.green() != bgColor.green() ||
				     settings->bgColor.blue() != bgColor.blue() ||
				     bDrawBackground )
					{
						settings->bgColor = bgColor;
						setBackgroundColor( settings->bgColor );
					}
			}
#endif

		if (bgColorSet)
			e->bgColor = bgColor;

		g_print ("parsed <body>\n");
	}
	else if (strncmp (str, "br", 2) == 0) {
		HTMLClearType clear;

		clear = HTML_CLEAR_NONE;

		if (!e->flow)
			html_engine_new_flow (e, clue);

		/* FIXME: poinSize here */
		html_clue_append (HTML_CLUE (e->flow),
				  html_vspace_new (14, clear));

		e->vspace_inserted = FALSE;
	}
	else if (strncmp (str, "b", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			e->bold = TRUE;
			html_engine_select_font (e);
			push_block (e, ID_B, 1, 
				    block_end_font,
				    FALSE, FALSE);
		}
	}
	else if (strncmp (str, "/b", 2) == 0) {
		pop_block (e, ID_B, clue);
	}
}


/*
  <center>         </center>
  <cite>           </cite>
  <code>           </code>
  <cell>           </cell>
  <comment>        </comment>      unimplemented
*/
/* EP CHECK OK except for the font in `<code>'.  */
static void
parse_c (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "center", 6) == 0) {
		e->divAlign = HTML_HALIGN_CENTER;
		e->flow = 0;
	}
	else if (strncmp (str, "/center", 7) == 0) {
		e->divAlign = HTML_HALIGN_LEFT;
		e->flow = 0;
	}
	else if (strncmp( str, "cite", 4 ) == 0) {
		e->italic = TRUE;
		e->bold = FALSE;
		html_engine_select_font (e);
		push_block(e, ID_CITE, 1, block_end_font, 0, 0);
	}
	else if (strncmp( str, "/cite", 5) == 0) {
		pop_block (e, ID_CITE, clue);
	} else if (strncmp(str, "code", 4 ) == 0 ) {
		select_font_full (e, e->settings->fixedFontFace,
				  e->settings->fontBaseSize, FALSE,
				  FALSE, FALSE);
		push_block (e, ID_CODE, 1, block_end_font, 0, 0);
	} else if (strncmp(str, "/code", 5 ) == 0 ) {
		pop_block (e, ID_CODE, clue);
	}
}


/*
  <dir             </dir>          partial
  <div             </div>
  <dl>             </dl>
  <dt>             </dt>
*/
/* EP CHECK: OK.  */
static void
parse_d ( HTMLEngine *e, HTMLObject *_clue, const char *str )
{
	if ( strncmp( str, "dir", 3 ) == 0 ) {
		close_anchor(e);
		push_block (e, ID_DIR, 2, block_end_list,
			    e->indent, FALSE);
		html_stack_push (e->listStack, html_list_new ( HTML_LIST_TYPE_DIR,
							       HTML_LIST_NUM_TYPE_NUMERIC ) );
		e->indent += INDENT_SIZE;
	} else if ( strncmp( str, "/dir", 4 ) == 0 ) {
		pop_block (e, ID_DIR, _clue);
	} else if ( strncmp( str, "div", 3 ) == 0 ) {
		push_block (e, ID_DIV, 1,
			    block_end_div,
			    e->divAlign, FALSE);

		string_tokenizer_tokenize( e->st, str + 4, " >" );
		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token = string_tokenizer_next_token (e->st);
			if ( strncasecmp( token, "align=", 6 ) == 0 )
			{
				if ( strcasecmp( token + 6, "right" ) == 0 )
					e->divAlign = HTML_HALIGN_RIGHT;
				else if ( strcasecmp( token + 6, "center" ) == 0 )
					e->divAlign = HTML_HALIGN_CENTER;
				else if ( strcasecmp( token + 6, "left" ) == 0 )
					e->divAlign = HTML_HALIGN_LEFT;
			}
		}

		e->flow = 0;
	} else if ( strncmp( str, "/div", 4 ) == 0 ) {
		pop_block (e, ID_DIV, _clue );
	} else if ( strncmp( str, "dl", 2 ) == 0 ) {
		e->vspace_inserted = html_engine_insert_vspace( e, _clue,
								e->vspace_inserted );
		close_anchor (e);

		if ( html_stack_top(e->glossaryStack) != NULL )
			e->indent += INDENT_SIZE;

		html_stack_push (e->glossaryStack, GINT_TO_POINTER (HTML_GLOSSARY_DL));
		e->flow = 0;
	} else if ( strncmp( str, "/dl", 3 ) == 0 ) {
		if ( html_stack_top (e->glossaryStack) == NULL)
			return;

		if ( GPOINTER_TO_INT (html_stack_top (e->glossaryStack))
		     == HTML_GLOSSARY_DD ) {
			html_stack_pop (e->glossaryStack);
			e->indent -= INDENT_SIZE;
			if (e->indent < 0)
				e->indent = 0;
		}
		html_stack_pop (e->glossaryStack);
		if ( html_stack_top (e->glossaryStack) != NULL ) {
			e->indent -= INDENT_SIZE;
			if (e->indent < 0)
				e->indent = 0;
		}
		e->vspace_inserted = html_engine_insert_vspace (e, _clue, e->vspace_inserted );
	} else if (strncmp( str, "dt", 2 ) == 0) {
		if (html_stack_top (e->glossaryStack) == NULL)
			return;

		if (GPOINTER_TO_INT (html_stack_top (e->glossaryStack))
		    == HTML_GLOSSARY_DD) {
			html_stack_pop (e->glossaryStack);
			e->indent -= INDENT_SIZE;
			if (e->indent < 0)
				e->indent = 0;
		}
		e->vspace_inserted = FALSE;
		e->flow = 0;
	} else if (strncmp( str, "dd", 2 ) == 0) {
		if (html_stack_top (e->glossaryStack) == NULL)
			return;

		if (GPOINTER_TO_INT (html_stack_top (e->glossaryStack))
		    != HTML_GLOSSARY_DD ) {
			html_stack_push (e->glossaryStack,
					 GINT_TO_POINTER (HTML_GLOSSARY_DD) );
			e->indent += INDENT_SIZE;
		}
		e->flow = 0;
	}
}


/*
  <em>             </em>
*/
/* EP CHECK: OK.  */
static void
parse_e (HTMLEngine *e, HTMLObject *_clue, const gchar *str)
{
	if ( strncmp( str, "em", 2 ) == 0 ) {
		e->italic = TRUE;
		html_engine_select_font (e);
		push_block (e, ID_EM, 1, block_end_font,
			    FALSE, FALSE);
	} else if ( strncmp( str, "/em", 3 ) == 0 ) {
		pop_block (e, ID_EM, _clue);
	}
}


/*
  <font>           </font>
  <form>           </form>         partial
  <frame           <frame>
  <frameset        </frameset>
*/
/* EP CHECK: Fonts are done wrong, the rest is missing.  */
static void
parse_f (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "font", 4) == 0) {
		GdkColor *color;
		gint newSize = (html_engine_get_current_font (p))->size;

		color = gdk_color_copy (html_stack_top (p->cs));

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
			else if (strncasecmp (token, "face=", 5) == 0);
			else if (strncasecmp (token, "color=", 6) == 0) {
				/* FIXME: Have a way to override colors? */
				html_engine_set_named_color (p, color, token + 6);
			}
		}

		/* FIXME this is wrong */
		p->fontsize = newSize;
		html_stack_push (p->cs, color);
		html_engine_select_font (p);
		push_block  (p, ID_FONT, 1, block_end_color_font,
			     FALSE, FALSE);
	}
	else if (strncmp (str, "/font", 5) == 0) {
		pop_block (p, ID_FONT, clue);
	}
	
}


/*
  <h[1-6]>         </h[1-6]>
  <hr
*/
/* EP CHECK: OK */
static void
parse_h (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (*(str) == 'h' &&
	    ( *(str+1)=='1' || *(str+1)=='2' || *(str+1)=='3' ||
	      *(str+1)=='4' || *(str+1)=='5' || *(str+1)=='6' ) ) {
		HTMLHAlignType align;

		p->vspace_inserted = html_engine_insert_vspace (p, clue,
								p->vspace_inserted);
		align = p->divAlign;

		string_tokenizer_tokenize (p->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			const gchar *token;

			token = string_tokenizer_next_token (p->st);
			if ( strncasecmp( token, "align=", 6 ) == 0 ) {
				if ( strcasecmp( token + 6, "center" ) == 0 )
					align = HTML_HALIGN_CENTER;
				else if ( strcasecmp( token + 6, "right" ) == 0 )
					align = HTML_HALIGN_RIGHT;
				else if ( strcasecmp( token + 6, "left" ) == 0 )
					align = HTML_HALIGN_LEFT;
			}
		}
		
		/* Start a new flow box */
		if (!p->flow)
			html_engine_new_flow (p, clue);
		HTML_CLUE (p->flow)->halign = align;

		p->bold = TRUE;
		p->italic = FALSE;

		switch (str[1]) {
		case '1':
			p->bold = TRUE;
			p->italic = FALSE;
			select_font_relative (p, +3);
			break;

		case '2':
			p->bold = TRUE;
			p->italic = FALSE;
			select_font_relative (p, +2);
			break;

		case '3':
			p->bold = TRUE;
			p->italic = FALSE;
			select_font_relative (p, +1);
			break;

		case '4':
			p->bold = TRUE;
			p->italic = FALSE;
			select_font_relative (p, 0);
			break;

		case '5':
			p->bold = FALSE;
			p->italic = TRUE;
			select_font_relative (p, 0);
			break;

		case '6':
			p->bold = TRUE;
			p->italic = FALSE;
			select_font_relative (p, -1);
			break;
		}

		/* Insert a vertical space and restore the old font at the closing
		   tag.  */
		push_block (p, ID_HEADER, 2, block_end_font, TRUE, 0);
	} else if (*(str) == '/' && *(str + 1) == 'h' &&
		   ( *(str+2)=='1' || *(str+2)=='2' || *(str+2)=='3' ||
		     *(str+2)=='4' || *(str+2)=='5' || *(str+2)=='6' )) {
		/* Close tag.  */
		pop_block (p, ID_HEADER, clue);
	}
	else if (strncmp (str, "hr", 2) == 0) {
		gint size = 1;
		gint length = clue->max_width;
		gint percent = 100;
		HTMLHAlignType align = p->divAlign;
		HTMLHAlignType oldAlign = p->divAlign;
		gboolean shade = TRUE;

		if (p->flow)
			oldAlign = align = HTML_CLUE (p->flow)->halign;

		string_tokenizer_tokenize (p->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			gchar *token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "align=", 6) == 0) {
				if (strcasecmp (token + 6, "left") == 0)
					align = HTML_HALIGN_LEFT;
				else if (strcasecmp (token + 6, "right") == 0)
					align = HTML_HALIGN_RIGHT;
				else if (strcasecmp (token + 6, "center") == 0)
					align = HTML_HALIGN_CENTER;
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
		html_clue_append (HTML_CLUE (p->flow),
				  html_rule_new (length, percent, size, shade));
		HTML_CLUE (p->flow)->halign = align;
		p->flow = 0;
		p->vspace_inserted = FALSE;
	}
}


/*
  <i>              </i>
  <img                             partial
  <input                           partial
*/
/* EP CHECK: map support missing.  `<input>' missing.  */
static void
parse_i (HTMLEngine *p, HTMLObject *_clue, const gchar *str)
{
	if (strncmp (str, "img", 3) == 0) {
		HTMLObject *image = 0;
		gchar *token = 0; 
		HTMLURL *tmpurl = NULL;
		gint width = -1;
		gint height = -1;
		gint percent = 0;
		HTMLHAlignType align = HTML_HALIGN_NONE;
		gint border = 0;
		HTMLVAlignType valign = HTML_VALIGN_NONE;
		
		p->vspace_inserted = FALSE;

		string_tokenizer_tokenize (p->st, str + 4, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "src=", 4) == 0) {

				tmpurl = parse_href (p, token + 4);
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
					align = HTML_HALIGN_LEFT;
				else if (strcasecmp (token + 6, "right") == 0)
					align = HTML_HALIGN_RIGHT;
				else if (strcasecmp (token + 6, "top") == 0)
					valign = HTML_VALIGN_TOP;
				else if (strcasecmp (token + 6, "bottom") ==0)
					valign = HTML_VALIGN_BOTTOM;
			}
#if 0							/* FIXME TODO map support */
			else if ( strncasecmp( token, "usemap=", 7 ) == 0 )
			{
				if ( *(token + 7 ) == '#' )
				{
					// Local map. Format: "#name"
					usemap = token + 7;
				}
				else
				{
					KURL u( baseURL, token + 7 );
					usemap = u.url();
				}
			}
			else if ( strncasecmp( token, "ismap", 5 ) == 0 )
			{
				ismap = true;
			}
#endif
		}
		if (tmpurl != 0) {
			gchar *string_url;
			gchar *string_url2;

			if (!p->flow)
				html_engine_new_flow (p, _clue);

			string_url = p->url ? html_url_to_string (p->url) : NULL;
			string_url2 = html_url_to_string (tmpurl);

			html_url_destroy(tmpurl);

			image = html_image_new (p->image_factory, string_url2,
						string_url,
						p->target,
						_clue->max_width, 
						width, height, percent, border);
			g_free(string_url2);

			if(string_url)
				g_free(string_url);
			
			if (align == HTML_HALIGN_NONE) {
				if (valign == HTML_VALIGN_NONE) {
					html_clue_append (HTML_CLUE (p->flow), image);
				}
				else {
					HTMLObject *valigned = html_clueh_new (0, 0, _clue->max_width);
					HTML_CLUE (valigned)->valign = valign;
					html_clue_append (HTML_CLUE (valigned), HTML_OBJECT (image));
					html_clue_append (HTML_CLUE (p->flow), valigned);
				}
			}
			/* We need to put the image in a HTMLClueAligned */
			else {
				HTMLClueAligned *aligned = HTML_CLUEALIGNED (html_cluealigned_new (NULL, 0, 0, _clue->max_width, 100));
				HTML_CLUE (aligned)->halign = align;
				html_clue_append (HTML_CLUE (aligned), HTML_OBJECT (image));
				html_clue_append (HTML_CLUE (p->flow), HTML_OBJECT (aligned));
			}
		}		       
	}
	else if ( strncmp (str, "i", 1 ) == 0 ) {
		if ( str[1] == '>' || str[1] == ' ' ) {
			p->italic = TRUE;
			html_engine_select_font (p);
			push_block (p, ID_I, 1, block_end_font,
				    FALSE, FALSE);
		}
	}
	else if ( strncmp( str, "/i", 2 ) == 0 ) {
		pop_block (p, ID_I, _clue);
	}
}


/*
  <kbd>            </kbd>
*/
/* EP CHECK: OK but font is wrong.  */
static void
parse_k (HTMLEngine *e, HTMLObject *_clue, const gchar *str)
{
	if ( strncmp(str, "kbd", 3 ) == 0 )
	{
#if 0
		selectFont( settings->fixedFontFace, settings->fontBaseSize,
			    QFont::Normal, FALSE );
#else
		e->bold = TRUE;
		html_engine_select_font (e);
#endif
		push_block (e, ID_KBD, 1, block_end_font, 0, 0);
	}
	else if ( strncmp(str, "/kbd", 4 ) == 0 )
	{
		pop_block (e, ID_KBD, _clue);
	}
}


/*
  <listing>        </listing>      unimplemented
  <li>
*/
/* EP CHECK: OK */
static void
parse_l (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "link", 4) == 0) {
	}
	else if (strncmp (str, "li", 2) == 0) {
		HTMLObject *f, *c, *vc;
		HTMLFont *font;
		HTMLListType listType = HTML_LIST_TYPE_UNORDERED;
		HTMLListNumType listNumType = HTML_LIST_NUM_TYPE_NUMERIC;
		gint listLevel = 1;
		gint itemNumber = 1;
		gint indentSize = INDENT_SIZE;

		close_anchor (p);

		if (! html_stack_is_empty (p->listStack)) {
			HTMLList *top;

			top = html_stack_top (p->listStack);

			listType = top->type;
			listNumType = top->numType;
			itemNumber = top->itemNumber;

			listLevel = html_stack_count (p->listStack);

			indentSize = p->indent;
		}
		
		f = html_clueflow_new ();

		html_clue_append (HTML_CLUE (clue), f);
		c = html_clueh_new (0, 0, clue->max_width);
		HTML_CLUE (c)->valign = HTML_VALIGN_TOP;
		html_clue_append (HTML_CLUE (f), c);

		/* Fixed width spacer */
		vc = html_cluev_new (0, 0, indentSize, 0);
		HTML_CLUE (vc)->valign = HTML_VALIGN_TOP;
		html_clue_append (HTML_CLUE (c), vc);

		switch (listType) {
		case HTML_LIST_TYPE_UNORDERED:
			p->flow = html_clueflow_new ();
			HTML_CLUE (p->flow)->halign = HTML_HALIGN_RIGHT;
			html_clue_append (HTML_CLUE (vc), p->flow);
			font = html_stack_top (p->fs);
			html_clue_append (HTML_CLUE (p->flow),
					  html_bullet_new (font->pointSize,
							   listLevel,
							   &p->settings->fontBaseColor));
			break;

		case HTML_LIST_TYPE_ORDERED:
		{
			HTMLObject *text;
			gchar *item;

			p->flow = html_clueflow_new ();
			HTML_CLUE (p->flow)->halign = HTML_HALIGN_RIGHT;
			html_clue_append (HTML_CLUE (vc), p->flow);

			switch ( listNumType )
			{
			case HTML_LIST_NUM_TYPE_LOWROMAN:
				item = to_roman ( itemNumber, FALSE );
				break;

			case HTML_LIST_NUM_TYPE_UPROMAN:
				item = to_roman ( itemNumber, TRUE );
				break;

			case HTML_LIST_NUM_TYPE_LOWALPHA:
				item = g_strdup_printf ("%c. ", 'a' + itemNumber - 1);
				break;

			case HTML_LIST_NUM_TYPE_UPALPHA:
				item = g_strdup_printf ("%c. ", 'A' + itemNumber - 1);
				break;

			default:
				item = g_strdup_printf( "%2d. ", itemNumber );
			}

			p->tempStrings = g_list_prepend (p->tempStrings, item);

			text = html_text_new (item, html_engine_get_current_font (p));

			html_clue_append (HTML_CLUE (p->flow), text);
			break;
		}

		default:
			break;

		}
		
		vc = html_cluev_new (0, 0, clue->max_width - indentSize, 100);
		html_clue_append (HTML_CLUE (c), vc);
		p->flow = html_clueflow_new ();
		html_clue_append (HTML_CLUE (vc), p->flow);

		if (! html_stack_is_empty (p->listStack)) {
			HTMLList *list;

			list = html_stack_top (p->listStack);
			list->itemNumber++;
		}

		p->vspace_inserted = TRUE;
	}
}

/*
 <meta>
*/

static void
parse_m (HTMLEngine *e, HTMLObject *_clue, const gchar *str )
{
	int refresh = 0;
	int refresh_delay = 0;
	HTMLURL *refresh_url = NULL;

	if ( strncmp( str, "meta", 4 ) == 0 ) {
		string_tokenizer_tokenize( e->st, str + 5, " >" );
		while ( string_tokenizer_has_more_tokens (e->st) ) {

			const gchar* token = string_tokenizer_next_token(e->st);
			if ( strncasecmp( token, "http-equiv=", 11 ) == 0 ) {
				if ( strncasecmp( token + 11, "refresh", 7 ) == 0 )
					refresh = 1;
			} else if ( strncasecmp( token, "content=", 8 ) == 0 ) {
				if(refresh) {
					gchar *url = NULL, *actualURL = NULL;
					const gchar *content;
					content = token + 8;

					/* The time in seconds until the refresh */
					refresh_delay = atoi(content);

					string_tokenizer_tokenize(e->st, content, ",;>");
					while ( string_tokenizer_has_more_tokens (e->st) ) {
						const gchar* token = string_tokenizer_next_token(e->st);
						if ( strncasecmp( token, "url=", 4 ) == 0 ) {
							token += 4;

							if(*token == '#') {
								refresh_url = html_url_dup(e->actualURL, HTML_URL_DUP_NOREFERENCE);
								html_url_set_reference(refresh_url, token + 1);

							} else {
								refresh_url = parse_href(e, token);
							}
						}
						
					}
					if(refresh_url == NULL)
						refresh_url = html_url_dup(e->actualURL, 0);
					
					if(refresh_url)
						url = html_url_to_string(refresh_url);
					if(e->actualURL)
						actualURL = html_url_to_string(e->actualURL);
					
					if(!(refresh_delay == 0 && !strcasecmp(url, actualURL)))
						gtk_signal_emit (GTK_OBJECT (e), signals[REDIRECT], url, refresh_delay);
					if(url)
						g_free(url);
					
					if(actualURL)
						g_free(actualURL);

				}
			}
		}
	}
}

/* FIXME TODO parse_n missing. */


/*
<ol>             </ol>           partial
<option
*/
/* EP CHECK: `<ol>' OK, missing `<option>' */
static void
parse_o (HTMLEngine *e, HTMLObject *_clue, const gchar *str )
{
	if ( strncmp( str, "ol", 2 ) == 0 ) {
		HTMLListNumType listNumType;
		HTMLList *list;

		close_anchor (e);

		if ( html_stack_is_empty (e->listStack) ) {
			e->vspace_inserted = html_engine_insert_vspace
				(e, _clue, e->vspace_inserted );
			push_block (e, ID_OL, 2, block_end_list, e->indent, TRUE);
		} else {
			push_block (e, ID_OL, 2, block_end_list, e->indent, FALSE);
		}

		listNumType = HTML_LIST_NUM_TYPE_NUMERIC;

		string_tokenizer_tokenize( e->st, str + 3, " >" );

		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token;

			token = string_tokenizer_next_token (e->st);

			if ( strncasecmp( token, "type=", 5 ) == 0 ) {
				switch ( *(token+5) )
				{
				case 'i':
					listNumType = HTML_LIST_NUM_TYPE_LOWROMAN;
					break;

				case 'I':
					listNumType = HTML_LIST_NUM_TYPE_UPROMAN;
					break;

				case 'a':
					listNumType = HTML_LIST_NUM_TYPE_LOWALPHA;
					break;

				case 'A':
					listNumType = HTML_LIST_NUM_TYPE_UPALPHA;
					break;
				}
			}
		}

		list = html_list_new (HTML_LIST_TYPE_ORDERED, listNumType);
		html_stack_push (e->listStack, list);

		e->indent += INDENT_SIZE;
	}
	else if ( strncmp( str, "/ol", 3 ) == 0 ) {
		pop_block (e, ID_OL, _clue);
	}
#if 0
	else if ( strncmp( str, "option", 6 ) == 0 ) {
		if ( !formSelect )
			return;

		QString value = 0;
		bool selected = false;

		stringTok->tokenize( str + 7, " >" );
		while ( stringTok->hasMoreTokens() )
		{
			const char* token = stringTok->nextToken();
			if ( strncasecmp( token, "value=", 6 ) == 0 )
			{
				const char *p = token + 6;
				value = p;
			}
			else if ( strncasecmp( token, "selected", 8 ) == 0 )
			{
				selected = true;
			}
		}

		if ( inOption )
			formSelect->setText( formText );

		formSelect->addOption( value, selected );

		inOption = true;
		formText = "";
	} else if ( strncmp( str, "/option", 7 ) == 0 ) {
		if ( inOption )
			formSelect->setText( formText );
		inOption = false;
	}
#endif
}


/*
  <p
  <pre             </pre>
*/
/* EP CHECK: OK except for the `<pre>' font.  */
static void
parse_p (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if ( strncmp( str, "pre", 3 ) == 0 ) {
		e->vspace_inserted = html_engine_insert_vspace (e, clue,
								e->vspace_inserted );
		select_font_full ( e,
				   e->settings->fixedFontFace,
				   e->settings->fontBaseSize,
				   FALSE, FALSE, FALSE );
		e->flow = 0;
		e->inPre = TRUE;
		push_block (e, ID_PRE, 2, block_end_pre, 0, 0);
	} else if ( strncmp( str, "/pre", 4 ) == 0 ) {
		pop_block (e, ID_PRE, clue);
	} else if (*(str) == 'p' && ( *(str + 1) == ' ' || *(str + 1) == '>')) {
		HTMLHAlignType align;
		gchar *token;

		close_anchor (e);
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
		align = e->divAlign;
		
		string_tokenizer_tokenize (e->st, (gchar *)(str + 2), " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			token = string_tokenizer_next_token (e->st);
			if (strncasecmp (token, "align=", 6) == 0) {
				if (strcasecmp (token + 6, "center") == 0)
					align = HTML_HALIGN_CENTER;
				else if (strcasecmp (token + 6, "right") == 0)
					align = HTML_HALIGN_RIGHT;
				else if (strcasecmp (token + 6, "left") == 0)
					align = HTML_HALIGN_LEFT;
			}
			
		}
		if (align != e->divAlign) {
			if (e->flow == 0) {
				html_engine_new_flow (e, clue);
				HTML_CLUE (e->flow)->halign = align;
			}
		}
	}
	else if (*(str) == '/' && *(str + 1) == 'p' && 
		 (*(str + 2) == ' ' || *(str + 2) == '>')) {
		close_anchor (e);
		e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
	}
}


/*
  <small>             </small>
*/
static void
parse_s (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if ( strncmp(str, "small", 3 ) == 0 ) {
		select_font_relative (e, -1);
		push_block (e, ID_BIG, 1, block_end_font, 0, 0);
	} else if ( strncmp(str, "/small", 4 ) == 0 ) {
		pop_block (e, ID_BIG, clue);
	}
	
}


/*
  <table           </table>        most
  <textarea        </textarea>
  <title>          </title>
  <tt>             </tt>
*/
/* EP CHECK: `<tt>' uses the wrong font.  `<textarea>' is missing.  Rest is
   OK.  */
static void
parse_t (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "table", 5) == 0) {
		close_anchor (e);

		if (!e->vspace_inserted || !e->flow)
			html_engine_new_flow (e, clue);

		parse_table (e, e->flow, clue->max_width, str + 6);

		e->flow = 0;
	}
	else if (strncmp (str, "title", 5) == 0) {
		e->inTitle = TRUE;
		e->title = g_string_new ("");
	}
	else if (strncmp (str, "/title", 6) == 0) {
		e->inTitle = FALSE;

		gtk_signal_emit (GTK_OBJECT (e), signals[TITLE_CHANGED]);
	}
	else if ( strncmp( str, "tt", 2 ) == 0 ) {
		select_font_full ( e, e->settings->fixedFontFace,
				   e->settings->fontBaseSize,
				   FALSE, FALSE, FALSE );
		push_block (e, ID_TT, 1, block_end_font, 0, 0);
	} else if ( strncmp( str, "/tt", 3 ) == 0 ) {
		pop_block (e, ID_TT, clue);
	}
}


/*
  <u>              </u>
  <ul              </ul>
*/
/* EP CHECK: OK */
static void
parse_u (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "ul", 2) == 0) {
		HTMLListType type;

		close_anchor (e);

		if (html_stack_is_empty (e->listStack)) {
			e->vspace_inserted = html_engine_insert_vspace (e, clue, e->vspace_inserted);
			push_block (e, ID_UL, 2, block_end_list, e->indent, TRUE);
		} else {
			push_block (e, ID_UL, 2, block_end_list, e->indent, FALSE);
		}

		type = HTML_LIST_TYPE_UNORDERED;

		string_tokenizer_tokenize (e->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			if (strncasecmp (token, "plain", 5) == 0)
				type = HTML_LIST_TYPE_UNORDEREDPLAIN;
		}
		
		html_stack_push (e->listStack,
				 html_list_new (type, HTML_LIST_NUM_TYPE_NUMERIC));
		e->indent += INDENT_SIZE;
		e->flow = 0;
	}
	else if (strncmp (str, "/ul", 3) == 0) {
		pop_block (e, ID_UL, clue);
	}
	else if (strncmp (str, "u", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			e->underline = TRUE;
			html_engine_select_font (e);
			push_block (e, ID_U, 1, block_end_font, FALSE, FALSE);
		}
	}
	else if (strncmp (str, "/u", 2) == 0) {
		pop_block (e, ID_U, clue);
	}
}


/*
  <var>            </var>
*/
/* EP CHECK: OK */
static void
parse_v (HTMLEngine *e, HTMLObject * _clue, const char *str )
{
	if ( strncmp(str, "var", 3 ) == 0 ) {
		e->italic = TRUE;
		html_engine_select_font (e);
	   	push_block(e, ID_VAR, 1, block_end_font, 0, 0);
	} else if ( strncmp( str, "/var", 4 ) == 0) {
		pop_block(e, ID_VAR, _clue);
	}
}


/* Parsing vtable.  */

typedef void (*HTMLParseFunc)(HTMLEngine *p, HTMLObject *clue, const gchar *str);
static HTMLParseFunc parseFuncArray[26] = {
	parse_a,
	parse_b,
	parse_c,
	parse_d,
	parse_e,
	parse_f,
	NULL,
	parse_h,
	parse_i,
	NULL,
	parse_k,
	parse_l,
	parse_m,
	NULL,
	parse_o,
	parse_p,
	NULL,
	NULL,
	parse_s,
	parse_t,
	parse_u,
	parse_v,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str)
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
			if (parseFuncArray[indx] != NULL) {
				(* parseFuncArray[indx])(p, clue, str);
			} else {
				g_warning ("Unsupported tag `%s'", str);
			}
		}

	}
}


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
html_engine_destroy (GtkObject *object)
{
	HTMLEngine *engine = HTML_ENGINE (object);
	GList *p;

	/* FIXME FIXME FIXME */

	html_cursor_destroy (engine->cursor);

	html_url_destroy (engine->actualURL);
	
	html_tokenizer_destroy   (engine->ht);
	string_tokenizer_destroy (engine->st);
	html_settings_destroy    (engine->settings);
	html_settings_destroy    (engine->defaultSettings);
	html_painter_destroy     (engine->painter);
	html_image_factory_free  (engine->image_factory);

	html_stack_destroy (engine->cs);
	html_stack_destroy (engine->fs);
	html_stack_destroy (engine->listStack);
	html_stack_destroy (engine->glossaryStack);

	for (p = engine->tempStrings; p != NULL; p = p->next)
		g_free (p->data);
	g_list_free (engine->tempStrings);

	html_draw_queue_destroy (engine->draw_queue);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
html_engine_class_init (HTMLEngineClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	signals [SET_BASE] =
		gtk_signal_new ("set_base",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, set_base),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	signals [SET_BASE_TARGET] =
		gtk_signal_new ("set_base_target",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, set_base_target),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	signals [LOAD_DONE] = 
		gtk_signal_new ("load_done",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, load_done),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	signals [TITLE_CHANGED] = 
		gtk_signal_new ("title_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, title_changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	signals [URL_REQUESTED] =
		gtk_signal_new ("url_requested",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, url_requested),
				gtk_marshal_NONE__POINTER_POINTER,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_STRING,
				GTK_TYPE_POINTER);

	signals [DRAW_PENDING] =
		gtk_signal_new ("draw_pending",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, draw_pending),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	signals [REDIRECT] =
		gtk_signal_new ("redirect",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, redirect),
				gtk_marshal_NONE__POINTER_INT,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_STRING,
				GTK_TYPE_INT);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = html_engine_destroy;

	/* Initialize the HTML objects.  */
	html_types_init ();
}

static void
html_engine_init (HTMLEngine *engine)
{
	/* STUFF might be missing here!   */

	engine->actualURL = NULL;
	engine->newPage = FALSE;

	engine->editable = FALSE;
	engine->cursor = html_cursor_new ();

	engine->ht = html_tokenizer_new ();
	engine->st = string_tokenizer_new ();
	engine->settings = html_settings_new ();
	engine->defaultSettings = html_settings_new ();
	engine->painter = html_painter_new ();
	engine->image_factory = html_image_factory_new(engine);

	engine->fs = html_stack_new ((HTMLStackFreeFunc) html_font_destroy);
	engine->cs = html_stack_new ((HTMLStackFreeFunc) gdk_color_free);
	engine->listStack = html_stack_new ((HTMLStackFreeFunc) html_list_destroy);
	engine->glossaryStack = html_stack_new (NULL);

	engine->url = NULL;
	engine->target = NULL;

	engine->leftBorder = LEFT_BORDER;
	engine->rightBorder = RIGHT_BORDER;
	engine->topBorder = TOP_BORDER;
	engine->bottomBorder = BOTTOM_BORDER;

	engine->inPre = FALSE;
	engine->inTitle = FALSE;

	engine->tempStrings = NULL;

	engine->draw_queue = html_draw_queue_new (engine);
}

HTMLEngine *
html_engine_new (void)
{
	HTMLEngine *engine;

	engine = gtk_type_new (html_engine_get_type ());

	return engine;
}


static void
draw_background (HTMLEngine *e, gint xval, gint yval, gint x, gint y, gint w, gint h)
{
	gint xoff = 0;
	gint yoff = 0;
	gint pw, ph, yp, xp;
	gint xOrigin, yOrigin;
	HTMLImagePointer *bgpixmap;

	xoff = xval;
	yoff = yval;
	xval = e->x_offset;
	yval = e->y_offset;

	bgpixmap = e->bgPixmapPtr;
	if (!bgpixmap || !bgpixmap->pixbuf) {
		/* FIXME it used to check if the bgColor was not set.  */
		html_painter_set_pen (e->painter, &e->bgColor);
		html_painter_fill_rect (e->painter, x, y, w, h);
		return;
	}

	pw = bgpixmap->pixbuf->art_pixbuf->width;
	ph = bgpixmap->pixbuf->art_pixbuf->height;

	xOrigin = x / pw*pw - xval % pw;
	yOrigin = y / ph*ph - yval % ph;

	xOrigin -= e->painter->x1;
	yOrigin -= e->painter->y1;

	/* Do the bgimage tiling */
	for (yp = yOrigin; yp < y + h; yp += ph) {
	        for (xp = xOrigin; xp < x + w; xp += pw) {
			html_painter_draw_background_pixmap (e->painter, 
							     xp, yp,
							     bgpixmap->pixbuf, 0, 0);
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

GtkHTMLStreamHandle
html_engine_begin (HTMLEngine *p, const char *url)
{
	GtkHTMLStream *new_stream;

	html_tokenizer_begin (p->ht);
	
	free_block (p); /* Clear the block stack */

	html_engine_stop_parser (p);
	p->writing = TRUE;

	new_stream = gtk_html_stream_new(GTK_HTML(p->widget),
					 url,
					 (GtkHTMLStreamWriteFunc)html_engine_write,
					 (GtkHTMLStreamEndFunc)html_engine_end,
					 (gpointer)p);

	html_engine_set_base_url (p, url);

	p->newPage = TRUE;

	gtk_signal_emit (GTK_OBJECT(p), signals [URL_REQUESTED], url, new_stream);

	return new_stream;
}

void
html_engine_write (GtkHTMLStreamHandle handle, gchar *buffer, size_t size, HTMLEngine *e)
{
	if (buffer == NULL)
		return;

	html_tokenizer_write (e->ht, buffer, size);

	if (e->parsing && e->timerId == 0) {
		e->timerId = gtk_timeout_add (TIMER_INTERVAL,
					      (GtkFunction) html_engine_timer_event,
					      e);
	}
}

static gboolean
html_engine_update_event (HTMLEngine *e)
{
	e->updateTimer = 0;

	html_engine_calc_size (e);
	
	/* Scroll page to the top on first display */
	if (e->newPage) {
		gtk_adjustment_set_value (GTK_LAYOUT (e->widget)->vadjustment, 0);
		e->newPage = FALSE;
	}

	html_engine_draw (e, 0, 0, e->width, e->height);
	
	if (!e->parsing) {
		/* Parsing done */
		
		/* Is y_offset too big? */
		if (html_engine_get_doc_height (e) - e->y_offset < e->height) {
			e->y_offset = html_engine_get_doc_height (e) - e->height;
			if (e->y_offset < 0)
				e->y_offset = 0;
		}
		
		/* Is x_offset too big? */
		if (html_engine_get_doc_width (e) - e->x_offset < e->width) {
			e->x_offset = html_engine_get_doc_width (e) - e->width;
			if (e->x_offset < 0)
				e->x_offset = 0;
		}

		/* Adjust the scrollbars */
		gtk_html_calc_scrollbars (e->widget);

	}

	return FALSE;
}

void
html_engine_schedule_update (HTMLEngine *p)
{
	if(p->updateTimer == 0)
		p->updateTimer = gtk_timeout_add (TIMER_INTERVAL, (GtkFunction) html_engine_update_event, p);
}

static gboolean
html_engine_timer_event (HTMLEngine *e)
{
	static const gchar *end[] = { "</body>", 0};
	HTMLFont *oldFont;
	gint lastHeight;
	gboolean retval = TRUE;

	/* Has more tokens? */
	if (!html_tokenizer_has_more_tokens (e->ht) && e->writing)
	{
		retval = FALSE;
		goto out;
	}

	/* Storing font info */
	oldFont = html_painter_get_font (e->painter);

	/* Setting font */
	html_painter_set_font (e->painter, html_stack_top (e->fs));

	/* Getting height */
	lastHeight = html_engine_get_doc_height (e);

	/* Restoring font */
	html_painter_set_font (e->painter, oldFont);

	e->parseCount = e->granularity;

	/* Parsing body height */
	if (parse_body (e, e->clue, end, TRUE))
	  	html_engine_stop_parser (e);

	html_engine_schedule_update (e);

	if(!e->parsing)
		retval = FALSE;

 out:
	if(!retval)
		e->timerId = 0;

	return retval;
}

static void
html_engine_end (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, HTMLEngine *e)
{
	e->writing = FALSE;

	html_tokenizer_end (e->ht);
	gtk_signal_emit (GTK_OBJECT (e), signals[LOAD_DONE]);
	html_image_factory_cleanup (e->image_factory);

	if (e->editable)
		html_cursor_home (e->cursor, e);
}

void
html_engine_draw (HTMLEngine *e, gint x, gint y, gint width, gint height)
{
	gint tx, ty;

	tx = -e->x_offset + e->leftBorder;
	ty = -e->y_offset + e->topBorder;

	html_painter_begin (e->painter, x, y, x + width, y + height);

	draw_background (e, e->x_offset, e->y_offset, x, y, width, height);

	if (e->clue)
		html_object_draw (e->clue, e->painter, e->cursor,
				  x - e->x_offset,
				  y + e->y_offset - e->topBorder,
				  width,
				  height,
				  tx, ty);

	html_painter_end (e->painter);
}

gint
html_engine_get_doc_width (HTMLEngine *e)
{
	if (e->clue)
		return e->clue->width + e->leftBorder + e->rightBorder;
	else
		return e->leftBorder + e->rightBorder;
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

	html_object_reset (p->clue);

	max_width = p->width - p->leftBorder - p->rightBorder;
	p->clue->width = max_width;

	min_width = html_object_calc_min_width (p->clue);
	if (min_width > max_width)
		max_width = min_width;

	html_object_set_max_width (p->clue, max_width);
	html_object_calc_size (p->clue, NULL);

	p->clue->x = 0;
	p->clue->y = p->clue->ascent;
}

void
html_engine_new_flow (HTMLEngine *e, HTMLObject *clue)
{
	if (e->inPre) 
		e->flow = html_clueh_new ( 0, 0, clue->max_width );
	else
		e->flow = html_clueflow_new ();

	HTML_CLUEFLOW (e->flow)->indent = e->indent;
	HTML_CLUE (e->flow)->halign = e->divAlign;

	html_clue_append (HTML_CLUE (clue), e->flow);
}

void
html_engine_parse (HTMLEngine *p)
{
	HTMLFont *f;

	g_print ("parse\n");
	html_engine_stop_parser (p);

	if (p->clue != NULL)
		html_object_destroy (p->clue);

	p->flow = 0;

	p->clue = html_cluev_new (0, 0, p->width - p->leftBorder - p->rightBorder, 100);
	HTML_CLUE (p->clue)->valign = HTML_VALIGN_TOP;
	HTML_CLUE (p->clue)->halign = HTML_HALIGN_LEFT;

	p->cursor->object = p->clue;

	/* Initialize the font stack with the default font */
	p->italic = FALSE;
	p->underline = FALSE;
	p->bold = FALSE;
	p->fontsize = p->settings->fontBaseSize;

	f = html_font_new (p->settings->fontBaseFace,
			   p->settings->fontBaseSize,
			   p->defaultSettings->fontSizes,
			   p->bold, p->italic, p->underline);

	html_stack_push (p->fs, f);

	/* Free the background pixmap */
	if (p->bgPixmapPtr) {
		html_image_factory_unregister(p->image_factory, p->bgPixmapPtr, NULL);
		p->bgPixmapPtr = NULL;
	}

	html_settings_alloc_colors
		(p->defaultSettings,
		 gdk_window_get_colormap (html_painter_get_window (p->painter)));
	html_settings_copy (p->settings, p->defaultSettings);

	/* Free the background color (if any) and alloc a new one */
	p->bgColor = p->settings->bgColor;

	html_stack_push (p->cs, gdk_color_copy (&p->settings->fontBaseColor));
	html_font_set_color (f, html_stack_top (p->cs));

	p->bodyParsed = FALSE;

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
		f = html_clueflow_new ();

		html_clue_append (HTML_CLUE (clue), f);

		/* FIXME: correct font size */
		t = html_vspace_new (e->defaultSettings->fontSizes[e->settings->fontBaseSize],
				     HTML_CLEAR_NONE);
		html_clue_append (HTML_CLUE (f), t);
		
		close_flow (e);
	}
	
	return TRUE;
}

void
html_engine_select_font (HTMLEngine *e)
{
	HTMLFont *f, *g;

	if (e->fontsize <0)
		e->fontsize = 0;
	else if (e->fontsize >= HTML_NUM_FONT_SIZES)
		e->fontsize = HTML_NUM_FONT_SIZES - 1;

	g = html_stack_top (e->fs);
	f = html_font_new (g->family, e->fontsize,
			   e->defaultSettings->fontSizes,
			   e->bold, e->italic, e->underline);

	html_font_set_color (f, html_stack_top (e->cs));

	html_stack_push (e->fs, f);
	html_painter_set_font (e->painter, f);
}

void
html_engine_pop_color (HTMLEngine *e)
{
	gdk_color_free (html_stack_pop (e->cs));

	/* FIXME: Check for empty */
}

void
html_engine_pop_font (HTMLEngine *e)
{
	HTMLFont *top;

	html_stack_pop (e->fs);
	
	top = html_stack_top (e->fs);
	if (! top) {
		g_warning ("No fonts on top.");
		return;
	}

	html_painter_set_font (e->painter, top);

	e->fontsize = top->size;
	e->bold = top->bold;
	e->italic = top->italic;
	e->underline = top->underline;
}

void
html_engine_insert_text (HTMLEngine *e, const gchar *str, HTMLFont *f)
{
	enum {unknown, fixed, variable} textType = unknown;
	int i = 0;
	HTMLObject *obj;
	gchar *remainingStr = 0;
	gboolean insertSpace = FALSE;
	gboolean insertNBSP = FALSE;
	gboolean insertBlock = FALSE;
	gchar *s;
	gchar *origStr;

	/* FIXME: Huge waste here and ugliness.  But the way it was done
           previously was just plainly broken.  We need to fix memory
           management here in a major way.  */

	origStr = g_strdup (str);
	s = origStr;

	for (;;) {
		if (((guchar *)s)[i] == 0xa0) {
			/* Non-breaking space */
			if (textType == variable) {
				/* We have a non-breaking space in a block of variable text.  We
				   need to split the text and insert a seperate non-breaking
				   space object */
				s[i] = 0x00; /* End of string */
				remainingStr = &(s[i+1]);
				insertBlock = TRUE; 
				insertNBSP = TRUE;
			} else {
				/* We have a non-breaking space: this makes the block fixed. */
				s[i] = 0x20; /* Normal space */
				textType = fixed;
			}
		} else if (s[i] == 0x20) {
			/* Normal space */
			if (textType == fixed) {
				/* We have a normal space in a block of fixed text.
				   We need to split the text and insert a separate normal space. */
				s[i] = 0x00; /* End of string */
				remainingStr = &(s [i+1]);
				insertBlock = TRUE;
				insertSpace = TRUE;
			}
			else {
				/* We have a normal space, if this is the first
				   character, we insert a normal space and continue */
				if (i == 0) {
					if (s [i+1] == 0x00) {
						s++;
						remainingStr = 0;
					}
					else {
						s [i] = 0x00; /* End of string */
						remainingStr = s+1;
					}
					insertBlock = TRUE;	/* Block is zero-length, no actual insertion */
					insertSpace = TRUE;
				}
				else if (s [i+1] == 0x00) {
					/* Last character is a space: Insert the block and 
					   a normal space */
					s[i] = 0x00;/* End of string */
					remainingStr = 0;
					insertBlock = TRUE;
					insertSpace = TRUE;
				} else {
					textType = variable;
				}
			}
		}
		else if (s[i] == 0x00) {
			/* End of string */
			insertBlock = TRUE;
			remainingStr = 0;
		}

		if (insertBlock) {
			if (*s) {
				gchar *url_string;

				/* FIXME this sucks */
				if (e->url != NULL)
					url_string = html_url_to_string (e->url);
				else
					url_string = NULL;

				if (textType == variable) {
					if (e->url || e->target)
						obj = html_link_text_master_new
							(g_strdup (s), f, url_string, e->target);
					else
						obj = html_text_master_new (g_strdup (s), f);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
				else {
					if (e->url || e->target)
						obj = html_link_text_new (g_strdup (s), f,
									  url_string, e->target);
					else
						obj = html_text_new (g_strdup (s), f);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}

				g_free (url_string);
			}

			if (insertSpace) {
				if (e->url || e->target) {
					gchar *url_string;

					url_string = html_url_to_string (e->url);
					obj = html_link_text_new (g_strdup (" "), f,
								  url_string, e->target);
					obj->flags |= HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
					g_free (url_string);
				} else {
					obj = html_hspace_new (f, FALSE);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
			}
			else if (insertNBSP) {
				if (e->url || e->target) {
					gchar *url_string;

					/* FIXME this sucks */
					url_string = html_url_to_string (e->url);
					obj = html_link_text_new (g_strdup (" "), f, url_string,
								  e->target);
					obj->flags &= ~HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
					g_free (url_string);
				} else {
					obj = html_hspace_new (f, FALSE);
					obj->flags &= ~HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
			}
		
			s = remainingStr;

			if (s == 0 || *s == 0x00) {
				g_free (origStr);
				return;
			}

			i = 0;
			textType = unknown;
			insertBlock = FALSE;
			insertSpace = FALSE;
			insertNBSP = FALSE;
		} else {
			i++;
		}
	}
}

HTMLFont *
html_engine_get_current_font (HTMLEngine *p)
{
	return html_stack_top (p->fs);
}

gboolean
html_engine_set_named_color (HTMLEngine *p, GdkColor *c, const gchar *name)
{
	if (! gdk_color_parse (name, c))
		return FALSE;
	gdk_colormap_alloc_color (gdk_window_get_colormap
				  (html_painter_get_window (p->painter)),
				  c, FALSE, TRUE);
	return TRUE;
}

#if 0
char *
html_engine_canonicalize_url (HTMLEngine *e, const char *in_url)
{
	char *ctmp, *ctmp2, *retval, *removebegin, *removeend, *curpos;

	g_return_val_if_fail(e, NULL);
	g_return_val_if_fail(in_url, NULL);

	ctmp = strstr(in_url, "://");
	if(ctmp)
	{
		retval = g_strdup(in_url);
		goto out;
	}
	else if(*in_url == '/')
	{
		ctmp = e->baseURL?strstr(e->baseURL, "://"):NULL;
		if(!ctmp)
		{
			retval = g_strconcat("file://", in_url, NULL);
			goto out;
		}

		ctmp2 = strchr(ctmp + 3, '/');

		retval = g_strconcat(e->baseURL, in_url, NULL);
		goto out;
	}

	/* XXX TODO - We should really do processing of .. and . in URLs */

	ctmp = e->baseURL?strstr(e->baseURL, "://"):NULL;
	if(!ctmp)
	{
		char *cwd;

		cwd = g_get_current_dir();
		ctmp = g_strconcat("file://", cwd, "/", in_url, NULL);
		g_free(cwd);

		retval = ctmp;
		goto out;
	}

	retval = g_strconcat(e->baseURL, "/", in_url, NULL);

 out:
	/* Now fix up the /. and /.. pieces */

	ctmp = strstr(retval, "://");
	g_assert(ctmp);
	ctmp += 3;
	ctmp = strchr(ctmp, '/');
	if(!ctmp) {
		ctmp = retval;
		retval = g_strconcat(retval, "/", NULL);
		g_free(ctmp);
		return retval;
	}

	removebegin = removeend = NULL;
	do {
		if(removebegin && removeend)
		{
			memmove(removebegin, removeend, strlen(removeend) + 1);
			removebegin = removeend = NULL;
		}
		curpos = ctmp;

	redo:
		ctmp2 = strstr(curpos, "/.");
		if(!ctmp2)
			break;

		if(*(ctmp2 + 2) == '.') /* We have to skip over stuff like /...blahblah or /.foo */
		{
			if(*(ctmp2 + 3) != '/'
			   && *(ctmp2 + 3) != '\0')
			{
				curpos = ctmp2 + 3;
				goto redo;
			}
		}
		else if(*(ctmp2 + 2) != '/' && *(ctmp2 + 2) != '\0')
		{
			curpos = ctmp2 + 2;
			goto redo;
		}

		switch(*(ctmp2+2))
		{
		case '/':
		case '\0':
			removebegin = ctmp2;
			removeend = ctmp2 + 2;
			break;
		case '.':
			removeend = ctmp2 + 3;
			ctmp2--;
			while((ctmp2 >= ctmp) && *ctmp2 != '/')
				ctmp2--;
			if(*ctmp2 == '/')
				removebegin = ctmp2;
			break;
		}

	} while(removebegin);

	return retval;
}
#endif

const gchar *
html_engine_get_link_at (HTMLEngine *e, gint x, gint y)
{
	HTMLObject *obj;

	if ( e->clue == NULL )
		return NULL;

#if 0
	// Make this frame the active one
	if ( bIsFrame && !bIsSelected )
		htmlView->setSelected( TRUE );
#endif

	obj = html_object_check_point (HTML_OBJECT (e->clue),
				       x + e->x_offset - e->leftBorder,
				       y + e->y_offset - e->topBorder);

	if ( obj != 0 )
		return html_object_get_url (obj);

	return NULL;
}

void
html_engine_set_editable (HTMLEngine *e, gboolean editable)
{
	e->editable = editable;
}

void
html_engine_set_base_url (HTMLEngine *e, const char *url)
{
	if (e->actualURL != NULL)
		html_url_destroy (e->actualURL);
	e->actualURL = html_url_new (url);

	gtk_signal_emit (GTK_OBJECT (e), signals[SET_BASE], url);
}


void
html_engine_make_cursor_visible (HTMLEngine *e)
{
	HTMLCursor *cursor;
	HTMLObject *object;
	gint x, y;

	g_return_if_fail (e != NULL);

	if (! e->editable)
		return;

	cursor = e->cursor;
	object = cursor->object;
	if (object == NULL)
		return;

	if (cursor->offset == 0)
		html_object_calc_abs_position (object, &x, &y);
	else
		html_text_calc_char_position (HTML_TEXT (object), cursor->offset, &x, &y);

	x += e->leftBorder;
	y += e->topBorder;

	if (x + e->leftBorder >= e->x_offset + e->width)
		e->x_offset = x + e->leftBorder - e->width + 1;
	else if (x - e->leftBorder < e->x_offset)
		e->x_offset = x - e->leftBorder;

	if (y + object->descent + e->topBorder >= e->y_offset + e->height)
		e->y_offset = y + object->descent + e->topBorder - e->height + 1;
	else if (y - object->ascent - e->topBorder < e->y_offset)
		e->y_offset = y - object->ascent - e->topBorder;
}


void
html_engine_flush_draw_queue (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);

	html_draw_queue_flush (e->draw_queue);
}

void
html_engine_queue_draw (HTMLEngine *e, HTMLObject *o)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (o != NULL);

	html_draw_queue_add (e->draw_queue, o);
}
