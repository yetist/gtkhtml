/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.

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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "htmltext.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-save.h"
#include "htmlentity.h"
#include "htmllinktext.h"
#include "htmlsettings.h"
#include "htmltextslave.h"
#include "htmlundo.h"


HTMLTextClass html_text_class;
static HTMLObjectClass *parent_class = NULL;

#define HT_CLASS(x) HTML_TEXT_CLASS (HTML_OBJECT (x)->klass)

static SpellError * spell_error_new     (guint off, guint len);
static void         spell_error_destroy (SpellError *se);
static void         move_spell_errors   (GList *spell_errors, guint offset, gint delta);
static GList *      remove_spell_errors (GList *spell_errors, guint offset, guint len);
static gchar *      convert_nbsp        (const gchar *s);

/* static void
debug_spell_errors (GList *se)
{
	for (;se;se = se->next)
		printf ("SE: %4d, %4d\n", ((SpellError *) se->data)->off, ((SpellError *) se->data)->len);
} */

static void
get_tags (const HTMLText *text,
	  const HTMLEngineSaveState *state,
	  gchar *opening_tags,
	  gchar *ending_tags)
{
	GtkHTMLFontStyle font_style;
	gchar *opening_p, *ending_p;
	guint size;

	font_style = text->font_style;

	/*
	  FIXME: eek this is completely broken in that there is no
	  possible way the tag order can come out right doing it this
	  way */

	opening_p = opening_tags;
	ending_p = ending_tags;

	if (!html_color_equal (text->color, html_colorset_get_color (state->engine->settings->color_set, HTMLTextColor))
	    && !html_color_equal (text->color, html_colorset_get_color (state->engine->settings->color_set, HTMLLinkColor))) {
		opening_p += sprintf (opening_p, "<FONT COLOR=#%02x%02x%02x>",
				      text->color->color.red   >> 8,
				      text->color->color.green >> 8,
				      text->color->color.blue  >> 8);
		ending_p += sprintf (ending_p, "</FONT>");
	}

	size = font_style & GTK_HTML_FONT_STYLE_SIZE_MASK;
	if (size != 0) {
		opening_p += sprintf (opening_p, "<FONT SIZE=%d>", size);
	}

	if (font_style & GTK_HTML_FONT_STYLE_BOLD) {
		opening_p += sprintf (opening_p, "<B>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC) {
		opening_p += sprintf (opening_p, "<I>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE) {
		opening_p += sprintf (opening_p, "<U>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT) {
		opening_p += sprintf (opening_p, "<S>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_FIXED) {
		opening_p += sprintf (opening_p, "<TT>");
		ending_p += sprintf (ending_p, "</TT>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT) {
		ending_p += sprintf (ending_p, "</S>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE) {
		ending_p += sprintf (ending_p, "</U>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC) {
		ending_p += sprintf (ending_p, "</I>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_BOLD) {
		ending_p += sprintf (ending_p, "</B>");
	}

	if (size != 0) {
		ending_p += sprintf (ending_p, "</FONT SIZE=%d>", size);
	}


	*opening_p = 0;
	*ending_p = 0;
}

static void
copy_helper (HTMLText *src,
	     HTMLText *dest,
	     guint offset,
	     gint len)
{
	GList *cur;

	if (len < 0)
		len = unicode_strlen (src->text, -1);

	dest->text = g_strndup (html_text_get_text (src, offset),
				unicode_offset_to_index (src->text, offset + len)
				- unicode_offset_to_index (src->text, offset));

	dest->text_len      = len;
	dest->font_style    = src->font_style;
	dest->face          = src->face;
	dest->color         = src->color;
	dest->select_start  = src->select_start;
	dest->select_length = src->select_length;

	html_color_ref (dest->color);

	dest->spell_errors = g_list_copy (src->spell_errors);
	cur = dest->spell_errors;
	while (cur) {
		SpellError *se = (SpellError *) cur->data;
		cur->data = spell_error_new (se->off, se->len);
		cur = cur->next;
	}
	dest->spell_errors = remove_spell_errors (dest->spell_errors, 0, offset);
	dest->spell_errors = remove_spell_errors (dest->spell_errors, offset + len, src->text_len - offset - len);
	move_spell_errors (dest->spell_errors, 0, -offset);
}


/* HTMLObject methods.  */

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	copy_helper (HTML_TEXT (self), HTML_TEXT (dest), 0, -1);
}

HTMLObject *
html_text_op_copy_helper (HTMLText *text, GList *from, GList *to, guint *len, HTMLTextHelperFunc f)
{
	gint begin, end;

	begin = (from) ? GPOINTER_TO_INT (from->data) : 0;
	end   = (to)   ? GPOINTER_TO_INT (to->data)   : text->text_len;

	*len += end - begin;

	return (*f) (text, begin, end);
}

HTMLObject *
html_text_op_cut_helper (HTMLText *text, HTMLEngine *e, GList *from, GList *to, GList *left, GList *right,
			 guint *len, HTMLTextHelperFunc f)
{
	HTMLObject *rv;
	gint begin, end;

	begin = (from) ? GPOINTER_TO_INT (from->data) : 0;
	end   = (to)   ? GPOINTER_TO_INT (to->data)   : text->text_len;

	g_assert (begin <= end);
	g_assert (end <= text->text_len);

	if (!html_object_could_remove_whole (HTML_OBJECT (text), from, to, left, right) || begin || end < text->text_len) {
		gchar *nt, *tail;

		if (begin == end)
			return (*f) (text, 0, 0);

		rv = (*f) (text, begin, end);

		tail = html_text_get_text (text, end);
		text->text [html_text_get_index (text, begin)] = 0;
		nt = g_strconcat (text->text, tail, NULL);
		g_free (text->text);
		text->text = nt;
		text->text_len -= end - begin;
		*len           += end - begin;

		move_spell_errors (text->spell_errors, end, - (end - begin));
	} else {
		text->spell_errors = remove_spell_errors (text->spell_errors, 0, text->text_len);
		html_object_move_cursor_before_remove (HTML_OBJECT (text), e);
		html_object_remove_child (HTML_OBJECT (text)->parent, HTML_OBJECT (text));

		rv    = HTML_OBJECT (text);
		*len += text->text_len;
	}

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
	return rv;
}

static HTMLObject *
new_text (HTMLText *t, gint begin, gint end)
{
	return HTML_OBJECT (html_text_new_with_len (html_text_get_text (t, begin),
						    end - begin, t->font_style, t->color));
}

static HTMLObject *
op_copy (HTMLObject *self, HTMLEngine *e, GList *from, GList *to, guint *len)
{
	return html_text_op_copy_helper (HTML_TEXT (self), from, to, len, new_text);
}

static HTMLObject *
op_cut (HTMLObject *self, HTMLEngine *e, GList *from, GList *to, GList *left, GList *right, guint *len)
{
	return html_text_op_cut_helper (HTML_TEXT (self), e, from, to, left, right, len, new_text);
}

static gboolean
object_merge (HTMLObject *self, HTMLObject *with)
{
	HTMLText *t1, *t2;
	gchar *to_free;

	t1 = HTML_TEXT (self);
	t2 = HTML_TEXT (with);

	if (t1->font_style != t2->font_style || t1->color != t2->color)
		return FALSE;

	move_spell_errors (t2->spell_errors, 0, t1->text_len);
	t1->spell_errors = g_list_concat (t1->spell_errors, t2->spell_errors);
	t2->spell_errors = NULL;

	to_free       = t1->text;
	t1->text      = g_strconcat (t1->text, t2->text, NULL);
	t1->text_len += t2->text_len;
	g_free (to_free);
	to_free       = t1->text;
	t1->text      = convert_nbsp (t1->text);
	g_free (to_free);

	/* printf ("--- after merge\n");
	debug_spell_errors (t1->spell_errors);
	printf ("---\n"); */

	html_object_change_set (self, HTML_CHANGE_ALL);

	return TRUE;
}

static void
object_split (HTMLObject *self, HTMLEngine *e, HTMLObject *child, gint offset, gint level, GList **left, GList **right)
{
	HTMLObject *dup, *prev;
	HTMLText *text;
	gchar *tt;

	g_assert (self->parent);

	html_clueflow_remove_text_slaves (HTML_CLUEFLOW (self->parent));

	text            = HTML_TEXT (self);
	dup             = html_object_dup (self);
	tt              = text->text;
	text->text      = g_strndup (tt, html_text_get_index (text, offset));
	g_free (tt);
	tt              = text->text;
	text->text      = convert_nbsp (text->text);
	g_free (tt);
	text->text_len  = offset;

	text            = HTML_TEXT (dup);
	tt              = text->text;
	text->text      = convert_nbsp (html_text_get_text (text, offset));
	text->text_len -= offset;
	g_free (tt);

	html_clue_append_after (HTML_CLUE (self->parent), dup, self);

	prev = self->prev;
	if (HTML_TEXT (self)->text_len == 0 && prev && html_object_merge (prev, self))
		self = prev;

	if (HTML_TEXT (dup)->text_len == 0 && dup->next)
		html_object_merge (dup, dup->next);

	/* printf ("--- before split offset %d dup len %d\n", offset, HTML_TEXT (dup)->text_len);
	   debug_spell_errors (HTML_TEXT (self)->spell_errors); */

	HTML_TEXT (self)->spell_errors = remove_spell_errors (HTML_TEXT (self)->spell_errors,
							      offset, HTML_TEXT (dup)->text_len);
	HTML_TEXT (dup)->spell_errors  = remove_spell_errors (HTML_TEXT (dup)->spell_errors,
							      0, HTML_TEXT (self)->text_len);
	move_spell_errors   (HTML_TEXT (dup)->spell_errors, 0, - HTML_TEXT (self)->text_len);

	/* printf ("--- after split\n");
	printf ("left\n");
	debug_spell_errors (HTML_TEXT (self)->spell_errors);
	printf ("right\n");
	debug_spell_errors (HTML_TEXT (dup)->spell_errors);
	printf ("---\n"); */

	*left  = g_list_prepend (*left, self);
	*right = g_list_prepend (*right, dup);

	html_object_change_set (self, HTML_CHANGE_ALL);
	html_object_change_set (dup,  HTML_CHANGE_ALL);

	level--;
	if (level)
		html_object_split (self->parent, e, dup, 0, level, left, right);
}

static gboolean
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	HTMLText *text = HTML_TEXT (self);
	GtkHTMLFontStyle style = html_text_get_font_style (text);

	self->width = 0;
	self->ascent = html_painter_calc_ascent (painter, style, text->face);
	self->descent = html_painter_calc_descent (painter, style, text->face);

	return FALSE;
}

static gint
calc_preferred_width (HTMLObject *self,
		      HTMLPainter *painter)
{
	HTMLText *text;
	GtkHTMLFontStyle font_style;

	text = HTML_TEXT (self);
	font_style = html_text_get_font_style (text);

	return html_painter_calc_text_width (painter,
					     text->text, text->text_len,
					     font_style, text->face);
}

static HTMLFitType
fit_line (HTMLObject *o,
	  HTMLPainter *painter,
	  gboolean startOfLine,
	  gboolean firstRun,
	  gint widthLeft) 
{
	HTMLText *text; 
	HTMLObject *next_obj;
	HTMLObject *text_slave;

	text = HTML_TEXT (o);

	if (o->flags & HTML_OBJECT_FLAG_NEWLINE)
		return HTML_FIT_COMPLETE;
	
	/* Remove existing slaves */
	next_obj = o->next;
	while (next_obj != NULL
	       && (HTML_OBJECT_TYPE (next_obj) == HTML_TYPE_TEXTSLAVE)) {
		o->next = next_obj->next;
		html_clue_remove (HTML_CLUE (next_obj->parent), next_obj);
		html_object_destroy (next_obj);
		next_obj = o->next;
	}
	
	/* Turn all text over to our slaves */
	text_slave = html_text_slave_new (text, 0, HTML_TEXT (text)->text_len);
	html_clue_append_after (HTML_CLUE (o->parent), text_slave, o);

	return HTML_FIT_COMPLETE;
}

static gint
forward_get_nb_width (HTMLText *text, HTMLPainter *painter, gboolean begin)
{
	HTMLObject *obj;

	g_assert (text);
	g_assert (html_object_is_text (HTML_OBJECT (text)));
	g_assert (text->text_len == 0);

	/* find prev/next object */
	obj = (begin)
		? html_object_prev_not_slave (HTML_OBJECT (text))
		: html_object_next_not_slave (HTML_OBJECT (text));

	/* if not found or not text return 0, otherwise forward get_nb_with there */
	if (!obj || !html_object_is_text (obj))
		return 0;
	else
		return html_text_get_nb_width (HTML_TEXT (obj), painter, begin);
}

/* return non-breakable text width on begin/end of this text */
gint
html_text_get_nb_width (HTMLText *text, HTMLPainter *painter, gboolean begin)
{
	gchar *t = text->text;

	/* handle "" case */
	if (text->text_len == 0)
		return forward_get_nb_width (text, painter, begin);

	/* if begins/ends with ' ' the width is 0 */
	if ((begin && html_text_get_char (text, 0) == ' ')
	    || (!begin && html_text_get_char (text, text->text_len - 1) == ' '))
		return 0;

	/* find end/begin of nb text */
	t = (begin) ? unicode_strchr (t, ' ') : strrchr (t, ' '); /* unicode_strrchr (t, ' '); */
	if (!t)
		return html_object_calc_preferred_width (HTML_OBJECT (text), painter);
	return html_painter_calc_text_width (painter, (begin) ? text->text : t + 1,
					     (begin)
					     ? unicode_index_to_offset (text->text, t - text->text)
					     : text->text_len - unicode_index_to_offset (text->text, t - text->text) + 1,
					     html_text_get_font_style (text), text->face);
}

static gint
calc_min_width (HTMLObject *self,
		HTMLPainter *painter)
{
	GtkHTMLFontStyle font_style;
	HTMLObject *obj;
	HTMLText *text;
	gchar *t, *space;
	gint w = 0, min_width = 0;

	text       = HTML_TEXT (self);
	font_style = html_text_get_font_style (text);
	t          = text->text;

	if (text->text_len == 0 || html_text_get_char (text, 0) != ' ') {
		obj = html_object_prev_not_slave (self);
		w = (obj && html_object_is_text (obj)) ? html_text_get_nb_width (HTML_TEXT (obj), painter, FALSE) : 0;
	}

	if (text->text_len)
		do {
			space = strchr (t, ' ');
			if (!space)
				space = text->text + strlen (text->text);
			w += html_painter_calc_text_width (painter, t, unicode_index_to_offset (t, space - t),
							   font_style, text->face);
			t = (*space) ? space + 1 : space;
			if (!(*t))
				break;
			if (w > min_width)
				min_width = w;
			w = 0;
		} while (1);

	if (text->text_len == 0 || html_text_get_char (text, text->text_len - 1) != ' ') {
		obj = html_object_next_not_slave (self);
		w += (obj && html_object_is_text (obj)) ? html_text_get_nb_width (HTML_TEXT (obj), painter, TRUE) : 0;
	}

	if (w > min_width)
		min_width = w;

	return min_width;
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x, gint y,
      gint width, gint height,
      gint tx, gint ty)
{
}

static gboolean
accepts_cursor (HTMLObject *object)
{
	return TRUE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	/* OK, doing these nasty things is not in my style, but in this case
           it's so unlikely to break and it's so handy and fast that I think
           it's almost acceptable.  */
#define RIDICULOUS_BUFFER_SIZE 16384
	gchar opening_tags[RIDICULOUS_BUFFER_SIZE];
	gchar closing_tags[RIDICULOUS_BUFFER_SIZE];
#undef RIDICULOUS_BUFFER_SIZE
	HTMLText *text;

	text = HTML_TEXT (self);

	get_tags (text, state, opening_tags, closing_tags);

	if (! html_engine_save_output_string (state, "%s", opening_tags))
		return FALSE;

	if (! html_engine_save_encode (state, text->text, text->text_len))
		return FALSE;

	if (! html_engine_save_output_string (state, "%s", closing_tags))
		return FALSE;

	return TRUE;
}

static gboolean
save_plain (HTMLObject *self,
	    HTMLEngineSaveState *state,
	    gint requested_width)
{
	HTMLText *text;
	gboolean rv;

	text = HTML_TEXT (self);

	rv  = html_engine_save_output_string (state, "%s", text->text);
	
	return rv;
}

static guint
get_length (HTMLObject *self)
{
	return HTML_TEXT (self)->text_len;
}

/* #define DEBUG_NBSP */

static gchar *
convert_nbsp (const gchar *s)
{
	/* state of automata:
	   0..Text
	   1..Sequence <space>&nbsp;...&nbsp;
	*/
	gint delta;
	guint state, pass;
	unicode_char_t uc;
	const gchar *p, *op;
	guchar *rv = NULL, *rp = NULL;

	pass  = 0;
	delta = 0;

#ifdef DEBUG_NBSP
	printf ("convert_nbsp: %s --> ", s);
#endif
	for (pass=0; pass < 2; pass++) {
		op = p = s;
		state = 0;
		while (*p && (p = unicode_get_utf8 (p, &uc))) {
			if (uc == ENTITY_NBSP || uc == ' ') {
				if (pass) {
					if (state) {
#ifdef DEBUG_NBSP
						printf ("&nbsp;");
#endif
						*rp = 0xc2; rp ++;
						*rp = 0xa0; rp ++;
					} else {
#ifdef DEBUG_NBSP
						printf (" ");
#endif
						*rp = ' '; rp++;
					}
				} else {
					if (uc == ENTITY_NBSP && !state) delta--;
					if (uc == ' ' && state) delta++;
				}
				state = 1;
			} else {
				state = 0;
				if (pass) {
#ifdef DEBUG_NBSP
					printf ("*");
#endif
					strncpy (rp, op, p - op);
					rp += p - op;
				}
			}
			op = p;
		}
		if (!pass) {
			rp = rv = g_malloc (strlen (s) + delta + 1);
			rv [strlen (s) + delta] = 0;
		}
	}
#ifdef DEBUG_NBSP
	printf ("\n");
#endif
	return rv;
}

static void 
move_spell_errors (GList *spell_errors, guint offset, gint delta) 
{ 
	SpellError *se; 

	if (!delta) return;

	while (spell_errors) { 
		se = (SpellError *) spell_errors->data; 
		if (se->off >= offset) 
			se->off += delta; 
 		spell_errors = spell_errors->next; 
  	} 
} 

inline static GList *
remove_one (GList *list, GList *link)
{
	spell_error_destroy ((SpellError *) link->data);
	return g_list_remove_link (list, link);
}

static GList *
remove_spell_errors (GList *spell_errors, guint offset, guint len)
{
	SpellError *se; 
	GList *cur, *cnext;

	cur = spell_errors;
	while (cur) { 
		cnext = cur->next;
		se = (SpellError *) cur->data;
		if (se->off < offset) {
			if (se->off + se->len > offset) {
				if (se->off + se->len <= offset + len)
					se->len = offset - se->off;
				else
					se->len -= len;
				if (se->len < 2)
					spell_errors = remove_one (spell_errors, cur);
			}
		} else if (se->off < offset + len) {
			if (se->off + se->len <= offset + len)
				spell_errors = remove_one (spell_errors, cur);
			else {
				se->len -= offset + len - se->off;
				se->off  = offset + len;
				if (se->len < 2)
					spell_errors = remove_one (spell_errors, cur);
			}
		}
 		cur = cnext;
  	} 
	return spell_errors;
}

static HTMLObject *
check_point (HTMLObject *self,
	     HTMLPainter *painter,
	     gint x, gint y,
	     guint *offset_return,
	     gboolean for_cursor)
{
	return NULL;
}

static void
queue_draw (HTMLText *text,
	    HTMLEngine *engine,
	    guint offset,
	    guint len)
{
	HTMLObject *obj;

	for (obj = HTML_OBJECT (text)->next; obj != NULL; obj = obj->next) {
		HTMLTextSlave *slave;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_TEXTSLAVE)
			continue;

		slave = HTML_TEXT_SLAVE (obj);

		if (offset < slave->posStart + slave->posLen
		    && (len == 0 || offset + len >= slave->posStart)) {
			html_engine_queue_draw (engine, obj);
			if (len != 0 && slave->posStart + slave->posLen > offset + len)
				break;
		}
	}
}

/* This is necessary to merge the text-specified font style with that of the
   HTMLClueFlow parent.  */
static GtkHTMLFontStyle
get_font_style (const HTMLText *text)
{
	HTMLObject *parent;
	GtkHTMLFontStyle font_style;

	parent = HTML_OBJECT (text)->parent;

	if (HTML_OBJECT_TYPE (parent) == HTML_TYPE_CLUEFLOW) {
		GtkHTMLFontStyle parent_style;

		parent_style = html_clueflow_get_default_font_style (HTML_CLUEFLOW (parent));
		font_style = gtk_html_font_style_merge (parent_style, text->font_style);
	} else {
		font_style = gtk_html_font_style_merge (GTK_HTML_FONT_STYLE_SIZE_3, text->font_style);
	}

	return font_style;
}

static HTMLColor *
get_color (HTMLText *text,
	   HTMLPainter *painter)
{
	return text->color;
}

static void
set_font_style (HTMLText *text,
		HTMLEngine *engine,
		GtkHTMLFontStyle style)
{
	if (text->font_style == style)
		return;

	text->font_style = style;

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);

	if (engine != NULL) {
		html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
		html_engine_queue_draw (engine, HTML_OBJECT (text));
	}
}

static void
set_color (HTMLText *text,
	   HTMLEngine *engine,
	   HTMLColor *color)
{
	if (html_color_equal (text->color, color))
		return;

	html_color_unref (text->color);
	html_color_ref (color);
	text->color = color;

	if (engine != NULL) {
		html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
		html_engine_queue_draw (engine, HTML_OBJECT (text));
	}
}

static void
destroy (HTMLObject *obj)
{
	HTMLText *text = HTML_TEXT (obj);
	html_color_unref (text->color);
	html_text_spell_errors_clear (text);
	g_free (text->text);
	HTML_OBJECT_CLASS (parent_class)->destroy (obj);
}


static gboolean
select_range (HTMLObject *self,
	      HTMLEngine *engine,
	      guint offset,
	      gint length,
	      gboolean queue_draw)
{
	HTMLText *text;
	HTMLObject *p;
	gboolean changed;

	text = HTML_TEXT (self);

	if (length < 0 || length + offset > HTML_TEXT (self)->text_len)
		length = HTML_TEXT (self)->text_len - offset;

	if (offset != text->select_start || length != text->select_length)
		changed = TRUE;
	else
		changed = FALSE;

	if (queue_draw) {
		for (p = self->next;
		     p != NULL && HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE;
		     p = p->next) {
			HTMLTextSlave *slave;
			gboolean was_selected, is_selected;
			guint max;

			slave = HTML_TEXT_SLAVE (p);

			max = slave->posStart + slave->posLen;

			if (text->select_start + text->select_length > slave->posStart
			    && text->select_start < max)
				was_selected = TRUE;
			else
				was_selected = FALSE;

			if (offset + length > slave->posStart && offset < max)
				is_selected = TRUE;
			else
				is_selected = FALSE;

			if (was_selected && is_selected) {
				gint diff1, diff2;

				diff1 = offset - slave->posStart;
				diff2 = text->select_start - slave->posStart;

				if (diff1 != diff2) {
					html_engine_queue_draw (engine, p);
				} else {
					diff1 = offset + length - slave->posStart;
					diff2 = (text->select_start + text->select_length
						 - slave->posStart);

					if (diff1 != diff2)
						html_engine_queue_draw (engine, p);
				}
			} else {
				if ((! was_selected && is_selected) || (was_selected && ! is_selected))
					html_engine_queue_draw (engine, p);
			}
		}
	}

	text->select_start = offset;
	text->select_length = length;

	if (length == 0)
		self->selected = FALSE;
	else
		self->selected = TRUE;

	return changed;
}

static HTMLObject *
set_link (HTMLObject *self, HTMLColor *color, const gchar *url, const gchar *target)
{
	HTMLText *text = HTML_TEXT (self);

	return html_link_text_new_with_len (text->text, text->text_len, text->font_style, color, url, target);
}

static void
append_selection_string (HTMLObject *self,
			 GString *buffer)
{
	HTMLText *text;
	const gchar *p, *last;

	text = HTML_TEXT (self);
	if (text->select_length == 0)
		return;

	/* FIXME: we need a `g_string_append()' that takes the number of
           characters to append as an extra parameter.  */

	p    = html_text_get_text (text, text->select_start);
	last = html_text_get_text (text, text->select_start + text->select_length);
	for (; p < last;) {
		g_string_append_c (buffer, *p);
		p++;
	}
}

static void
get_cursor (HTMLObject *self,
	    HTMLPainter *painter,
	    guint offset,
	    gint *x1, gint *y1,
	    gint *x2, gint *y2)
{
	HTMLObject *slave;
	guint ascent, descent;

	html_object_get_cursor_base (self, painter, offset, x2, y2);

	slave = self->next;
	if (slave == NULL || HTML_OBJECT_TYPE (slave) != HTML_TYPE_TEXTSLAVE) {
		ascent = 0;
		descent = 0;
	} else {
		ascent = slave->ascent;
		descent = slave->descent;
	}

	*x1 = *x2;
	*y1 = *y2 - ascent;
	*y2 += descent - 1;
}

static void
get_cursor_base (HTMLObject *self,
		 HTMLPainter *painter,
		 guint offset,
		 gint *x, gint *y)
{
	HTMLObject *obj;

	for (obj = self->next; obj != NULL; obj = obj->next) {
		HTMLTextSlave *slave;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_TEXTSLAVE)
			break;

		slave = HTML_TEXT_SLAVE (obj);

		if (offset <= slave->posStart + slave->posLen
		    || obj->next == NULL
		    || HTML_OBJECT_TYPE (obj->next) != HTML_TYPE_TEXTSLAVE) {
			html_object_calc_abs_position (obj, x, y);
			if (offset != slave->posStart) {
				HTMLText *text;
				GtkHTMLFontStyle font_style;

				text = HTML_TEXT (self);

				font_style = html_text_get_font_style (text);
				*x += html_painter_calc_text_width (painter,
								    html_text_get_text (text, slave->posStart),
								    offset - slave->posStart,
								    font_style, text->face);
			}

			return;
		}
	}

	g_warning ("Getting cursor base for an HTMLText with no slaves -- %p\n",
		   self);
}

void
html_text_type_init (void)
{
	html_text_class_init (&html_text_class, HTML_TYPE_TEXT, sizeof (HTMLText));
}

void
html_text_class_init (HTMLTextClass *klass,
		      HTMLType type,
		      guint object_size)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type, object_size);

	object_class->destroy = destroy;
	object_class->copy = copy;
	object_class->op_copy = op_copy;
	object_class->op_cut = op_cut;
	object_class->merge = object_merge;
	object_class->split = object_split;
	object_class->draw = draw;
	object_class->accepts_cursor = accepts_cursor;
	object_class->calc_size = calc_size;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_min_width = calc_min_width;
	object_class->fit_line = fit_line;
	object_class->get_cursor = get_cursor;
	object_class->get_cursor_base = get_cursor_base;
	object_class->save = save;
	object_class->save_plain = save_plain;
	object_class->check_point = check_point;
	object_class->select_range = select_range;
	object_class->get_length = get_length;
	object_class->set_link = set_link;
	object_class->append_selection_string = append_selection_string;

	/* HTMLText methods.  */

	klass->queue_draw = queue_draw;
	klass->get_font_style = get_font_style;
	klass->get_color = get_color;
	klass->set_font_style = set_font_style;
	klass->set_color = set_color;

	parent_class = &html_object_class;
}

inline static gint
text_len (const gchar *str, gint len)
{
	return len == -1 ? unicode_strlen (str, -1) : len;
}

void
html_text_init (HTMLText *text,
		HTMLTextClass *klass,
		const gchar *str,
		gint len,
		GtkHTMLFontStyle font_style,
		HTMLColor *color)
{
	g_assert (color);

	html_object_init (HTML_OBJECT (text), HTML_OBJECT_CLASS (klass));

	text->text          = len == -1 ? g_strdup (str) : g_strndup (str, unicode_offset_to_index (str, len));
	text->text_len      = text_len (str, len);
	text->font_style    = font_style;
	text->face          = NULL;
	text->color         = color;
	text->spell_errors  = NULL;
	text->select_start  = 0;
	text->select_length = 0;

	html_color_ref (color);
}

HTMLObject *
html_text_new_with_len (const gchar *str, gint len, GtkHTMLFontStyle font, HTMLColor *color)
{
	HTMLText *text;

	text = g_new (HTMLText, 1);

	html_text_init (text, &html_text_class, str, len, font, color);

	return HTML_OBJECT (text);
}

HTMLObject *
html_text_new (const gchar *text,
	       GtkHTMLFontStyle font,
	       HTMLColor *color)
{
	return html_text_new_with_len (text, -1, font, color);
}

void
html_text_queue_draw (HTMLText *text,
		      HTMLEngine *engine,
		      guint offset,
		      guint len)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (engine != NULL);

	(* HT_CLASS (text)->queue_draw) (text, engine, offset, len);
}


