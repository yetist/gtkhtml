/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000, 2001 Helix Code, Inc.
    Authors:                 Radek Doulik (rodo@helixcode.com)

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
#include <string.h>

#include "gtkhtmldebug.h"
#include "gtkhtml-private.h"
#include "gtkhtml-properties.h"

#include "htmlclue.h"
#include "htmlcursor.h"
#include "htmlcolorset.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-selection-updater.h"
#include "htmlinterval.h"
#include "htmllinktext.h"
#include "htmlobject.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmltext.h"
#include "htmlundo.h"
#include "htmlundo-action.h"

static void        delete_object (HTMLEngine *e, HTMLObject **ret_object, guint *ret_len, HTMLUndoDirection dir);
static void        insert_object (HTMLEngine *e, HTMLObject *obj, guint len, HTMLUndoDirection dir);
static HTMLObject *get_tail_leaf (HTMLObject *o);
static HTMLObject *get_head_leaf (HTMLObject *o);

/* helper functions -- need refactor */

static void
get_tree_bounds_for_merge (HTMLObject *obj, GList **first, GList **last)
{
	HTMLObject *cur;

	* first = *last = NULL;

	cur = obj;
	while (cur) {
		*first = g_list_append (*first, cur);
		cur = html_object_head (cur);
	}

	cur = obj;
	while (cur) {
		*last = g_list_append (*last, cur);
		cur = html_object_tail_not_slave (cur);
	}
}

static void
html_cursor_get_left (HTMLCursor *cursor, HTMLObject **obj, gint *off)
{
	if (cursor->offset == 0) {
		*obj = html_object_prev_not_slave (cursor->object);
		if (*obj) {
			*off = html_object_get_length (*obj);
			return;
		}
	}
	*obj = cursor->object;
	*off = cursor->offset;
}

static void
html_cursor_get_right (HTMLCursor *cursor, HTMLObject **obj, gint *off)
{
	if (cursor->offset >= html_object_get_length (cursor->object)) {
		*obj = html_object_next_not_slave (cursor->object);
		if (*obj) {
			*off = 0;
		}
		return;
	}
	*obj = cursor->object;
	*off = cursor->offset;
}

static void
html_point_get_left (HTMLPoint *source, HTMLPoint *dest, gboolean allow_null)
{
	if (source->offset == 0) {
		dest->object = html_object_prev_not_slave (source->object);
		if (dest->object) {
			dest->offset = html_object_get_length (dest->object);
			return;
		}
		if (allow_null && html_object_get_length (source->object))
			return;
	}

	*dest = *source;
}

static void
html_point_get_right (HTMLPoint *source, HTMLPoint *dest, gboolean allow_null)
{
	if (source->offset >= html_object_get_length (source->object)) {
		dest->object = html_object_next_not_slave (source->object);
		if (dest->object) {
			dest->offset = 0;
			return;
		}
		if (allow_null && html_object_get_length (source->object))
			return;
	}

	*dest = *source;
}

static GList *
get_parent_list (HTMLPoint *point, gint level, gboolean include_offset)
{
	GList *list;
	HTMLObject *o;

	list = include_offset ? g_list_prepend (NULL, GINT_TO_POINTER (point->offset)) : NULL;
	o    = point->object;

	while (level > 0 && o) {
		list = g_list_prepend (list, o);
		o = o->parent;
		level--;
	}

	return list;
}

static gint
get_parent_level (HTMLObject *from, HTMLObject *to)
{
	gint level = 1;

	while (from && to && from != to) {
		from = from->parent;
		to   = to->parent;
		level++;
	}

	return level;
}

