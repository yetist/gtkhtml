/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Helix Code, Inc.

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

#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmltext.h"
#include "htmltextmaster.h"
#include "htmllinktextmaster.h"

#include "htmlengine-edit.h"
#include "htmlengine-edit-cursor.h"
#include "htmlengine-edit-delete.h"
#include "htmlengine-edit-copy.h"
#include "htmlengine-edit-paste.h"
#include "htmlengine-cutbuffer.h"
#include "htmlengine-edit-movement.h"

#include "htmlengine-edit-insert.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlengine-edit-paste.h"


/* Paragraph insertion.  */

/* FIXME this sucks.  */
static HTMLObject *
get_flow (HTMLObject *object)
{
	HTMLObject *p;

	for (p = object->parent; p != NULL; p = p->parent) {
		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_CLUEFLOW)
			break;
	}

	return p;
}

static HTMLObject *
split_object (HTMLObject *object,
	      guint offset,
	      HTMLEngine *e)
{
	if (html_object_is_text (object))
		return HTML_OBJECT (html_text_split (HTML_TEXT (object), offset));

	return html_text_master_new ("", GTK_HTML_FONT_STYLE_DEFAULT, e->insertion_color);
}

static void
insert_para (HTMLEngine *engine)
{
	HTMLObject *flow;
	HTMLObject *next_flow;
	HTMLObject *current;
	guint offset;

	current = engine->cursor->object;
	offset = engine->cursor->offset;

	flow = get_flow (current);
	if (flow == NULL) {
		g_warning ("%s: Object %p (%s) is not contained in an HTMLClueFlow\n",
			   __FUNCTION__, current, html_type_name (HTML_OBJECT_TYPE (current)));
		return;
	}

	html_engine_hide_cursor (engine);

	if (html_object_is_text (current))
		html_text_master_destroy_slaves (HTML_TEXT_MASTER (current));

	if (offset > 0) {
		if (current->next != NULL) {
			next_flow = HTML_OBJECT (html_clueflow_split (HTML_CLUEFLOW (flow),
								      current->next));
		} else {
			/* FIXME we need a `html_clueflow_like_another_one()'.  */
			next_flow = html_clueflow_new (HTML_CLUEFLOW (flow)->style,
						       HTML_CLUEFLOW (flow)->level);
		}
	} else {
		next_flow = HTML_OBJECT (html_clueflow_split (HTML_CLUEFLOW (flow), current));
		if (current->prev == NULL) {
			HTMLObject *empty_text_element;
			GtkHTMLFontStyle font_style;
			HTMLColor *color;

			if (html_object_is_text (current)) {
				font_style = HTML_TEXT (current)->font_style;
				color = HTML_TEXT (current)->color;
			} else {
				font_style = GTK_HTML_FONT_STYLE_DEFAULT;
				color = engine->insertion_color;
			}

			empty_text_element = html_text_master_new ("", font_style, color);
			html_clue_append (HTML_CLUE (flow), empty_text_element);
		}
	}

	html_clue_append_after (HTML_CLUE (flow->parent), next_flow, flow);

	if (offset > 0) {
		HTMLObject *second_half;

		second_half = split_object (current, offset, engine);
		html_clue_prepend (HTML_CLUE (next_flow), second_half);

		engine->cursor->object = second_half;
		engine->cursor->offset = 0;
		engine->cursor->have_target_x = FALSE;
	}

	if (flow->parent == NULL) {
		html_object_relayout (flow, engine, NULL);
		html_engine_queue_draw (engine, flow);
	} else {
		html_object_calc_size (next_flow, engine->painter);
		html_object_relayout (flow->parent, engine, flow);
		html_engine_queue_draw (engine, flow->parent);
	}

	engine->cursor->position++;

	html_engine_show_cursor (engine);
}


static guint
insert_chars_different_style (HTMLEngine *e,
			      const gchar *text,
			      guint len,
			      GtkHTMLFontStyle style,
			      HTMLColor *color)
{
	HTMLText *right_side;
	HTMLObject *new;
	HTMLObject *curr;
	guint offset;
	guint retval;

	curr = e->cursor->object;
	offset = e->cursor->offset;

	if (offset == 0) {
		HTMLObject *p;

		/* If we are at the beginning of the element, we might
                   have an element on the left with the same
                   properties.  Look for it.  */

		p = html_object_prev_not_slave (curr);

		if (p != NULL
		    && HTML_OBJECT_TYPE (p) == HTML_OBJECT_TYPE (curr)
		    && HTML_TEXT (p)->font_style == style
		    && html_color_equal (HTML_TEXT (p)->color, color)) {
			e->cursor->object = p;
			e->cursor->offset = HTML_TEXT (p)->text_len;
			return html_text_insert_text (HTML_TEXT (p), e, e->cursor->offset, text, len);
		}
	}

	new = html_text_master_new ("", style, color);
	retval = html_text_insert_text (HTML_TEXT (new), e, 0, text, len);
	if (retval == 0) {
		html_object_destroy (new);
		return 0;
	}

	if (offset > 0) {
		right_side = html_text_split (HTML_TEXT (curr), offset);
		if (right_side != NULL)
			html_clue_append_after (HTML_CLUE (curr->parent), HTML_OBJECT (right_side), curr);
		html_clue_append_after (HTML_CLUE (curr->parent), new, curr);
	} else if (curr->prev != NULL) {
		html_clue_append_after (HTML_CLUE (curr->parent), new, curr->prev);
	} else {
		html_clue_prepend (HTML_CLUE (curr->parent), new);
	}

	html_engine_queue_draw (e, new);

	html_object_relayout (curr->parent, e, curr);

	e->cursor->object = new;
	e->cursor->offset = 0;

	return retval;
}

