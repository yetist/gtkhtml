/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 * Copyright (C) 1997 Torben Weis (weis@kde.org)
 * Copyright (C) 1999, 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <math.h>

#define PANGO_ENABLE_BACKEND /* Required to get PANGO_GLYPH_EMPTY */

#include <pango/pango.h>

#include "htmltext.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcluealigned.h"
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlgdkpainter.h"
#include "htmlplainpainter.h"
#include "htmlprinter.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-save.h"
#include "htmlentity.h"
#include "htmlsettings.h"
#include "htmltextslave.h"
#include "htmlundo.h"

HTMLTextClass html_text_class;
static HTMLObjectClass *parent_class = NULL;
static const PangoAttrClass html_pango_attr_font_size_klass;

#define HT_CLASS(x) HTML_TEXT_CLASS (HTML_OBJECT (x)->klass)

#ifdef PANGO_GLYPH_EMPTY
#define EMPTY_GLYPH PANGO_GLYPH_EMPTY
#else
#define EMPTY_GLYPH 0
#endif

static SpellError * spell_error_new         (guint off, guint len);
static void         spell_error_destroy     (SpellError *se);
static void         move_spell_errors       (GList *spell_errors, guint offset, gint delta);
static GList *      remove_spell_errors     (GList *spell_errors, guint offset, guint len);
static GList *      merge_spell_errors      (GList *se1, GList *se2);
static void         remove_text_slaves      (HTMLObject *self);

/* void
debug_spell_errors (GList *se)
{
	for (; se; se = se->next)
		printf ("SE: %4d, %4d\n", ((SpellError *) se->data)->off, ((SpellError *) se->data)->len);
} */

static inline gboolean
is_in_the_save_cluev (HTMLObject *text,
                      HTMLObject *o)
{
	return html_object_nth_parent (o, 2) == html_object_nth_parent (text, 2);
}

/* HTMLObject methods.  */

HTMLTextPangoInfo *
html_text_pango_info_new (gint n)
{
	HTMLTextPangoInfo *pi;

	pi = g_new (HTMLTextPangoInfo, 1);
	pi->n = n;
	pi->entries = g_new0 (HTMLTextPangoInfoEntry, n);
	pi->attrs = NULL;
	pi->have_font = FALSE;
	pi->font_style = GTK_HTML_FONT_STYLE_DEFAULT;
	pi->face = NULL;

	return pi;
}

void
html_text_pango_info_destroy (HTMLTextPangoInfo *pi)
{
	gint i;

	for (i = 0; i < pi->n; i++) {
		pango_item_free (pi->entries[i].glyph_item.item);
		if (pi->entries[i].glyph_item.glyphs)
			pango_glyph_string_free (pi->entries[i].glyph_item.glyphs);
		g_free (pi->entries[i].widths);
	}
	g_free (pi->entries);
	g_free (pi->attrs);
	g_free (pi->face);
	g_free (pi);
}

static void
pango_info_destroy (HTMLText *text)
{
	if (text->pi) {
		html_text_pango_info_destroy (text->pi);
		text->pi = NULL;
	}
}

static void
free_links (GSList *list)
{
	if (list) {
		GSList *l;

		for (l = list; l; l = l->next)
			html_link_free ((Link *) l->data);
		g_slist_free (list);
	}
}

void
html_text_free_attrs (GSList *attrs)
{
	if (attrs) {
		GSList *l;

		for (l = attrs; l; l = l->next)
			pango_attribute_destroy ((PangoAttribute *) l->data);
		g_slist_free (attrs);
	}
}

static void
copy (HTMLObject *s,
      HTMLObject *d)
{
	HTMLText *src  = HTML_TEXT (s);
	HTMLText *dest = HTML_TEXT (d);
	GList *cur;
	GSList *csl;

	(* HTML_OBJECT_CLASS (parent_class)->copy) (s, d);

	dest->text = g_strdup (src->text);
	dest->text_len      = src->text_len;
	dest->text_bytes    = src->text_bytes;
	dest->font_style    = src->font_style;
	dest->face          = g_strdup (src->face);
	dest->color         = src->color;
	dest->select_start  = 0;
	dest->select_length = 0;
	dest->attr_list     = pango_attr_list_copy (src->attr_list);
	dest->extra_attr_list = src->extra_attr_list ? pango_attr_list_copy (src->extra_attr_list) : NULL;

	html_color_ref (dest->color);

	dest->spell_errors = g_list_copy (src->spell_errors);
	cur = dest->spell_errors;
	while (cur) {
		SpellError *se = (SpellError *) cur->data;
		cur->data = spell_error_new (se->off, se->len);
		cur = cur->next;
	}

	dest->links = g_slist_copy (src->links);

	for (csl = dest->links; csl; csl = csl->next)
		csl->data = html_link_dup ((Link *) csl->data);

	dest->pi = NULL;
	dest->direction = src->direction;
}

/* static void
debug_word_width (HTMLText *t)
{
	guint i;
 *
	printf ("words: %d | ", t->words);
	for (i = 0; i < t->words; i++)
		printf ("%d ", t->word_width[i]);
	printf ("\n");
}
 *
static void
word_get_position (HTMLText *text,
 *                 guint off,
 *                 guint *word_out,
 *                 guint *left_out,
 *                 guint *right_out)
{
	const gchar *s, *ls;
	guint coff, loff;
 *
	coff      = 0;
	*word_out = 0;
	s         = text->text;
	do {
		ls    = s;
		loff  = coff;
		s     = strchr (s, ' ');
		coff += s ? g_utf8_pointer_to_offset (ls, s) : g_utf8_strlen (ls, -1);
		(*word_out) ++;
		if (s)
			s++;
	} while (s && coff < off);
 *
	*left_out  = off - loff;
	*right_out = coff - off;
 *
	printf ("get position w: %d l: %d r: %d\n", *word_out, *left_out, *right_out);
} */

static gboolean
cut_attr_list_filter (PangoAttribute *attr,
                      gpointer data)
{
	PangoAttribute *range = (PangoAttribute *) data;
	gint delta;

	if (attr->start_index >= range->start_index && attr->end_index <= range->end_index)
		return TRUE;

	delta = range->end_index - range->start_index;
	if (attr->start_index > range->end_index) {
		attr->start_index -= delta;
		attr->end_index -= delta;
	} else if (attr->start_index > range->start_index) {
		attr->start_index = range->start_index;
		attr->end_index -= delta;
		if (attr->end_index <= attr->start_index)
			return TRUE;
	} else if (attr->end_index >= range->end_index)
		attr->end_index -= delta;
	else if (attr->end_index >= range->start_index)
		attr->end_index = range->start_index;

	return FALSE;
}

static void
cut_attr_list_list (PangoAttrList *attr_list,
                    gint begin_index,
                    gint end_index)
{
	PangoAttrList *removed;
	PangoAttribute range;

	range.start_index = begin_index;
	range.end_index = end_index;

	removed = pango_attr_list_filter (attr_list, cut_attr_list_filter, &range);
	if (removed)
		pango_attr_list_unref (removed);
}

static void
cut_attr_list (HTMLText *text,
               gint begin_index,
               gint end_index)
{
	cut_attr_list_list (text->attr_list, begin_index, end_index);
	if (text->extra_attr_list)
		cut_attr_list_list (text->extra_attr_list, begin_index, end_index);
}

static void
cut_links_full (HTMLText *text,
                gint start_offset,
                gint end_offset,
                gint start_index,
                gint end_index,
                gint shift_offset,
                gint shift_index)
{
	GSList *l, *next;
	Link *link;

	for (l = text->links; l; l = next) {
		next = l->next;
		link = (Link *) l->data;

		if (start_offset <= link->start_offset && link->end_offset <= end_offset) {
			html_link_free (link);
			text->links = g_slist_delete_link (text->links, l);
		} else if (end_offset <= link->start_offset) {
			link->start_offset -= shift_offset;
			link->start_index -= shift_index;
			link->end_offset -= shift_offset;
			link->end_index -= shift_index;
		} else if (start_offset <= link->start_offset)  {
			link->start_offset = end_offset - shift_offset;
			link->end_offset -= shift_offset;
			link->start_index = end_index - shift_index;
			link->end_index -= shift_index;
		} else if (end_offset <= link->end_offset) {
			if (shift_offset > 0) {
				link->end_offset -= shift_offset;
				link->end_index -= shift_index;
			} else {
				if (link->end_offset == end_offset) {
					link->end_offset = start_offset;
					link->end_index = start_index;
				} else if (link->start_offset == start_offset) {
					link->start_offset = end_offset;
					link->start_index = end_index;
				} else {
					Link *dup = html_link_dup (link);

					link->start_offset = end_offset;
					link->start_index = end_index;
					dup->end_offset = start_offset;
					dup->end_index = start_index;

					l = g_slist_prepend (l, dup);
					next = l->next->next;
				}
			}
		} else if (start_offset < link->end_offset) {
			link->end_offset = start_offset;
			link->end_index = start_index;
		}
	}
}

static void
cut_links (HTMLText *text,
           gint start_offset,
           gint end_offset,
           gint start_index,
           gint end_index)
{
	cut_links_full (text, start_offset, end_offset, start_index, end_index, end_offset - start_offset, end_index - start_index);
}

HTMLObject *
html_text_op_copy_helper (HTMLText *text,
                          GList *from,
                          GList *to,
                          guint *len)
{
	HTMLObject *rv;
	HTMLText *rvt;
	gchar *tail, *nt;
	gint begin, end, begin_index, end_index;

	begin = (from) ? GPOINTER_TO_INT (from->data) : 0;
	end   = (to)   ? GPOINTER_TO_INT (to->data)   : text->text_len;

	tail = html_text_get_text (text, end);
	begin_index = html_text_get_index (text, begin);
	end_index = tail - text->text;

	*len += end - begin;

	rv = html_object_dup (HTML_OBJECT (text));
	rvt = HTML_TEXT (rv);
	rvt->text_len = end - begin;
	rvt->text_bytes = end_index - begin_index;
	nt = g_strndup (rvt->text + begin_index, rvt->text_bytes);
	g_free (rvt->text);
	rvt->text = nt;

	rvt->spell_errors = remove_spell_errors (rvt->spell_errors, 0, begin);
	rvt->spell_errors = remove_spell_errors (rvt->spell_errors, end, text->text_len - end);

	if (end_index < text->text_bytes)
		cut_attr_list (rvt, end_index, text->text_bytes);
	if (begin_index > 0)
		cut_attr_list (rvt, 0, begin_index);
	if (end < text->text_len)
		cut_links (rvt, end, text->text_len, end_index, text->text_bytes);
	if (begin > 0)
		cut_links (rvt, 0, begin, 0, begin_index);

	return rv;
}

HTMLObject *
html_text_op_cut_helper (HTMLText *text,
                         HTMLEngine *e,
                         GList *from,
                         GList *to,
                         GList *left,
                         GList *right,
                         guint *len)
{
	HTMLObject *rv;
	HTMLText *rvt;
	gint begin, end;

	begin = (from) ? GPOINTER_TO_INT (from->data) : 0;
	end   = (to)   ? GPOINTER_TO_INT (to->data)   : text->text_len;

	g_assert (begin <= end);
	g_assert (end <= text->text_len);

	/* printf ("before cut '%s'\n", text->text);
	 * debug_word_width (text); */

	remove_text_slaves (HTML_OBJECT (text));
	if (!html_object_could_remove_whole (HTML_OBJECT (text), from, to, left, right) || begin || end < text->text_len) {
		gchar *nt, *tail;
		gint begin_index, end_index;

		if (begin == end)
			return HTML_OBJECT (html_text_new_with_len ("", 0, text->font_style, text->color));

		rv = html_object_dup (HTML_OBJECT (text));
		rvt = HTML_TEXT (rv);

		tail = html_text_get_text (text, end);
		begin_index = html_text_get_index (text, begin);
		end_index = tail - text->text;
		text->text_bytes -= tail - (text->text + begin_index);
		text->text[begin_index] = 0;
		cut_attr_list (text, begin_index, end_index);
		if (end_index < rvt->text_bytes)
			cut_attr_list (rvt, end_index, rvt->text_bytes);
		if (begin_index > 0)
			cut_attr_list (rvt, 0, begin_index);
		cut_links (text, begin, end, begin_index, end_index);
		if (end < rvt->text_len)
			cut_links (rvt, end, rvt->text_len, end_index, rvt->text_bytes);
		if (begin > 0)
			cut_links (rvt, 0, begin, 0, begin_index);
		nt = g_strconcat (text->text, tail, NULL);
		g_free (text->text);

		rvt->spell_errors = remove_spell_errors (rvt->spell_errors, 0, begin);
		rvt->spell_errors = remove_spell_errors (rvt->spell_errors, end, text->text_len - end);
		move_spell_errors (rvt->spell_errors, begin, -begin);

		text->text = nt;
		text->text_len -= end - begin;
		*len           += end - begin;

		nt = g_strndup (rvt->text + begin_index, end_index - begin_index);
		g_free (rvt->text);
		rvt->text = nt;
		rvt->text_len = end - begin;
		rvt->text_bytes = end_index - begin_index;

		text->spell_errors = remove_spell_errors (text->spell_errors, begin, end - begin);
		move_spell_errors (text->spell_errors, end, - (end - begin));

		html_text_convert_nbsp (text, TRUE);
		html_text_convert_nbsp (rvt, TRUE);
		pango_info_destroy (text);
	} else {
		text->spell_errors = remove_spell_errors (text->spell_errors, 0, text->text_len);
		html_object_move_cursor_before_remove (HTML_OBJECT (text), e);
		html_object_change_set (HTML_OBJECT (text)->parent, HTML_CHANGE_ALL_CALC);
		/* force parent redraw */
		HTML_OBJECT (text)->parent->width = 0;
		html_object_remove_child (HTML_OBJECT (text)->parent, HTML_OBJECT (text));

		rv    = HTML_OBJECT (text);
		*len += text->text_len;
	}

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL_CALC);

	/* printf ("after cut '%s'\n", text->text);
	 * debug_word_width (text); */

	return rv;
}

static HTMLObject *
op_copy (HTMLObject *self,
         HTMLObject *parent,
         HTMLEngine *e,
         GList *from,
         GList *to,
         guint *len)
{
	return html_text_op_copy_helper (HTML_TEXT (self), from, to, len);
}

