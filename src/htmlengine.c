/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
    Copyright (C) 1997 Torben Weis (weis@kde.org)
    Copyright (C) 1999 Anders Carlsson (andersca@gnu.org)
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

/* FIXME check all the `push_block()'s and `set_font()'s.  */
/* FIXME there are many functions that should be static instead of being
   exported.  I will slowly do this as time permits.  -- Ettore  */
/* Some stuff is not re-initialized properly when clearing and reloading stuff
   from scratch -- Ettore */

/* RULE: You should never create a new flow without inserting anything in it.
   If `e->flow' is not NULL, it must contain something.  */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "gtkhtml-private.h"

#include "htmlengine.h"
#include "htmlengine-edit.h"

#include "htmlanchor.h"
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
#include "htmlclueflow.h"
#include "htmlstack.h"
#include "stringtokenizer.h"
#include "htmlform.h"
#include "htmlbutton.h"
#include "htmltextinput.h"
#include "htmlradio.h"
#include "htmlcheckbox.h"
#include "htmlhidden.h"
#include "htmlselect.h"
#include "htmltextarea.h"
#include "htmlimageinput.h"
#include "htmlstack.h"


static void html_engine_class_init (HTMLEngineClass *klass);
static void html_engine_init (HTMLEngine *engine);
static gboolean html_engine_timer_event (HTMLEngine *e);
static gboolean html_engine_update_event (HTMLEngine *e);
static void html_engine_write (GtkHTMLStreamHandle handle, gchar *buffer, size_t size, HTMLEngine *e);
static void html_engine_end (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, HTMLEngine *e);

static void parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str);
static void parse_input (HTMLEngine *e, const gchar *s, HTMLObject *_clue);
static void parse_f (HTMLEngine *p, HTMLObject *clue, const gchar *str);
static gboolean html_engine_goto_anchor (HTMLEngine *e);
static void html_engine_set_base_url (HTMLEngine *e, const char *url);


static GtkLayoutClass *parent_class = NULL;

enum {
	SET_BASE_TARGET,
	SET_BASE,
	LOAD_DONE,
	TITLE_CHANGED,
	URL_REQUESTED,
	DRAW_PENDING,
	REDIRECT,
	SUBMIT,
	OBJECT_REQUESTED,
	LAST_SIGNAL
};
	
static guint signals [LAST_SIGNAL] = { 0 };

#define TIMER_INTERVAL 30

enum ID {
	ID_ADDRESS, ID_B, ID_BIG, ID_BLOCKQUOTE, ID_CAPTION, ID_CITE, ID_CODE,
	ID_DIR, ID_DIV, ID_EM, ID_FONT, ID_HEADER, ID_I, ID_KBD, ID_OL, ID_PRE,
	ID_SMALL, ID_U, ID_UL, ID_TEXTAREA, ID_TD, ID_TH, ID_TT, ID_VAR
};


/* Font handling.  */

static HTMLFontStyle
current_font_style (HTMLEngine *e)
{
	HTMLFontStyle style;

	if (html_stack_is_empty (e->font_style_stack))
		return HTML_FONT_STYLE_DEFAULT;

	style = GPOINTER_TO_INT (html_stack_top (e->font_style_stack));
	return style;
}

static HTMLFontStyle
push_font_style (HTMLEngine *e,
		 HTMLFontStyle new_attrs)
{
	HTMLFontStyle current;
	HTMLFontStyle new;

	current = current_font_style (e);
	new = html_font_style_merge (current, new_attrs);

	html_stack_push (e->font_style_stack, GINT_TO_POINTER (new));

	return new;
}

static void
pop_font_style (HTMLEngine *e)
{
	html_stack_pop (e->font_style_stack);
}


/* Color handling.  */

static const GdkColor *
current_color (HTMLEngine *e)
{
	static GdkColor black = { 0, 0, 0, 0 };
	const GdkColor *color;

	if (html_stack_is_empty (e->color_stack))
		return &black;	/* FIXME use settings */

	color = html_stack_top (e->color_stack);
	return color;
}

static void
push_color (HTMLEngine *e,
	    GdkColor *color)
{
	html_stack_push (e->color_stack, color);
}

static void
pop_color (HTMLEngine *e)

{
	html_stack_pop (e->color_stack);
}

static gboolean
parse_color (const gchar *text,
	     GdkColor *color)
{
	gchar *tmp;

	if (gdk_color_parse (text, color))
		return TRUE;

	tmp = alloca (strlen (text) + 2);
	*tmp = '#';
	strcpy (tmp + 1, text);

	return gdk_color_parse (tmp, color);
}


/* ClueFlow style handling.  */

static HTMLClueFlowStyle
current_clueflow_style (HTMLEngine *e)
{
	HTMLClueFlowStyle style;

	if (html_stack_is_empty (e->clueflow_style_stack))
		return HTML_CLUEFLOW_STYLE_NORMAL;

	style = (HTMLClueFlowStyle) GPOINTER_TO_INT (html_stack_top (e->clueflow_style_stack));
	return style;
}

static void
push_clueflow_style (HTMLEngine *e,
		     HTMLClueFlowStyle style)
{
	html_stack_push (e->clueflow_style_stack, GINT_TO_POINTER (style));
}

static void
pop_clueflow_style (HTMLEngine *e)
{
	html_stack_pop (e->clueflow_style_stack);
}



/* Utility functions.  */

static void new_flow (HTMLEngine *e, HTMLObject *clue, HTMLObject *first_object);
static void close_flow (HTMLEngine *e, HTMLObject *clue);

static HTMLObject *
create_empty_text (HTMLEngine *e)
{
	return html_text_master_new (g_strdup (""), current_font_style (e), current_color (e));
}

static void
insert_paragraph_break (HTMLEngine *e,
			HTMLObject *clue)
{
	close_flow (e, clue);
	new_flow (e, clue, create_empty_text (e));
	close_flow (e, clue);
}

static void
add_line_break (HTMLEngine *e,
		HTMLObject *clue,
		HTMLClearType clear)
{
	if (e->flow != NULL) {
		if (HTML_OBJECT_TYPE (HTML_CLUE (e->flow)->tail) == HTML_TYPE_VSPACE)
			html_clue_append (HTML_CLUE (e->flow), create_empty_text (e));
		html_clue_append (HTML_CLUE (e->flow), html_vspace_new (clear));
		return;
	}

	new_flow (e, clue, create_empty_text (e));

	e->flow = NULL;
}

static void
close_anchor (HTMLEngine *e)
{
	if (e->url != NULL) {
		g_free (e->url);
		e->url = NULL;
	}

	g_free (e->target);
	e->target = NULL;
}

static void
close_flow (HTMLEngine *e,
	    HTMLObject *clue)
{
	HTMLObject *last;

	if (e->flow == NULL)
		return;

	last = HTML_CLUE (e->flow)->tail;
	if (last == NULL) {
		html_clue_append (HTML_CLUE (e->flow), create_empty_text (e));
	} else if (HTML_OBJECT_TYPE (last) == HTML_TYPE_VSPACE) {
		html_clue_remove (HTML_CLUE (e->flow), last);
		html_object_destroy (last);
	}

	e->flow = NULL;
}

static void
new_flow (HTMLEngine *e,
	  HTMLObject *clue,
	  HTMLObject *first_object)
{
	close_flow (e, clue);

	e->flow = html_clueflow_new (HTML_FONT_STYLE_DEFAULT,
				     current_clueflow_style (e),
				     e->list_level,
				     e->quote_level);

	HTML_CLUE (e->flow)->halign = e->divAlign;

	html_clue_append (HTML_CLUE (e->flow), first_object);
	html_clue_append (HTML_CLUE (clue), e->flow);
}

static void
append_element (HTMLEngine *e,
		HTMLObject *clue,
		HTMLObject *obj)
{
	if (e->pending_para) {
		insert_paragraph_break (e, clue);
		e->pending_para = FALSE;
	}

	e->avoid_para = FALSE;

	if (e->flow == NULL)
		new_flow (e, clue, obj);
	else
		html_clue_append (HTML_CLUE (e->flow), obj);
}


static gboolean
check_prev (const HTMLObject *p,
	    HTMLType type,
	    HTMLFontStyle font_style)
{
	if (p == NULL)
		return FALSE;

	if (HTML_OBJECT_TYPE (p) != type)
		return FALSE;

	if (HTML_TEXT (p)->font_style != font_style)
		return FALSE;

	return TRUE;
}