static void
prepare_delete_bounds (HTMLEngine *e, GList **from_list, GList **to_list,
		       GList **bound_left, GList **bound_right)
{
	HTMLPoint b_left, b_right, begin, end;
	gint level;

	g_assert (e->selection);

	html_point_get_right (&e->selection->from, &begin, FALSE);
	html_point_get_left  (&e->selection->to,   &end,   FALSE);

	level = get_parent_level (begin.object, end.object);

	*from_list = get_parent_list (&begin, level, TRUE);
	*to_list   = get_parent_list (&end,   level, TRUE);

	if (bound_left && bound_right) {
		html_point_get_left  (&e->selection->from, &b_left,  TRUE);
		html_point_get_right (&e->selection->to,   &b_right, TRUE);

		level = get_parent_level (b_left.object, b_right.object);

		*bound_left  = b_left.object ? get_parent_list (&b_left, level - 1, FALSE) : NULL;
		*bound_right = b_right.object ? get_parent_list (&b_right, level - 1, FALSE) : NULL;
	}
}

static void
remove_empty_and_merge (HTMLEngine *e, gboolean merge, GList *left, GList *right, HTMLCursor *c)
{
	HTMLObject *lo, *ro, *prev;
	gint len;

	while (left && left->data && right && right->data) {

		lo  = HTML_OBJECT (left->data);
		ro  = HTML_OBJECT (right->data);
		len = html_object_get_length (lo);

		if ((lo->prev || merge) &&
		    html_object_is_text (lo) && !*HTML_TEXT (lo)->text) {
			HTMLObject *nlo = lo->prev;
			if (e->cursor->object == lo)
				e->cursor->object = ro;
			if (c && c->object == lo)
				c->object = ro;
			html_object_remove_child (lo->parent, lo);
			html_object_destroy (lo);
			lo = nlo;
		} else if ((ro->next || merge) &&
			   html_object_is_text (ro) && !*HTML_TEXT (ro)->text) {
			HTMLObject *nro = ro->next;
			html_object_remove_child (ro->parent, ro);
			html_object_destroy (ro);
			ro = nro;
		}

		if (merge && lo && ro) {
			if (!html_object_merge (lo, ro))
				break;
			if (ro == e->cursor->object) {
				e->cursor->object  = lo;
				e->cursor->offset += len;
			}
		}
		left  = left->next;
		right = right->next;
	}

	prev = html_object_prev_not_slave (e->cursor->object);
	if (prev && e->cursor->offset == 0) {
		e->cursor->object = prev;
		e->cursor->offset = html_object_get_length (e->cursor->object);
	}
}

static void
split_and_add_empty_texts (HTMLEngine *e, gint level, GList **left, GList **right)
{
	html_object_split (e->cursor->object, e, NULL, e->cursor->offset, level, left, right);
}

/* end of helper */

void
html_engine_copy (HTMLEngine *e)
{
	GList *from, *to;

	if (html_engine_is_selection_active (e)) {
		html_engine_freeze (e);
		prepare_delete_bounds (e, &from, &to, NULL, NULL);
		e->clipboard_len = 0;
		e->clipboard     = html_object_op_copy (HTML_OBJECT (from->data), e, from->next, to->next,
							&e->clipboard_len);
		html_engine_thaw (e);
	}
}

struct _DeleteUndo {
	HTMLUndoData data;

	HTMLObject *buffer;
	guint       buffer_len;
};
typedef struct _DeleteUndo DeleteUndo;

static void
delete_undo_destroy (HTMLUndoData *data)
{
	DeleteUndo *undo = (DeleteUndo *) data;

	if (undo->buffer)
		html_object_destroy (undo->buffer);
}

static void
delete_undo_action (HTMLEngine *e, HTMLUndoData *data, HTMLUndoDirection dir)
{
	DeleteUndo *undo;

	undo = (DeleteUndo *) data;
	insert_object (e, undo->buffer, undo->buffer_len, html_undo_direction_reverse (dir));
	undo->buffer = NULL;
}

static void
delete_setup_undo (HTMLEngine *e, HTMLObject *buffer, guint len, HTMLUndoDirection dir)
{
	DeleteUndo *undo;

	undo = g_new (DeleteUndo, 1);

	html_undo_data_init (HTML_UNDO_DATA (undo));
	undo->data.destroy = delete_undo_destroy;
	undo->buffer       = buffer;
	undo->buffer_len   = len;

	html_undo_add_action (e->undo,
			      html_undo_action_new ("Delete", delete_undo_action,
						    HTML_UNDO_DATA (undo), html_cursor_get_position (e->cursor)),
			      dir);
}