static HTMLObject *
op_cut (HTMLObject *self,
        HTMLEngine *e,
        GList *from,
        GList *to,
        GList *left,
        GList *right,
        guint *len)
{
	return html_text_op_cut_helper (HTML_TEXT (self), e, from, to, left, right, len);
}

static void
merge_links (HTMLText *t1,
             HTMLText *t2)
{
	Link *tail, *head;
	GSList *l;

	if (t2->links) {
		for (l = t2->links; l; l = l->next) {
			Link *link = (Link *) l->data;

			link->start_offset += t1->text_len;
			link->start_index += t1->text_bytes;
			link->end_offset += t1->text_len;
			link->end_index += t1->text_bytes;
		}

		if (t1->links) {
			head = (Link *) t1->links->data;
			tail = (Link *) g_slist_last (t2->links)->data;

			if (tail->start_offset == head->end_offset && html_link_equal (head, tail)) {
				tail->start_offset = head->start_offset;
				tail->start_index = head->start_index;
				html_link_free (head);
				t1->links = g_slist_delete_link (t1->links, t1->links);
			}
		}

		t1->links = g_slist_concat (t2->links, t1->links);
		t2->links = NULL;
	}
}

static gboolean
object_merge (HTMLObject *self,
              HTMLObject *with,
              HTMLEngine *e,
              GList **left,
              GList **right,
              HTMLCursor *cursor)
{
	HTMLText *t1, *t2;
	gchar *to_free;

	t1 = HTML_TEXT (self);
	t2 = HTML_TEXT (with);

	/* printf ("merge '%s' '%s'\n", t1->text, t2->text); */

	/* merge_word_width (t1, t2, e->painter); */

	if (e->cursor->object == with) {
		e->cursor->object  = self;
		e->cursor->offset += t1->text_len;
	}

	/* printf ("--- before merge\n");
	 * debug_spell_errors (t1->spell_errors);
	 * printf ("---\n");
	 * debug_spell_errors (t2->spell_errors);
	 * printf ("---\n");
	*/
	move_spell_errors (t2->spell_errors, 0, t1->text_len);
	t1->spell_errors = merge_spell_errors (t1->spell_errors, t2->spell_errors);
	t2->spell_errors = NULL;

	pango_attr_list_splice (t1->attr_list, t2->attr_list, t1->text_bytes, t2->text_bytes);
	if (t2->extra_attr_list) {
		if (!t1->extra_attr_list)
			t1->extra_attr_list = pango_attr_list_new ();
		pango_attr_list_splice (t1->extra_attr_list, t2->extra_attr_list, t1->text_bytes, t2->text_bytes);
	}
	merge_links (t1, t2);

	to_free       = t1->text;
	t1->text      = g_strconcat (t1->text, t2->text, NULL);
	t1->text_len += t2->text_len;
	t1->text_bytes += t2->text_bytes;
	g_free (to_free);
	html_text_convert_nbsp (t1, TRUE);
	html_object_change_set (self, HTML_CHANGE_ALL_CALC);
	pango_info_destroy (t1);
	pango_info_destroy (t2);

	/* html_text_request_word_width (t1, e->painter); */
	/* printf ("merged '%s'\n", t1->text);
	 * printf ("--- after merge\n");
	 * debug_spell_errors (t1->spell_errors);
	 * printf ("---\n"); */

	return TRUE;
}

static gboolean
split_attrs_filter_head (PangoAttribute *attr,
                         gpointer data)
{
	gint index = GPOINTER_TO_INT (data);

	if (attr->start_index >= index)
		return TRUE;
	else if (attr->end_index > index)
		attr->end_index = index;

	return FALSE;
}

static gboolean
split_attrs_filter_tail (PangoAttribute *attr,
                         gpointer data)
{
	gint index = GPOINTER_TO_INT (data);

	if (attr->end_index <= index)
		return TRUE;

	if (attr->start_index > index)
		attr->start_index -= index;
	else
		attr->start_index = 0;
	attr->end_index -= index;

	return FALSE;
}

static void
split_attrs (HTMLText *t1,
             HTMLText *t2,
             gint index)
{
	PangoAttrList *delete;

	delete = pango_attr_list_filter (t1->attr_list, split_attrs_filter_head, GINT_TO_POINTER (index));
	if (delete)
		pango_attr_list_unref (delete);
	if (t1->extra_attr_list) {
		delete = pango_attr_list_filter (t1->extra_attr_list, split_attrs_filter_head, GINT_TO_POINTER (index));
		if (delete)
			pango_attr_list_unref (delete);
	}
	delete = pango_attr_list_filter (t2->attr_list, split_attrs_filter_tail, GINT_TO_POINTER (index));
	if (delete)
		pango_attr_list_unref (delete);
	if (t2->extra_attr_list) {
		delete = pango_attr_list_filter (t2->extra_attr_list, split_attrs_filter_tail, GINT_TO_POINTER (index));
		if (delete)
			pango_attr_list_unref (delete);
	}
}

static void
split_links (HTMLText *t1,
             HTMLText *t2,
             gint offset,
             gint index)
{
	GSList *l, *prev = NULL;

	for (l = t1->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		if (link->start_offset < offset) {
			if (link->end_offset > offset) {
				link->end_offset = offset;
				link->end_index = index;
			}

			if (prev) {
				prev->next = NULL;
				free_links (t1->links);
			}
			t1->links = l;
			break;
		}
		prev = l;

		if (!l->next) {
			free_links (t1->links);
			t1->links = NULL;
			break;
		}
	}

	prev = NULL;
	for (l = t2->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		if (link->start_offset < offset) {
			if (link->end_offset > offset) {
				link->start_offset = offset;
				link->start_index = index;
				prev = l;
				l = l->next;
			}
			if (prev) {
				prev->next = NULL;
				free_links (l);
			} else {
				free_links (t2->links);
				t2->links = NULL;
			}
			break;
		}
		prev = l;
	}

	for (l = t2->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		link->start_offset -= offset;
		link->start_index -= index;
		link->end_offset -= offset;
		link->end_index -= index;
	}
}

static void
object_split (HTMLObject *self,
              HTMLEngine *e,
              HTMLObject *child,
              gint offset,
              gint level,
              GList **left,
              GList **right)
{
	HTMLObject *dup, *prev;
	HTMLText *t1, *t2;
	gchar *tt;
	gint split_index;

	g_assert (self->parent);

	html_clue_remove_text_slaves (HTML_CLUE (self->parent));

	t1              = HTML_TEXT (self);
	dup             = html_object_dup (self);
	tt              = t1->text;
	split_index     = html_text_get_index (t1, offset);
	t1->text        = g_strndup (tt, split_index);
	t1->text_len    = offset;
	t1->text_bytes  = split_index;
	g_free (tt);
	html_text_convert_nbsp (t1, TRUE);

	t2              = HTML_TEXT (dup);
	tt              = t2->text;
	t2->text        = html_text_get_text (t2, offset);
	t2->text_len   -= offset;
	t2->text_bytes -= split_index;
	split_attrs (t1, t2, split_index);
	split_links (t1, t2, offset, split_index);
	if (!html_text_convert_nbsp (t2, FALSE))
		t2->text = g_strdup (t2->text);
	g_free (tt);

	html_clue_append_after (HTML_CLUE (self->parent), dup, self);

	prev = self->prev;
	if (t1->text_len == 0 && prev && html_object_merge (prev, self, e, NULL, NULL, NULL))
		self = prev;

	if (t2->text_len == 0 && dup->next)
		html_object_merge (dup, dup->next, e, NULL, NULL, NULL);

	/* printf ("--- before split offset %d dup len %d\n", offset, HTML_TEXT (dup)->text_len);
	 * debug_spell_errors (HTML_TEXT (self)->spell_errors); */

	HTML_TEXT (self)->spell_errors = remove_spell_errors (HTML_TEXT (self)->spell_errors,
							      offset, HTML_TEXT (dup)->text_len);
	HTML_TEXT (dup)->spell_errors  = remove_spell_errors (HTML_TEXT (dup)->spell_errors,
							      0, HTML_TEXT (self)->text_len);
	move_spell_errors   (HTML_TEXT (dup)->spell_errors, 0, - HTML_TEXT (self)->text_len);

	/* printf ("--- after split\n");
	 * printf ("left\n");
	 * debug_spell_errors (HTML_TEXT (self)->spell_errors);
	 * printf ("right\n");
	 * debug_spell_errors (HTML_TEXT (dup)->spell_errors);
	 * printf ("---\n");
	*/

	*left  = g_list_prepend (*left, self);
	*right = g_list_prepend (*right, dup);

	html_object_change_set (self, HTML_CHANGE_ALL_CALC);
	html_object_change_set (dup,  HTML_CHANGE_ALL_CALC);

	pango_info_destroy (HTML_TEXT (self));

	level--;
	if (level)
		html_object_split (self->parent, e, dup, 0, level, left, right);
}

static gboolean
html_text_real_calc_size (HTMLObject *self,
                          HTMLPainter *painter,
                          GList **changed_objs)
{
	self->width = 0;
	html_object_calc_preferred_width (self, painter);

	return FALSE;
}

static const gchar *
html_utf8_strnchr (const gchar *s,
                   gchar c,
                   gint len,
                   gint *offset)
{
	const gchar *res = NULL;

	*offset = 0;
	while (s && *s && *offset < len) {
		if (*s == c) {
			res = s;
			break;
		}
		s = g_utf8_next_char (s);
		(*offset) ++;
	}

	return res;
}

gint
html_text_text_line_length (const gchar *text,
                            gint *line_offset,
                            guint len,
                            gint *tabs)
{
	const gchar *tab, *found_tab;
	gint cl, l, skip, sum_skip;

	/* printf ("lo: %d len: %d t: '%s'\n", line_offset, len, text); */
	if (tabs)
		*tabs = 0;
	l = 0;
	sum_skip = skip = 0;
	tab = text;
	while (tab && (found_tab = html_utf8_strnchr (tab, '\t', len - l, &cl)) && l < len) {
		l   += cl;
		if (l >= len)
			break;
		if (*line_offset != -1) {
			*line_offset  += cl;
			skip = 8 - (*line_offset % 8);
		}
		tab  = found_tab + 1;

		*line_offset  += skip;
		if (*line_offset != -1)
			sum_skip += skip - 1;
		l++;
		if (tabs)
			(*tabs) ++;
	}

	if (*line_offset != -1)
		(*line_offset) += len - l;
	/* printf ("ll: %d\n", len + sum_skip); */

	return len + sum_skip;
}

static guint
get_line_length (HTMLObject *self,
                 HTMLPainter *p,
                 gint line_offset)
{
	return html_clueflow_tabs (HTML_CLUEFLOW (self->parent), p)
		? html_text_text_line_length (HTML_TEXT (self)->text, &line_offset, HTML_TEXT (self)->text_len, NULL)
		: HTML_TEXT (self)->text_len;
}

gint
html_text_get_line_offset (HTMLText *text,
                           HTMLPainter *painter,
                           gint offset)
{
	gint line_offset = -1;

	if (html_clueflow_tabs (HTML_CLUEFLOW (HTML_OBJECT (text)->parent), painter)) {
		line_offset = html_clueflow_get_line_offset (HTML_CLUEFLOW (HTML_OBJECT (text)->parent),
							     painter, HTML_OBJECT (text));
		if (offset) {
			gchar *s = text->text;

			while (offset > 0 && s && *s) {
				if (*s == '\t')
					line_offset += 8 - (line_offset % 8);
				else
					line_offset++;
				s = g_utf8_next_char (s);
				offset--;
			}
		}
	}

	return line_offset;
}

gint
html_text_get_item_index (HTMLText *text,
                          HTMLPainter *painter,
                          gint offset,
                          gint *item_offset)
{
	HTMLTextPangoInfo *pi = html_text_get_pango_info (text, painter);
	gint idx = 0;

	if (pi->n > 0) {
		while (idx < pi->n - 1 && offset >= pi->entries[idx].glyph_item.item->num_chars) {
			offset -= pi->entries[idx].glyph_item.item->num_chars;
			idx++;
		}

		*item_offset = offset;
	}

	return idx;
}

static void
update_asc_dsc (HTMLPainter *painter,
                PangoItem *item,
                gint *asc,
                gint *dsc)
{
	PangoFontMetrics *pfm;

	pfm = pango_font_get_metrics (item->analysis.font, item->analysis.language);
	if (asc)
		*asc = MAX (*asc, pango_font_metrics_get_ascent (pfm));
	if (dsc)
		*dsc = MAX (*dsc, pango_font_metrics_get_descent (pfm));
	pango_font_metrics_unref (pfm);
}

static void
html_text_get_attr_list_list (PangoAttrList *get_attrs,
                              PangoAttrList *attr_list,
                              gint start_index,
                              gint end_index)
{
	PangoAttrIterator *iter = pango_attr_list_get_iterator (attr_list);

	if (iter) {
		do {
			gint begin, end;

			pango_attr_iterator_range (iter, &begin, &end);

			if (MAX (begin, start_index) < MIN (end, end_index)) {
				GSList *c, *l = pango_attr_iterator_get_attrs (iter);

				for (c = l; c; c = c->next) {
					PangoAttribute *attr = (PangoAttribute *) c->data;

					if (attr->start_index < start_index)
						attr->start_index = 0;
					else
						attr->start_index -= start_index;

					if (attr->end_index > end_index)
						attr->end_index = end_index - start_index;
					else
						attr->end_index -= start_index;

					c->data = NULL;
					pango_attr_list_insert (get_attrs, attr);
				}
				g_slist_free (l);
			}
		} while (pango_attr_iterator_next (iter));

		pango_attr_iterator_destroy (iter);
	}
}

PangoAttrList *
html_text_get_attr_list (HTMLText *text,
                         gint start_index,
                         gint end_index)
{
	PangoAttrList *attrs = pango_attr_list_new ();

	html_text_get_attr_list_list (attrs, text->attr_list, start_index, end_index);
	if (text->extra_attr_list)
		html_text_get_attr_list_list (attrs, text->extra_attr_list, start_index, end_index);

	return attrs;
}

void
html_text_calc_text_size (HTMLText *t,
                          HTMLPainter *painter,
                          gint start_byte_offset,
                          guint len,
                          HTMLTextPangoInfo *pi,
                          GList *glyphs,
                          gint *line_offset,
                          gint *width,
                          gint *asc,
                          gint *dsc)
{
		gchar *text = t->text + start_byte_offset;

		html_painter_calc_entries_size (painter, text, len, pi, glyphs,
						line_offset, width, asc, dsc);
}

