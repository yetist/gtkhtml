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

#include <stdio.h>
#include <string.h>

#include "htmltext.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlentity.h"


HTMLTextClass html_text_class;
static HTMLObjectClass *parent_class = NULL;

#define HT_CLASS(x) HTML_TEXT_CLASS (HTML_OBJECT (x)->klass)

#ifdef GTKHTML_HAVE_PSPELL
static SpellError * spell_error_new     (guint off, guint len);
static void         spell_error_destroy (SpellError *se);
static void         move_spell_errors   (GList *spell_errors, guint offset, gint delta);
static GList *      remove_spell_errors (GList *spell_errors, guint offset, guint len);
#endif


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
		opening_p += sprintf (opening_p, "<font color=#%02x%02x%02x>",
				      text->color->color.red   >> 8,
				      text->color->color.green >> 8,
				      text->color->color.blue  >> 8);
		ending_p += sprintf (ending_p, "</font>");
	}

	size = font_style & GTK_HTML_FONT_STYLE_SIZE_MASK;
	if (size != 0) {
		opening_p += sprintf (opening_p, "<font size=%d>", size);
	}

	if (font_style & GTK_HTML_FONT_STYLE_BOLD) {
		opening_p += sprintf (opening_p, "<b>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC) {
		opening_p += sprintf (opening_p, "<i>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE) {
		opening_p += sprintf (opening_p, "<u>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT) {
		opening_p += sprintf (opening_p, "<s>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_FIXED) {
		opening_p += sprintf (opening_p, "<tt>");
		ending_p += sprintf (ending_p, "</tt>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT) {
		ending_p += sprintf (ending_p, "</s>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE) {
		ending_p += sprintf (ending_p, "</u>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC) {
		ending_p += sprintf (ending_p, "</i>");
	}

	if (font_style & GTK_HTML_FONT_STYLE_BOLD) {
		ending_p += sprintf (ending_p, "</b>");
	}

	if (size != 0) {
		ending_p += sprintf (ending_p, "</font size=%d>", size);
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
		len = strlen (src->text);

	dest->text = g_strndup (src->text + offset, len);
	dest->text_len = len;

	dest->font_style = src->font_style;
	dest->color = src->color;
	html_color_ref (dest->color);

#ifdef GTKHTML_HAVE_PSPELL
	dest->spell_errors = g_list_copy (src->spell_errors);
	cur = dest->spell_errors;
	while (cur) {
		SpellError *se = (SpellError *) cur->data;
		cur->data = spell_error_new (se->off, se->len);
		cur = cur->next;
	}
	dest->spell_errors = remove_spell_errors (dest->spell_errors, 0, offset);
	dest->spell_errors = remove_spell_errors (dest->spell_errors, offset+len, src->text_len - offset - len);
	move_spell_errors (dest->spell_errors, 0, -offset);
#endif
}


/* HTMLObject methods.  */

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	(* HTML_OBJECT_CLASS (parent_class)->copy) (self, dest);

	copy_helper (HTML_TEXT (self), HTML_TEXT (dest), 0, -1);
}

static gboolean
calc_size (HTMLObject *self,
	   HTMLPainter *painter)
{
	HTMLText *text;
	GtkHTMLFontStyle font_style;
	gint new_ascent, new_descent, new_width;
	gboolean changed;

	text = HTML_TEXT (self);
	font_style = html_text_get_font_style (text);

	new_ascent = html_painter_calc_ascent (painter, font_style);
	new_descent = html_painter_calc_descent (painter, font_style);
	new_width = html_painter_calc_text_width (painter, text->text, text->text_len, font_style);

	changed = FALSE;

	if (new_ascent != self->ascent) {
		self->ascent = new_ascent;
		changed = TRUE;
	}

	if (new_descent != self->descent) {
		self->descent = new_descent;
		changed = TRUE;
	}

	if (new_width != self->width) {
		self->width = new_width;
		changed = TRUE;
	}

	return changed;
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
					     font_style);
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
	if ((begin && t [0] == ' ') || (!begin && t [text->text_len - 1] == ' '))
		return 0;

	/* find end/begin of nb text */
	t = (begin) ? strchr (t, ' ') : strrchr (t, ' ');
	if (!t)
		return html_object_calc_preferred_width (HTML_OBJECT (text), painter);
	return html_painter_calc_text_width (painter, (begin) ? text->text : t + 1,
					     (begin) ? t - text->text : text->text_len - (t - text->text + 1),
					     html_text_get_font_style (text));
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

	if (text->text_len == 0 || t [0] != ' ') {
		obj = html_object_prev_not_slave (self);
		w = (obj && html_object_is_text (obj)) ? html_text_get_nb_width (HTML_TEXT (obj), painter, FALSE) : 0;
	}

	if (text->text_len)
		do {
			space = strchr (t, ' ');
			if (!space)
				space = text->text + text->text_len;
			w += html_painter_calc_text_width (painter, t, space - t, font_style);
			t = (*space) ? space + 1 : space;
			if (!(*t))
				break;
			if (w > min_width)
				min_width = w;
			w = 0;
		} while (1);

	if (text->text_len == 0 || text->text [text->text_len - 1] != ' ') {
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
	GtkHTMLFontStyle font_style;
	HTMLText *text = HTML_TEXT (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	font_style = html_text_get_font_style (text);
	html_painter_set_font_style (p, font_style);

	html_color_alloc (text->color, p);
	html_painter_set_pen (p, &text->color->color);
	html_painter_draw_text (p, o->x + tx, o->y + ty, text->text, -1);
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
      HTMLEngineSaveState *state)
{
	HTMLText *text;
	text = HTML_TEXT (self);

	if (! html_engine_save_output_string (state, "%s", text->text))
		return FALSE;
	
	return TRUE;
}




/* HTMLText methods.  */

static void
queue_draw (HTMLText *text,
	    HTMLEngine *engine,
	    guint offset,
	    guint len)
{
	html_engine_queue_draw (engine, HTML_OBJECT (text));
}

static HTMLText *
extract_text (HTMLText *text,
	      guint offset,
	      gint len)
{
	HTMLText *new;

	if (len < 0)
		len = text->text_len;

	if (offset + len > text->text_len)
		len = text->text_len - offset;

	new = g_malloc (HTML_OBJECT (text)->klass->object_size);

	(* HTML_OBJECT_CLASS (parent_class)->copy) (HTML_OBJECT (text), HTML_OBJECT (new));

	copy_helper (HTML_TEXT (text), HTML_TEXT (new), offset, len);

	return new;
}

static guint
get_length (HTMLObject *self)
{
	return HTML_TEXT (self)->text_len;
}

/* #define DEBUG_NBSP */

static void
convert_nbsp (guchar *s, guint len)
{
	/* state of automata:
	   0..Text
	   1..Sequence <space>&nbsp;...&nbsp;
	*/
	guint state=0;

#ifdef DEBUG_NBSP
	printf ("convert_nbsp: ");
#endif
	while (len) {
		if (*s == ENTITY_NBSP || *s == ' ') {
			*s = state ? ENTITY_NBSP : ' ';
			state = 1;
		} else
			state = 0;
#ifdef DEBUG_NBSP
		if (*s == ENTITY_NBSP)
			printf ("&nbsp;");
		else
			printf ("%c", *s);
#endif
		len--;
		s++;
	}
#ifdef DEBUG_NBSP
	printf ("\n");
#endif
}

#ifdef GTKHTML_HAVE_PSPELL
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

static GList *
remove_spell_errors (GList *spell_errors, guint offset, guint len)
{
	SpellError *se; 
	GList *cur, *cnext;

	cur = spell_errors;
	while (cur) { 
		cnext = cur->next;
		se = (SpellError *) cur->data; 
		if (offset <= se->off && se->off <= offset + len) {
			spell_errors = g_list_remove_link (spell_errors, cur);
			spell_error_destroy (se);
		}
 		cur = cnext;
  	} 
	return spell_errors;
}
#endif

static guint
insert_text (HTMLText *text,
	     HTMLEngine *engine,
	     guint offset,
	     const gchar *s,
	     guint len)
{
	gchar *new_buffer;
	guint old_len;
	guint new_len;

	old_len = text->text_len;
	if (offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::insert_text().");

		/* This should never happen, but the following will make sure
                   things are always fixed up in a non-segfaulting way.  */
		offset = old_len;
	}

	new_len    = old_len + len;
	new_buffer = g_malloc (new_len + 1);

	/* concatenate strings */
	memcpy (new_buffer,                text->text,        offset);
	memcpy (new_buffer + offset,       s,                 len);
	memcpy (new_buffer + offset + len, text->text+offset, old_len - offset);
	new_buffer[new_len] = '\0';

#ifdef GTKHTML_HAVE_PSPELL
	/* spell checking update */
	move_spell_errors (text->spell_errors, offset, len);
#endif
	/* do <space>&nbsp;...&nbsp; hack */
	convert_nbsp (new_buffer, new_len);

	/* set new values */
	g_free (text->text);
	text->text = new_buffer;
	text->text_len = new_len;

	/* update */
	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
	if (HTML_OBJECT (text)->parent != NULL) {
		if (! html_object_relayout (HTML_OBJECT (text)->parent,
					    engine,
					    HTML_OBJECT (text))) 
			html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);
	}

	return len;
}

static guint
remove_text (HTMLText *text,
	     HTMLEngine *engine,
	     guint offset,
	     guint len)
{
	gchar *new_buffer;
	guint old_len;
	guint new_len;

	/* The following code is very stupid and quite inefficient, but it is
           just for interactive editing so most likely people won't even
           notice.  */

	old_len = strlen (text->text);

	if (offset > old_len) {
		g_warning ("Cursor offset out of range for HTMLText::remove_text().");
		return 0;
	}

	if (offset + len > old_len || len == 0)
		len = old_len - offset;

	new_len = old_len - len;

	/* concat strings */
	new_buffer = g_malloc (new_len + 1);
	memcpy (new_buffer,          text->text,                offset);
	memcpy (new_buffer + offset, text->text + offset + len, old_len - offset - len + 1);

#ifdef GTKHTML_HAVE_PSPELL
	/* spell checking update */
	text->spell_errors = remove_spell_errors (text->spell_errors, offset, len);
	move_spell_errors (text->spell_errors, offset + len, -len);
#endif
	/* do <space>&nbsp;...&nbsp; hack */
	convert_nbsp (new_buffer, new_len);

	/* set new values */
	g_free (text->text);
	text->text = new_buffer;
	text->text_len = new_len;

	/* update */
	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
	html_object_relayout (HTML_OBJECT (text)->parent, engine, HTML_OBJECT (text));
	html_engine_queue_draw (engine, HTML_OBJECT (text)->parent);

	return len;
}

static void
get_cursor (HTMLObject *self,
	    HTMLPainter *painter,
	    guint offset,
	    gint *x1, gint *y1,
	    gint *x2, gint *y2)
{
	html_object_get_cursor_base (self, painter, offset, x2, y2);

	*x1 = *x2;
	*y1 = *y2 - self->ascent;
	*y2 += self->descent - 1;
}

static void
get_cursor_base (HTMLObject *self,
		 HTMLPainter *painter,
		 guint offset,
		 gint *x, gint *y)
{
	GtkHTMLFontStyle font_style;

	html_object_calc_abs_position (HTML_OBJECT (self), x, y);

	if (offset > 0) {
		font_style = html_text_get_font_style (HTML_TEXT (self));
		*x += html_painter_calc_text_width (painter,
						    HTML_TEXT (self)->text,
						    offset, font_style);
	}
}



#ifdef GTKHTML_HAVE_PSPELL
static void
split_spell_errors (HTMLText *self, HTMLText *new, guint offset)
{
	GList *cur;
	SpellError *se;

	cur = self->spell_errors;
	while (cur) {
		se = (SpellError *) cur->data;
		if (se->off >= offset) {
			if (cur->prev)
				cur->prev->next = NULL;
			else
				self->spell_errors = NULL;
			new->spell_errors = cur;
			move_spell_errors (new->spell_errors, offset, -offset);
			break;
		}
		cur = cur->next;
	}
}
#endif

static HTMLText *
split (HTMLText *self,
       guint offset)
{
	HTMLText *new;

	new = g_malloc (HTML_OBJECT (self)->klass->object_size);
	html_text_init (new, HTML_TEXT_CLASS (HTML_OBJECT (self)->klass),
			self->text + offset, -1, self->font_style, self->color);

	self->text = g_realloc (self->text, offset + 1);
	self->text[offset] = '\0';
	self->text_len = offset;
	html_object_change_set (HTML_OBJECT (self), HTML_CHANGE_MIN_WIDTH);

#ifdef GTKHTML_HAVE_PSPELL
	split_spell_errors (self, new, offset);
#endif

	return new;
}

static void
merge (HTMLText *text,
       HTMLText *other,
       gboolean prepend)
{
	g_warning ("HTMLText::merge not implemented.");
}

static gboolean
check_merge (HTMLText *self,
	     HTMLText *text)
{
	if (HTML_OBJECT_TYPE (self) != HTML_OBJECT_TYPE (text))
		return FALSE;
	if (self->text_len == 0 || text->text_len == 0)
		return TRUE;
	if (self->font_style != text->font_style)
		return FALSE;
	if (! html_color_equal (self->color, text->color))
		return FALSE;

	return TRUE;
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
#ifdef GTKHTML_HAVE_PSPELL
	html_text_spell_errors_clear (text);
#endif
	g_free (text->text);
	HTML_OBJECT_CLASS (parent_class)->destroy (obj);
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
	object_class->draw = draw;
	object_class->accepts_cursor = accepts_cursor;
	object_class->calc_size = calc_size;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_min_width = calc_min_width;
	object_class->get_cursor = get_cursor;
	object_class->get_cursor_base = get_cursor_base;
	object_class->save = save;
	object_class->save_plain = save_plain;
	object_class->get_length = get_length;

	/* HTMLText methods.  */

	klass->queue_draw = queue_draw;
	klass->extract_text = extract_text;
	klass->insert_text = insert_text;
	klass->remove_text = remove_text;
	klass->split = split;
	klass->get_font_style = get_font_style;
	klass->get_color = get_color;
	klass->set_font_style = set_font_style;
	klass->set_color = set_color;
	klass->merge = merge;
	klass->check_merge = check_merge;

	parent_class = &html_object_class;
}

void
html_text_init (HTMLText *text_object,
		HTMLTextClass *klass,
		const gchar *text,
		gint len,
		GtkHTMLFontStyle font_style,
		HTMLColor *color)
{
	HTMLObject *object;

	g_assert (color);

	object = HTML_OBJECT (text_object);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	if (len == -1) {
		text_object->text_len = strlen (text);
		text_object->text = g_strdup (text);
	} else {
		text_object->text_len = len;
		text_object->text = g_strndup (text, len);
	}

	/* do <space>&nbsp;...&nbsp; hack */
	convert_nbsp (text_object->text, text_object->text_len);

	text_object->font_style = font_style;
	html_color_ref (color);
	text_object->color = color;
#ifdef GTKHTML_HAVE_PSPELL
	text_object->spell_errors = NULL;
#endif
}

HTMLObject *
html_text_new_with_len (const gchar *text,
			gint len,
			GtkHTMLFontStyle font,
			HTMLColor *color)
{
	HTMLText *text_object;

	text_object = g_new (HTMLText, 1);

	html_text_init (text_object, &html_text_class, text, len, font, color);

	return HTML_OBJECT (text_object);
}

HTMLObject *
html_text_new (const gchar *text,
	       GtkHTMLFontStyle font,
	       HTMLColor *color)
{
	return html_text_new_with_len (text, -1, font, color);
}

guint
html_text_insert_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       const gchar *p,
		       guint len)
{
	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (engine != NULL, 0);
	g_return_val_if_fail (p != NULL, 0);

	if (len == 0)
		return 0;

	return (* HT_CLASS (text)->insert_text) (text, engine, offset, p, len);
}

guint
html_text_remove_text (HTMLText *text,
		       HTMLEngine *engine,
		       guint offset,
		       guint len)
{
	g_return_val_if_fail (text != NULL, 0);
	g_return_val_if_fail (engine != NULL, 0);

	return (* HT_CLASS (text)->remove_text) (text, engine, offset, len);
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

HTMLText *
html_text_extract_text (HTMLText *text,
			guint offset,
			gint len)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (offset <= text->text_len, NULL);

	return (* HT_CLASS (text)->extract_text) (text, offset, len);
}

HTMLText *
html_text_split (HTMLText *text,
		 guint offset)
{
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (offset > 0, NULL);

	return (* HT_CLASS (text)->split) (text, offset);
}

void
html_text_merge (HTMLText *text,
		 HTMLText *other,
		 gboolean prepend)
{
	g_return_if_fail (text != NULL);
	g_return_if_fail (other != NULL);
	g_return_if_fail (HTML_OBJECT_TYPE (text) == HTML_OBJECT_TYPE (other));

	return (* HT_CLASS (text)->merge) (text, other, prepend);
}

gboolean
html_text_check_merge (HTMLText *self,
		       HTMLText *text)
{
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (text != NULL, FALSE);

	return (* HT_CLASS (text)->check_merge) (self, text);
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
html_text_set_text (HTMLText *text, const gchar *new_text)
{
	g_free (text->text);
	text->text = g_strdup (new_text);
	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
}

#ifdef GTKHTML_HAVE_PSPELL

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

	/* printf ("html_text_spell_errors_clear_interval %d %d\n", offset, len); */

	while (cur) {
		cnext = cur->next;
		se    = (SpellError *) cur->data;
		/* test overlap */
		if (MAX (offset, se->off) <= MIN (se->off + se->len, offset + len)) {
			text->spell_errors = g_list_remove_link (text->spell_errors, cur);
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

#endif