static guint
insert_chars_same_style (HTMLEngine *e,
			 const gchar *text,
			 guint len)
{
	HTMLObject *curr;
	guint offset;

	curr = e->cursor->object;
	offset = e->cursor->offset;

	return html_text_insert_text (HTML_TEXT (curr), e, offset, text, len);
}

static guint
insert_chars_at_not_text (HTMLEngine *e,
			  const gchar *text,
			  guint len,
			  GtkHTMLFontStyle style,
			  HTMLColor *color)
{
	HTMLObject *curr;
	HTMLObject *new_text;

	curr = e->cursor->object;

	new_text = html_text_master_new_with_len (text, len, style, color);

	if (e->cursor->offset == 0) {
		if (curr->prev == NULL)
			html_clue_prepend (HTML_CLUE (curr->parent), new_text);
		else
			html_clue_append_after (HTML_CLUE (curr->parent), new_text, curr->prev);
	} else {
		html_clue_append_after (HTML_CLUE (curr->parent), new_text, curr);
	}

	html_engine_queue_draw (e, new_text);
	html_object_relayout (curr->parent, e, new_text);

	e->cursor->object = new_text;
	e->cursor->offset = 0;

	return HTML_TEXT (new_text)->text_len;
}

static guint
insert_chars (HTMLEngine *e,
	      const gchar *text,
	      guint len,
	      GtkHTMLFontStyle style,
	      HTMLColor *color)
{
	HTMLObject *curr;

	curr = e->cursor->object;

	/* Case #1: the cursor is on an element that is not text.  */
	if (! html_object_is_text (curr)) {
		if (e->cursor->offset == 0) {
			if (curr->prev == NULL || ! html_object_is_text (curr->prev))
				return insert_chars_at_not_text (e, text, len, style, color);
			e->cursor->object = curr->prev;
			e->cursor->offset = HTML_TEXT (curr->prev)->text_len;
		} else {
			if (curr->next == NULL || ! html_object_is_text (curr->next))
				return insert_chars_at_not_text (e, text, len, style, color);
			e->cursor->object = curr->next;
			e->cursor->offset = 0;
		}
	}

	/* Case #2: the cursor is on a text, element, but the style is
           different.  This means that we possibly have to split the
           element.  */
	if (HTML_TEXT (curr)->font_style != style
	    || !html_color_equal (HTML_TEXT (curr)->color, color))
		return insert_chars_different_style (e, text, len, style, color);

	/* Case #3: we can simply add the text to the current element.  */
	return insert_chars_same_style (e, text, len);
}

static guint
do_insert (HTMLEngine *engine,
	   const gchar *text,
	   guint len,
	   GtkHTMLFontStyle style,
	   HTMLColor *color)
{
	const gchar *p, *q;
	guint count;
	guint insert_count;

	insert_count = 0;

	p = text;
	while (len > 0) {
		q = memchr (p, '\n', len);

		if (q == NULL) {
			count = insert_chars (engine, p, len, style, color);
			html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_RIGHT, count);
			insert_count += count;
			break;
		}

		if (q != p) {
			count = insert_chars (engine, p, q - p, style, color);
			html_engine_move_cursor (engine, HTML_ENGINE_CURSOR_RIGHT, count);
			insert_count += count;
		}

		while (*q == '\n') {
			insert_para (engine);
			insert_count++;
			q++;
		}

		len -= q - p;
		p = q;
	}

	return insert_count;
}


/* Undo/redo.  */

struct _ActionData {
	/* Reference count.  This is necessary because we want to share the data between
           undo and redo.  */
	guint ref_count;

	/* The string to insert.  */
	gchar *chars;
	gint num_chars;

	/* The font style.  */
	GtkHTMLFontStyle style;
	HTMLColor *color;
};
typedef struct _ActionData ActionData;

static void  closure_destroy  (gpointer closure);
static void  do_redo          (HTMLEngine *engine, gpointer closure);
static void  do_undo          (HTMLEngine *engine, gpointer closure);
static void  setup_undo       (HTMLEngine *engine, ActionData *data);
static void  setup_redo       (HTMLEngine *engine, ActionData *data);

static void
closure_destroy (gpointer closure)
{
	ActionData *data;

	data = (ActionData *) closure;
	g_assert (data->ref_count > 0);

	data->ref_count--;
	if (data->ref_count > 0)
		return;

	html_color_unref (data->color);

	g_free (data->chars);
	g_free (data);
}

