/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

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

#include "gtkhtmldebug.h"
#include "htmlclue.h"
#include "htmltextmaster.h"

#include "htmlengine-edit-fontstyle.h"


static void
merge_backward (HTMLObject *object,
		HTMLCursor *cursor)
{
	GtkHTMLFontStyle font_style;
	HTMLObject *p, *pprev;
	static HTMLText *merge_list[2] = { NULL, NULL };

	font_style = HTML_TEXT (object)->font_style;

	for (p = object->prev; p != NULL; p = pprev) {
		pprev = p->prev;
		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE) {
			html_clue_remove (HTML_CLUE (p->parent), p);
			html_object_destroy (p);
		} else if (HTML_OBJECT_TYPE (object) == HTML_OBJECT_TYPE (p)
			   && html_text_check_merge (HTML_TEXT (object), HTML_TEXT (p))) {
			merge_list[0] = HTML_TEXT (object);
			html_text_merge (HTML_TEXT (p), merge_list);
			html_clue_remove (HTML_CLUE (object->parent), object);

			if (object == cursor->object) {
				cursor->object = p;
				cursor->offset += HTML_TEXT (object)->text_len;
			}

			html_object_destroy (object);
		} else {
			break;
		}
	}
}

static void
merge_forward (HTMLObject *object,
	       HTMLCursor *cursor)
{
	GtkHTMLFontStyle font_style;
	HTMLObject *p, *pnext;
	static HTMLText *merge_list[2] = { NULL, NULL };

	font_style = HTML_TEXT (object)->font_style;

	for (p = object->next; p != NULL; p = pnext) {
		pnext = p->next;
		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE) {
			html_clue_remove (HTML_CLUE (p->parent), p);
			html_object_destroy (p);
		} else if (HTML_OBJECT_TYPE (object) == HTML_OBJECT_TYPE (p)
			   && html_text_check_merge (HTML_TEXT (object), HTML_TEXT (p))) {
			merge_list[0] = HTML_TEXT (p);
			html_text_merge (HTML_TEXT (object), merge_list);
			html_clue_remove (HTML_CLUE (p->parent), p);
			html_object_destroy (p);
		} else {
			break;
		}
	}
}

struct _SetFontStyleForallData {
	HTMLCursor *cursor;
	GtkHTMLFontStyle and_mask;
	GtkHTMLFontStyle or_mask;
};
typedef struct _SetFontStyleForallData SetFontStyleForallData;

static void
set_font_style_in_selection_forall (HTMLObject *self,
				    gpointer closure)
{
	SetFontStyleForallData *data;
	GtkHTMLFontStyle font_style;
	GtkHTMLFontStyle last_font_style;
	HTMLTextMaster *master;
	HTMLTextMaster *curr;
	HTMLObject *next;

	if (! self->selected)
		return;

	if (! html_object_is_text (self))
		return;

	data = (SetFontStyleForallData *) closure;

	font_style = (HTML_TEXT (self)->font_style & data->and_mask) | data->or_mask;
	if (font_style == HTML_TEXT (self)->font_style)
		return;

	/* FIXME this is gross.  */
	if (HTML_OBJECT_TYPE (self) != HTML_TYPE_TEXTMASTER
	    && HTML_OBJECT_TYPE (self) != HTML_TYPE_LINKTEXTMASTER)
		return;

	master = HTML_TEXT_MASTER (self);

	/* Split the first selected element at the initial selection position.  */
	if (master->select_start == 0) {
		curr = master;
	} else {
		curr = HTML_TEXT_MASTER (html_text_split (HTML_TEXT (self), master->select_start));
		html_clue_append_after (HTML_CLUE (self->parent), HTML_OBJECT (curr), self);
	}

	last_font_style = HTML_TEXT (curr)->font_style;

	/* Merge as many selected following elements as possible.  FIXME: we
           could be faster and merge all of them at once, but for now we really
           don't care.  */

	next = HTML_OBJECT (curr)->next;
	while (next != NULL) {
		if (HTML_OBJECT_TYPE (next) == HTML_TYPE_TEXTSLAVE) {
			html_clue_remove (HTML_CLUE (next->parent), next);
			html_object_destroy (next);
			next = HTML_OBJECT (curr)->next;
		} else if (HTML_OBJECT_TYPE (next) == HTML_OBJECT_TYPE (self)
			   && next->selected
			   && (((HTML_TEXT (next)->font_style & data->and_mask) | data->or_mask)
			       == font_style)) {
			static HTMLText *merge_list[2] = { NULL, NULL };

			last_font_style = HTML_TEXT (next)->font_style;

			merge_list[0] = HTML_TEXT (next);
			html_text_merge (HTML_TEXT (curr), merge_list);

			html_clue_remove (HTML_CLUE (next->parent), next);
			html_object_destroy (next);
			next = HTML_OBJECT (curr)->next;
		} else {
			break;
		}
	}

	/* Split the current element so that we leave the non-selected part in
           the correct style.  */

	if (last_font_style != font_style
	    && curr->select_start + curr->select_length < HTML_TEXT (curr)->text_len) {
		HTMLText *new;

		new = html_text_split (HTML_TEXT (curr),
				       curr->select_start + curr->select_length);
		html_text_set_font_style (new, NULL, last_font_style);
		html_clue_append_after (HTML_CLUE (HTML_OBJECT (curr)->parent),
					HTML_OBJECT (new), HTML_OBJECT (curr));
	}

	/* Finally set the style.  */

	html_text_set_font_style (HTML_TEXT (curr), NULL, font_style);

	/* At this point, we might have elements before or after us with the
           same font style.  We don't want to have contiguous equal text
           elements, so we merge more stuff here.  */

	merge_forward (HTML_OBJECT (curr), data->cursor);
	merge_backward (HTML_OBJECT (curr), data->cursor); /* This must be *after* `merge_forward()'.  */
}