gint
html_text_calc_part_width (HTMLText *text,
                           HTMLPainter *painter,
                           gchar *start,
                           gint offset,
                           gint len,
                           gint *asc,
                           gint *dsc)
{
	gint idx, width = 0, line_offset;
	gint ascent = 0, descent = 0; /* Quiet GCC */
	gboolean need_ascent_descent = asc || dsc;
	HTMLTextPangoInfo *pi;
	PangoLanguage *language = NULL;
	PangoFont *font = NULL;
	gchar *s;

	if (offset < 0)
		return 0;

	if (offset + len > text->text_len)
		return 0;

	if (need_ascent_descent) {
		ascent = html_painter_engine_to_pango (painter,
						       html_painter_get_space_asc (painter, html_text_get_font_style (text), text->face));
		descent = html_painter_engine_to_pango (painter,
							html_painter_get_space_dsc (painter, html_text_get_font_style (text), text->face));
	}

	if (text->text_len == 0 || len == 0)
		goto out;

	line_offset = html_text_get_line_offset (text, painter, offset);

	if (start == NULL)
		start = html_text_get_text (text, offset);

	s = start;

	pi = html_text_get_pango_info (text, painter);

	idx = html_text_get_item_index (text, painter, offset, &offset);
	if (need_ascent_descent) {
		update_asc_dsc (painter, pi->entries[idx].glyph_item.item, &ascent, &descent);
		font = pi->entries[idx].glyph_item.item->analysis.font;
		language = pi->entries[idx].glyph_item.item->analysis.language;
	}
	while (len > 0) {
		gint old_idx;

		if (*s == '\t') {
			gint skip = 8 - (line_offset % 8);
			width += skip * pi->entries[idx].widths[offset];
			line_offset += skip;
		} else {
			width += pi->entries[idx].widths[offset];
			line_offset++;
		}
		len--;

		old_idx = idx;
		if (html_text_pi_forward (pi, &idx, &offset) && idx != old_idx)
			if (len > 0 && (need_ascent_descent) && (pi->entries[idx].glyph_item.item->analysis.font != font
								 || pi->entries[idx].glyph_item.item->analysis.language != language)) {
				update_asc_dsc (painter, pi->entries[idx].glyph_item.item, &ascent, &descent);
			}

		s = g_utf8_next_char (s);
	}

out:
	if (asc)
		*asc = html_painter_pango_to_engine (painter, ascent);
	if (dsc)
		*dsc = html_painter_pango_to_engine (painter, descent);

	return html_painter_pango_to_engine (painter, width);
}

static gint
calc_preferred_width (HTMLObject *self,
                      HTMLPainter *painter)
{
	HTMLText *text;
	gint width;

	text = HTML_TEXT (self);

	width = html_text_calc_part_width (text, painter, text->text, 0, text->text_len, &self->ascent, &self->descent);
	self->y = self->ascent;
	if (html_clueflow_tabs (HTML_CLUEFLOW (self->parent), painter)) {
		gint line_offset;
		gint tabs;

		line_offset = html_text_get_line_offset (text, painter, 0);
		width += (html_text_text_line_length (text->text, &line_offset, text->text_len, &tabs) - text->text_len)*
			html_painter_get_space_width (painter, html_text_get_font_style (text), text->face);
	}

	return MAX (1, width);
}

static void
remove_text_slaves (HTMLObject *self)
{
	HTMLObject *next_obj;

	/* Remove existing slaves */
	next_obj = self->next;
	while (next_obj != NULL
	       && (HTML_OBJECT_TYPE (next_obj) == HTML_TYPE_TEXTSLAVE)) {
		self->next = next_obj->next;
		html_clue_remove (HTML_CLUE (next_obj->parent), next_obj);
		html_object_destroy (next_obj);
		next_obj = self->next;
	}
}

static HTMLFitType
ht_fit_line (HTMLObject *o,
             HTMLPainter *painter,
             gboolean startOfLine,
             gboolean firstRun,
             gboolean next_to_floating,
             gint widthLeft)
{
	HTMLText *text;
	HTMLObject *text_slave;

	text = HTML_TEXT (o);

	remove_text_slaves (o);

	/* Turn all text over to our slaves */
	text_slave = html_text_slave_new (text, 0, HTML_TEXT (text)->text_len);
	html_clue_append_after (HTML_CLUE (o->parent), text_slave, o);

	return HTML_FIT_COMPLETE;
}

#if 0  /* No longer used? */
static gint
min_word_width_calc_tabs (HTMLText *text,
                          HTMLPainter *p,
                          gint idx,
                          gint *len)
{
	gchar *str, *end;
	gint rv = 0, line_offset, wt, wl, i;
	gint epos;
	gboolean tab = FALSE;

	if (!html_clueflow_tabs (HTML_CLUEFLOW (HTML_OBJECT (text)->parent), p))
		return 0;

	/* printf ("tabs %d\n", idx); */

	str = text->text;
	i = idx;
	while (i > 0 && *str) {
		if (*str == ' ')
			i--;

		str = g_utf8_next_char (str);
	}

	if (!*str)
		return 0;

	epos = 0;
	end = str;
	while (*end && *end != ' ') {
		tab |= *end == '\t';

		end = g_utf8_next_char (end);
		epos++;
	}

	if (tab) {
		line_offset = 0;

		if (idx == 0) {
			HTMLObject *prev;

			prev = html_object_prev_not_slave (HTML_OBJECT (text));
			if (prev && html_object_is_text (prev) /* FIXME-words && HTML_TEXT (prev)->words > 0 */) {
				min_word_width_calc_tabs (HTML_TEXT (prev), p, /* FIXME-words HTML_TEXT (prev)->words - 1 */ HTML_TEXT (prev)->text_len - 1, &line_offset);
				/* printf ("lo: %d\n", line_offset); */
			}
		}

		wl = html_text_text_line_length (str, &line_offset, epos, &wt);
	} else {
		wl = epos;
	}

	rv = wl - epos;

	if (len)
		*len = wl;

	/* printf ("tabs delta %d\n", rv); */
	return rv;
}
#endif

gint
html_text_pango_info_get_index (HTMLTextPangoInfo *pi,
                                gint byte_offset,
                                gint idx)
{
	while (idx < pi->n && pi->entries[idx].glyph_item.item->offset + pi->entries[idx].glyph_item.item->length <= byte_offset)
		idx++;

	return idx;
}

static void
html_text_add_cite_color (PangoAttrList *attrs,
                          HTMLText *text,
                          HTMLClueFlow *flow,
                          HTMLEngine *e)
{
	HTMLColor *cite_color = html_colorset_get_color (e->settings->color_set, HTMLCiteColor);

	if (cite_color && flow->levels->len > 0 && flow->levels->data[0] == HTML_LIST_TYPE_BLOCKQUOTE_CITE) {
		PangoAttribute *attr;

		attr = pango_attr_foreground_new (cite_color->color.red, cite_color->color.green, cite_color->color.blue);
		attr->start_index = 0;
		attr->end_index = text->text_bytes;
		pango_attr_list_change (attrs, attr);
	}
}

void
html_text_remove_unwanted_line_breaks (gchar *s,
                                       gint len,
                                       PangoLogAttr *attrs)
{
	gint i;
	gunichar last_uc = 0;

	for (i = 0; i < len; i++) {
		gunichar uc = g_utf8_get_char (s);

		if (attrs[i].is_line_break) {
			if (last_uc == '.' || last_uc == '/' ||
			    last_uc == '-' || last_uc == '$' ||
			    last_uc == '+' || last_uc == '?' ||
			    last_uc == ')' ||
			    last_uc == '}' ||
			    last_uc == ']' ||
			    last_uc == '>')
				attrs[i].is_line_break = 0;
			else if ((uc == '(' ||
				  uc == '{' ||
				  uc == '[' ||
				  uc == '<'
				  )
				 && i > 0 && !attrs[i - 1].is_white)
				attrs[i].is_line_break = 0;
		}
		s = g_utf8_next_char (s);
		last_uc = uc;
	}
}

PangoAttrList *
html_text_prepare_attrs (HTMLText *text,
                         HTMLPainter *painter)
{
	PangoAttrList *attrs;
	HTMLClueFlow *flow = NULL;
	HTMLEngine *e = NULL;
	PangoAttribute *attr;

	attrs = pango_attr_list_new ();

	if (HTML_OBJECT (text)->parent && HTML_IS_CLUEFLOW (HTML_OBJECT (text)->parent))
		flow = HTML_CLUEFLOW (HTML_OBJECT (text)->parent);

	if (painter->widget && GTK_IS_HTML (painter->widget))
		e = html_object_engine (HTML_OBJECT (text), GTK_HTML (painter->widget)->engine);

	if (flow && e) {
		html_text_add_cite_color (attrs, text, flow, e);
	}

	if (HTML_IS_PLAIN_PAINTER (painter)) {
		attr = pango_attr_family_new (painter->font_manager.fixed.face ? painter->font_manager.fixed.face : "Monospace");
		attr->start_index = 0;
		attr->end_index = text->text_bytes;
		pango_attr_list_insert (attrs, attr);
		if (painter->font_manager.fix_size != painter->font_manager.var_size || fabs (painter->font_manager.magnification - 1.0) > 0.001) {
			attr = pango_attr_size_new (painter->font_manager.fix_size * painter->font_manager.magnification);
			attr->start_index = 0;
			attr->end_index = text->text_bytes;
			pango_attr_list_insert (attrs, attr);
		}
	} else {
		if (fabs (painter->font_manager.magnification - 1.0) > 0.001) {
			attr = pango_attr_size_new (painter->font_manager.var_size * painter->font_manager.magnification);
			attr->start_index = 0;
			attr->end_index = text->text_bytes;
			pango_attr_list_insert (attrs, attr);
		}
		pango_attr_list_splice (attrs, text->attr_list, 0, 0);
	}

	if (text->extra_attr_list)
		pango_attr_list_splice (attrs, text->extra_attr_list, 0, 0);
	if (!HTML_IS_PLAIN_PAINTER (painter)) {
		if (flow && e)
			html_text_change_attrs (attrs, html_clueflow_get_default_font_style (flow), e, 0, text->text_bytes, TRUE);
	}

	if (text->links && e) {
		HTMLColor *link_color;
		GSList *l;

		for (l = text->links; l; l = l->next) {
			Link *link;

			link = (Link *) l->data;

			if (link->is_visited == FALSE)
				link_color = html_colorset_get_color (e->settings->color_set, HTMLLinkColor);
			else
				link_color = html_colorset_get_color (e->settings->color_set, HTMLVLinkColor);
			attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
			attr->start_index = link->start_index;
			attr->end_index = link->end_index;
			pango_attr_list_change (attrs, attr);

			attr = pango_attr_foreground_new (link_color->color.red, link_color->color.green, link_color->color.blue);
			attr->start_index = link->start_index;
			attr->end_index = link->end_index;
			pango_attr_list_change (attrs, attr);
		}
	}

	return attrs;
}

PangoDirection
html_text_get_pango_direction (HTMLText *text)
{
	if (HTML_OBJECT (text)->change & HTML_CHANGE_RECALC_PI)
		return pango_find_base_dir (text->text, text->text_bytes);
	else
		return text->direction;
}

static PangoDirection
get_pango_base_direction (HTMLText *text)
{
	switch (html_object_get_direction (HTML_OBJECT (text))) {
	case HTML_DIRECTION_RTL:
		return PANGO_DIRECTION_RTL;
	case HTML_DIRECTION_LTR:
		return PANGO_DIRECTION_LTR;
	case HTML_DIRECTION_DERIVED:
	default:
		if (text->text)
			return html_text_get_pango_direction (text);
		else
			return PANGO_DIRECTION_LTR;
	}
}

/**
 * pango_glyph_string_get_logical_widths:
 * @glyphs: a #PangoGlyphString
 * @text: the text corresponding to the glyphs
 * @length: the length of @text, in bytes
 * @embedding_level: the embedding level of the string
 * @logical_widths: an array whose length is g_utf8_strlen (text, length)
 *                  to be filled in with the resulting character widths.
 *
 * Given a #PangoGlyphString resulting from pango_shape() and the corresponding
 * text, determine the screen width corresponding to each character. When
 * multiple characters compose a single cluster, the width of the entire
 * cluster is divided equally among the characters.
 **/

void
html_tmp_fix_pango_glyph_string_get_logical_widths (PangoGlyphString *glyphs,
                                                    const gchar *text,
                                                    gint length,
                                                    gint embedding_level,
                                                    gint *logical_widths)
{
  gint i, j;
  gint last_cluster = 0;
  gint width = 0;
  gint last_cluster_width = 0;
  const gchar *p = text;		/* Points to start of current cluster */

  /* printf ("html_tmp_fix_pango_glyph_string_get_logical_widths"); */

  for (i = 0; i <= glyphs->num_glyphs; i++)
    {
      gint glyph_index = (embedding_level % 2 == 0) ? i : glyphs->num_glyphs - i - 1;

      /* If this glyph belongs to a new cluster, or we're at the end, find
       * the start of the next cluster, and assign the widths for this cluster.
       */
      if (i == glyphs->num_glyphs || p != text + glyphs->log_clusters[glyph_index])
	{
	  gint next_cluster = last_cluster;

	  if (i < glyphs->num_glyphs)
	    {
	      while (p < text + glyphs->log_clusters[glyph_index])
		{
		  next_cluster++;
		  p = g_utf8_next_char (p);
		}
	    }
	  else
	    {
	      while (p < text + length)
		{
		  next_cluster++;
		  p = g_utf8_next_char (p);
		}
	    }

	  for (j = last_cluster; j < next_cluster; j++) {
	    logical_widths[j] = (width - last_cluster_width) / (next_cluster - last_cluster);
	    /* printf (" %d", logical_widths [j]); */
	  }

	  if (last_cluster != next_cluster) {
		  last_cluster = next_cluster;
		  last_cluster_width = width;
	  }
	}

      if (i < glyphs->num_glyphs)
	width += glyphs->glyphs[glyph_index].geometry.width;
    }
  /* printf ("\n"); */
}