GtkHTMLFontStyle
html_text_get_font_style (const HTMLText *text)
{
	g_return_val_if_fail (text != NULL, GTK_HTML_FONT_STYLE_DEFAULT);

	return (* HT_CLASS (text)->get_font_style) (text);
}

HTMLColor *
html_text_get_color (HTMLText *text,
		     HTMLPainter *painter)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (painter != NULL, NULL);

	return (* HT_CLASS (text)->get_color) (text, painter);
}

void
html_text_set_font_style (HTMLText *text,
			  HTMLEngine *engine,
			  GtkHTMLFontStyle style)
{
	g_return_if_fail (text != NULL);

	return (* HT_CLASS (text)->set_font_style) (text, engine, style);
}

void
html_text_set_color (HTMLText *text,
		     HTMLEngine *engine,
		     HTMLColor *color)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (color != NULL);

	return (* HT_CLASS (text)->set_color) (text, engine, color);
}

void
html_text_set_font_face (HTMLText *text, HTMLFontFace *face)
{
	if (text->face)
		g_free (text->face);
	text->face = g_strdup (face);
}

void
html_text_set_text (HTMLText *text, const gchar *new_text)
{
	g_free (text->text);
	text->text = g_strdup (new_text);
	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
}

/* spell checking */

#include "htmlinterval.h"