static void
set_font_style_in_selection (HTMLEngine *engine,
			     GtkHTMLFontStyle and_mask,
			     GtkHTMLFontStyle or_mask)
{
	SetFontStyleForallData *data;

	g_return_if_fail (engine->clue != NULL);

	g_print ("Tree before changing font style:\n");
	gtk_html_debug_dump_tree (engine->clue, 2);

	data = g_new (SetFontStyleForallData, 1);
	data->and_mask = and_mask;
	data->or_mask = or_mask;
	data->cursor = engine->cursor;

	html_object_forall (engine->clue, set_font_style_in_selection_forall, data);

	g_free (data);

	g_print ("Tree after changing font style:\n");
	gtk_html_debug_dump_tree (engine->clue, 2);
}


GtkHTMLFontStyle
html_engine_get_current_insertion_font_style (HTMLEngine *engine)
{
	HTMLObject *curr;

	g_return_val_if_fail (engine != NULL, GTK_HTML_FONT_STYLE_DEFAULT);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), GTK_HTML_FONT_STYLE_DEFAULT);
	g_return_val_if_fail (engine->editable, GTK_HTML_FONT_STYLE_DEFAULT);

	curr = engine->cursor->object;
	if (curr == NULL)
		return GTK_HTML_FONT_STYLE_DEFAULT;

	if (engine->insertion_font_style != GTK_HTML_FONT_STYLE_DEFAULT)
		return engine->insertion_font_style;

	if (! html_object_is_text (curr))
		return GTK_HTML_FONT_STYLE_DEFAULT;

	return HTML_TEXT (curr)->font_style;
}

/**
 * html_engine_set_font_style:
 * @engine: An HTMLEngine
 * @style: An HTMLFontStyle
 * 
 * Set the current font style for `engine'.  This has the same semantics as the
 * bold, italics etc. buttons in a word processor, i.e.:
 *
 * - If there is a selection, the selection gets the specified style.
 *
 * - If there is no selection, the style gets "attached" to the cursor.  So
 *   inserting text after this will cause text to have this style.
 *
 * Instead of specifying an "absolute" style, we specify it as a "difference"
 * from the current one, through an AND mask and an OR mask.
 *
 **/
void
html_engine_set_font_style (HTMLEngine *engine,
			    GtkHTMLFontStyle and_mask,
			    GtkHTMLFontStyle or_mask)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->editable);

	if (engine->active_selection) {
		html_engine_freeze (engine);
		set_font_style_in_selection (engine, and_mask, or_mask);
		html_engine_thaw (engine);
		return;
	}

	engine->insertion_font_style &= and_mask;
	engine->insertion_font_style |= or_mask;
}
