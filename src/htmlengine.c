/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  Copyright (C) 1999 Anders Carlsson (andersca@gnu.org)
              (C) 1997 Martin Jones (mjones@kde.org)
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
#include "htmllist.h"
#include "htmllinktext.h"
#include "htmllinktextmaster.h"
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
static void html_engine_write (GtkHTMLStreamHandle handle, gchar *buffer, size_t size, HTMLEngine *e);
static void html_engine_end (GtkHTMLStreamHandle handle, GtkHTMLStreamStatus status, HTMLEngine *e);

static GtkLayoutClass *parent_class = NULL;
guint html_engine_signals [LAST_SIGNAL] = { 0 };

#define TIMER_INTERVAL 30
#define INDENT_SIZE 30

extern gint defaultFontSizes [7];

enum ID { ID_FONT, ID_B, ID_EM, ID_HEADER, ID_U, ID_UL, ID_TD };


static void
close_anchor (HTMLEngine *e)
{
	if (e->url) {
		html_engine_pop_color (e);
		html_engine_pop_font (e);
		g_free (e->url);
	}

	e->url = NULL;

	g_free (e->target);
	e->target = NULL;
}


/* Parsing functions.  */

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
			if (p->parseFuncArray[indx]) {
				(*(p->parseFuncArray[indx]))(p, clue, str);
			}
			else {
			  /*				g_print ("Unsupported tag %s\n", str);*/
			}
		}

	}
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

			parse_one_token (p, clue, str);
		}
	}

	if (!html_tokenizer_has_more_tokens (p->ht) && toplevel && 
	    !p->writing)
		html_engine_stop_parser (p);

	return 0;
}

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
    }
    else if ( strncmp( str, "address", 7) == 0 )
    {
		/* vspace_inserted = insertVSpace( _clue, vspace_inserted ); */
		e->flow = 0;
		e->italic = TRUE;
		e->bold = FALSE;
		/*  selectFont(); FIXME TODO */
		html_engine_push_block (e, ID_ADDRESS, 2,
								html_engine_block_end_font,
								TRUE);
    }
    else if ( strncmp( str, "/address", 8) == 0 )
    {
		html_engine_pop_block ( e, ID_ADDRESS, _clue);
    }
	else