static void
move_cursor_before_delete (HTMLEngine *e)
{
	if (e->cursor->offset == 0) {
		if (html_object_prev_not_slave (e->cursor->object)) {
			HTMLObject *obj;
			gint off;

			html_cursor_get_left (e->cursor, &obj, &off);
			if (obj) {
				e->cursor->object = obj;
				e->cursor->offset = off;
			}
		} else {
			HTMLObject *obj;
			gint off;

			html_cursor_get_right (e->mark, &obj, &off);
			if (obj) {
				e->cursor->object = obj;
				e->cursor->offset = 0;
			}
		}
	}
}

static void
place_cursor_before_mark (HTMLEngine *e)
{
	if (e->mark->position < e->cursor->position) {
		HTMLCursor *tmp;

		tmp = e->cursor;
		e->cursor = e->mark;
		e->mark = tmp;
	}
}

static void
delete_object_do (HTMLEngine *e, HTMLObject **object, guint *len)
{
	GList *from, *to, *left, *right;

	if (html_engine_is_selection_active (e)) {
		html_engine_freeze (e);
		prepare_delete_bounds (e, &from, &to, &left, &right);
		place_cursor_before_mark (e);
		move_cursor_before_delete (e);
		html_engine_disable_selection (e);
		*len     = 0;
		*object  = html_object_op_cut  (HTML_OBJECT (from->data), e, from->next, to->next, len);
		remove_empty_and_merge (e, TRUE, left, right, NULL);
		html_engine_spell_check_range (e, e->cursor, e->cursor);
		html_engine_thaw (e);
	}
}

static void
delete_object (HTMLEngine *e, HTMLObject **ret_object, guint *ret_len, HTMLUndoDirection dir)
{
	html_engine_edit_selection_updater_update_now (e->selection_updater);
	if (html_engine_is_selection_active (e)) {
		HTMLObject *object;
		guint len;

		/* if (e->mark->position < e->cursor->position) {
			HTMLCursor *tmp;

			tmp = e->cursor;
			e->cursor = e->mark;
			e->mark = tmp;
		}

		{
			HTMLObject *obj;
			gint off;

			html_cursor_get_left (e->cursor, &obj, &off);
			if (obj) {
				e->cursor->object = obj;
				e->cursor->offset = off;
			}
		}
		if (!e->cursor->object || (e->cursor->object->prev == NULL && e->cursor->offset == 0)) {
			e->cursor->object = e->mark->object;
			e->cursor->offset = 0;
			} */

		delete_object_do (e, &object, &len);
		if (ret_object && ret_len) {
			*ret_object = html_object_op_copy (object, e, NULL, NULL, ret_len);
			*ret_len    = len;
		}
		delete_setup_undo (e, object, len, dir);
	}
}

void
html_engine_delete (HTMLEngine *e)
{
	delete_object (e, NULL, NULL, HTML_UNDO_UNDO);
}

void
html_engine_cut (HTMLEngine *e)
{
	html_engine_clipboard_clear (e);
	delete_object (e, &e->clipboard, &e->clipboard_len, HTML_UNDO_UNDO);
}

/*
 * PASTE/INSERT
 */

static HTMLObject *
get_tail_leaf (HTMLObject *o)
{
	HTMLObject *tail, *rv = o;

	do {
		tail = html_object_tail_not_slave (rv);
		if (tail)
			rv = tail;
	} while (tail);

	return rv;
}

static HTMLObject *
get_head_leaf (HTMLObject *o)
{
	HTMLObject *head, *rv = o;

	do {
		head = html_object_head (rv);
		if (head)
			rv = head;
	} while (head);

	return rv;
}