static SpellError *
spell_error_new (guint off, guint len)
{
	SpellError *se = g_new (SpellError, 1);

	se->off = off;
	se->len = len;

	return se;
}

static void
spell_error_destroy (SpellError *se)
{
	g_free (se);
}

void
html_text_spell_errors_clear (HTMLText *text)
{
	g_list_foreach (text->spell_errors, (GFunc) spell_error_destroy, NULL);
	g_list_free    (text->spell_errors);
	text->spell_errors = NULL;
}

void
html_text_spell_errors_clear_interval (HTMLText *text, HTMLInterval *i)
{
	GList *cur, *cnext;
	SpellError *se;
	guint offset, len;

	offset = html_interval_get_start  (i, HTML_OBJECT (text));
	len    = html_interval_get_length (i, HTML_OBJECT (text));
	cur    = text->spell_errors;

	/* printf ("html_text_spell_errors_clear_interval %s %d %d\n", text->text, offset, len); */

	while (cur) {
		cnext = cur->next;
		se    = (SpellError *) cur->data;
		/* test overlap */
		if (MAX (offset, se->off) <= MIN (se->off + se->len, offset + len)) {
			text->spell_errors = g_list_remove_link (text->spell_errors, cur);
			spell_error_destroy (se);
			g_list_free (cur);
		}
		cur = cnext;
	}
}