#endif

    if ( strncmp( str, "a ", 2 ) == 0 ) {
		gchar *tmpurl = NULL;
		gboolean visited = FALSE;
		gchar *target = NULL;
		const gchar *p;

		close_anchor (e);

		string_tokenizer_tokenize( e->st, str + 2, " >" );

		while ( ( p = string_tokenizer_next_token (e->st) ) != 0 ) {
			if ( strncasecmp( p, "href=", 5 ) == 0 ) {
				p += 5;

				if ( *p == '#' ) {
					/* FIXME TODO */
					g_warning ("References are not implemented yet");
					tmpurl = g_strdup ("blahblah");
				} else {
					/* FIXME TODO concatenate correctly */
					tmpurl = g_strdup (p);
				}		

				/* FIXME TODO */
#if 0
				visited = URLVisited( tmpurl );
#endif
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

		if ( !target
			 && e->baseTarget != NULL
			 && e->baseTarget[0] != '\0' ) {
			target = g_strdup (e->baseTarget);
			/*  parsedTargets.append( target ); FIXME TODO */
		}

		if (tmpurl != NULL && tmpurl[0] != '\0') {
			e->vspace_inserted = FALSE;

			if ( visited )
				html_color_stack_push
					(e->cs, gdk_color_copy (html_color_stack_top (e->cs))
					 /*  new QColor(settings->vLinkColor) */);
			else
				html_color_stack_push
					(e->cs, gdk_color_copy (html_color_stack_top (e->cs))
					 /*  new QColor( settings->linkColor) */);

			if (TRUE /*  FIXME TODO settings->underlineLinks */ )
				e->underline = TRUE;

			html_engine_select_font (e);

			e->url = tmpurl;

			printf ("HREF: %s\n", e->url);

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


static void
parse_b (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	/* FIXME TODO missing stuff! */

	if (strncmp (str, "body", 4) == 0) {
		GdkColor bgcolor;
		gboolean bgColorSet = FALSE;

		if (e->bodyParsed) {
			g_print ("body is parsed\n");
			return;
		}

		e->bodyParsed = TRUE;
		
		string_tokenizer_tokenize (e->st, str + 5, " >");
		while (string_tokenizer_has_more_tokens (e->st)) {
			gchar *token = string_tokenizer_next_token (e->st);
			g_print ("token is: %s\n", token);

			if (strncasecmp (token, "bgcolor=", 8) == 0) {
				g_print ("setting color\n");
				html_engine_set_named_color (e, &bgcolor, token + 8);
				g_print ("bgcolor is set\n");
				bgColorSet = TRUE;
			}
			else if (strncasecmp (token, "background=", 11) == 0) {
			  char *realurl;

			  realurl = html_engine_canonicalize_url(e, token + 11);
			  e->bgPixmapPtr = html_image_factory_register(e->image_factory, NULL, realurl);
			  g_free(realurl);
			}
		}
		
		if (bgColorSet)
			e->bgColor = bgcolor;

		g_print ("parsed <body>\n");
	}
	else if (strncmp (str, "br", 2) == 0) {
		ClearType clear = CNone;

		if (!e->flow)
			html_engine_new_flow (e, clue);

		html_clue_append (HTML_CLUE (e->flow), html_vspace_new (14, clear));

		e->vspace_inserted = FALSE;
	}
	else if (strncmp (str, "b", 1) == 0) {
		if (str[1] == '>' || str[1] == ' ') {
			e->bold = TRUE;
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


static void
parse_e (HTMLEngine *e, HTMLObject *_clue, const gchar *str)
{
	if ( strncmp( str, "em", 2 ) == 0 ) {
		e->italic = TRUE;
		html_engine_select_font (e);
		html_engine_push_block (e, ID_EM, 1, html_engine_block_end_font, 0, 0);
	} else if ( strncmp( str, "/em", 3 ) == 0 ) {
		html_engine_pop_block (e, ID_EM, _clue);
	}
}


static void
parse_c (HTMLEngine *e, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "center", 6) == 0) {
		e->divAlign = HCenter;
		e->flow = 0;
	}
	else if (strncmp (str, "/center", 7) == 0) {
		e->divAlign = Left;
		e->flow = 0;
	}
}


static void
parse_f (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "font", 4) == 0) {
		GdkColor *color;
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
			else if (strncasecmp (token, "face=", 5) == 0);
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


static void
parse_h (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (*(str) == 'h' &&
	    ( *(str+1)=='1' || *(str+1)=='2' || *(str+1)=='3' ||
		  *(str+1)=='4' || *(str+1)=='5' || *(str+1)=='6' ) ) {
		HAlignType align;

		p->vspace_inserted = html_engine_insert_vspace (p, clue,
														p->vspace_inserted);
		align = p->divAlign;

		string_tokenizer_tokenize (p->st, str + 3, " >");
		while (string_tokenizer_has_more_tokens (p->st)) {
			const gchar *token;

			token = string_tokenizer_next_token (p->st);
			if ( strncasecmp( token, "align=", 6 ) == 0 ) {
				if ( strcasecmp( token + 6, "center" ) == 0 )
					align = HCenter;
				else if ( strcasecmp( token + 6, "right" ) == 0 )
					align = Right;
				else if ( strcasecmp( token + 6, "left" ) == 0 )
					align = Left;
			}
		}
		
		/* Start a new flow box */
		if (!p->flow)
			html_engine_new_flow (p, clue);
		HTML_CLUE (p->flow)->halign = align;

		switch (str[1]) {
		case '1':
			p->bold = TRUE;
			p->fontsize += 3;
			break;

		case '2':
			p->bold = TRUE;
			p->italic = FALSE;
			p->fontsize += 2;
			html_engine_select_font (p);
			break;

		case '3':
			p->bold = TRUE;
			p->italic = FALSE;
			p->fontsize += 2;
			break;

		case '4':
			p->bold = TRUE;
			p->italic = FALSE;
			p->fontsize += 1;
			break;

		case '5':
			p->bold = FALSE;
			p->italic = TRUE;
			break;

		case '6':
			p->bold = TRUE;
			p->italic = FALSE;
			p->fontsize -= 1;
			break;
		}

		html_engine_select_font (p);

		/* Insert a vertical space and restore the old font at the closing
           tag.  */
		html_engine_push_block (p, ID_HEADER, 2, html_engine_block_end_font, TRUE, 0);
	} else if (*(str) == '/' && *(str + 1) == 'h' &&
	    ( *(str+2)=='1' || *(str+2)=='2' || *(str+2)=='3' ||
 	      *(str+2)=='4' || *(str+2)=='5' || *(str+2)=='6' )) {
		/* Close tag.  */
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
		html_clue_append (HTML_CLUE (p->flow),
						  html_rule_new (length, percent, size, shade));
		HTML_CLUE (p->flow)->halign = align;
		p->flow = 0;
		p->vspace_inserted = FALSE;
	}
}


static void
parse_i (HTMLEngine *p, HTMLObject *clue, const gchar *str)
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
		  filename = html_engine_canonicalize_url(p, filename);
		  if (!p->flow)
		    html_engine_new_flow (p, clue);

		  image = html_image_new (p->image_factory, filename, clue->max_width, 
					  width, height, percent, border);
		}

		if (align == None) {
			if (valign == VNone) {
				html_clue_append (HTML_CLUE (p->flow), image);
			}
			else {
				HTMLObject *valigned = html_clueh_new (0, 0, clue->max_width);
				HTML_CLUE (valigned)->valign = valign;
				html_clue_append (HTML_CLUE (valigned), HTML_OBJECT (image));
				html_clue_append (HTML_CLUE (p->flow), valigned);
			}
		}
		/* We need to put the image in a HTMLClueAligned */
		else {
			HTMLClueAligned *aligned = HTML_CLUEALIGNED (html_cluealigned_new (HTML_CLUE (p->flow), 0, 0, clue->max_width, 100));
			HTML_CLUE (aligned)->halign = align;
			html_clue_append (HTML_CLUE (aligned), HTML_OBJECT (image));
			html_clue_append (HTML_CLUE (p->flow), HTML_OBJECT (aligned));
		}
		       
	}
}


static void
parse_l (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "link", 4) == 0) {
	}
	else if (strncmp (str, "li", 2) == 0) {
		/* FIXME: close anchor */
		HTMLObject *f, *c, *vc;

		ListType listType = Unordered;
		ListNumType listNumType = Numeric;
		gint listLevel = 1;
		gint itemNumber = 1;
		gint indentSize = INDENT_SIZE;

		/* Color */

		if (html_list_stack_count (p->listStack) > 0) {
			listType = (html_list_stack_top (p->listStack))->type;
			listNumType = (html_list_stack_top (p->listStack))->numType;
			itemNumber = (html_list_stack_top (p->listStack))->itemNumber;
			listLevel = html_list_stack_count (p->listStack);
			indentSize = p->indent;
		}
		
		f = html_clueflow_new (0, 0, clue->max_width, 100);
		html_clue_append (HTML_CLUE (clue), f);
		c = html_clueh_new (0, 0, clue->max_width);
		HTML_CLUE (c)->valign = Top;
		html_clue_append (HTML_CLUE (f), c);

		/* Fixed width spacer */
		vc = html_cluev_new (0, 0, indentSize, 0);
		HTML_CLUE (vc)->valign = Top;
		html_clue_append (HTML_CLUE (c), vc);

		switch (listType) {
		case Unordered:
			p->flow = html_clueflow_new (0, 0, vc->max_width, 0);
			HTML_CLUE (p->flow)->halign = Right;
			html_clue_append (HTML_CLUE (vc), p->flow);
			html_clue_append (HTML_CLUE (p->flow), html_bullet_new ((html_font_stack_top (p->fs))->pointSize,
										listLevel,
										&p->settings->fontbasecolor));
			break;
		default:
			break;

		}
		
		vc = html_cluev_new (0, 0, clue->max_width - indentSize, 100);
		html_clue_append (HTML_CLUE (c), vc);
		p->flow = html_clueflow_new (0,0, vc->max_width, 100);
		html_clue_append (HTML_CLUE (vc), p->flow);

		p->vspace_inserted = TRUE;
	}
}