static void
insert_object_do (HTMLEngine *e, HTMLObject *obj, guint len)
{
	HTMLObject *cur;
	HTMLCursor *orig;
	GList *left = NULL, *right = NULL;
	GList *first = NULL, *last = NULL;
	gint level;

	html_engine_freeze (e);
	orig = html_cursor_dup (e->cursor);

	/* FIXME for tables */
	level = 0;
	cur   = get_head_leaf (obj);
	while (cur) {
		level++;
		cur = cur->parent;
	}

	split_and_add_empty_texts (e, level, &left, &right);
        get_tree_bounds_for_merge (obj, &first, &last);

	e->cursor->position += len;
	e->cursor->object    = get_tail_leaf (obj);
	e->cursor->offset    = html_object_get_length (e->cursor->object);

	if ((left && left->data) || (right && (right->data))) {
		HTMLObject *parent, *where;
		if (left && left->data) {
			where  = HTML_OBJECT (left->data);
			parent = where->parent;
		} else {
			where  = NULL;
			parent = HTML_OBJECT (right->data)->parent;
		}
		if (parent)
			html_clue_append_after (HTML_CLUE (parent), obj, where);
	}

	remove_empty_and_merge (e, TRUE, last, right, orig);
	remove_empty_and_merge (e, TRUE, left, first, orig);

        html_engine_spell_check_range (e, orig, e->cursor);
	html_cursor_destroy (orig);
	html_engine_thaw (e);
}

struct _InsertUndo {
	HTMLUndoData data;

	guint len;
};
typedef struct _InsertUndo InsertUndo;

static void
insert_undo_action (HTMLEngine *e, HTMLUndoData *data, HTMLUndoDirection dir)
{
	InsertUndo *undo;

	undo = (InsertUndo *) data;

	html_engine_set_mark (e);
	html_engine_move_cursor (e, HTML_ENGINE_CURSOR_LEFT, undo->len);
	delete_object (e, NULL, NULL, html_undo_direction_reverse (dir));
}

static void
insert_setup_undo (HTMLEngine *e, guint len, HTMLUndoDirection dir)
{
	InsertUndo *undo;

	undo = g_new (InsertUndo, 1);

	html_undo_data_init (HTML_UNDO_DATA (undo));
	undo->len = len;

	html_undo_add_action (e->undo,
			      html_undo_action_new ("Insert", insert_undo_action,
						    HTML_UNDO_DATA (undo), html_cursor_get_position (e->cursor)),
			      dir);
}

static void
insert_object (HTMLEngine *e, HTMLObject *obj, guint len, HTMLUndoDirection dir)
{
	insert_object_do (e, obj, len);
	insert_setup_undo (e, len, dir);
}

void
html_engine_insert_object (HTMLEngine *e, HTMLObject *o, guint len)
{
	insert_object (e, o, len, HTML_UNDO_UNDO);
}

void
html_engine_paste_object (HTMLEngine *e, HTMLObject *o, guint len)
{
	html_engine_delete (e);
	html_engine_insert_object (e, o, len);
}

void
html_engine_paste (HTMLEngine *e)
{
	if (e->clipboard) {
		HTMLObject *copy;
		guint len = 0;

		copy = html_object_op_copy (e->clipboard, e, NULL, NULL, &len);
		html_engine_paste_object (e, copy, e->clipboard_len);
	}
}

static void
check_magic_link (HTMLEngine *e, const gchar *text, guint len)
{
	if (GTK_HTML_PROPERTY (e->widget, magic_links) && len == 1
	    && (*text == ' ' || text [0] == '\n' || text [0] == '>' || text [0] == ')'))
		html_text_magic_link (HTML_TEXT (e->cursor->object), e, html_object_get_length (e->cursor->object));
}