static void
html_text_shape_tab (HTMLText *text,
                     PangoGlyphString *glyphs)
{
	/* copied from pango sources */
	pango_glyph_string_set_size (glyphs, 1);

	glyphs->glyphs[0].glyph = EMPTY_GLYPH;
	glyphs->glyphs[0].geometry.x_offset = 0;
	glyphs->glyphs[0].geometry.y_offset = 0;
	glyphs->glyphs[0].attr.is_cluster_start = 1;

	glyphs->log_clusters[0] = 0;

	glyphs->glyphs[0].geometry.width = 48 * PANGO_SCALE;
}

HTMLTextPangoInfo *
html_text_get_pango_info (HTMLText *text,
                          HTMLPainter *painter)
{
	if (HTML_OBJECT (text)->change & HTML_CHANGE_RECALC_PI)	{
		pango_info_destroy (text);
		HTML_OBJECT (text)->change &= ~HTML_CHANGE_RECALC_PI;
		text->direction = pango_find_base_dir (text->text, text->text_bytes);
	}
	if (!text->pi) {
		GList *items, *cur;
		PangoAttrList *attrs;
		gint i, offset;

		attrs = html_text_prepare_attrs (text, painter);
		items = pango_itemize_with_base_dir (painter->pango_context, get_pango_base_direction (text), text->text, 0, text->text_bytes, attrs, NULL);
		pango_attr_list_unref (attrs);

		/* create pango info */
		text->pi = html_text_pango_info_new (g_list_length (items));
		text->pi->have_font = TRUE;
		text->pi->font_style = html_text_get_font_style (text);
		text->pi->face = g_strdup (text->face);
		text->pi->attrs = g_new (PangoLogAttr, text->text_len + 1);

		/* get line breaks */
		offset = 0;
		for (cur = items; cur; cur = cur->next) {
			PangoItem tmp_item;
			PangoItem *item;
			gint start_offset;

			start_offset = offset;
			item = (PangoItem *) cur->data;
			offset += item->num_chars;
			tmp_item = *item;
			while (cur->next) {
				PangoItem *next_item = (PangoItem *) cur->next->data;
				if (tmp_item.analysis.lang_engine == next_item->analysis.lang_engine) {
					tmp_item.length += next_item->length;
					tmp_item.num_chars += next_item->num_chars;
					offset += next_item->num_chars;
					cur = cur->next;
				} else
					break;
			}

			pango_break (text->text + tmp_item.offset, tmp_item.length, &tmp_item.analysis, text->pi->attrs + start_offset, tmp_item.num_chars + 1);
		}

		if (text->pi && text->pi->attrs)
			html_text_remove_unwanted_line_breaks (text->text, text->text_len, text->pi->attrs);

		for (i = 0, cur = items; i < text->pi->n; i++, cur = cur->next)
			text->pi->entries[i].glyph_item.item = (PangoItem *) cur->data;

		for (i = 0; i < text->pi->n; i++) {
			PangoItem *item;
			PangoGlyphString *glyphs;

			item = text->pi->entries[i].glyph_item.item;
			glyphs = text->pi->entries[i].glyph_item.glyphs = pango_glyph_string_new ();

			/* printf ("item pos %d len %d\n", item->offset, item->length); */

			text->pi->entries[i].widths = g_new (PangoGlyphUnit, item->num_chars);
			if (text->text[item->offset] == '\t')
				html_text_shape_tab (text, glyphs);
			else
				pango_shape (text->text + item->offset, item->length, &item->analysis, glyphs);
			html_tmp_fix_pango_glyph_string_get_logical_widths (glyphs, text->text + item->offset, item->length,
									    item->analysis.level, text->pi->entries[i].widths);
		}

		g_list_free (items);
	}
	return text->pi;
}

gboolean
html_text_pi_backward (HTMLTextPangoInfo *pi,
                       gint *ii,
                       gint *io)
{
	if (*io <= 0) {
		if (*ii <= 0)
			return FALSE;
		(*ii) --;
		*io = pi->entries [*ii].glyph_item.item->num_chars - 1;
	} else
		(*io) --;

	return TRUE;
}

gboolean
html_text_pi_forward (HTMLTextPangoInfo *pi,
                      gint *ii,
                      gint *io)
{
	if (*io >= pi->entries[*ii].glyph_item.item->num_chars - 1) {
		if (*ii >= pi->n -1)
			return FALSE;
		(*ii) ++;
		*io = 0;
	} else
		(*io) ++;

	return TRUE;
}

/**
 * html_text_tail_white_space:
 * @text: a #HTMLText object
 * @painter: a #HTMLPainter object
 * @offset: offset into the text of @text, in characters
 * @ii: index of current item
 * @io: offset within current item, in characters
 * @white_len: length of found trailing white space, in characters
 * @line_offset:
 * @s: pointer into the text of @text corresponding to @offset
 *
 * Used to chop off one character worth of whitespace at a particular position.
 *
 * Return value: width of found trailing white space, in Pango units
 **/
gint
html_text_tail_white_space (HTMLText *text,
                            HTMLPainter *painter,
                            gint offset,
                            gint ii,
                            gint io,
                            gint *white_len,
                            gint line_offset,
                            gchar *s)
{
	HTMLTextPangoInfo *pi = html_text_get_pango_info (text, painter);
	gint wl = 0;
	gint ww = 0;

	if (html_text_pi_backward (pi, &ii, &io)) {
		s = g_utf8_prev_char (s);
		offset--;
		if (pi->attrs[offset].is_white) {
			if (*s == '\t' && offset > 1) {
				gint skip = 8, co = offset - 1;

				do {
					s = g_utf8_prev_char (s);
					co--;
					if (*s != '\t')
						skip--;
				} while (s && co > 0 && *s != '\t');

				ww += skip * pi->entries[ii].widths[io];
			} else {
				ww += pi->entries[ii].widths[io];
			}
			wl++;
		}
	}

	if (white_len)
		*white_len = wl;

	return ww;
}

static void
update_mw (HTMLText *text,
           HTMLPainter *painter,
           gint offset,
           gint *last_offset,
           gint *ww,
           gint *mw,
           gint ii,
           gint io,
           gchar *s,
           gint line_offset) {
	*ww -= html_text_tail_white_space (text, painter, offset, ii, io, NULL, line_offset, s);
        if (*ww > *mw)
		*mw = *ww;
	*ww = 0;

	*last_offset = offset;
}

gboolean
html_text_is_line_break (PangoLogAttr attr)
{
	return attr.is_line_break;
}

static gint
calc_min_width (HTMLObject *self,
                HTMLPainter *painter)
{
	HTMLText *text = HTML_TEXT (self);
	HTMLTextPangoInfo *pi = html_text_get_pango_info (text, painter);
	gint mw = 0, ww;
	gint ii, io, offset, last_offset, line_offset;
	gchar *s;

	ww = 0;

	last_offset = offset = 0;
	ii = io = 0;
	line_offset = html_text_get_line_offset (text, painter, 0);
	s = text->text;
	while (offset < text->text_len) {
		if (offset > 0 && html_text_is_line_break (pi->attrs[offset]))
			update_mw (text, painter, offset, &last_offset, &ww, &mw, ii, io, s, line_offset);

		if (*s == '\t') {
			gint skip = 8 - (line_offset % 8);
			ww += skip * pi->entries[ii].widths[io];
			line_offset += skip;
		} else {
			ww += pi->entries[ii].widths[io];
			line_offset++;
		}

		s = g_utf8_next_char (s);
		offset++;

		html_text_pi_forward (pi, &ii, &io);
	}

	if (ww > mw)
		mw = ww;

	return MAX (1, html_painter_pango_to_engine (painter, mw));
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x,
      gint y,
      gint width,
      gint height,
      gint tx,
      gint ty)
{
}

static gboolean
accepts_cursor (HTMLObject *object)
{
	return TRUE;
}

static gboolean
save_open_attrs (HTMLEngineSaveState *state,
                 GSList *attrs)
{
	gboolean rv = TRUE;

	for (; attrs; attrs = attrs->next) {
		PangoAttribute *attr = (PangoAttribute *) attrs->data;
		HTMLEngine *e = state->engine;
		const gchar *tag = NULL;
		gboolean free_tag = FALSE;

		switch (attr->klass->type) {
		case PANGO_ATTR_WEIGHT:
			tag = "<B>";
			break;
		case PANGO_ATTR_STYLE:
			tag = "<I>";
			break;
		case PANGO_ATTR_UNDERLINE:
			tag = "<U>";
			break;
		case PANGO_ATTR_STRIKETHROUGH:
			tag = "<S>";
			break;
		case PANGO_ATTR_SIZE:
			if (attr->klass == &html_pango_attr_font_size_klass) {
				HTMLPangoAttrFontSize *size = (HTMLPangoAttrFontSize *) attr;
				if ((size->style & GTK_HTML_FONT_STYLE_SIZE_MASK) != GTK_HTML_FONT_STYLE_SIZE_3 && (size->style & GTK_HTML_FONT_STYLE_SIZE_MASK) != 0) {
					tag = g_strdup_printf ("<FONT SIZE=\"%d\">", size->style & GTK_HTML_FONT_STYLE_SIZE_MASK);
					free_tag = TRUE;
				}
			}
			break;
		case PANGO_ATTR_FAMILY: {
			PangoAttrString *family_attr = (PangoAttrString *) attr;

			if (!g_ascii_strcasecmp (e->painter->font_manager.fixed.face
					? e->painter->font_manager.fixed.face : "Monospace",
					family_attr->value))
				tag = "<TT>";
		}
		break;
		case PANGO_ATTR_FOREGROUND: {
			PangoAttrColor *color = (PangoAttrColor *) attr;
			tag = g_strdup_printf ("<FONT COLOR=\"#%02x%02x%02x\">",
					       (color->color.red >> 8) & 0xff, (color->color.green >> 8) & 0xff, (color->color.blue >> 8) & 0xff);
			free_tag = TRUE;
		}
			break;
		default:
			break;
		}

		if (tag) {
			if (!html_engine_save_output_string (state, "%s", tag))
				rv = FALSE;
			if (free_tag)
				g_free ((gpointer) tag);
			if (!rv)
				break;
		}
	}

	return TRUE;
}

static gboolean
save_close_attrs (HTMLEngineSaveState *state,
                  GSList *attrs)
{
	for (; attrs; attrs = attrs->next) {
		PangoAttribute *attr = (PangoAttribute *) attrs->data;
		HTMLEngine *e = state->engine;
		const gchar *tag = NULL;

		switch (attr->klass->type) {
		case PANGO_ATTR_WEIGHT:
			tag = "</B>";
			break;
		case PANGO_ATTR_STYLE:
			tag = "</I>";
			break;
		case PANGO_ATTR_UNDERLINE:
			tag = "</U>";
			break;
		case PANGO_ATTR_STRIKETHROUGH:
			tag = "</S>";
			break;
		case PANGO_ATTR_SIZE:
			if (attr->klass == &html_pango_attr_font_size_klass) {
				HTMLPangoAttrFontSize *size = (HTMLPangoAttrFontSize *) attr;
				if ((size->style & GTK_HTML_FONT_STYLE_SIZE_MASK) != GTK_HTML_FONT_STYLE_SIZE_3 && (size->style & GTK_HTML_FONT_STYLE_SIZE_MASK) != 0)
					tag = "</FONT>";
			}
			break;
		case PANGO_ATTR_FOREGROUND:
			tag = "</FONT>";
			break;
		case PANGO_ATTR_FAMILY: {
			PangoAttrString *family_attr = (PangoAttrString *) attr;

			if (!g_ascii_strcasecmp (e->painter->font_manager.fixed.face
					? e->painter->font_manager.fixed.face : "Monospace",
					family_attr->value))
				tag = "</TT>";
		}
		break;
		default:
			break;
		}

		if (tag)
			if (!html_engine_save_output_string (state, "%s", tag))
				return FALSE;
	}

	return TRUE;
}

static gboolean
save_text_part (HTMLText *text,
                HTMLEngineSaveState *state,
                guint start_index,
                guint end_index)
{
	gchar *str;
	gint len;
	gboolean rv;

	str = g_strndup (text->text + start_index, end_index - start_index);
	len = g_utf8_pointer_to_offset (text->text + start_index, text->text + end_index);

	rv = html_engine_save_encode (state, str, len);
	g_free (str);
	return rv;
}

static gboolean
save_link_open (Link *link,
                HTMLEngineSaveState *state)
{
	return html_engine_save_delims_and_vals (state,
			"<A HREF=\"", link->url,
			"\">", NULL);
}

static gboolean
save_link_close (Link *link,
                 HTMLEngineSaveState *state)
{
	return html_engine_save_output_string (state, "%s", "</A>");
}

static gboolean
save_text (HTMLText *text,
           HTMLEngineSaveState *state,
           guint start_index,
           guint end_index,
           GSList **l,
           gboolean *link_started)
{
	if (*l) {
		Link *link;

		link = (Link *) (*l)->data;

		while (*l && ((!*link_started && start_index <= link->start_index && link->start_index < end_index)
			      || (*link_started && link->end_index <= end_index))) {
			if (!*link_started && start_index <= link->start_index && link->start_index < end_index) {
				if (!save_text_part (text, state, start_index, link->start_index))
					return FALSE;
				*link_started = TRUE;
				save_link_open (link, state);
				start_index = link->start_index;
			}
			if (*link_started && link->end_index <= end_index) {
				if (!save_text_part (text, state, start_index, link->end_index))
					return FALSE;
				save_link_close (link, state);
				*link_started = FALSE;
				(*l) = (*l)->next;
				start_index = link->end_index;
				if (*l)
					link = (Link *) (*l)->data;
			}
		}

	}

	if (start_index < end_index)
		return save_text_part (text, state, start_index, end_index);

	return TRUE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	HTMLText *text = HTML_TEXT (self);
	PangoAttrIterator *iter = pango_attr_list_get_iterator (text->attr_list);

	if (iter) {
		GSList *l, *links = g_slist_reverse (g_slist_copy (text->links));
		gboolean link_started = FALSE;

		l = links;

		do {
			GSList *attrs;
			gint start_index, end_index;

			attrs = pango_attr_iterator_get_attrs (iter);
			pango_attr_iterator_range (iter,
						   &start_index,
						   &end_index);
			if (end_index > text->text_bytes)
				end_index = text->text_bytes;

			if (attrs)
				save_open_attrs (state, attrs);
			save_text (text, state, start_index, end_index, &l, &link_started);
			if (attrs) {
				attrs = g_slist_reverse (attrs);
				save_close_attrs (state, attrs);
				html_text_free_attrs (attrs);
			}
		} while (pango_attr_iterator_next (iter));

		pango_attr_iterator_destroy (iter);
		g_slist_free (links);
	}

	return TRUE;
}