static void
do_redo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;
	guint n;

	data = (ActionData *) closure;

	n = do_insert (engine, data->chars, data->num_chars, data->style, data->color);
	setup_undo (engine, data);
}

static void
setup_undo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *action;

	data->ref_count ++;

	/* FIXME i18n */
	action = html_undo_action_new ("insert",
				       do_undo,
				       closure_destroy,
				       data,
				       html_cursor_get_position (engine->cursor));

	html_undo_add_undo_action (engine->undo, action);
}

static void
do_undo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;

	data = (ActionData *) closure;

	html_engine_delete (engine, data->num_chars, FALSE, TRUE);

	setup_redo (engine, data);
}

static void
setup_redo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *action;

	data->ref_count ++;

	/* FIXME i18n */
	action = html_undo_action_new ("insert",
				       do_redo,
				       closure_destroy,
				       data,
				       html_cursor_get_position (engine->cursor));

	html_undo_add_redo_action (engine->undo, action);
}


static ActionData *
create_action_data (HTMLEngine *engine,
		    const gchar *chars,
		    gint num_chars,
		    GtkHTMLFontStyle style,
		    HTMLColor *color)
{
	ActionData *data;

	data = g_new (ActionData, 1);
	data->ref_count = 0;
	data->chars = g_strndup (chars, num_chars);
	data->num_chars = num_chars;
	data->style = style;
	data->color = color;
	html_color_ref (color);

	return data;
}


guint
html_engine_insert (HTMLEngine *e,
		    const gchar *text,
		    guint len)
{
	HTMLObject *current_object;
	guint       current_offset;
	guint n;

	g_return_val_if_fail (e != NULL, 0);
	g_return_val_if_fail (HTML_IS_ENGINE (e), 0);
	g_return_val_if_fail (text != NULL, 0);

	if (len == -1)
		len = strlen (text);
	if (len == 0)
		return 0;

	current_object = e->cursor->object;
	current_offset = e->cursor->offset;
	if (current_object == NULL)
		return 0;

	html_undo_discard_redo (e->undo);

	html_engine_hide_cursor (e);

	n = do_insert (e, text, len, e->insertion_font_style, e->insertion_color);

	setup_undo (e, create_action_data (e, text, len, e->insertion_font_style, e->insertion_color));

	/* printf ("text '%s' len %d type %d\n", text, len, HTML_OBJECT_TYPE (current_object)); */

	/* magic links */
	if (GTK_HTML_PROPERTY (e->widget, magic_links) && len == 1
	    && (text [0] == ' ' || text [0] == '\n')
	    && HTML_OBJECT_TYPE (current_object) == HTML_TYPE_TEXTMASTER)
		html_text_master_magic_link (HTML_TEXT_MASTER (current_object),
					     e, current_offset);

	html_engine_show_cursor (e);

	return n;
}

void
html_engine_insert_link (HTMLEngine *e, const gchar *url, const gchar *target)
{
	HTMLObject *linked;
	HTMLColor *color = html_colorset_get_color (e->settings->color_set, HTMLLinkColor);

	if (e->active_selection) {
		GList *cur;

		html_engine_cut_and_paste_begin (e, "Insert link");
		cur = e->cut_buffer;
		while (cur) {
			linked = html_object_set_link (HTML_OBJECT (cur->data), color, url, target);
			if (linked) {
				html_object_destroy (HTML_OBJECT (cur->data));
				cur->data = linked;
			}
			cur = cur->next;
		}
		html_engine_cut_and_paste_end (e);
	} else {
		/* linked = html_link_text_master_new_with_len
			("", 0,
			 e->insertion_font_style,
			 color,
			 url, target);
		html_engine_paste_object (e, linked, TRUE);
		html_object_destroy (linked); */
		html_engine_set_url    (e, url);
		html_engine_set_target (e, target);
	}
}

void
html_engine_remove_link (HTMLEngine *e)
{
	HTMLObject *unlinked;
	HTMLColor *color = html_colorset_get_color (e->settings->color_set, HTMLTextColor);
	GList *cur;

	if (e->active_selection) {
		html_engine_cut_and_paste_begin (e, "Remove link");
		cur = e->cut_buffer;
		while (cur) {
			unlinked = html_object_remove_link (HTML_OBJECT (cur->data), color);
			if (unlinked) {
				html_object_destroy (HTML_OBJECT (cur->data));
				cur->data = unlinked;
			}
			cur = cur->next;
		}
		html_engine_cut_and_paste_end (e);
	} else {
		html_engine_set_url    (e, NULL);
		html_engine_set_target (e, NULL);
	}
}

void
html_engine_remove_link_object (HTMLEngine *e, HTMLObject *obj)
{
	HTMLObject *unlinked;
	HTMLColor *color = html_colorset_get_color (e->settings->color_set, HTMLTextColor);

	unlinked = html_object_remove_link (obj, color);
	if (unlinked) {
		html_engine_replace_by_object (e, obj, 0, obj, html_object_get_length (obj), unlinked);
		html_object_destroy (unlinked);
	}
}