void
html_engine_insert_empty_paragraph (HTMLEngine *e)
{
	GList *left=NULL, *right=NULL;
	HTMLCursor *orig;

	html_engine_freeze (e);
	orig = html_cursor_dup (e->cursor);
	split_and_add_empty_texts (e, 2, &left, &right);
	remove_empty_and_merge (e, FALSE, left, right, orig);
	html_cursor_forward (e->cursor, e);
	insert_setup_undo (e, 1, HTML_UNDO_UNDO);
	g_list_free (left);
	g_list_free (right);
	html_engine_spell_check_range (e, orig, e->cursor);
	html_cursor_destroy (orig);

	html_cursor_backward (e->cursor, e);
	check_magic_link (e, "\n", 1);
	html_cursor_forward (e->cursor, e);

	html_engine_thaw (e);

	gtk_html_editor_event_command (e->widget, GTK_HTML_COMMAND_INSERT_PARAGRAPH);
}

void
html_engine_insert_text (HTMLEngine *e, const gchar *text, guint len)
{
	gchar *nl;
	gint alen;

	if (len == -1)
		len = strlen (text);
	if (!len)
		return;

	do {
		nl   = unicode_strchr (text, '\n');
		alen = nl ? unicode_index_to_offset (text, nl - text) : len;
		if (alen) {
			check_magic_link (e, text, alen);
			html_engine_insert_object (e, html_engine_new_text (e, text, alen), alen);
		}
		if (nl) {
			html_engine_insert_empty_paragraph (e);
			len -= unicode_index_to_offset (text, nl - text) + 1;
			text = nl + 1;
		}
	} while (nl);
}

void
html_engine_paste_text (HTMLEngine *e, const gchar *text, guint len)
{
	html_engine_delete (e);
	html_engine_insert_text (e, text, len);
}

void
html_engine_delete_n (HTMLEngine *e, guint len, gboolean forward)
{
	if (html_engine_is_selection_active (e))
		html_engine_cut (e);
	else {
		html_engine_set_mark (e);
		while (len != 0) {
			if (forward)
				html_cursor_forward (e->cursor, e);
			else
				html_cursor_backward (e->cursor, e);
			len --;
		}
		html_engine_delete (e);
	}
}

void
html_engine_cut_line (HTMLEngine *e)
{
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	html_engine_set_mark (e);
	html_engine_end_of_line (e);

	if (e->cursor->position == e->mark->position)
		html_cursor_forward (e->cursor, e);

	html_engine_cut (e);
}

typedef struct {
	HTMLColor   *color;
	const gchar *url;
	const gchar *target;
} HTMLEngineLinkInsertData;

static void
change_link (HTMLObject *o, HTMLEngine *e, gpointer data)
{
	HTMLObject *changed;
	HTMLEngineLinkInsertData *d = (HTMLEngineLinkInsertData *) data;

	changed = d->url ? html_object_set_link (o, d->color, d->url, d->target) : html_object_remove_link (o, d->color);
	if (changed) {
		if (o->parent) {
			HTMLObject *prev;

			prev = o->prev;
			g_assert (HTML_OBJECT_TYPE (o->parent) == HTML_TYPE_CLUEFLOW);

			html_clue_append_after (HTML_CLUE (o->parent), changed, o);
			html_clue_remove (HTML_CLUE (o->parent), o);
			html_object_destroy (o);
			if (changed->prev)
				html_object_merge (changed->prev, changed);
		} else {
			html_object_destroy (e->clipboard);
			e->clipboard     = changed;
			e->clipboard_len = html_object_get_length (changed);
		}
	}
}

void
html_engine_insert_link (HTMLEngine *e, const gchar *url, const gchar *target)
{
	if (html_engine_is_selection_active (e)) {
		HTMLEngineLinkInsertData data;

		data.url    = url;
		data.target = target;
		data.color  = url
			? html_colorset_get_color (e->settings->color_set, HTMLLinkColor)
			: html_colorset_get_color (e->settings->color_set, HTMLTextColor);
		html_engine_cut_and_paste (e, url ? "Insert link" : "Remove link", change_link, &data);
	} else {
		html_engine_set_url    (e, url);
		html_engine_set_target (e, target);
	}
}