static gboolean
save_plain (HTMLObject *self,
            HTMLEngineSaveState *state,
            gint requested_width)
{
	HTMLText *text;

	text = HTML_TEXT (self);

	return html_engine_save_output_string (state, "%s", text->text);
}

static guint
get_length (HTMLObject *self)
{
	return HTML_TEXT (self)->text_len;
}

/* #define DEBUG_NBSP */

struct TmpDeltaRecord
{
	gint index;		/* Byte index within original string  */
	gint delta;		/* New delta (character at index was modified,
				 * new delta applies to characters afterwards)
				 */
};

/* Called when current character is not white space or at end of string */
static gboolean
check_last_white (gint white_space,
                  gunichar last_white,
                  gint *delta_out)
{
	if (white_space > 0 && last_white == ENTITY_NBSP) {
		(*delta_out) --; /* &nbsp; => &sp; is one byte shorter in UTF-8 */
		return TRUE;
	}

	return FALSE;
}

/* Called when current character is white space */
static gboolean
check_prev_white (gint white_space,
                  gunichar last_white,
                  gint *delta_out)
{
	if (white_space > 0 && last_white == ' ') {
		(*delta_out) ++; /* &sp; => &nbsp; is one byte longer in UTF-8 */
		return TRUE;
	}

	return FALSE;
}

static GSList *
add_change (GSList *list,
            gint index,
            gint delta)
{
	struct TmpDeltaRecord *rec = g_new (struct TmpDeltaRecord, 1);

	rec->index = index;
	rec->delta = delta;

	return g_slist_prepend (list, rec);
}

/* This function does a pre-scan for the transformation in convert_nbsp,
 * which converts a sequence of N white space characters (&sp; or &nbsp;)
 * into N-1 &nbsp and 1 &sp;.
 *
 * delta_out: total change in byte length of string
 * changes_out: location to store series of records for each change in offset
 *              between the original string and the new string.
 * returns: %TRUE if any records were stored in changes_out
 */
static gboolean
is_convert_nbsp_needed (const gchar *s,
                        gint *delta_out,
                        GSList **changes_out)
{
	gunichar uc, last_white = 0;
	gboolean change;
	gint white_space;
	const gchar *p, *last_p;

	*delta_out = 0;

	last_p = NULL;		/* Quiet GCC */
	white_space = 0;
	for (p = s; *p; p = g_utf8_next_char (p)) {
		uc = g_utf8_get_char (p);

		if (uc == ENTITY_NBSP || uc == ' ') {
			change = check_prev_white (white_space, last_white, delta_out);
			white_space++;
			last_white = uc;
		} else {
			change = check_last_white (white_space, last_white, delta_out);
			white_space = 0;
		}
		if (change)
			*changes_out = add_change (*changes_out, last_p - s, *delta_out);
		last_p = p;
	}

	if (check_last_white (white_space, last_white, delta_out))
		*changes_out = add_change (*changes_out, last_p - s, *delta_out);

	*changes_out = g_slist_reverse (*changes_out);

	return *changes_out != NULL;
}

/* Called when current character is white space */
static void
write_prev_white_space (gint white_space,
                        gchar **fill)
{
	if (white_space > 0) {
#ifdef DEBUG_NBSP
		printf ("&nbsp;");
#endif
		**fill = 0xc2; (*fill) ++;
		**fill = 0xa0; (*fill) ++;
	}
}

/* Called when current character is not white space or at end of string */
static void
write_last_white_space (gint white_space,
                        gchar **fill)
{
	if (white_space > 0) {
#ifdef DEBUG_NBSP
		printf (" ");
#endif
		**fill = ' '; (*fill) ++;
	}
}

/* converts a sequence of N white space characters (&sp; or &nbsp;)
 * into N-1 &nbsp and 1 &sp;.
 */
static void
convert_nbsp (gchar *fill,
              const gchar *text)
{
	gint white_space;
	gunichar uc;
	const gchar *this_p, *p;

	p = text;
	white_space = 0;

#ifdef DEBUG_NBSP
	printf ("convert_nbsp: %s --> \"", p);
#endif

	while (*p) {
		this_p = p;
		uc = g_utf8_get_char (p);
		p = g_utf8_next_char (p);

		if (uc == ENTITY_NBSP || uc == ' ') {
			write_prev_white_space (white_space, &fill);
			white_space++;
		} else {
			write_last_white_space (white_space, &fill);
			white_space = 0;
#ifdef DEBUG_NBSP
			printf ("*");
#endif
			strncpy (fill, this_p, p - this_p);
			fill += p - this_p;
		}
	}

	write_last_white_space (white_space, &fill);
	*fill = 0;

#ifdef DEBUG_NBSP
	printf ("\"\n");
#endif
}

static void
update_index_interval (guint *start_index,
                       guint *end_index,
                       GSList *changes)
{
	GSList *c;
	gint index, delta;

	index = delta = 0;

	for (c = changes; c && *start_index > index; c = c->next) {
		struct TmpDeltaRecord *rec = c->data;

		if (*start_index > index && *start_index <= rec->index) {
			(*start_index) += delta;
			break;
		}
		index = rec->index;
		delta = rec->delta;
	}

	if (c == NULL && *start_index > index) {
		(*start_index) += delta;
		(*end_index) += delta;
		return;
	}

	for (; c && *end_index > index; c = c->next) {
		struct TmpDeltaRecord *rec = c->data;

		if (*end_index > index && *end_index <= rec->index) {
			(*end_index) += delta;
			break;
		}
		index = rec->index;
		delta = rec->delta;
	}

	if (c == NULL && *end_index > index)
		(*end_index) += delta;
}

static gboolean
update_attributes_filter (PangoAttribute *attr,
                          gpointer data)
{
	update_index_interval (&attr->start_index, &attr->end_index, (GSList *) data);

	return FALSE;
}

static void
update_attributes (PangoAttrList *attrs,
                   GSList *changes)
{
	pango_attr_list_filter (attrs, update_attributes_filter, changes);
}

static void
update_links (GSList *links,
              GSList *changes)
{
	GSList *cl;

	for (cl = links; cl; cl = cl->next) {
		Link *link = (Link *) cl->data;
		update_index_interval (&link->start_index, &link->end_index, changes);
	}
}

static void
free_changes (GSList *changes)
{
	GSList *c;

	for (c = changes; c; c = c->next)
		g_free (c->data);
	g_slist_free (changes);
}

gboolean
html_text_convert_nbsp (HTMLText *text,
                        gboolean free_text)
{
	GSList *changes = NULL;
	gint delta;

	if (is_convert_nbsp_needed (text->text, &delta, &changes)) {
		gchar *to_free;

		to_free = text->text;
		text->text = g_malloc (strlen (to_free) + delta + 1);
		text->text_bytes += delta;
		convert_nbsp (text->text, to_free);
		if (free_text)
			g_free (to_free);
		if (changes) {
			if (text->attr_list)
				update_attributes (text->attr_list, changes);
			if (text->extra_attr_list)
				update_attributes (text->extra_attr_list, changes);
			if (text->links)
				update_links (text->links, changes);
			free_changes (changes);
		}
		html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
		return TRUE;
	}
	return FALSE;
}

static void
move_spell_errors (GList *spell_errors,
                   guint offset,
                   gint delta)
{
	SpellError *se;

	if (!delta)
		return;

	while (spell_errors) {
		se = (SpellError *) spell_errors->data;
		if (se->off >= offset)
			se->off += delta;
		spell_errors = spell_errors->next;
	}
}

static GList *
remove_one (GList *list,
            GList *link)
{
	spell_error_destroy ((SpellError *) link->data);
	list = g_list_remove_link (list, link);
	g_list_free (link);

	return list;
}

static GList *
remove_spell_errors (GList *spell_errors,
                     guint offset,
                     guint len)
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

static gint
se_cmp (SpellError *a,
        SpellError *b)
{
	guint o1 = a->off;
	guint o2 = b->off;

	return (o1 < o2) ? -1 : (o1 == o2) ? 0 : 1;
}

static GList *
merge_spell_errors (GList *se1,
                    GList *se2)
{
	GList *merged = NULL;
	GList *link;

	/* Build the merge list in reverse order. */
	while (se1 != NULL && se2 != NULL) {

		/* Pop the lesser of the two list heads. */
		if (se_cmp (se1->data, se2->data) < 0) {
			link = se1;
			se1 = g_list_remove_link (se1, link);
		} else {
			link = se2;
			se2 = g_list_remove_link (se2, link);
		}

		/* Merge unique items, discard duplicates. */
		if (merged == NULL || se_cmp (link->data, merged->data) != 0)
			merged = g_list_concat (link, merged);
		else {
			spell_error_destroy (link->data);
			g_list_free (link);
		}
	}

	merged = g_list_reverse (merged);

	/* At this point at least one of the two input lists are empty,
	 * so just append them both to the end of the merge list. */

	merged = g_list_concat (merged, se1);
	merged = g_list_concat (merged, se2);

	return merged;
}