static void
insert_text (HTMLEngine *e,
	     HTMLObject *clue,
	     const gchar *text)
{
	HTMLFontStyle font_style;
	HTMLObject *prev;
	HTMLType type;
	const GdkColor *color;

	font_style = current_font_style (e);
	color = current_color (e);

	if (e->pending_para || e->flow == NULL || HTML_CLUE (e->flow)->head == NULL) {
		while (*text == ' ')
			text++;

		if (*text == 0)
			return;
	}

	if (e->flow == NULL)
		prev = NULL;
	else
		prev = HTML_CLUE (e->flow)->tail;

	if (e->url != NULL || e->target != NULL)
		type = HTML_TYPE_LINKTEXT;
	else
		type = HTML_TYPE_TEXT;

	if (! check_prev (prev, type, font_style)) {
		HTMLObject *obj;

		if (e->url != NULL || e->target != NULL)
			obj = html_link_text_master_new (g_strdup (text),
							 font_style,
							 color,
							 e->url,
							 e->target);
		else
			obj = html_text_master_new (g_strdup (text),
						    font_style,
						    color);

		append_element (e, clue, obj);
	} else {
		HTMLText *text_prev;
		gchar *new_text;

		g_warning ("Reusing existing text.\n");

		/* FIXME this sucks.  */

		text_prev = HTML_TEXT (prev);

		new_text = g_strconcat (text_prev->text, text, NULL);
		g_free (text_prev->text);
		text_prev->text = new_text;
	}
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
	    BlockFunc exitFunc,
	    gint miscData1, gint miscData2)
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
	pop_font_style (e);
}

static void
block_end_clueflow_style (HTMLEngine *e,
			  HTMLObject *clue,
			  HTMLBlockStackElement *elem)
{
	close_flow (e, clue);
	pop_clueflow_style (e);
}

static void
block_end_pre ( HTMLEngine *e, HTMLObject *_clue, HTMLBlockStackElement *elem)
{
	block_end_clueflow_style (e, _clue, elem);
	e->inPre = FALSE;
}

static void
block_end_color_font (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	pop_color (e);
	block_end_font (e, clue, elem);
}

static void
block_end_list (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	html_list_destroy (html_stack_pop (e->listStack));

	close_flow (e, clue);
	
	e->list_level = elem->miscData1;

	if (e->list_level == 0) {
		e->pending_para = FALSE;
		e->avoid_para = TRUE;
	}
}

static void
block_end_quote (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	close_flow (e, clue);

	e->quote_level = elem->miscData1;
	e->list_level = elem->miscData2;

	e->pending_para = FALSE;
	e->avoid_para = TRUE;
}

static void
block_end_div (HTMLEngine *e, HTMLObject *clue, HTMLBlockStackElement *elem)
{
	close_flow (e, clue);

	e->divAlign =  (HTMLHAlignType) elem->miscData1;
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
			/* if in* is set this text belongs in a form element */
			if (p->inOption || p->inTextArea)
				p->formText = g_string_append (p->formText, " ");
			else if (p->inTitle) {
				g_string_append (p->title, " ");
			} else if (p->flow != NULL) {
				insert_text (p, clue, " ");
			}
		} else if (*str != TAG_ESCAPE) {
			if (p->inOption || p->inTextArea)
				g_string_append (p->formText, str);
			else if (p->inTitle) {
				g_string_append (p->title, str);
			}
			else {
				insert_text (p, clue, str);
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
			if (*str == '\n')
				add_line_break (p, clue, HTML_CLEAR_NONE);
			else
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
	gint old_quote_level = e->quote_level;
	gint old_list_level = e->list_level;
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
			if (parse_color (token + 8, &tableColor)) {
				rowColor = tableColor;
				have_rowColor = have_tableColor = TRUE;
			}
		}
		else if (strncasecmp (token, "background=", 11) == 0
			 && !e->defaultSettings->forceDefault) {
			tablePixmapPtr = html_image_factory_register(e->image_factory, NULL, token + 11);

			if(tablePixmapPtr) {
				rowPixmapPtr = tablePixmapPtr;
				have_tablePixmap = have_rowPixmap = TRUE;
			}
		}
	}

	table = HTML_TABLE (html_table_new (0, 0, max_width, width, 
					    percent, padding,
					    spacing, border));
	e->list_level = 0;
	e->quote_level = 0;

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
						} else if (strncasecmp (token, "align", 6) == 0) {
							if (strcasecmp (token + 6, "left") == 0)
								rowhalign = HTML_HALIGN_LEFT;
							else if (strcasecmp (token + 6, "right") == 0)
								rowhalign = HTML_HALIGN_RIGHT;
							else if (strcasecmp (token + 6, "center") == 0)
								rowhalign = HTML_HALIGN_CENTER;
						} else if (strncasecmp (token, "bgcolor=", 8) == 0) {
							have_rowColor = parse_color (token + 8, &rowColor);
						} else if (strncasecmp (token, "background=", 11) == 0
							   && !e->defaultSettings->forceDefault) {
							rowPixmapPtr = html_image_factory_register(e->image_factory, NULL, token + 11);
							if(rowPixmapPtr)
								have_rowPixmap = TRUE;
						}
					}
					break;
				} /* Hack to fix broken html in bonsai */
				else if (strncmp (str, "<form", 5) == 0 || strncmp (str, "</form", 6) == 0) {

					parse_f (e, clue, str + 1);
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
					e->noWrap = FALSE;

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
								have_bgColor = parse_color (token + 8,
											    &bgColor);
							}
							else if (strncasecmp (token, "nowrap", 6) == 0) {

								e->flow = 0;
								e->noWrap = TRUE;

							}
							else if (strncasecmp (token, "background=", 11) == 0
								 && !e->defaultSettings->forceDefault) {
								
								bgPixmapPtr = html_image_factory_register(e->image_factory, 
													  NULL, token + 11);
								if(bgPixmapPtr)
									have_bgPixmap = TRUE;

							}
						}
					}

					if (e->pending_para) {
						insert_paragraph_break (e, clue);
						e->pending_para = FALSE;
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
					e->flow = NULL;

					e->avoid_para = TRUE;

					if (heading) {
						/* FIXME this is wrong.  */
						push_font_style (e, HTML_FONT_STYLE_BOLD);
						push_block (e, ID_TH, 3,
							    block_end_font,
							    FALSE, 0);
						str = parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
						pop_block (e, ID_TH, HTML_OBJECT (cell));
						if (e->pending_para) {
							insert_paragraph_break (e, HTML_OBJECT (cell));
							e->pending_para = FALSE;
						}
					}

					if (!tableEntry) {
						/* Put all the junk between <table>
						   and the first table tag into one row */
						push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endall, FALSE);
						pop_block (e, ID_TD, HTML_OBJECT (cell));

						if (e->pending_para) {
							insert_paragraph_break (e, HTML_OBJECT (cell));
							e->pending_para = FALSE;
						}

						html_table_end_row (table);
						html_table_start_row (table);
					} else {
						push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
						pop_block (e, ID_TD, HTML_OBJECT (cell));
						if (e->pending_para) {
							insert_paragraph_break (e, HTML_OBJECT (cell));
							e->pending_para = FALSE;
						}
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

					e->noWrap = FALSE;

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
		
	e->quote_level = old_quote_level;
	e->list_level = old_list_level;

	e->divAlign = olddivalign;

	e->flow = HTML_OBJECT (oldflow);

	if (has_cell) {
		/* The ending "</table>" might be missing, so we close the table
		   here...  */
		if (!firstRow)
			html_table_end_row (table);
		html_table_end_table (table);

		if (align != HTML_HALIGN_LEFT && align != HTML_HALIGN_RIGHT) {
			close_flow (e, clue);
			append_element (e, clue, HTML_OBJECT (table));
			close_flow (e, clue);
		} else {
			HTMLClueAligned *aligned;

			aligned = HTML_CLUEALIGNED (html_cluealigned_new (NULL,
									  0, 0,
									  clue->max_width,
									  100));
			HTML_CLUE (aligned)->halign = align;
			html_clue_append (HTML_CLUE (aligned), HTML_OBJECT (table));
			if (e->flow == NULL)
				new_flow (e, clue, HTML_OBJECT (aligned));
			else
				html_clue_append (HTML_CLUE (e->flow), HTML_OBJECT (aligned));
		}
	} else {
		/* Last resort: remove tables that do not contain any cells */
		html_object_destroy (HTML_OBJECT (table));
	}
	
	g_print ("Returning: %s\n", str);
	return str;
}