static void
parse_p (HTMLEngine *p, HTMLObject *clue, const gchar *str)
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


static const gchar *
parse_table (HTMLEngine *e, HTMLObject *clue, gint max_width,
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
	GdkColor tableColor, rowColor, bgcolor;
	gboolean have_tableColor, have_rowColor, have_bgcolor;
	gint rowSpan;
	gint colSpan;
	gint cellwidth;
	gint cellpercent;
	gboolean fixedWidth;
	VAlignType valign;
	HTMLTableCell *cell;

	have_tableColor = FALSE;
	have_rowColor = FALSE;
	have_bgcolor = FALSE;

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
			html_engine_set_named_color (e, &tableColor, token + 8);
			rowColor = tableColor;
			have_rowColor = have_tableColor = TRUE;
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

					if (have_tableColor) {
						rowColor = tableColor;
						have_rowColor = TRUE;
					} else {
						have_rowColor = FALSE;
					}

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
							html_engine_set_named_color (e, &rowColor,
														 token + 8);
							have_rowColor = TRUE;
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
						bgcolor = rowColor;
						have_bgcolor = TRUE;
					} else {
						have_bgcolor = FALSE;
					}

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
								html_engine_set_named_color (e, &bgcolor, token + 8);
								have_bgcolor = TRUE;
							}
						}
					}

					cell = HTML_TABLE_CELL (html_table_cell_new (0, 0, cellwidth,
								    cellpercent,
								    rowSpan, colSpan,
								    padding));
					html_object_set_bg_color (HTML_OBJECT (cell),
											  have_bgcolor ? &bgcolor : NULL);
					HTML_CLUE (cell)->valign = valign;
					if (fixedWidth)
						HTML_OBJECT (cell)->flags |= HTML_OBJECT_FLAG_FIXEDWIDTH;
					html_table_add_cell (table, cell);
					has_cell = 1;
					e->flow = 0;
					if (heading) {
						/* FIXME: do heading stuff */
					}
					if (!tableEntry) {
						/* Put all the junk between <table>
						   and the first table tag into one row */
						html_engine_push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endall, FALSE);
						html_engine_pop_block (e, ID_TD, HTML_OBJECT (cell));
						html_table_end_row (table);
						html_table_start_row (table);
						
					}
					else {
						/* Ignore <p> and such at the beginning */
						e->vspace_inserted = TRUE;
						html_engine_push_block (e, ID_TD, 3, NULL, 0, 0);
						str = parse_body (e, HTML_OBJECT (cell), endthtd, FALSE);
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
			html_clue_append (HTML_CLUE (clue), HTML_OBJECT (table));
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


static void
parse_t (HTMLEngine *p, HTMLObject *clue, const gchar *str)
{
	if (strncmp (str, "table", 5) == 0) {
		/* FIXME: Close anchor */
		if (!p->vspace_inserted || !p->flow)
			html_engine_new_flow (p, clue);

		parse_table (p, p->flow, clue->max_width, str + 6);

		p->flow = 0;
	}
	else if (strncmp (str, "title", 5) == 0) {
		p->inTitle = TRUE;
		p->title = g_string_new ("");
	}
	else if (strncmp (str, "/title", 6) == 0) {
		p->inTitle = FALSE;

		gtk_signal_emit (GTK_OBJECT (p->widget), html_signals[TITLE_CHANGED]);
	}
}


static void
parse_u (HTMLEngine *p, HTMLObject *clue, const gchar *str)
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

	/* FIXME FIXME FIXME */
	
	html_tokenizer_destroy   (engine->ht);
	string_tokenizer_destroy (engine->st);
	html_font_stack_destroy  (engine->fs);
	html_color_stack_destroy (engine->cs);
	html_list_stack_destroy  (engine->listStack);
	html_settings_destroy    (engine->settings);
	html_painter_destroy     (engine->painter);
	html_image_factory_free  (engine->image_factory);

	g_free (engine->baseTarget);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
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

	object_class->destroy = html_engine_destroy;

	/* Initialize the HTML objects.  */
	html_types_init ();
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
	engine->image_factory = html_image_factory_new(engine);

	engine->url = NULL;
	engine->target = NULL;
	engine->baseTarget = NULL;

	engine->leftBorder = LEFT_BORDER;
	engine->rightBorder = RIGHT_BORDER;
	engine->topBorder = TOP_BORDER;
	engine->bottomBorder = BOTTOM_BORDER;
	
	/* Set up parser functions */
	engine->parseFuncArray[0] = parse_a;
	engine->parseFuncArray[1] = parse_b;
	engine->parseFuncArray[2] = parse_c;
	engine->parseFuncArray[3] = NULL;
	engine->parseFuncArray[4] = parse_e;
	engine->parseFuncArray[5] = parse_f;
	engine->parseFuncArray[6] = NULL;
	engine->parseFuncArray[7] = parse_h;
	engine->parseFuncArray[8] = parse_i;
	engine->parseFuncArray[9] = NULL;
	engine->parseFuncArray[10] = NULL;
	engine->parseFuncArray[11] = parse_l;
	engine->parseFuncArray[12] = NULL;
	engine->parseFuncArray[13] = NULL;
	engine->parseFuncArray[14] = NULL;
	engine->parseFuncArray[15] = parse_p;
	engine->parseFuncArray[16] = NULL;
	engine->parseFuncArray[17] = NULL;
	engine->parseFuncArray[18] = NULL;
	engine->parseFuncArray[19] = parse_t;
	engine->parseFuncArray[20] = parse_u;
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


void
html_engine_calc_absolute_pos (HTMLEngine *e)
{
	if (e->clue)
		html_object_calc_absolute_pos (e->clue, 0, 0);
}

void
html_engine_draw_background (HTMLEngine *e, gint xval, gint yval, gint x, gint y, gint w, gint h)
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

	html_painter_set_clip_rectangle(e->painter, x, y, w, h);

	/* Do the bgimage tiling */
	for (yp = yOrigin; yp < y + h; yp += ph) {
	        for (xp = xOrigin; xp < x + w; xp += pw) {
			html_painter_draw_pixmap (e->painter, 
						  xp, 
						  yp,
						  bgpixmap->pixbuf,
						  0, 0, 0, 0);
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
	html_tokenizer_begin (p->ht);
	
	html_engine_free_block (p); /* Clear the block stack */

	if (url != 0) {
		char *ctmp, *ctmp2;

		p->actualURL = html_engine_canonicalize_url(p, url);

		ctmp = g_dirname(p->actualURL);
		p->baseURL = html_engine_canonicalize_url(p, ctmp);
		g_free(ctmp);

		g_print ("baseURL: %s\n", p->baseURL);
		g_print ("actualURL: %s\n", p->actualURL);
	}

	html_engine_stop_parser (p);
	p->writing = TRUE;

	return gtk_html_stream_new(GTK_HTML(p->widget),
				   p->actualURL,
				   (GtkHTMLStreamWriteFunc)html_engine_write,
				   (GtkHTMLStreamEndFunc)html_engine_end,
				   (gpointer)p);
}

void
html_engine_write (GtkHTMLStreamHandle handle, gchar *buffer, size_t size, HTMLEngine *e)
{
	if (buffer == 0)
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
  html_engine_calc_absolute_pos (e);

  html_engine_draw (e, 0, 0, e->width, e->height);
	
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
	html_painter_set_font (e->painter, html_font_stack_top (e->fs));

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
	gtk_signal_emit(GTK_OBJECT(e->widget), html_signals[LOAD_DONE]);
	html_image_factory_cleanup(e->image_factory);
}

void
html_engine_draw (HTMLEngine *e, gint x, gint y, gint width, gint height)
{
	gint tx, ty;

	tx = -e->x_offset + e->leftBorder;
	ty = -e->y_offset + e->topBorder;

	html_painter_begin (e->painter, x, y, x + width, y + height);

	html_engine_draw_background (e, e->x_offset, e->y_offset, 
				     x, y,
				     width, height);

	if (e->clue)
		html_object_draw (e->clue, e->painter,
				  x - e->x_offset,
				  y + e->y_offset - e->topBorder,
				  width,
				  height,
				  tx, ty);

	html_painter_end (e->painter);
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

	/* Set the clue size */
	p->clue->width = p->width - p->leftBorder - p->rightBorder;

	min_width = html_object_calc_min_width (p->clue);

	if (min_width > max_width) {
		max_width = min_width;
	}

	html_object_set_max_width (p->clue, max_width);
	html_object_calc_size (p->clue, NULL);

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

	html_clue_append (HTML_CLUE (clue), p->flow);
}

void
html_engine_parse (HTMLEngine *p)
{
	HTMLFont *f;
	GdkColor *c;

	g_print ("parse\n");
	html_engine_stop_parser (p);

	if (p->clue != NULL)
		html_object_destroy (p->clue);

	p->flow = 0;

	p->clue = html_cluev_new (0, 0, p->width - p->leftBorder - p->rightBorder, 100);
	HTML_CLUE (p->clue)->valign = Top;
	HTML_CLUE (p->clue)->halign = Left;
	
	/* Initialize the font stack with the default font */
	p->italic = FALSE;
	p->underline = FALSE;
	p->bold = FALSE;
	p->fontsize = p->settings->fontBaseSize;

	f = html_font_new ("lucida", p->settings->fontBaseSize, defaultFontSizes, p->bold, p->italic, p->underline);

	html_font_stack_push (p->fs, f);

	/* Free the background pixmap */
	if (p->bgPixmapPtr) {
		html_image_factory_unregister(p->image_factory, p->bgPixmapPtr, NULL);
		p->bgPixmapPtr = NULL;
	}

	/* Free the background color (if any) and alloc a new one */
	p->bgColor = p->settings->bgcolor;

	/* FIXME: is this nice to do? */
	c = g_new0 (GdkColor, 1);
	gdk_color_black (gdk_window_get_colormap (html_painter_get_window (p->painter)), c);
	html_color_stack_push (p->cs, c);
	f->textColor = html_color_stack_top (p->cs);

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
		f = html_clueflow_new (0, 0, clue->max_width, 100);
		html_clue_append (HTML_CLUE (clue), f);

		/* FIXME: correct font size */
		t = html_vspace_new (defaultFontSizes[e->settings->fontBaseSize], CNone);
		html_clue_append (HTML_CLUE (f), t);
		
		e->flow = NULL;
	}
	
	return TRUE;
}

void
html_engine_push_block (HTMLEngine *e, gint id, gint level,
						HTMLBlockFunc exitFunc, gint miscData1, gint miscData2)
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
			   e->bold, e->italic, e->underline);

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
	e->bold = html_font_stack_top (e->fs)->bold;
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
        if (((guchar *)str)[i] == 0xa0) {
            /* Non-breaking space */
            if (textType == variable) {
                /* We have a non-breaking space in a block of variable text We
                   need to split the text and insert a seperate non-breaking
                   space object */
                str[i] = 0x00; /* End of string */
                remainingStr = &(str[i+1]);
                insertBlock = TRUE; 
                insertNBSP = TRUE;
            } else {
                /* We have a non-breaking space: this makes the block fixed. */
                str[i] = 0x20; /* Normal space */
                textType = fixed;
            }
        } else if (str[i] == 0x20) {
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
					insertBlock = TRUE;	/* Block is zero-length, no actual insertion */
					insertSpace = TRUE;
				}
				else if (str [i+1] == 0x00) {
            	    /* Last character is a space: Insert the block and 
					   a normal space */
					str[i] = 0x00;/* End of string */
					remainingStr = 0;
					insertBlock = TRUE;
					insertSpace = TRUE;
				} else {
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
					if (e->url || e->target)
						obj = html_link_text_master_new
							(str, f, e->painter, e->url, e->target);
					else
						obj = html_text_master_new (str, f, e->painter);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
				else {
					if (e->url || e->target)
						obj = html_link_text_new (str, f,
									  e->painter, e->url, e->target);
					else
						obj = html_text_new (str, f, e->painter);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
			}

			if (insertSpace) {
				if (e->url || e->target) {
					obj = html_link_text_new (" ", f, e->painter, e->url,
											  e->target);
					obj->flags |= HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
				} else {
					obj = html_hspace_new (f, e->painter, FALSE);
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
			}
			else if (insertNBSP) {
				if (e->url || e->target) {
					obj = html_link_text_new (" ", f, e->painter,
											  e->url, e->target);
					obj->flags &= ~HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
				} else {
					obj = html_hspace_new (f, e->painter, FALSE);
					obj->flags &= ~HTML_OBJECT_FLAG_SEPARATOR;
					html_clue_append (HTML_CLUE (e->flow), obj);
				}
			}
		
			str = remainingStr;

			if ((str == 0) || (*str == 0x00))
				return; /* Finished */
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
	return html_font_stack_top (p->fs);
}

void
html_engine_set_named_color (HTMLEngine *p, GdkColor *c, const gchar *name)
{
	gdk_color_parse (name, c);
	gdk_colormap_alloc_color (
		gdk_window_get_colormap (
			html_painter_get_window (p->painter)),
		c, FALSE, TRUE);
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