static HTMLObject *
check_point (HTMLObject *self,
             HTMLPainter *painter,
             gint x,
             gint y,
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
 * HTMLClueFlow parent.  */
static GtkHTMLFontStyle
get_font_style (const HTMLText *text)
{
	HTMLObject *parent;
	GtkHTMLFontStyle font_style;

	parent = HTML_OBJECT (text)->parent;

	if (parent && HTML_OBJECT_TYPE (parent) == HTML_TYPE_CLUEFLOW) {
		GtkHTMLFontStyle parent_style;

		parent_style = html_clueflow_get_default_font_style (HTML_CLUEFLOW (parent));
		font_style = gtk_html_font_style_merge (parent_style, text->font_style);
	} else {
		font_style = gtk_html_font_style_merge (GTK_HTML_FONT_STYLE_SIZE_3, text->font_style);
	}

	return font_style;
}

static void
set_font_style (HTMLText *text,
                HTMLEngine *engine,
                GtkHTMLFontStyle style)
{
	if (text->font_style == style)
		return;

	text->font_style = style;

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL_CALC);

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
	g_free (text->face);
	pango_info_destroy (text);
	pango_attr_list_unref (text->attr_list);
	text->attr_list = NULL;
	if (text->extra_attr_list) {
		pango_attr_list_unref (text->extra_attr_list);
		text->extra_attr_list = NULL;
	}
	free_links (text->links);
	text->links = NULL;

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
	HTMLTextPangoInfo *pi = html_text_get_pango_info (HTML_TEXT (self), engine->painter);
	gboolean changed;

	text = HTML_TEXT (self);

	if (length < 0 || length + offset > HTML_TEXT (self)->text_len)
		length = HTML_TEXT (self)->text_len - offset;

	/* extend to cursor positions */
	while (offset > 0 && !pi->attrs[offset].is_cursor_position) {
		offset--;
		length++;
	}

	while (offset + length < text->text_len && !pi->attrs[offset + length].is_cursor_position)
		length++;

	/* printf ("updated offset: %d length: %d (end offset %d)\n", offset, length, offset + length); */

	if (offset != text->select_start || length != text->select_length)
		changed = TRUE;
	else
		changed = FALSE;

	/* printf ("select range %d, %d\n", offset, length); */
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

				/* printf ("offsets diff 1: %d 2: %d\n", diff1, diff2); */
				if (diff1 != diff2) {
					html_engine_queue_draw (engine, p);
				} else {
					diff1 = offset + length - slave->posStart;
					diff2 = (text->select_start + text->select_length
						 - slave->posStart);

					/* printf ("lens diff 1: %d 2: %d\n", diff1, diff2); */
					if (diff1 != diff2)
						html_engine_queue_draw (engine, p);
				}
			} else {
				if ((!was_selected && is_selected) || (was_selected && !is_selected))
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
set_link (HTMLObject *self,
          HTMLColor *color,
          const gchar *url,
          const gchar *target)
{
	/* HTMLText *text = HTML_TEXT (self); */

	/* FIXME-link return url ? html_link_text_new_with_len (text->text, text->text_len, text->font_style, color, url, target) : NULL; */
	return NULL;
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

	p    = html_text_get_text (text, text->select_start);
	last = g_utf8_offset_to_pointer (p, text->select_length);

	/* OPTIMIZED
	last = html_text_get_text (text,
				 * text->select_start + text->select_length);
	*/
	html_engine_save_string_append_nonbsp (buffer,
					       (guchar *) p,
					       last - p);

}

static void
get_cursor (HTMLObject *self,
            HTMLPainter *painter,
            guint offset,
            gint *x1,
            gint *y1,
            gint *x2,
            gint *y2)
{
	HTMLObject *slave;
	guint ascent, descent;

	html_object_get_cursor_base (self, painter, offset, x2, y2);

	slave = self->next;
	if (slave == NULL || HTML_OBJECT_TYPE (slave) != HTML_TYPE_TEXTSLAVE) {
		ascent = self->ascent;
		descent = self->descent;
	} else {
		ascent = slave->ascent;
		descent = slave->descent;
	}

	*x1 = *x2;
	*y1 = *y2 - ascent;
	*y2 += descent - 1;
}

static void
html_text_get_cursor_base (HTMLObject *self,
                           HTMLPainter *painter,
                           guint offset,
                           gint *x,
                           gint *y)
{
	HTMLTextSlave *slave = html_text_get_slave_at_offset (HTML_TEXT (self), NULL, offset);

	/* printf ("slave: %p\n", slave); */

	if (slave)
		html_text_slave_get_cursor_base (slave, painter, offset - slave->posStart, x, y);
	else {
		g_warning ("Getting cursor base for an HTMLText with no slaves -- %p\n", (gpointer) self);
		html_object_calc_abs_position (self, x, y);
	}
}

Link *
html_text_get_link_at_offset (HTMLText *text,
                              gint offset)
{
	GSList *l;

	for (l = text->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		if (link->start_offset <= offset && offset <= link->end_offset)
			return link;
	}

	return NULL;
}

static const gchar *
get_url (HTMLObject *object,
         gint offset)
{
	Link *link = html_text_get_link_at_offset (HTML_TEXT (object), offset);

	return link ? link->url : NULL;
}

static const gchar *
get_target (HTMLObject *object,
            gint offset)
{
	Link *link = html_text_get_link_at_offset (HTML_TEXT (object), offset);

	return link ? link->target : NULL;
}

HTMLTextSlave *
html_text_get_slave_at_offset (HTMLText *text,
                               HTMLTextSlave *start,
                               gint offset)
{
	HTMLObject *obj = start ? HTML_OBJECT (start) : HTML_OBJECT (text)->next;

	while (obj && HTML_IS_TEXT_SLAVE (obj) && HTML_TEXT_SLAVE (obj)->posStart + HTML_TEXT_SLAVE (obj)->posLen < offset)
		obj = obj->next;

	if (obj && HTML_IS_TEXT_SLAVE (obj) && HTML_TEXT_SLAVE (obj)->posStart + HTML_TEXT_SLAVE (obj)->posLen >= offset)
		return HTML_TEXT_SLAVE (obj);

	return NULL;
}

static gboolean
html_text_cursor_prev_slave (HTMLObject *slave,
                             HTMLPainter *painter,
                             HTMLCursor *cursor)
{
	gint offset = cursor->offset;

	while (slave->prev && HTML_IS_TEXT_SLAVE (slave->prev)) {
		if (HTML_TEXT_SLAVE (slave->prev)->posLen) {
			if (html_text_slave_cursor_tail (HTML_TEXT_SLAVE (slave->prev), cursor, painter)) {
				cursor->position += cursor->offset - offset;
				return TRUE;
			} else
				break;
		}
		slave = slave->prev;
	}

	return FALSE;
}

static gboolean
html_text_cursor_next_slave (HTMLObject *slave,
                             HTMLPainter *painter,
                             HTMLCursor *cursor)
{
	gint offset = cursor->offset;

	while (slave->next && HTML_IS_TEXT_SLAVE (slave->next)) {
		if (HTML_TEXT_SLAVE (slave->next)->posLen) {
			if (html_text_slave_cursor_head (HTML_TEXT_SLAVE (slave->next), cursor, painter)) {
				cursor->position += cursor->offset - offset;
				return TRUE;
			} else
				break;
		}
		slave = slave->next;
	}

	return FALSE;
}

static gboolean
html_text_cursor_forward (HTMLObject *self,
                          HTMLCursor *cursor,
                          HTMLEngine *engine)
{
	HTMLText *text;
	HTMLTextPangoInfo *pi = NULL;
	gint len, attrpos = 0;
	gboolean retval = FALSE;

	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	text = HTML_TEXT (self);
	pi = html_text_get_pango_info (text, engine->painter);
	len = html_object_get_length (self);
	do {
		attrpos = cursor->offset;
		if (attrpos < len) {
			cursor->offset++;
			cursor->position++;
			retval = TRUE;
		} else {
			retval = FALSE;
			break;
		}
	} while (attrpos < len &&
		 !pi->attrs[attrpos].is_sentence_end &&
		 !pi->attrs[attrpos + 1].is_cursor_position);

	return retval;
}

static gboolean
html_cursor_allow_zero_offset (HTMLCursor *cursor,
                               HTMLObject *o)
{
	if (cursor->offset == 1) {
		HTMLObject *prev;

		prev = html_object_prev_not_slave (o);
		if (!prev || HTML_IS_CLUEALIGNED (prev))
			return TRUE;
		else {
			while (prev && !html_object_accepts_cursor (prev))
				prev = html_object_prev_not_slave (prev);

			if (!prev)
				return TRUE;
		}
	}

	return FALSE;
}

static gboolean
html_text_cursor_backward (HTMLObject *self,
                           HTMLCursor *cursor,
                           HTMLEngine *engine)
{
	HTMLText *text;
	HTMLTextPangoInfo *pi = NULL;
	gint attrpos = 0;
	gboolean retval = FALSE;

	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	text = HTML_TEXT (self);
	pi = html_text_get_pango_info (text, engine->painter);
	do {
		attrpos = cursor->offset;
		if (cursor->offset > 1 ||
		    html_cursor_allow_zero_offset (cursor, self)) {
			cursor->offset--;
			cursor->position--;
			retval = TRUE;
		} else {
			retval = FALSE;
			break;
		}
	} while (attrpos > 0 &&
		 !pi->attrs[attrpos].is_sentence_start &&
		 !pi->attrs[attrpos - 1].is_cursor_position);

	return retval;
}

static gboolean
html_text_cursor_right (HTMLObject *self,
                        HTMLPainter *painter,
                        HTMLCursor *cursor)
{
	HTMLTextSlave *slave;

	g_assert (self);
	g_assert (cursor->object == self);

	slave = html_text_get_slave_at_offset (HTML_TEXT (self), NULL, cursor->offset);

	if (slave) {
		if (html_text_slave_cursor_right (slave, painter, cursor))
			return TRUE;
		else {
			if (self->parent) {
				if (html_object_get_direction (self->parent) == HTML_DIRECTION_RTL)
					return html_text_cursor_prev_slave (HTML_OBJECT (slave), painter, cursor);
				else
					return html_text_cursor_next_slave (HTML_OBJECT (slave), painter, cursor);
			}
		}
	}

	return FALSE;
}

static gboolean
html_text_cursor_left (HTMLObject *self,
                       HTMLPainter *painter,
                       HTMLCursor *cursor)
{
	HTMLTextSlave *slave;

	g_assert (self);
	g_assert (cursor->object == self);

	slave = html_text_get_slave_at_offset (HTML_TEXT (self), NULL, cursor->offset);

	if (slave) {
		if (html_text_slave_cursor_left (slave, painter, cursor))
			return TRUE;
		else {
			if (self->parent) {
				if (html_object_get_direction (self->parent) == HTML_DIRECTION_RTL)
					return html_text_cursor_next_slave (HTML_OBJECT (slave), painter, cursor);
				else
					return html_text_cursor_prev_slave (HTML_OBJECT (slave), painter, cursor);
			}
		}
	}

	return FALSE;
}

static gboolean
html_text_backspace (HTMLObject *self,
                     HTMLCursor *cursor,
                     HTMLEngine *engine)
{
	HTMLText *text;
	HTMLTextPangoInfo *pi = NULL;
	guint attrpos = 0, prevpos;
	gboolean retval = FALSE;

	g_assert (self);
	g_assert (cursor->object == self);

	text = HTML_TEXT (self);
	pi = html_text_get_pango_info (text, engine->painter);
	prevpos = cursor->offset;
	do {
		attrpos = cursor->offset;
		if (cursor->offset > 1 ||
		    html_cursor_allow_zero_offset (cursor, self)) {
			cursor->offset--;
			cursor->position--;
			retval = TRUE;
		} else {
			if (cursor->offset == prevpos)
				retval = FALSE;
			break;
		}
	} while (attrpos > 0 && !pi->attrs[attrpos].is_cursor_position);

	if (!retval) {
		HTMLObject *prev;
		gint offset = cursor->offset;

		/* maybe no characters in this line. */
		prev = html_object_prev_cursor (cursor->object, &offset);
		cursor->offset = offset;
		if (prev) {
			if (!html_object_is_container (prev))
				cursor->offset = html_object_get_length (prev);
			cursor->object = prev;
			cursor->position--;
			retval = TRUE;
		}
	}
	if (retval) {
		if (pi->attrs[attrpos].backspace_deletes_character) {
			gchar *cluster_text = &text->text[prevpos];
			gchar *normalized_text = NULL;
			glong len;
			gint offset = cursor->offset, pos = cursor->position;

			normalized_text = g_utf8_normalize (cluster_text,
							    prevpos - attrpos,
							    G_NORMALIZE_NFD);
			len = g_utf8_strlen (normalized_text, -1);
			html_engine_delete (engine);
			if (len > 1) {
				html_engine_insert_text (engine, normalized_text,
							 g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text);
				html_cursor_jump_to (cursor, engine, self, offset);
			}
			if (normalized_text)
				g_free (normalized_text);
			/* restore a cursor position and offset for a split cursor */
			engine->cursor->offset = offset;
			engine->cursor->position = pos;
		} else {
			html_engine_delete (engine);
		}
	}

	return retval;
}

static gint
html_text_get_right_edge_offset (HTMLObject *o,
                                 HTMLPainter *painter,
                                 gint offset)
{
	HTMLTextSlave *slave = html_text_get_slave_at_offset (HTML_TEXT (o), NULL, offset);

	if (slave) {
		return html_text_slave_get_right_edge_offset (slave, painter);
	} else {
		g_warning ("getting right edge offset from text object without slave(s)");

		return HTML_TEXT (o)->text_len;
	}
}

static gint
html_text_get_left_edge_offset (HTMLObject *o,
                                HTMLPainter *painter,
                                gint offset)
{
	HTMLTextSlave *slave = html_text_get_slave_at_offset (HTML_TEXT (o), NULL, offset);

	if (slave) {
		return html_text_slave_get_left_edge_offset (slave, painter);
	} else {
		g_warning ("getting left edge offset from text object without slave(s)");

		return 0;
	}
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
	object_class->calc_size = html_text_real_calc_size;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->calc_min_width = calc_min_width;
	object_class->fit_line = ht_fit_line;
	object_class->get_cursor = get_cursor;
	object_class->get_cursor_base = html_text_get_cursor_base;
	object_class->save = save;
	object_class->save_plain = save_plain;
	object_class->check_point = check_point;
	object_class->select_range = select_range;
	object_class->get_length = get_length;
	object_class->get_line_length = get_line_length;
	object_class->set_link = set_link;
	object_class->append_selection_string = append_selection_string;
	object_class->get_url = get_url;
	object_class->get_target = get_target;
	object_class->cursor_forward = html_text_cursor_forward;
	object_class->cursor_backward = html_text_cursor_backward;
	object_class->cursor_right = html_text_cursor_right;
	object_class->cursor_left = html_text_cursor_left;
	object_class->backspace = html_text_backspace;
	object_class->get_right_edge_offset = html_text_get_right_edge_offset;
	object_class->get_left_edge_offset = html_text_get_left_edge_offset;

	/* HTMLText methods.  */

	klass->queue_draw = queue_draw;
	klass->get_font_style = get_font_style;
	klass->set_font_style = set_font_style;

	parent_class = &html_object_class;
}

/* almost identical copy of glib's _g_utf8_make_valid() */
static gchar *
_html_text_utf8_make_valid (const gchar *name,
                            gint len)
{
	GString *string;
	const gchar *remainder, *invalid;
	gint remaining_bytes, valid_bytes, total_bytes;

	g_return_val_if_fail (name != NULL, NULL);

	string = NULL;
	remainder = name;
	if (len == -1) {
		remaining_bytes = strlen (name);
	} else {
		const gchar *start = name, *end = name;

		while (len > 0) {
			gunichar uc = g_utf8_get_char_validated (end, -1);

			if (uc == (gunichar) -2 || uc == (gunichar) -1) {
				end++;
			} else if (uc == 0) {
				break;
			} else {
				end = g_utf8_next_char (end);
			}

			len--;
		}

		remaining_bytes = end - start;
	}

	total_bytes = remaining_bytes;

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid))
			break;
		valid_bytes = invalid - remainder;

		if (string == NULL)
			string = g_string_sized_new (remaining_bytes);

		g_string_append_len (string, remainder, valid_bytes);
		/* append U+FFFD REPLACEMENT CHARACTER */
		g_string_append (string, "\357\277\275");

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL)
		return g_strndup (name, total_bytes);

	g_string_append (string, remainder);

	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

/**
 * html_text_sanitize:
 * @str_in: text string to sanitize (in)
 * @str_out: newly allocated text string sanitized (out)
 * @len: length of text, in characters (in/out). (A value of
 *       -1 on input means to use all characters in @str)
 *
 * Validates a UTF-8 string up to the given number of characters.
 *
 * Return value: number of bytes in the output value of @str
 **/
gsize
html_text_sanitize (const gchar *str_in,
                    gchar **str_out,
                    gint *len)
{
	g_return_val_if_fail (str_in != NULL, 0);
	g_return_val_if_fail (str_out != NULL, 0);
	g_return_val_if_fail (len != NULL, 0);

	*str_out = _html_text_utf8_make_valid (str_in, *len);
	g_return_val_if_fail (*str_out != NULL, 0);

	*len = g_utf8_strlen (*str_out, -1);
	return strlen (*str_out);
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

	text->text_bytes = html_text_sanitize (str, &text->text, &len);
	text->text_len = len;

	text->font_style    = font_style;
	text->face          = NULL;
	text->color         = color;
	text->spell_errors  = NULL;
	text->select_start  = 0;
	text->select_length = 0;
	text->pi            = NULL;
	text->attr_list     = pango_attr_list_new ();
	text->extra_attr_list = NULL;
	text->links         = NULL;

	html_color_ref (color);
}

HTMLObject *
html_text_new_with_len (const gchar *str,
                        gint len,
                        GtkHTMLFontStyle font,
                        HTMLColor *color)
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

void
html_text_set_font_style (HTMLText *text,
                          HTMLEngine *engine,
                          GtkHTMLFontStyle style)
{
	g_return_if_fail (text != NULL);

	(* HT_CLASS (text)->set_font_style) (text, engine, style);
}

void
html_text_set_font_face (HTMLText *text,
                         HTMLFontFace *face)
{
	if (text->face)
		g_free (text->face);
	text->face = g_strdup (face);
}

void
html_text_set_text (HTMLText *text,
                    const gchar *new_text)
{
	g_free (text->text);
	text->text = NULL;
	text->text_len = -1;
	text->text_bytes = html_text_sanitize (new_text, &text->text,
					       (gint *) &text->text_len);
	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
}

/* spell checking */

#include "htmlinterval.h"

static SpellError *
spell_error_new (guint off,
                 guint len)
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
	g_list_free_full (g_steal_pointer (&text->spell_errors), g_free);
	g_list_free    (text->spell_errors);
	text->spell_errors = NULL;
}

void
html_text_spell_errors_clear_interval (HTMLText *text,
                                       HTMLInterval *i)
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

void
html_text_spell_errors_add (HTMLText *text,
                            guint off,
                            guint len)
{
	text->spell_errors = merge_spell_errors (
		text->spell_errors, g_list_prepend (
		NULL, spell_error_new (off, len)));
}

guint
html_text_get_bytes (HTMLText *text)
{
	return strlen (text->text);
}