static void
parse_input (HTMLEngine *e, const gchar *str, HTMLObject *_clue) {
	enum InputType { CheckBox, Hidden, Radio, Reset, Submit, Text, Image,
			 Button, Password, Undefined };

	HTMLObject *element = NULL;
	const char *p;
	enum InputType type = Text;
	gchar *name = NULL;
	gchar *value = NULL;
	gchar *imgSrc = NULL;
	gboolean checked = FALSE;
	int size = 20;
	int maxLen = -1;

	string_tokenizer_tokenize (e->st, str, " >");

	while (string_tokenizer_has_more_tokens (e->st)) {

		const gchar *token = string_tokenizer_next_token (e->st);

		if ( strncasecmp( token, "type=", 5 ) == 0 ) {
			p = token + 5;
			if ( strncasecmp( p, "checkbox", 8 ) == 0 )
				type = CheckBox;
			else if ( strncasecmp( p, "password", 8 ) == 0 )
				type = Password;
			else if ( strncasecmp( p, "hidden", 6 ) == 0 )
				type = Hidden;
			else if ( strncasecmp( p, "radio", 5 ) == 0 )
				type = Radio;
			else if ( strncasecmp( p, "reset", 5 ) == 0 )
				type = Reset;
			else if ( strncasecmp( p, "submit", 5 ) == 0 )
				type = Submit;
			else if ( strncasecmp( p, "button", 6 ) == 0 )
				type = Button;
			else if ( strncasecmp( p, "text", 5 ) == 0 )
				type = Text;
			else if ( strncasecmp( p, "Image", 5 ) == 0 )
				type = Image;
		}
		else if ( strncasecmp( token, "name=", 5 ) == 0 ) {
			name = g_strdup(token + 5);
		}
		else if ( strncasecmp( token, "value=", 6 ) == 0 ) {
			value = g_strdup(token + 6);
		}
		else if ( strncasecmp( token, "size=", 5 ) == 0 ) {
			size = atoi( token + 5 );
		}
		else if ( strncasecmp( token, "maxlength=", 10 ) == 0 ) {
			maxLen = atoi( token + 10 );
		}
		else if ( strncasecmp( token, "checked", 7 ) == 0 ) {
			checked = TRUE;
		}
		else if ( strncasecmp( token, "src=", 4 ) == 0 ) {
			imgSrc = g_strdup (token + 4);
		}
		else if ( strncasecmp( token, "onClick=", 8 ) == 0 ) {
			/* TODO: Implement Javascript */
		}
	}
	switch ( type ) {
	case CheckBox:
		element = html_checkbox_new(GTK_WIDGET(e->widget), name, value, checked);
		break;
	case Hidden:
		{
		HTMLObject *hidden = html_hidden_new(name, value);

		html_form_add_hidden (e->form, HTML_HIDDEN (hidden));

		break;
		}
	case Radio:
		element = html_radio_new(GTK_WIDGET(e->widget), name, value, checked);
		break;
	case Reset:
		element = html_button_new(GTK_WIDGET(e->widget), name, value, BUTTON_RESET);
		break;
	case Submit:
		element = html_button_new(GTK_WIDGET(e->widget), name, value, BUTTON_SUBMIT);
		break;
	case Button:
		element = html_button_new(GTK_WIDGET(e->widget), name, value, BUTTON_NORMAL);
	case Text:
	case Password:
		element = html_text_input_new(GTK_WIDGET(e->widget), name, value, size, maxLen, (type == Password));
		break;
	case Image:
		element = html_imageinput_new (e->image_factory, name, imgSrc);
		break;
	case Undefined:
		g_warning ("Unknown <input type>\n");
		break;
	}
	if (element) {

		append_element (e, _clue, element);
		html_form_add_element (e->form, HTML_EMBEDDED (element));
	}

	if (name)
		g_free (name);
	if (value)
		g_free (value);
	if (imgSrc)
		g_free (imgSrc);
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
		if ( strncmp( str, "address", 7) == 0 ) {
			push_clueflow_style (e, HTML_CLUEFLOW_STYLE_ADDRESS);
			close_flow (e, _clue);
			push_block (e, ID_ADDRESS, 2, block_end_clueflow_style, TRUE, 0);
		} else if ( strncmp( str, "/address", 8) == 0 ) {
			pop_block (e, ID_ADDRESS, _clue);
		} else if ( strncmp( str, "a ", 2 ) == 0 ) {
			gchar *tmpurl = NULL;
			gchar *target = NULL;
			const gchar *p;

			close_anchor (e);

			string_tokenizer_tokenize( e->st, str + 2, " >" );

			while ( ( p = string_tokenizer_next_token (e->st) ) != 0 ) {
				if ( strncasecmp( p, "href=", 5 ) == 0 ) {

					tmpurl = g_strdup (p + 5);

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
				if (e->url != NULL)
					g_free (e->url);
				e->url = tmpurl;
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
		push_font_style (e, HTML_FONT_STYLE_SIZE_3);
		push_block (e, ID_BIG, 1, block_end_font, 0, 0);
	} else if ( strncmp(str, "/big", 4 ) == 0 ) {
		pop_block (e, ID_BIG, clue);
	} else if ( strncmp(str, "blockquote", 10 ) == 0 ) {
		push_block (e, ID_BLOCKQUOTE, 2, block_end_quote, e->quote_level, e->list_level);
		e->avoid_para = TRUE;
		e->pending_para = FALSE;
		e->quote_level = e->quote_level + e->list_level + 1;
		e->list_level = 0;
		close_flow (e, clue);
	} else if ( strncmp(str, "/blockquote", 11 ) == 0 ) {
		e->avoid_para = TRUE;
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
				if (parse_color (token + 8, &bgColor)) {
					g_print ("bgcolor is set\n");
					bgColorSet = TRUE;
				} else {
					g_print ("Color `%s' could not be parsed\n", token);
				}
			}
			else if (strncasecmp (token, "background=", 11) == 0
				 && !e->defaultSettings->forceDefault) {

				gchar *bgurl;

				bgurl = g_strdup (token + 11);
				
				e->bgPixmapPtr = html_image_factory_register(e->image_factory, NULL, bgurl);

				g_free (bgurl);

			} else if ( strncasecmp( token, "text=", 5 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				if (parse_color (token + 5, &e->settings->fontBaseColor)) {
					if (! html_stack_is_empty (e->color_stack))
						pop_color (e);
					push_color (e, gdk_color_copy (&e->settings->fontBaseColor));
				}
			} else if ( strncasecmp( token, "link=", 5 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				parse_color (token + 5, &e->settings->linkColor);
			} else if ( strncasecmp( token, "vlink=", 6 ) == 0
				    && !e->defaultSettings->forceDefault ) {
				parse_color (token + 6, &e->settings->linkColor);
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

		if (bgColorSet) {
			if (! gdk_color_equal (&bgColor, &e->bgColor)) {
				/* FIXME dealloc existing color? */
				e->bgColor_allocated = FALSE;
				e->bgColor = bgColor;
			}
		}

		g_print ("parsed <body>\n");
	}
	else if (strncmp (str, "br", 2) == 0) {
		HTMLClearType clear;

		clear = HTML_CLEAR_NONE;

		string_tokenizer_tokenize (e->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			
			if (strncasecmp (token, "clear=", 6) == 0) {
				g_print ("%s\n", token);
				if (strncasecmp (token + 6, "left", 4) == 0)
					clear = HTML_CLEAR_LEFT;
				else if (strncasecmp (token + 6, "right", 5) == 0)
					clear = HTML_CLEAR_RIGHT;
				else if (strncasecmp (token + 6, "all", 3) == 0)
					clear = HTML_CLEAR_ALL;
			}
		}

		add_line_break (e, clue, clear);
	}
	else if (strncmp (str, "b", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			push_font_style (e, HTML_FONT_STYLE_BOLD);
			push_block (e, ID_B, 1, block_end_font, FALSE, FALSE);
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
		close_flow (e, clue);
		e->divAlign = HTML_HALIGN_CENTER;
	}
	else if (strncmp (str, "/center", 7) == 0) {
		close_flow (e, clue);
		e->divAlign = HTML_HALIGN_LEFT;
	}
	else if (strncmp( str, "cite", 4 ) == 0) {
		push_font_style (e, HTML_FONT_STYLE_ITALIC | HTML_FONT_STYLE_BOLD);
		push_block(e, ID_CITE, 1, block_end_font, 0, 0);
	}
	else if (strncmp( str, "/cite", 5) == 0) {
		pop_block (e, ID_CITE, clue);
	} else if (strncmp(str, "code", 4 ) == 0 ) {
		push_font_style (e, HTML_FONT_STYLE_FIXED);
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
/* EP CHECK: dl/dt might be wrong.  */
/* EP CHECK: dir might be wrong.  */
static void
parse_d ( HTMLEngine *e, HTMLObject *_clue, const char *str )
{
	if ( strncmp( str, "dir", 3 ) == 0 ) {
		close_anchor(e);
		push_block (e, ID_DIR, 2, block_end_list, e->list_level, FALSE);
		html_stack_push (e->listStack, html_list_new (HTML_LIST_TYPE_DIR,
							      HTML_LIST_NUM_TYPE_NUMERIC));
		e->list_level++;
		/* FIXME shouldn't it create a new flow? */
	} else if ( strncmp( str, "/dir", 4 ) == 0 ) {
		pop_block (e, ID_DIR, _clue);
	} else if ( strncmp( str, "div", 3 ) == 0 ) {
		push_block (e, ID_DIV, 1,
			    block_end_div,
			    e->divAlign, FALSE);

		string_tokenizer_tokenize( e->st, str + 4, " >" );
		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token = string_tokenizer_next_token (e->st);
			if ( strncasecmp( token, "align=", 6 ) == 0 ) {
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
		close_anchor (e);
		if ( html_stack_top(e->glossaryStack) != NULL )
			e->quote_level++;
		html_stack_push (e->glossaryStack, GINT_TO_POINTER (HTML_GLOSSARY_DL));
		/* FIXME shouldn't it create a new flow? */
		add_line_break (e, _clue, HTML_CLEAR_ALL);
	} else if ( strncmp( str, "/dl", 3 ) == 0 ) {
		if ( html_stack_top (e->glossaryStack) == NULL)
			return;

		if ( GPOINTER_TO_INT (html_stack_top (e->glossaryStack))
		     == HTML_GLOSSARY_DD ) {
			html_stack_pop (e->glossaryStack);
			if (e->quote_level > 0)
				e->quote_level--;
		}

		html_stack_pop (e->glossaryStack);
		if ( html_stack_top (e->glossaryStack) != NULL ) {
			if (e->quote_level > 0)
				e->quote_level--;
		}

		add_line_break (e, _clue, HTML_CLEAR_ALL);
	} else if (strncmp( str, "dt", 2 ) == 0) {
		if (html_stack_top (e->glossaryStack) == NULL)
			return;

		if (GPOINTER_TO_INT (html_stack_top (e->glossaryStack)) == HTML_GLOSSARY_DD) {
			html_stack_pop (e->glossaryStack);
			if (e->quote_level > 0)
				e->quote_level--;
		}

		close_flow (e, _clue);
	} else if (strncmp( str, "dd", 2 ) == 0) {
		if (html_stack_top (e->glossaryStack) == NULL)
			return;

		if (GPOINTER_TO_INT (html_stack_top (e->glossaryStack)) != HTML_GLOSSARY_DD ) {
			html_stack_push (e->glossaryStack,
					 GINT_TO_POINTER (HTML_GLOSSARY_DD) );
			e->quote_level++;
		}

		close_flow (e, _clue);
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
		push_font_style (e, HTML_FONT_STYLE_ITALIC);
		push_block (e, ID_EM, 1, block_end_font, FALSE, FALSE);
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
		gint newSize;

		newSize = current_font_style (p) & HTML_FONT_STYLE_SIZE_MASK;

		/* The GdkColor API is not const safe!  */
		color = gdk_color_copy ((GdkColor *) current_color (p));

		string_tokenizer_tokenize (p->st, str + 5, " >");

		while (string_tokenizer_has_more_tokens (p->st)) {
			const gchar *token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "size=", 5) == 0) {
				gint num = atoi (token + 5);

				/* FIXME implement basefont */
				if (*(token + 5) == '+' || *(token + 5) == '-')
					newSize = HTML_FONT_STYLE_SIZE_3 + num;
				else
					newSize = num;
				if (newSize > HTML_FONT_STYLE_SIZE_MAX)
					newSize = HTML_FONT_STYLE_SIZE_MAX;
				else if (newSize < HTML_FONT_STYLE_SIZE_1)
					newSize = HTML_FONT_STYLE_SIZE_1;
			} else if (strncasecmp (token, "face=", 5) == 0) {
			} else if (strncasecmp (token, "color=", 6) == 0) {
				parse_color (token + 6, color);
			}
		}

		push_color (p, color);
		push_font_style (p, newSize);

		push_block  (p, ID_FONT, 1, block_end_color_font, FALSE, FALSE);
	}
	else if (strncmp (str, "/font", 5) == 0) {
		pop_block (p, ID_FONT, clue);
	}
	else if (strncmp (str, "form", 4) == 0) {
                gchar *action = NULL;
                gchar *method = "GET";
                gchar *target = NULL;
                    
		string_tokenizer_tokenize (p->st, str + 5, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			const gchar *token = string_tokenizer_next_token (p->st);

                        if ( strncasecmp( token, "action=", 7 ) == 0 )
                        {
                                action = g_strdup (token + 7);
                        }
                        else if ( strncasecmp( token, "method=", 7 ) == 0 )
                        {
                                if ( strncasecmp( token + 7, "post", 4 ) == 0 )
                                        method = "POST";
                        }
                        else if ( strncasecmp( token, "target=", 7 ) == 0 )
                        {
				target = g_strdup(token + 7);
                        }
                }

                p->form = html_form_new (p, action, method);
                p->formList = g_list_append(p->formList, p->form);
		
		if(action)
			g_free(action);
		if(target)
			g_free(target);
	}
	else if (strncmp (str, "/form", 5) == 0) {
		p->form = NULL;
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

		push_clueflow_style (p, HTML_CLUEFLOW_STYLE_H1 + (str[1] - '1'));
		close_flow (p, clue);

		/* HTML_CLUE (p->flow)->halign = align; FIXME this has to be
                    redone differently */

		push_block (p, ID_HEADER, 2, block_end_clueflow_style, TRUE, 0);

		p->pending_para = FALSE;
		p->avoid_para = TRUE;
	} else if (*(str) == '/' && *(str + 1) == 'h' &&
		   ( *(str+2)=='1' || *(str+2)=='2' || *(str+2)=='3' ||
		     *(str+2)=='4' || *(str+2)=='5' || *(str+2)=='6' )) {
		/* Close tag.  */
		pop_block (p, ID_HEADER, clue);
		p->avoid_para = TRUE;
		p->pending_para = FALSE;
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

		append_element (p, clue, html_rule_new (length, percent, size, shade, align));
		close_flow (p, clue);
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
		gint width = -1;
		gchar *tmpurl = NULL;
		gint height = -1;
		gint percent = 0;
		HTMLHAlignType align = HTML_HALIGN_NONE;
		gint border = 0;
		HTMLVAlignType valign = HTML_VALIGN_NONE;

		string_tokenizer_tokenize (p->st, str + 4, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			token = string_tokenizer_next_token (p->st);
			if (strncasecmp (token, "src=", 4) == 0) {

				tmpurl = g_strdup (token + 4);
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
			image = html_image_new (p->image_factory, tmpurl,
						p->url,
						p->target,
						_clue->max_width, 
						width, height, percent, border);

			g_free(tmpurl);
				
			if (align == HTML_HALIGN_NONE) {
				if (valign == HTML_VALIGN_NONE) {
					append_element (p, _clue, image);
				} else {
					HTMLObject *valigned = html_clueh_new (0, 0, _clue->max_width);
					HTML_CLUE (valigned)->valign = valign;
					html_clue_append (HTML_CLUE (valigned), HTML_OBJECT (image));
					append_element (p, _clue, valigned);
				}
			}
			/* We need to put the image in a HTMLClueAligned */
			else {
				HTMLClueAligned *aligned = HTML_CLUEALIGNED (html_cluealigned_new (NULL, 0, 0, _clue->max_width, 100));
				HTML_CLUE (aligned)->halign = align;
				html_clue_append (HTML_CLUE (aligned), HTML_OBJECT (image));
				append_element (p, _clue, HTML_OBJECT (aligned));
			}
		}		       
	}
	else if (strncmp( str, "input", 5 ) == 0) {
		if (p->form == NULL)
			return;
		
		parse_input( p, str + 6, _clue );
	}
	else if ( strncmp (str, "i", 1 ) == 0 ) {
		if ( str[1] == '>' || str[1] == ' ' ) {
			push_font_style (p, HTML_FONT_STYLE_ITALIC);
			push_block (p, ID_I, 1, block_end_font, FALSE, FALSE);
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
	if ( strncmp(str, "kbd", 3 ) == 0 ) {
		push_font_style (e, HTML_FONT_STYLE_FIXED);
		push_block (e, ID_KBD, 1, block_end_font, 0, 0);
	} else if ( strncmp(str, "/kbd", 4 ) == 0 ) {
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
		HTMLListType listType;
		HTMLListNumType listNumType;
		gint listLevel;
		gint itemNumber;

		listType = HTML_LIST_TYPE_UNORDERED;
		listNumType = HTML_LIST_NUM_TYPE_NUMERIC;

		listLevel = 1;
		itemNumber = 1;

		close_anchor (p);

		if (! html_stack_is_empty (p->listStack)) {
			HTMLList *top;

			top = html_stack_top (p->listStack);

			listType = top->type;
			listNumType = top->numType;
			itemNumber = top->itemNumber;

			listLevel = html_stack_count (p->listStack);
		}

		if (p->pending_para) {
			insert_paragraph_break (p, clue);
			p->pending_para = FALSE;
		}

		close_flow (p, clue);

		p->flow = html_clueflow_new (HTML_FONT_STYLE_DEFAULT,
					     HTML_CLUEFLOW_STYLE_ITEMDOTTED,
					     p->list_level,
					     p->quote_level);
		html_clue_append (HTML_CLUE (clue), p->flow);

		p->avoid_para = TRUE;

		if (! html_stack_is_empty (p->listStack)) {
			HTMLList *list;

			list = html_stack_top (p->listStack);
			list->itemNumber++;
		}
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
	gchar *refresh_url = NULL;

	if ( strncmp( str, "meta", 4 ) == 0 ) {
		string_tokenizer_tokenize( e->st, str + 5, " >" );
		while ( string_tokenizer_has_more_tokens (e->st) ) {

			const gchar* token = string_tokenizer_next_token(e->st);
			if ( strncasecmp( token, "http-equiv=", 11 ) == 0 ) {
				if ( strncasecmp( token + 11, "refresh", 7 ) == 0 )
					refresh = 1;
			} else if ( strncasecmp( token, "content=", 8 ) == 0 ) {
				if(refresh) {
					const gchar *content;
					content = token + 8;

					/* The time in seconds until the refresh */
					refresh_delay = atoi(content);

					string_tokenizer_tokenize(e->st, content, ",;>");
					while ( string_tokenizer_has_more_tokens (e->st) ) {
						const gchar* token = string_tokenizer_next_token(e->st);
						if ( strncasecmp( token, "url=", 4 ) == 0 )
							refresh_url = g_strdup (token + 4);
					}
					
					gtk_signal_emit (GTK_OBJECT (e), signals[REDIRECT], refresh_url, refresh_delay);
					
					if(refresh_url)
						g_free(refresh_url);
				}
			}
		}
	}
}

/* FIXME TODO parse_n missing. */

/* called when some state in an embedded html object has changed ... do a redraw */
static void
html_object_changed(GtkHTMLEmbedded *eb, HTMLEngine *e)
{
	void html_engine_schedule_update (HTMLEngine *p);
	static gboolean html_engine_update_event (HTMLEngine *e);
	HTMLEmbedded *el;

	el = gtk_object_get_data(GTK_OBJECT(eb), "embeddedelement");
	if (el) {
		html_embedded_size_recalc(el);
	}
	html_engine_schedule_update(e);
}


/*
<ol>             </ol>           partial
<option
*/
/* EP CHECK: `<ol>' does not handle vspace correctly.  */
static void
parse_o (HTMLEngine *e, HTMLObject *_clue, const gchar *str )
{
	if ( strncmp( str, "ol", 2 ) == 0 ) {
		HTMLListNumType listNumType;
		HTMLList *list;

		close_anchor (e);

		if ( html_stack_is_empty (e->listStack) ) {
			/* FIXME */
			push_block (e, ID_OL, 2, block_end_list, e->list_level, TRUE);
		} else {
			push_block (e, ID_OL, 2, block_end_list, e->list_level, FALSE);
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

		e->list_level++;
	}
	else if ( strncmp( str, "/ol", 3 ) == 0 ) {
		pop_block (e, ID_OL, _clue);
	}
	else if ( strncmp( str, "option", 6 ) == 0 ) {
		gchar *value = NULL;
		gboolean selected = FALSE;

		if ( !e->formSelect )
			return;

		string_tokenizer_tokenize( e->st, str + 3, " >" );

		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token;
			
			token = string_tokenizer_next_token (e->st);
			
			if ( strncasecmp( token, "value=", 6 ) == 0 ) {

				value = g_strdup (token + 6);
			}
			else if ( strncasecmp( token, "selected", 8 ) == 0 ) {

				selected = TRUE;
			}
		}

		if ( e->inOption )
			html_select_set_text (e->formSelect, e->formText->str);

		html_select_add_option (e->formSelect, value, selected );

		e->inOption = TRUE;
		g_string_assign (e->formText, "");
	} else if ( strncmp( str, "/option", 7 ) == 0 ) {
		if ( e->inOption )
			html_select_set_text (e->formSelect, e->formText->str);

		e->inOption = FALSE;
	} else if ( strncmp( str, "object", 6 ) == 0 ) {
		char *classid=NULL, *name=NULL;
		int width=-1,height=-1;

		string_tokenizer_tokenize( e->st, str + 7, " >" );

		/* this might have to do something different for form object
		   elements - check the spec MPZ */
		while ( string_tokenizer_has_more_tokens (e->st) ) {
			const char* token;

			token = string_tokenizer_next_token (e->st);
			if ( strncasecmp( token, "classid=", 8 ) == 0 ) {
				classid = g_strdup(token+8);
			} else if ( strncasecmp( token, "name=", 6 ) == 0 ) {
				name = g_strdup(token+6);
			} else if ( strncasecmp( token, "width=", 6 ) == 0 ) {
				width = atoi(token+6);
			} else if ( strncasecmp( token, "height=", 7 ) == 0 ) {
				height = atoi(token+7);
			}
		}

		if (classid) {
			GtkHTMLEmbedded *eb;

			eb = (GtkHTMLEmbedded *)gtk_html_embedded_new(classid, name, width, height);
			html_stack_push (e->embeddedStack, eb);
			g_free(classid);
			g_free(name);
		} else {
			g_warning("Object with no classid, ignored\n");
		}

	} else if ( strncmp( str, "/object", 7 ) == 0 ) {

		if (! html_stack_is_empty (e->embeddedStack)) {
			HTMLEmbedded *el;
			GtkHTMLEmbedded *eb;

			eb = html_stack_pop (e->embeddedStack);

			gtk_signal_emit (GTK_OBJECT (e), signals[OBJECT_REQUESTED], eb);

			el = html_embedded_new_widget(GTK_WIDGET (e->widget), eb);
			gtk_object_set_data(GTK_OBJECT(eb), "embeddedelement", el);
			gtk_signal_connect(GTK_OBJECT(eb), "changed", html_object_changed, e);

			append_element(e, _clue, HTML_OBJECT(el));
			/* automatically add this to a form if it is part of one */
			if (e->form) {
				html_form_add_element (e->form, HTML_EMBEDDED (el));
			}
		}
	}
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
		push_clueflow_style (e, HTML_CLUEFLOW_STYLE_PRE);
		e->inPre = TRUE;
		push_block (e, ID_PRE, 2, block_end_pre, 0, 0);
	} else if ( strncmp( str, "/pre", 4 ) == 0 ) {
		pop_block (e, ID_PRE, clue);
		close_flow (e, clue);
	} else if ( strncmp( str, "param", 5) == 0 ) {
		if (! html_stack_is_empty (e->embeddedStack)) {
			GtkHTMLEmbedded *eb;
			char *name = NULL, *value = NULL;

			eb = html_stack_top (e->embeddedStack);
			string_tokenizer_tokenize (e->st, str + 6, " >");
			while ( string_tokenizer_has_more_tokens (e->st) ) {
				const char *token = string_tokenizer_next_token (e->st);
				if ( strncasecmp( token, "name=", 5 ) == 0 ) {
					name = g_strdup(token+5);
				} else if ( strncasecmp( token, "value=", 6 ) == 0 ) {
					value = g_strdup(token+6);
				}
			}

			if (name!=NULL) {
				gtk_html_embedded_set_parameter(eb, name, value);
			}
			g_free(name);
			g_free(value);
		}					
	} else if (*(str) == 'p' && ( *(str + 1) == ' ' || *(str + 1) == '>')) {
		if (! e->avoid_para) {
			HTMLHAlignType align;
			gchar *token;

			close_anchor (e);

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

			/* FIXME align para */

			e->avoid_para = TRUE;
			e->pending_para = TRUE;
		}
	} else if (*(str) == '/' && *(str + 1) == 'p'
		   && (*(str + 2) == ' ' || *(str + 2) == '>')) {
		if (! e->avoid_para) {
			e->avoid_para = TRUE;
			e->pending_para = TRUE;
		}
	}
}


/*
  <small>             </small>
*/
static void
parse_s (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if ( strncmp(str, "small", 3 ) == 0 ) {
		push_font_style (e, 2);
		push_block (e, ID_SMALL, 1, block_end_font, 0, 0);
	} else if ( strncmp(str, "/small", 4 ) == 0 ) {
		pop_block (e, ID_SMALL, clue);
	}
	else if (strncmp (str, "select", 6) == 0) {
                gchar *name = NULL;
		gint size = 0;
		gboolean multi = FALSE;

		if (!e->form)
			return;
                    
		string_tokenizer_tokenize (e->st, str + 7, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			const gchar *token = string_tokenizer_next_token (e->st);

                        if ( strncasecmp( token, "name=", 5 ) == 0 )
                        {
				name = g_strdup(token + 5);
                        }
                        else if ( strncasecmp( token, "size=", 5 ) == 0 )
                        {
				size = atoi (token + 5);
                        }
                        else if ( strncasecmp( token, "multiple", 8 ) == 0 )
                        {
				multi = TRUE;
                        }
                }
                
                e->formSelect = HTML_SELECT (html_select_new (GTK_WIDGET(e->widget), name, size, multi));
                html_form_add_element (e->form, HTML_EMBEDDED ( e->formSelect ));

		append_element (e, clue, HTML_OBJECT (e->formSelect));
		
		if (name)
			g_free(name);
	}
	else if (strncmp (str, "/select", 7) == 0) {
		if ( e->inOption )
			html_select_set_text (e->formSelect, e->formText->str);

		e->inOption = FALSE;
		e->formSelect = NULL;
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

		close_flow (e, clue);
		parse_table (e, clue, clue->max_width, str + 6);
		close_flow (e, clue);
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
		push_font_style (e, HTML_FONT_STYLE_FIXED);
		push_block (e, ID_TT, 1, block_end_font, 0, 0);
	} else if ( strncmp( str, "/tt", 3 ) == 0 ) {
		pop_block (e, ID_TT, clue);
	}
	else if (strncmp (str, "textarea", 8) == 0) {
                gchar *name = NULL;
		gint rows = 5, cols = 40;

		if (!e->form)
			return;
                    
		string_tokenizer_tokenize (e->st, str + 9, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			const gchar *token = string_tokenizer_next_token (e->st);

                        if ( strncasecmp( token, "name=", 5 ) == 0 )
                        {
				name = g_strdup(token + 5);
                        }
                        else if ( strncasecmp( token, "rows=", 5 ) == 0 )
                        {
				rows = atoi (token + 5);
                        }
                        else if ( strncasecmp( token, "cols=", 5 ) == 0 )
                        {
				cols = atoi (token + 5);
                        }
                }
                
                e->formTextArea = HTML_TEXTAREA (html_textarea_new (GTK_WIDGET(e->widget), name, rows, cols));
                html_form_add_element (e->form, HTML_EMBEDDED ( e->formTextArea ));

		append_element (e, clue, HTML_OBJECT (e->formTextArea));

		g_string_assign (e->formText, "");
		e->inTextArea = TRUE;

		push_block(e, ID_TEXTAREA, 3, NULL, 0, 0);
		
		if(name)
			g_free(name);
	}
	else if (strncmp (str, "/textarea", 9) == 0) {
		pop_block(e, ID_TEXTAREA, clue);

		if ( e->inTextArea )
			html_textarea_set_text (e->formTextArea, e->formText->str);

		e->inTextArea = FALSE;
		e->formTextArea = NULL;
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
		close_flow (e, clue);

		if (html_stack_is_empty (e->listStack))
			push_block (e, ID_UL, 2, block_end_list, e->list_level, TRUE);
		else
			push_block (e, ID_UL, 2, block_end_list, e->list_level, FALSE);

		type = HTML_LIST_TYPE_UNORDERED;

		string_tokenizer_tokenize (e->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			if (strncasecmp (token, "plain", 5) == 0)
				type = HTML_LIST_TYPE_UNORDEREDPLAIN;
		}
		
		html_stack_push (e->listStack, html_list_new (type, HTML_LIST_NUM_TYPE_NUMERIC));
		e->flow = NULL;

		if (e->pending_para && e->list_level > 0)
			insert_paragraph_break (e, clue);

		e->list_level++;

		e->avoid_para = TRUE;
		e->pending_para = FALSE;
	} else if (strncmp (str, "/ul", 3) == 0) {
		pop_block (e, ID_UL, clue);
	}
	else if (strncmp (str, "u", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			push_font_style (e, HTML_FONT_STYLE_UNDERLINE);
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
		push_font_style (e, HTML_FONT_STYLE_FIXED);
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
	HTMLEngine *engine;
	GList *p;

	engine = HTML_ENGINE (object);

	html_color_set_destroy (engine->color_set);

	if (engine->invert_gc != NULL)
		gdk_gc_destroy (engine->invert_gc);

	html_cursor_destroy (engine->cursor);

	html_tokenizer_destroy   (engine->ht);
	string_tokenizer_destroy (engine->st);
	html_settings_destroy    (engine->settings);
	html_settings_destroy    (engine->defaultSettings);
	html_painter_destroy     (engine->painter);
	html_image_factory_free  (engine->image_factory);

	html_stack_destroy (engine->color_stack);
	html_stack_destroy (engine->font_style_stack);
	html_stack_destroy (engine->clueflow_style_stack);

	html_stack_destroy (engine->listStack);
	html_stack_destroy (engine->glossaryStack);
	html_stack_destroy (engine->embeddedStack);

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

	signals [SUBMIT] =
		gtk_signal_new ("submit",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, submit),
				gtk_marshal_NONE__POINTER_POINTER_POINTER,
				GTK_TYPE_NONE, 3,
				GTK_TYPE_STRING,
				GTK_TYPE_STRING,
				GTK_TYPE_STRING);

	signals [OBJECT_REQUESTED] =
		gtk_signal_new ("object_requested",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (HTMLEngineClass, object_requested),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = html_engine_destroy;

	/* Initialize the HTML objects.  */
	html_types_init ();
}

static void
html_engine_init (HTMLEngine *engine)
{
	/* STUFF might be missing here!   */

	engine->window = NULL;
	engine->invert_gc = NULL;

	engine->color_set = html_color_set_new ();
	engine->painter = html_painter_new ();
	html_painter_set_color_set (engine->painter, engine->color_set);
	
	engine->newPage = FALSE;

	engine->editable = FALSE;
	engine->cursor = html_cursor_new ();

	engine->ht = html_tokenizer_new ();
	engine->st = string_tokenizer_new ();
	engine->settings = html_settings_new ();
	engine->defaultSettings = html_settings_new ();
	engine->image_factory = html_image_factory_new(engine);

	engine->font_style_stack = html_stack_new (NULL);
	engine->color_stack = html_stack_new ((HTMLStackFreeFunc) gdk_color_free);
	engine->clueflow_style_stack = html_stack_new (NULL);

	engine->listStack = html_stack_new ((HTMLStackFreeFunc) html_list_destroy);
	engine->glossaryStack = html_stack_new (NULL);
	engine->embeddedStack = html_stack_new ((HTMLStackFreeFunc) gtk_object_unref);

	engine->url = NULL;
	engine->target = NULL;

	engine->leftBorder = LEFT_BORDER;
	engine->rightBorder = RIGHT_BORDER;
	engine->topBorder = TOP_BORDER;
	engine->bottomBorder = BOTTOM_BORDER;

	engine->inPre = FALSE;
	engine->noWrap = FALSE;
	engine->inTitle = FALSE;

	engine->tempStrings = NULL;

	engine->draw_queue = html_draw_queue_new (engine);

	engine->formList = NULL;
	engine->reference = NULL;

	engine->bgColor_allocated = FALSE;

	engine->avoid_para = TRUE;
	engine->pending_para = FALSE;

	engine->quote_level = 0;
	engine->list_level = 0;
}

HTMLEngine *
html_engine_new (void)
{
	HTMLEngine *engine;

	engine = gtk_type_new (html_engine_get_type ());

	return engine;
}

void
html_engine_realize (HTMLEngine *e,
		     GdkWindow *window)
{
	GdkGCValues gc_values;

	g_return_if_fail (e != NULL);
	g_return_if_fail (window != NULL);
	g_return_if_fail (e->window == NULL);

	e->window = window;

	html_painter_realize (e->painter, window);

	gc_values.function = GDK_INVERT;
	e->invert_gc = gdk_gc_new_with_values (e->window, &gc_values, GDK_GC_FUNCTION);
}


static void
draw_background (HTMLEngine *e,
		 gint xval, gint yval,
		 gint x, gint y, gint w, gint h)
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
		if (! e->bgColor_allocated) {
			html_painter_alloc_color (e->painter, &e->bgColor);
			e->bgColor_allocated = TRUE;
		}
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

	if (e->timerId != 0) {
		gtk_timeout_remove (e->timerId);
		/*		e->timerId = 0;*/
	}
	
	e->parsing = FALSE;

	html_stack_clear (e->color_stack);
	html_stack_clear (e->font_style_stack);
	html_stack_clear (e->clueflow_style_stack);
}

GtkHTMLStreamHandle
html_engine_begin (HTMLEngine *p, const char *url)
{
	GtkHTMLStream *new_stream;

	/* Is it a reference? */
	if (*url == '#') {
		p->reference = g_strdup (url + 1);
		
		if (!html_engine_goto_anchor (p)) /* If anchor not found, scroll to the top of the page */
			gtk_adjustment_set_value (GTK_LAYOUT (p->widget)->vadjustment, 0);

		gtk_signal_emit (GTK_OBJECT (p), signals[LOAD_DONE]);

		return NULL;
	}

	html_tokenizer_begin (p->ht);
	
	free_block (p); /* Clear the block stack */

	html_engine_stop_parser (p);
	p->writing = TRUE;

	new_stream = gtk_html_stream_new(GTK_HTML(p->widget),
					 url,
					 (GtkHTMLStreamWriteFunc)html_engine_write,
					 (GtkHTMLStreamEndFunc)html_engine_end,
					 (gpointer)p);

	if (p->reference) {

		g_free (p->reference);
		p->reference = NULL;
	}

	if (strchr (url, '#'))
		p->reference = g_strdup (strchr (url, '#') + 1);

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

	if (e->reference ) {
		if (html_engine_goto_anchor (e)) {
			g_free (e->reference);
			e->reference = NULL;
		}
	}

	if (! e->parsing && e->editable)
		html_cursor_home (e->cursor, e);

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
		p->updateTimer = gtk_timeout_add (TIMER_INTERVAL,
						  (GtkFunction) html_engine_update_event,
						  p);
}


static gboolean
html_engine_goto_anchor (HTMLEngine *e)
{
	HTMLAnchor *a;
	gint x = 0, y = 0;

	if (!e->clue || !e->reference)
		return FALSE;

	if ((a = html_object_find_anchor (e->clue, e->reference, &x, &y)) == NULL)
		return FALSE;

	if (y < ( GTK_LAYOUT (e->widget)->vadjustment->upper - 
		  GTK_LAYOUT (e->widget)->vadjustment->page_size) )
		gtk_adjustment_set_value (GTK_LAYOUT (e->widget)->vadjustment, y);
	else
		gtk_adjustment_set_value (GTK_LAYOUT (e->widget)->vadjustment, 
					  GTK_LAYOUT (e->widget)->vadjustment->upper -
					  GTK_LAYOUT (e->widget)->vadjustment->page_size);

	return TRUE;
}

static gboolean
html_engine_timer_event (HTMLEngine *e)
{
	static const gchar *end[] = { "</body>", 0};
	gint lastHeight;
	gboolean retval = TRUE;

	/* Has more tokens? */
	if (!html_tokenizer_has_more_tokens (e->ht) && e->writing) {
		retval = FALSE;
		goto out;
	}

	/* Getting height */
	lastHeight = html_engine_get_doc_height (e);

	e->parseCount = e->granularity;

	/* Parsing body height */
	if (parse_body (e, e->clue, end, TRUE))
	  	html_engine_stop_parser (e);

	html_engine_schedule_update (e);

	if (!e->parsing)
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
}


void
html_engine_draw_cursor_in_area (HTMLEngine *e,
				 gint x, gint y,
				 gint width, gint height)
{
	HTMLObject *obj;
	guint offset;
	gint x1, y1, x2, y2;

	g_assert (e->editable);

	obj = e->cursor->object;
	if (obj == NULL)
		return;

	offset = e->cursor->offset;

	if (width < 0 || height < 0) {
		width = e->width;
		height = e->height;
		x = 0;
		y = 0;
	}

	html_object_get_cursor (obj, e->painter, offset, &x1, &y1, &x2, &y2);

	x1 = x1 + e->leftBorder - e->x_offset;
	y1 = y1 + e->topBorder - e->y_offset;
	x2 = x2 + e->leftBorder - e->x_offset;
	y2 = y2 + e->topBorder - e->y_offset;

	if (x1 >= x + width)
		return;
	if (y1 >= y + height)
		return;

	if (x2 < x)
		return;
	if (y2 < y)
		return;

	if (x2 >= x + width)
		x2 = x + width - 1;
	if (y2 >= y + height)
		y2 = y + height - 1;

	if (x1 < x)
		x1 = x;
	if (y1 < y)
		y1 = y;

	gdk_draw_line (e->window, e->invert_gc, x1, y1, x2, y2);
}

void
html_engine_draw (HTMLEngine *e,
		  gint x, gint y,
		  gint width, gint height)
{
	gint tx, ty;

	tx = -e->x_offset + e->leftBorder;
	ty = -e->y_offset + e->topBorder;

	html_painter_begin (e->painter, x, y, x + width, y + height);

	draw_background (e, e->x_offset, e->y_offset, x, y, width, height);

	if (e->clue)
		html_object_draw (e->clue,
				  e->painter,
				  x - e->x_offset,
				  y + e->y_offset - e->topBorder,
				  width,
				  height,
				  tx, ty);

	html_painter_end (e->painter);

	if (e->editable)
		html_engine_draw_cursor_in_area (e, x, y, width, height);
}

void
html_engine_draw_cursor (HTMLEngine *e)
{
	html_engine_draw_cursor_in_area (e, 0, 0, -1, -1);
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

	min_width = html_object_calc_min_width (p->clue, p->painter);
	if (min_width > max_width)
		max_width = min_width;

	html_object_set_max_width (p->clue, max_width);
	html_object_calc_size (p->clue, p->painter);

	p->clue->x = 0;
	p->clue->y = p->clue->ascent;
}

static void
destroy_form (gpointer data, gpointer user_data)
{
	html_form_destroy (HTML_FORM(data));
}

void
html_engine_parse (HTMLEngine *p)
{
	html_engine_stop_parser (p);

	if (p->clue != NULL)
		html_object_destroy (p->clue);

	g_list_foreach (p->formList, destroy_form, NULL);

	g_list_free (p->formList);

	p->formList = NULL;
	p->form = NULL;
	p->formSelect = NULL;
	p->formTextArea = NULL;
	p->inOption = FALSE;
	p->inTextArea = FALSE;
	p->formText = g_string_new ("");

	p->flow = 0;

	p->clue = html_cluev_new (0, 0, p->width - p->leftBorder - p->rightBorder, 100);
	HTML_CLUE (p->clue)->valign = HTML_VALIGN_TOP;
	HTML_CLUE (p->clue)->halign = HTML_HALIGN_LEFT;

	p->cursor->object = p->clue;

	/* Free the background pixmap */
	if (p->bgPixmapPtr) {
		html_image_factory_unregister(p->image_factory, p->bgPixmapPtr, NULL);
		p->bgPixmapPtr = NULL;
	}

	/* FIXME */
	html_settings_alloc_colors (p->defaultSettings,
				    gdk_window_get_colormap (html_painter_get_window (p->painter)));
	html_settings_copy (p->settings, p->defaultSettings);

	/* Free the background color (if any) and alloc a new one */
	p->bgColor = p->settings->bgColor;

	p->bodyParsed = FALSE;

	p->parsing = TRUE;
	p->avoid_para = TRUE;
	p->pending_para = FALSE;

	p->pending_para_alignment = HTML_HALIGN_LEFT;

	p->timerId = gtk_timeout_add (TIMER_INTERVAL,
				      (GtkFunction) html_engine_timer_event,
				      p);
}


HTMLObject *
html_engine_get_object_at (HTMLEngine *e,
			   gint x, gint y,
			   guint *offset_return,
			   gboolean for_cursor)
{
	HTMLObject *obj;

	if (e->clue == NULL)
		return NULL;

	obj = html_object_check_point (HTML_OBJECT (e->clue),
				       e->painter,
				       x - e->leftBorder, y - e->topBorder,
				       offset_return,
				       for_cursor);

	return obj;
}

const gchar *
html_engine_get_link_at (HTMLEngine *e, gint x, gint y)
{
	HTMLObject *obj;

	if (e->clue == NULL)
		return NULL;

	obj = html_engine_get_object_at (e, x, y, NULL, FALSE);

	if (obj != NULL)
		return html_object_get_url (obj);

	return NULL;
}

void
html_engine_set_editable (HTMLEngine *e,
			  gboolean editable)
{
	if (! e->editable && editable)
		html_cursor_home (e->cursor, e);

	if ((! e->editable && editable) || (e->editable && ! editable))
		html_engine_draw (e, 0, 0, e->width, e->height);

	e->editable = editable;
}

static void
html_engine_set_base_url (HTMLEngine *e, const char *url)
{
	gtk_signal_emit (GTK_OBJECT (e), signals[SET_BASE], url);
}


void
html_engine_make_cursor_visible (HTMLEngine *e)
{
	HTMLCursor *cursor;
	HTMLObject *object;
	gint x1, y1, x2, y2;

	g_return_if_fail (e != NULL);

	if (! e->editable)
		return;

	cursor = e->cursor;
	object = cursor->object;
	if (object == NULL)
		return;

	html_object_get_cursor (object, e->painter, cursor->offset, &x1, &y1, &x2, &y2);

	x1 += e->leftBorder;
	y1 += e->topBorder;
	x2 += e->leftBorder;
	y2 += e->topBorder;

	if (x1 + e->leftBorder >= e->x_offset + e->width)
		e->x_offset = x1 + e->leftBorder - e->width + 1;
	else if (x1 < e->x_offset + e->leftBorder)
		e->x_offset = x1 - e->leftBorder;

	if (y2 + e->topBorder >= e->y_offset + e->height)
		e->y_offset = y2 + e->topBorder - e->height + 1;
	else if (y1 < e->y_offset + e->topBorder)
		e->y_offset = y1 - e->topBorder;
}


void
html_engine_flush_draw_queue (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);

	if (e->editable)
		html_engine_draw_cursor (e);

	html_draw_queue_flush (e->draw_queue);

	if (e->editable)
		html_engine_draw_cursor (e);
}

void
html_engine_queue_draw (HTMLEngine *e, HTMLObject *o)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (o != NULL);

	html_draw_queue_add (e->draw_queue, o);
}

void
html_engine_queue_clear (HTMLEngine *e,
			 gint x, gint y,
			 guint width, guint height)
{
	g_return_if_fail (e != NULL);

	html_draw_queue_add_clear (e->draw_queue, x, y, width, height, &e->bgColor);
}


void
html_engine_form_submitted (HTMLEngine *e,
			    const gchar *method,
			    const gchar *action,
			    const gchar *encoding)
{
	gtk_signal_emit (GTK_OBJECT (e), signals[SUBMIT], method, action, encoding);

}


struct _SelectRegionData {
	HTMLEngine *engine;
	HTMLObject *obj1, *obj2;
	guint offset1, offset2;
	gint x1, y1, x2, y2;
	gboolean select : 1;
	gboolean queue_draw : 1;
};
typedef struct _SelectRegionData SelectRegionData;

static void
select_region_forall (HTMLObject *self,
		      gpointer data)
{
	SelectRegionData *select_data;
	gboolean changed;

	select_data = (SelectRegionData *) data;
	changed = FALSE;

	if (self == select_data->obj1 || self == select_data->obj2) {
		if (select_data->obj1 == select_data->obj2) {
			if (select_data->offset2 > select_data->offset1)
				changed = html_object_select_range (select_data->obj1,
								    select_data->engine,
								    select_data->offset1,
								    select_data->offset2 - select_data->offset1,
								    select_data->queue_draw);
			else if (select_data->offset2 < select_data->offset1)
				changed = html_object_select_range (select_data->obj1,
								    select_data->engine,
								    select_data->offset2,
								    select_data->offset1 - select_data->offset2,
								    select_data->queue_draw);
			select_data->select = FALSE;
		} else {
			guint offset;

			if (self == select_data->obj1)
				offset = select_data->offset1;
			else
				offset = select_data->offset2;

			if (select_data->select) {
				changed = html_object_select_range (self,
								    select_data->engine,
								    0, offset,
								    select_data->queue_draw);
				select_data->select = FALSE;
			} else {
				changed = html_object_select_range (self,
								    select_data->engine,
								    offset, -1,
								    select_data->queue_draw);
				select_data->select = TRUE;
			}
		}
	} else {
		if (select_data->select) {
			changed = html_object_select_range (self, select_data->engine, 0, -1, select_data->queue_draw);
		} else {
			changed = html_object_select_range (self, select_data->engine, 0, 0, select_data->queue_draw);
		}
	}

	if (self->parent == select_data->obj1 || self->parent == select_data->obj2) {
		HTMLObject *next;
		gint y;

		if (self->parent == select_data->obj1)
			y = select_data->y1;
		else
			y = select_data->y2;

		next = self->next;
		while (next != NULL
		       && (HTML_OBJECT_TYPE (next) == HTML_TYPE_TEXTMASTER
			   || HTML_OBJECT_TYPE (next) == HTML_TYPE_LINKTEXTMASTER)) {
			next = next->next;
		}

		if (next == NULL
		    || (next->y + next->descent != self->y + self->descent
			&& next->y - next->ascent > y)) {
			select_data->select = ! select_data->select;
		}
	}
}

/* FIXME this implementation could be definitely smarter.  */
/* FIXME maybe this should go into `htmlengine-edit.c'.  */
void
html_engine_select_region (HTMLEngine *e,
			   gint x1, gint y1,
			   gint x2, gint y2,
			   gboolean queue_draw)
{
	SelectRegionData *data;
	gint x, y;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (e->clue == NULL)
		return;

	data = g_new (SelectRegionData, 1);

	data->engine = e;
	data->obj1 = html_engine_get_object_at (e, x1, y1, &data->offset1, TRUE);
	data->obj2 = html_engine_get_object_at (e, x2, y2, &data->offset2, TRUE);
	data->select = FALSE;
	data->queue_draw = queue_draw;

	if (data->obj1 == NULL || data->obj2 == NULL
	    || (data->obj1 == data->obj2 && data->offset1 == data->offset2)) {
		html_engine_unselect_all (e, queue_draw);
		return;
	}

	html_object_calc_abs_position (data->obj1, &x, &y);
	data->x1 = x1 - x;
	data->y1 = y1 - y;

	html_object_calc_abs_position (data->obj2, &x, &y);
	data->x2 = x2 - x;
	data->y2 = y2 - y;

	html_object_forall (e->clue, select_region_forall, data);

	g_free (data);
}

static void
unselect_forall (HTMLObject *self,
		 gpointer data)
{
	SelectRegionData *select_data;

	select_data = (SelectRegionData *) data;
	html_object_select_range (self, select_data->engine, 0, 0, select_data->queue_draw);
}

void
html_engine_unselect_all (HTMLEngine *e,
			  gboolean queue_draw)
{
	SelectRegionData *select_data;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	if (e->clue == NULL)
		return;

	select_data = g_new (SelectRegionData, 1);
	select_data->engine = e;
	select_data->queue_draw = queue_draw;

	html_object_forall (e->clue, unselect_forall, select_data);

	g_free (select_data);
}