static gint
se_cmp (gconstpointer a, gconstpointer b)
{
	guint o1, o2;

	o1 = ((SpellError *) a)->off;
	o2 = ((SpellError *) b)->off;

	if (o1 < o2)  return -1;
	if (o1 == o2) return 0;
	return 1;
}

void
html_text_spell_errors_add (HTMLText *text, guint off, guint len)
{
	/* GList *cur;
	SpellError *se;
	cur = */

	text->spell_errors = g_list_insert_sorted (text->spell_errors, spell_error_new (off, len), se_cmp);

	/* printf ("---------------------------------------\n");
	while (cur) {
		se = (SpellError *) cur->data;
		printf ("off: %d len: %d\n", se->off, se->len);
		cur = cur->next;
	}
	printf ("---------------------------------------\n"); */
}

guint
html_text_get_bytes (HTMLText *text)
{
	return strlen (text->text);
}

guint
html_text_get_index (HTMLText *text, guint offset)
{
	return unicode_offset_to_index (text->text, offset);
}

unicode_char_t
html_text_get_char (HTMLText *text, guint offset)
{
	unicode_char_t uc;

	unicode_get_utf8 (text->text + html_text_get_index (text, offset), &uc);
	return uc;
}

gchar *
html_text_get_text (HTMLText *text, guint offset)
{
	return text->text + html_text_get_index (text, offset);
}