gchar *
html_text_get_text (HTMLText *text,
                    guint offset)
{
	gchar *s = text->text;

	while (offset-- && s && *s)
		s = g_utf8_next_char (s);

	return s;
}

guint
html_text_get_index (HTMLText *text,
                     guint offset)
{
	return html_text_get_text (text, offset) - text->text;
}

gunichar
html_text_get_char (HTMLText *text,
                    guint offset)
{
	gunichar uc;

	uc = g_utf8_get_char (html_text_get_text (text, offset));
	return uc;
}

/* magic links */

struct _HTMLMagicInsertMatch
{
	const gchar *regex;
	regex_t *preg;
	const gchar *prefix;
};

typedef struct _HTMLMagicInsertMatch HTMLMagicInsertMatch;

static HTMLMagicInsertMatch mim[] = {
	/* prefixed expressions */
	{ "(news|telnet|nntp|file|http|ftp|sftp|https|webcal)://([-a-z0-9]+(:[-a-z0-9]+)?@)?[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(([.])?/[-a-z0-9_$.+!*(),;:@%&=?/~#']*[^]'.}>\\) ,?!;:\"]?)?", NULL, NULL },
	{ "(sip|h323|callto):([-_a-z0-9.'\\+]+(:[0-9]{1,5})?(/[-_a-z0-9.']+)?)(@([-_a-z0-9.%=?]+|([0-9]{1,3}.){3}[0-9]{1,3})?)?(:[0-9]{1,5})?", NULL, NULL },
	{ "mailto:[-_a-z0-9.'\\+]+@[-_a-z0-9.%=?]+", NULL, NULL },
	/* not prefixed expression */
	{ "www\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(([.])?/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]'.}>\\) ,?!;:\"]?)?", NULL, "http://" },
	{ "ftp\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(([.])?/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]'.}>\\) ,?!;:\"]?)?", NULL, "ftp://" },
	{ "[-_a-z0-9.'\\+]+@[-_a-z0-9.%=?]+", NULL, "mailto:" }
};

void
html_engine_init_magic_links (void)
{
	gint i;

	for (i = 0; i < G_N_ELEMENTS (mim); i++) {
		mim[i].preg = g_new0 (regex_t, 1);
		if (regcomp (mim[i].preg, mim[i].regex, REG_EXTENDED | REG_ICASE)) {
			/* error */
			g_free (mim[i].preg);
			mim[i].preg = 0;
		}
	}
}

static void
paste_link (HTMLEngine *engine,
            HTMLText *text,
            gint so,
            gint eo,
            const gchar *prefix)
{
	gchar *href;
	gchar *base;

	base = g_strndup (html_text_get_text (text, so), html_text_get_index (text, eo) - html_text_get_index (text, so));
	href = (prefix) ? g_strconcat (prefix, base, NULL) : g_strdup (base);
	g_free (base);

	html_text_add_link (text, engine, href, NULL, so, eo);
	g_free (href);
}

gboolean
html_text_magic_link (HTMLText *text,
                      HTMLEngine *engine,
                      guint offset)
{
	regmatch_t pmatch[2];
	gint i;
	gboolean rv = FALSE, exec = TRUE;
	gint saved_position;
	gunichar uc;
	gchar *str, *cur;

	if (!offset)
		return FALSE;
	offset--;

	/* printf ("html_text_magic_link\n"); */

	html_undo_level_begin (engine->undo, "Magic link", "Remove magic link");
	saved_position = engine->cursor->position;

	cur = str = html_text_get_text (text, offset);

	/* check forward to ensure chars are < 0x80, could be removed once we have utf8 regex */
	while (cur && *cur) {
		cur = g_utf8_next_char (cur);
		if (!*cur)
			break;
		uc = g_utf8_get_char (cur);
		if (uc >= 0x80) {
			exec = FALSE;
			break;
		} else if (uc == ' ' || uc == ENTITY_NBSP) {
			break;
		}
	}

	uc = g_utf8_get_char (str);
	if (uc >= 0x80)
		exec = FALSE;
	while (exec && uc != ' ' && uc != ENTITY_NBSP && offset) {
		str = g_utf8_prev_char (str);
		uc = g_utf8_get_char (str);
		if (uc >= 0x80)
			exec = FALSE;
		offset--;
	}

	if (uc == ' ' || uc == ENTITY_NBSP)
		str = g_utf8_next_char (str);

	if (exec) {
		gboolean done = FALSE;
		guint32 str_offset = 0, str_length = strlen (str);

		while (!done) {
			done = TRUE;
			for (i = 0; i < G_N_ELEMENTS (mim); i++) {
				if (mim[i].preg && !regexec (mim[i].preg, str + str_offset, 2, pmatch, 0)) {
					paste_link (engine, text,
						    g_utf8_pointer_to_offset (text->text, str + str_offset + pmatch[0].rm_so),
						    g_utf8_pointer_to_offset (text->text, str + str_offset + pmatch[0].rm_eo), mim[i].prefix);
						rv = TRUE;
						str_offset += pmatch[0].rm_eo + 1;
						done = str_offset >= str_length;
						break;
				}
			}
		}
	}

	html_undo_level_end (engine->undo, engine);
	html_cursor_jump_to_position_no_spell (engine->cursor, engine, saved_position);

	return rv;
}

/*
 * magic links end
 */

gint
html_text_trail_space_width (HTMLText *text,
                             HTMLPainter *painter)
{
	return text->text_len > 0 && html_text_get_char (text, text->text_len - 1) == ' '
		? html_painter_get_space_width (painter, html_text_get_font_style (text), text->face) : 0;
}

void
html_text_append (HTMLText *text,
                  const gchar *pstr,
                  gint len)
{
	gchar *to_delete, *str = NULL;
	guint bytes;

	to_delete       = text->text;
	bytes = html_text_sanitize (pstr, &str, &len);
	text->text_len += len;
	text->text      = g_malloc (text->text_bytes + bytes + 1);

	memcpy (text->text, to_delete, text->text_bytes);
	memcpy (text->text + text->text_bytes, str, bytes);
	text->text_bytes += bytes;
	text->text[text->text_bytes] = '\0';

	g_free (to_delete);
	g_free (str);

	html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_ALL);
}

void
html_text_append_link_full (HTMLText *text,
                            gchar *url,
                            gchar *target,
                            gint start_index,
                            gint end_index,
                            gint start_offset,
                            gint end_offset)
{
	text->links = g_slist_prepend (text->links, html_link_new (url, target, start_index, end_index, start_offset, end_offset, FALSE));
}

static void
html_text_offsets_to_indexes (HTMLText *text,
                              gint so,
                              gint eo,
                              gint *si,
                              gint *ei)
{
	*si = html_text_get_index (text, so);
	*ei = g_utf8_offset_to_pointer (text->text + *si, eo - so) - text->text;
}

void
html_text_append_link (HTMLText *text,
                       gchar *url,
                       gchar *target,
                       gint start_offset,
                       gint end_offset)
{
	gint start_index, end_index;

	html_text_offsets_to_indexes (text, start_offset, end_offset, &start_index, &end_index);
	html_text_append_link_full (text, url, target, start_index, end_index, start_offset, end_offset);
}

void
html_text_add_link_full (HTMLText *text,
                         HTMLEngine *e,
                         gchar *url,
                         gchar *target,
                         gint start_index,
                         gint end_index,
                         gint start_offset,
                         gint end_offset)
{
	GSList *l, *prev = NULL;
	Link *link;

	cut_links_full (text, start_offset, end_offset, start_index, end_index, 0, 0);

	if (text->links == NULL)
		html_text_append_link_full (text, url, target, start_index, end_index, start_offset, end_offset);
	else {
		Link *plink = NULL, *new_link = html_link_new (url, target, start_index, end_index, start_offset, end_offset, FALSE);

		for (l = text->links; new_link && l; l = l->next) {
			link = (Link *) l->data;
			if (new_link->start_offset >= link->end_offset) {
				if (new_link->start_offset == link->end_offset && html_link_equal (link, new_link)) {
					link->end_offset = end_offset;
					link->end_index = end_index;
					html_link_free (new_link);
					new_link = NULL;
				} else {
					text->links = g_slist_insert_before (text->links, l, new_link);
					link = new_link;
					new_link = NULL;
				}
				if (plink && html_link_equal (plink, link) && plink->start_offset == link->end_offset) {
					plink->start_offset = link->start_offset;
					plink->start_index = link->start_index;
					text->links = g_slist_remove (text->links, link);
					html_link_free (link);
					link = plink;
				}
				plink = link;
				prev = l;
			}
		}

		if (new_link && prev) {
			if (prev->next)
				text->links = g_slist_insert_before (text->links, prev->next, new_link);
			else
				text->links = g_slist_append (text->links, new_link);
		} else if (new_link)
			text->links = g_slist_prepend (text->links, new_link);
	}

	HTML_OBJECT (text)->change |= HTML_CHANGE_RECALC_PI;
}

void
html_text_add_link (HTMLText *text,
                    HTMLEngine *e,
                    gchar *url,
                    gchar *target,
                    gint start_offset,
                    gint end_offset)
{
	gint start_index, end_index;

	html_text_offsets_to_indexes (text, start_offset, end_offset, &start_index, &end_index);
	html_text_add_link_full (text, e, url, target, start_index, end_index, start_offset, end_offset);
}

void
html_text_remove_links (HTMLText *text)
{
	if (text->links) {
		free_links (text->links);
		text->links = NULL;
		html_object_change_set (HTML_OBJECT (text), HTML_CHANGE_RECALC_PI);
	}
}

/* HTMLTextSlave * */
/* html_text_get_slave_at_offset (HTMLObject *o, gint offset) */
/* { */
/*	if (!o || (!HTML_IS_TEXT (o) && !HTML_IS_TEXT_SLAVE (o))) */
/*		return NULL; */

/*	if (HTML_IS_TEXT (o)) */
/*		o = o->next; */

/*	while (o && HTML_IS_TEXT_SLAVE (o)) { */
/*		if (HTML_IS_TEXT_SLAVE (o) && HTML_TEXT_SLAVE (o)->posStart <= offset */
/*		    && (offset < HTML_TEXT_SLAVE (o)->posStart + HTML_TEXT_SLAVE (o)->posLen */
/*			|| (offset == HTML_TEXT_SLAVE (o)->posStart + HTML_TEXT_SLAVE (o)->posLen && HTML_TEXT_SLAVE (o)->owner->text_len == offset))) */
/*			return HTML_TEXT_SLAVE (o); */
/*		o = o->next; */
/*	} */

/*	return NULL; */
/* } */

Link *
html_text_get_link_slaves_at_offset (HTMLText *text,
                                     gint offset,
                                     HTMLTextSlave **start,
                                     HTMLTextSlave **end)
{
	Link *link = html_text_get_link_at_offset (text, offset);

	if (link) {
		*start = html_text_get_slave_at_offset (text, NULL, link->start_offset);
		*end = html_text_get_slave_at_offset (text, *start, link->end_offset);

		if (*start && *end)
			return link;
	}

	return NULL;
}

gboolean
html_text_get_link_rectangle (HTMLText *text,
                              HTMLPainter *painter,
                              gint offset,
                              gint *x1,
                              gint *y1,
                              gint *x2,
                              gint *y2)
{
	HTMLTextSlave *start;
	HTMLTextSlave *end;
	Link *link;

	link = html_text_get_link_slaves_at_offset (text, offset, &start, &end);
	if (link) {
		gint xs, ys, xe, ye;

		html_object_calc_abs_position (HTML_OBJECT (start), &xs, &ys);
		xs += html_text_calc_part_width (text, painter, html_text_slave_get_text (start), start->posStart, link->start_offset - start->posStart, NULL, NULL);
		ys -= HTML_OBJECT (start)->ascent;

		html_object_calc_abs_position (HTML_OBJECT (end), &xe, &ye);
		xe += HTML_OBJECT (end)->width;
		xe -= html_text_calc_part_width (text, painter, text->text + link->end_index, link->end_offset, end->posStart + start->posLen - link->end_offset, NULL, NULL);
		ye += HTML_OBJECT (end)->descent;

		*x1 = MIN (xs, xe);
		*y1 = MIN (ys, ye);
		*x2 = MAX (xs, xe);
		*y2 = MAX (ys, ye);

		return TRUE;
	}

	return FALSE;
}

gboolean
html_text_prev_link_offset (HTMLText *text,
                            gint *offset)
{
	GSList *l;

	for (l = text->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		if (link->start_offset <= *offset && *offset <= link->end_offset) {
			if (l->next) {
				*offset = ((Link *) l->next->data)->end_offset - 1;
				return TRUE;
			}
			break;
		}
	}

	return FALSE;
}

gboolean
html_text_next_link_offset (HTMLText *text,
                            gint *offset)
{
	GSList *l, *prev = NULL;

	for (l = text->links; l; l = l->next) {
		Link *link = (Link *) l->data;

		if (link->start_offset <= *offset && *offset <= link->end_offset) {
			if (prev) {
				*offset = ((Link *) prev->data)->start_offset + 1;
				return TRUE;
			}
			break;
		}
		prev = l;
	}

	return FALSE;
}

gboolean
html_text_first_link_offset (HTMLText *text,
                             gint *offset)
{
	if (text->links)
		*offset = ((Link *) g_slist_last (text->links)->data)->start_offset + 1;

	return text->links != NULL;
}

gboolean
html_text_last_link_offset (HTMLText *text,
                            gint *offset)
{
	if (text->links)
		*offset = ((Link *) text->links->data)->end_offset - 1;

	return text->links != NULL;
}

gchar *
html_text_get_link_text (HTMLText *text,
                         gint offset)
{
	Link *link = html_text_get_link_at_offset (text, offset);
	gchar *start;

	start = html_text_get_text (text, link->start_offset);

	return g_strndup (start, g_utf8_offset_to_pointer (start, link->end_offset - link->start_offset) - start);
}

void
html_link_set_url_and_target (Link *link,
                              gchar *url,
                              gchar *target)
{
	if (!link)
		return;

	g_free (link->url);
	g_free (link->target);

	link->url = g_strdup (url);
	link->target = g_strdup (target);
}

Link *
html_link_dup (Link *l)
{
	Link *nl = g_new (Link, 1);

	nl->url = g_strdup (l->url);
	nl->target = g_strdup (l->target);
	nl->start_offset = l->start_offset;
	nl->end_offset = l->end_offset;
	nl->start_index = l->start_index;
	nl->end_index = l->end_index;
	nl->is_visited = l->is_visited;

	return nl;
}

