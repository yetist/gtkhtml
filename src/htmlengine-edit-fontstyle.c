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

#include "htmlclue.h"
#include "htmltextmaster.h"

#include "htmlengine-edit-fontstyle.h"


static void
set_font_style_in_selection_forall (HTMLObject *self,
				    gpointer closure)
{
	GtkHTMLFontStyle font_style;
	GtkHTMLFontStyle last_font_style;
	HTMLTextMaster *master;
	HTMLTextMaster *curr;
	HTMLObject *next;

	if (! self->selected)
		return;

	if (! html_object_is_text (self))
		return;

	font_style = GPOINTER_TO_INT (closure);
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
		curr = HTML_TEXT_MASTER (html_text_split (HTML_TEXT (self),
							  master->select_start));
		html_clue_append_after (HTML_CLUE (self->parent), HTML_OBJECT (curr), self);
	}

	last_font_style = HTML_TEXT (curr)->font_style;

	/* Merge as many selected following elements as possible.  FIXME: we
           could be faster and merge all of them at once, but for now we really
           don't care.  */

	next = HTML_OBJECT (curr)->next;
	while (next != NULL
	       && HTML_OBJECT_TYPE (next) == HTML_OBJECT_TYPE (self)
	       && next->selected) {
		static HTMLText *merge_list[2] = { NULL, NULL };
		HTMLObject *next;

		last_font_style = HTML_TEXT (next)->font_style;

		merge_list[0] = HTML_TEXT (next);
		html_text_merge (HTML_TEXT (curr), merge_list);

		html_clue_remove (HTML_CLUE (HTML_OBJECT (curr)->parent), next);
		html_object_destroy (next);

		next = HTML_OBJECT (curr)->next;
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
}

static void
set_font_style_in_selection (HTMLEngine *engine,
			     GtkHTMLFontStyle style)
{
	g_return_if_fail (engine->clue != NULL);

	html_object_forall (engine->clue,
			    set_font_style_in_selection_forall,
			    GINT_TO_POINTER (style));
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

	if (html_object_is_text (curr))
		return gtk_html_font_style_merge (HTML_TEXT (curr)->font_style,
						  engine->insertion_font_style);

	return engine->insertion_font_style;
}

/**
 * html_engine_set_fontstyle:
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
 **/
void
html_engine_set_font_style (HTMLEngine *engine,
			    GtkHTMLFontStyle style)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->editable);

	if (engine->active_selection) {
		html_engine_freeze (engine);
		set_font_style_in_selection (engine, style);
		html_engine_thaw (engine);
		return;
	}

	engine->insertion_font_style = style;
}