/* magic links */

struct _HTMLMagicInsertMatch
{
	gchar *regex;
	regex_t *preg;
	gchar *prefix;
};

typedef struct _HTMLMagicInsertMatch HTMLMagicInsertMatch;

static HTMLMagicInsertMatch mim [] = {
	{ "(news|telnet|nttp|file|http|ftp|https)://[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-a-z0-9_$.+!*(),;:@&=?/~#]*[^]'.}>) ,\"]*)?", NULL, NULL },
	{ "www[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#]*[^]'.}>) ,\"]*)?", NULL, "http://" },
	{ "ftp[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#]*[^]'.}>) ,\"]*)?", NULL, "ftp://" },
	{ "[-_a-z0-9.]+@[-_a-z0-9.]+", NULL, "mailto:" }
};

#define MIM_N (sizeof (mim) / sizeof (mim [0]))

void
html_engine_init_magic_links (void)
{
	gint i;

	for (i=0; i<MIM_N; i++) {
		mim [i].preg = g_new0 (regex_t, 1);
		if (regcomp (mim [i].preg, mim [i].regex, REG_EXTENDED | REG_ICASE)) {
			/* error */
			g_free (mim [i].preg);
			mim [i].preg = 0;
		}
	}
}

static void
paste_link (HTMLEngine *engine, HTMLText *text, gint so, gint eo, gchar *prefix)
{
	HTMLObject *new_obj;
	gchar *href;
	gchar *base;

	base = g_strndup (html_text_get_text (text, so), html_text_get_index (text, eo) - html_text_get_index (text, so));
	href = (prefix) ? g_strconcat (prefix, base, NULL) : g_strdup (base);
	g_free (base);

	new_obj = html_link_text_new_with_len
		(html_text_get_text (text, so),
		 eo - so,
		 text->font_style,
		 html_colorset_get_color (engine->settings->color_set, HTMLLinkColor),
		 href, NULL);

	html_cursor_jump_to_position (engine->cursor, engine, engine->cursor->position + so - engine->cursor->offset);
	html_engine_set_mark (engine);
	html_cursor_jump_to_position (engine->cursor, engine, engine->cursor->position + eo - engine->cursor->offset);

	html_undo_level_begin    (engine->undo, "Magic link");
	html_engine_paste_object (engine, new_obj, eo - so);
	html_undo_level_end      (engine->undo);

	g_free (href);
}