void
html_link_free (Link *link)
{
	g_return_if_fail (link != NULL);

	g_free (link->url);
	g_free (link->target);
	g_free (link);
}

gboolean
html_link_equal (Link *l1,
                 Link *l2)
{
	return l1->url && l2->url && !g_ascii_strcasecmp (l1->url, l2->url)
		&& (l1->target == l2->target || (l1->target && l2->target && !g_ascii_strcasecmp (l1->target, l2->target)));
}

Link *
html_link_new (gchar *url,
               gchar *target,
               guint start_index,
               guint end_index,
               gint start_offset,
               gint end_offset,
               gboolean is_visited)
{
	Link *link = g_new0 (Link, 1);

	link->url = g_strdup (url);
	link->target = g_strdup (target);
	link->start_offset = start_offset;
	link->end_offset = end_offset;
	link->start_index = start_index;
	link->end_index = end_index;
	link->is_visited = is_visited;

	return link;
}

/* extended pango attributes */

static PangoAttribute *
html_pango_attr_font_size_copy (const PangoAttribute *attr)
{
	HTMLPangoAttrFontSize *font_size_attr = (HTMLPangoAttrFontSize *) attr, *new_attr;

	new_attr = (HTMLPangoAttrFontSize *) html_pango_attr_font_size_new (font_size_attr->style);
	new_attr->attr_int.value = font_size_attr->attr_int.value;

	return (PangoAttribute *) new_attr;
}

static void
html_pango_attr_font_size_destroy (PangoAttribute *attr)
{
	g_free (attr);
}

static gboolean
html_pango_attr_font_size_equal (const PangoAttribute *attr1,
                                 const PangoAttribute *attr2)
{
	const HTMLPangoAttrFontSize *font_size_attr1 = (const HTMLPangoAttrFontSize *) attr1;
	const HTMLPangoAttrFontSize *font_size_attr2 = (const HTMLPangoAttrFontSize *) attr2;

	return (font_size_attr1->style == font_size_attr2->style);
}

void
html_pango_attr_font_size_calc (HTMLPangoAttrFontSize *attr,
                                HTMLEngine *e)
{
	gint size, base_size, real_size;

	base_size = (attr->style & GTK_HTML_FONT_STYLE_FIXED) ? e->painter->font_manager.fix_size : e->painter->font_manager.var_size;
	if ((attr->style & GTK_HTML_FONT_STYLE_SIZE_MASK) != 0)
		size = (attr->style & GTK_HTML_FONT_STYLE_SIZE_MASK) - GTK_HTML_FONT_STYLE_SIZE_3;
	else
		size = 0;
	real_size = e->painter->font_manager.magnification * ((gdouble) base_size + (size > 0 ? (1 << size) : size) * base_size / 8.0);

	attr->attr_int.value = real_size;
}

static const PangoAttrClass html_pango_attr_font_size_klass = {
	PANGO_ATTR_SIZE,
	html_pango_attr_font_size_copy,
	html_pango_attr_font_size_destroy,
	html_pango_attr_font_size_equal
};

PangoAttribute *
html_pango_attr_font_size_new (GtkHTMLFontStyle style)
{
	HTMLPangoAttrFontSize *result = g_new (HTMLPangoAttrFontSize, 1);
	result->attr_int.attr.klass = &html_pango_attr_font_size_klass;
	result->style = style;

	return (PangoAttribute *) result;
}

static gboolean
calc_font_size_filter (PangoAttribute *attr,
                       gpointer data)
{
	HTMLEngine *e = HTML_ENGINE (data);

	if (attr->klass->type == PANGO_ATTR_SIZE)
		html_pango_attr_font_size_calc ((HTMLPangoAttrFontSize *) attr, e);
	else if (attr->klass->type == PANGO_ATTR_FAMILY) {
		/* FIXME: this is not very nice. we set it here as it's only used when fonts changed.
		 * once family in style is used again, that code must be updated */
		PangoAttrString *sa = (PangoAttrString *) attr;
		g_free (sa->value);
		sa->value = g_strdup (e->painter->font_manager.fixed.face ? e->painter->font_manager.fixed.face : "Monospace");
	}

	return FALSE;
}

void
html_text_calc_font_size (HTMLText *text,
                          HTMLEngine *e)
{
	pango_attr_list_filter (text->attr_list, calc_font_size_filter, e);
}

static GtkHTMLFontStyle
style_from_attrs (PangoAttrIterator *iter)
{
	GtkHTMLFontStyle style = GTK_HTML_FONT_STYLE_DEFAULT;
	GSList *list, *l;

	list = pango_attr_iterator_get_attrs (iter);
	for (l = list; l; l = l->next) {
		PangoAttribute *attr = (PangoAttribute *) l->data;

		switch (attr->klass->type) {
		case PANGO_ATTR_WEIGHT:
			style |= GTK_HTML_FONT_STYLE_BOLD;
			break;
		case PANGO_ATTR_UNDERLINE:
			style |= GTK_HTML_FONT_STYLE_UNDERLINE;
			break;
		case PANGO_ATTR_STRIKETHROUGH:
			style |= GTK_HTML_FONT_STYLE_STRIKEOUT;
			break;
		case PANGO_ATTR_STYLE:
			style |= GTK_HTML_FONT_STYLE_ITALIC;
			break;
		case PANGO_ATTR_SIZE:
			style |= ((HTMLPangoAttrFontSize *) attr)->style;
			break;
		case PANGO_ATTR_FAMILY:
			style |= GTK_HTML_FONT_STYLE_FIXED;
			break;
		default:
			break;
		}
	}

	html_text_free_attrs (list);

	return style;
}

GtkHTMLFontStyle
html_text_get_fontstyle_at_index (HTMLText *text,
                                  gint index)
{
	GtkHTMLFontStyle style = GTK_HTML_FONT_STYLE_DEFAULT;
	PangoAttrIterator *iter = pango_attr_list_get_iterator (text->attr_list);

	if (iter) {
		do {
			gint start_index, end_index;

			pango_attr_iterator_range (iter, &start_index, &end_index);
			if (start_index <= index && index <= end_index) {
				style |= style_from_attrs (iter);
				break;
			}
		} while (pango_attr_iterator_next (iter));

		pango_attr_iterator_destroy (iter);
	}

	return style;
}

GtkHTMLFontStyle
html_text_get_style_conflicts (HTMLText *text,
                               GtkHTMLFontStyle style,
                               gint start_index,
                               gint end_index)
{
	GtkHTMLFontStyle conflicts = GTK_HTML_FONT_STYLE_DEFAULT;
	PangoAttrIterator *iter = pango_attr_list_get_iterator (text->attr_list);

	if (iter) {
		do {
			gint iter_start_index, iter_end_index;

			pango_attr_iterator_range (iter, &iter_start_index, &iter_end_index);
			if (MAX (start_index, iter_start_index)  < MIN (end_index, iter_end_index))
				conflicts |= style_from_attrs (iter) ^ style;
			if (iter_start_index > end_index)
				break;
		} while (pango_attr_iterator_next (iter));

		pango_attr_iterator_destroy (iter);
	}

	return conflicts;
}

void
html_text_change_attrs (PangoAttrList *attr_list,
                        GtkHTMLFontStyle style,
                        HTMLEngine *e,
                        gint start_index,
                        gint end_index,
                        gboolean avoid_default_size)
{
	PangoAttribute *attr;

	/* style */
	if (style & GTK_HTML_FONT_STYLE_BOLD) {
		attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}

	if (style & GTK_HTML_FONT_STYLE_ITALIC) {
		attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}

	if (style & GTK_HTML_FONT_STYLE_UNDERLINE) {
		attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}

	if (style & GTK_HTML_FONT_STYLE_STRIKEOUT) {
		attr = pango_attr_strikethrough_new (TRUE);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}

	if (style & GTK_HTML_FONT_STYLE_FIXED) {
		attr = pango_attr_family_new (e->painter->font_manager.fixed.face ? e->painter->font_manager.fixed.face : "Monospace");
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}

	if (!avoid_default_size
	    || (((style & GTK_HTML_FONT_STYLE_SIZE_MASK) != GTK_HTML_FONT_STYLE_DEFAULT)
		&& ((style & GTK_HTML_FONT_STYLE_SIZE_MASK) != GTK_HTML_FONT_STYLE_SIZE_3))
	    || ((style & GTK_HTML_FONT_STYLE_FIXED) &&
		e->painter->font_manager.fix_size != e->painter->font_manager.var_size)) {
		attr = html_pango_attr_font_size_new (style);
		html_pango_attr_font_size_calc ((HTMLPangoAttrFontSize *) attr, e);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_change (attr_list, attr);
	}
}

void
html_text_set_style_in_range (HTMLText *text,
                              GtkHTMLFontStyle style,
                              HTMLEngine *e,
                              gint start_index,
                              gint end_index)
{
	html_text_change_attrs (text->attr_list, style, e, start_index, end_index, TRUE);
}

void
html_text_set_style (HTMLText *text,
                     GtkHTMLFontStyle style,
                     HTMLEngine *e)
{
	html_text_set_style_in_range (text, style, e, 0, text->text_bytes);
}

static gboolean
unset_style_filter (PangoAttribute *attr,
                    gpointer data)
{
	GtkHTMLFontStyle style = GPOINTER_TO_INT (data);

	switch (attr->klass->type) {
	case PANGO_ATTR_WEIGHT:
		if (style & GTK_HTML_FONT_STYLE_BOLD)
			return TRUE;
		break;
	case PANGO_ATTR_STYLE:
		if (style & GTK_HTML_FONT_STYLE_ITALIC)
			return TRUE;
		break;
	case PANGO_ATTR_UNDERLINE:
		if (style & GTK_HTML_FONT_STYLE_UNDERLINE)
			return TRUE;
		break;
	case PANGO_ATTR_STRIKETHROUGH:
		if (style & GTK_HTML_FONT_STYLE_STRIKEOUT)
			return TRUE;
		break;
	case PANGO_ATTR_SIZE:
		if (((HTMLPangoAttrFontSize *) attr)->style & style)
			return TRUE;
		break;
	case PANGO_ATTR_FAMILY:
		if (style & GTK_HTML_FONT_STYLE_FIXED)
			return TRUE;
		break;
	default:
		break;
	}

	return FALSE;
}

void
html_text_unset_style (HTMLText *text,
                       GtkHTMLFontStyle style)
{
	pango_attr_list_filter (text->attr_list, unset_style_filter, GINT_TO_POINTER (style));
}

static HTMLColor *
color_from_attrs (PangoAttrIterator *iter)
{
	HTMLColor *color = NULL;
	GSList *list, *l;

	list = pango_attr_iterator_get_attrs (iter);
	for (l = list; l; l = l->next) {
		PangoAttribute *attr = (PangoAttribute *) l->data;
		PangoAttrColor *ca;

		switch (attr->klass->type) {
		case PANGO_ATTR_FOREGROUND:
			ca = (PangoAttrColor *) attr;
			color = html_color_new_from_rgb (ca->color.red, ca->color.green, ca->color.blue);
			break;
		default:
			break;
		}
	}

	html_text_free_attrs (list);

	return color;
}

static HTMLColor *
html_text_get_first_color_in_range (HTMLText *text,
                                    HTMLEngine *e,
                                    gint start_index,
                                    gint end_index)
{
	HTMLColor *color = NULL;
	PangoAttrIterator *iter = pango_attr_list_get_iterator (text->attr_list);

	if (iter) {
		do {
			gint iter_start_index, iter_end_index;

			pango_attr_iterator_range (iter, &iter_start_index, &iter_end_index);
			if (MAX (iter_start_index, start_index) <= MIN (iter_end_index, end_index)) {
				color = color_from_attrs (iter);
				break;
			}
		} while (pango_attr_iterator_next (iter));

		pango_attr_iterator_destroy (iter);
	}

	if (!color) {
		color = html_colorset_get_color (e->settings->color_set, HTMLTextColor);
		html_color_ref (color);
	}

	return color;
}

HTMLColor *
html_text_get_color_at_index (HTMLText *text,
                              HTMLEngine *e,
                              gint index)
{
	return html_text_get_first_color_in_range (text, e, index, index);
}

HTMLColor *
html_text_get_color (HTMLText *text,
                     HTMLEngine *e,
                     gint start_index)
{
	return html_text_get_first_color_in_range (text, e, start_index, text->text_bytes);
}

void
html_text_set_color_in_range (HTMLText *text,
                              HTMLColor *color,
                              gint start_index,
                              gint end_index)
{
	PangoAttribute *attr = pango_attr_foreground_new (color->color.red, color->color.green, color->color.blue);

	attr->start_index = start_index;
	attr->end_index = end_index;
	pango_attr_list_change (text->attr_list, attr);
}

void
html_text_set_color (HTMLText *text,
                     HTMLColor *color)
{
	html_text_set_color_in_range (text, color, 0, text->text_bytes);
}

HTMLDirection
html_text_direction_pango_to_html (PangoDirection pdir)
{
	switch (pdir) {
	case PANGO_DIRECTION_RTL:
		return HTML_DIRECTION_RTL;
	case PANGO_DIRECTION_LTR:
		return HTML_DIRECTION_LTR;
	default:
		return HTML_DIRECTION_DERIVED;
	}
}

void
html_text_change_set (HTMLText *text,
                      HTMLChangeFlags flags)
{
	HTMLObject *slave = HTML_OBJECT (text)->next;

	for (; slave && HTML_IS_TEXT_SLAVE (slave) && HTML_TEXT_SLAVE (slave)->owner == text; slave = slave->next)
		slave->change |= flags;

	html_object_change_set (HTML_OBJECT (text), flags);
}

void
html_text_set_link_visited (HTMLText *text,
                            gint offset,
                            HTMLEngine *engine,
                            gboolean is_visited)
{
	HTMLEngine *object_engine = html_object_engine (HTML_OBJECT (text),engine);
	Link *link = html_text_get_link_at_offset (text,offset);

	if (link) {
		link->is_visited  = is_visited;
		html_text_change_set (text, HTML_CHANGE_RECALC_PI);
		html_text_queue_draw (text, object_engine, offset, 1);
		html_engine_flush_draw_queue (object_engine);
	}
}