gboolean
html_text_magic_link (HTMLText *text, HTMLEngine *engine, guint offset)
{
	regmatch_t pmatch [2];
	gint i;

	if (!offset)
		return FALSE;
	offset--;

	while (html_text_get_char (text, offset) != ' ' && html_text_get_char (text, offset) != ENTITY_NBSP && offset)
		offset--;
	if (html_text_get_char (text, offset) == ' ' || html_text_get_char (text, offset) == ENTITY_NBSP)
		offset++;

	while (offset < text->text_len) {
		for (i=0; i<MIM_N; i++) {
			if (mim [i].preg && !regexec (mim [i].preg, html_text_get_text (text, offset), 2, pmatch, 0)) {
				gint o = html_text_get_text (text, offset) - text->text;
				paste_link (engine, text,
					    unicode_index_to_offset (text->text, pmatch [0].rm_so + o),
					    unicode_index_to_offset (text->text, pmatch [0].rm_eo + o), mim [i].prefix);
				return TRUE;
			}
		}
		offset++;
	}

	return FALSE;
}

/*
 * magic links end
 */

gint
html_text_trail_space_width (HTMLText *text, HTMLPainter *painter)
{
	if (text->text_len > 0 && html_text_get_char (text, text->text_len - 1) == ' ') {
		GtkHTMLFontStyle font_style;

		font_style = html_text_get_font_style (text);
		return html_painter_calc_text_width (painter, " ", 1, font_style, text->face);
	} else {
		return 0;
	}
}

void
html_text_append (HTMLText *text, const gchar *str, gint len)
{
	gchar *to_delete;

	to_delete       = text->text;
	text->text      = g_strconcat (to_delete, str, NULL);
	text->text_len += text_len (str, len);

	g_free (to_delete);

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
}
