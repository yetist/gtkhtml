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
#include "htmlclueflow.h"
#include "htmltext.h"
#include "htmltextmaster.h"

#include "gtkhtmldebug.h"

#include "htmlengine-edit-paste.h"


/* This split text at the current cursor position to prepare it for insertion
   of new elements.  */
static void
split_text (HTMLEngine *engine)
{
	HTMLObject *object;
	HTMLText *text;
	HTMLText *new;
	HTMLObject *parent;
	guint offset;

	object = engine->cursor->object;
	offset = engine->cursor->offset;
	parent = object->parent;
	text = HTML_TEXT (object);

	new = html_text_split (text, offset);
	html_clue_append_after (HTML_CLUE (parent), HTML_OBJECT (new), HTML_OBJECT (text));

	/* Update the cursor pointer.  This is legal, as the relative position
           does not change at all.  */
	engine->cursor->object = HTML_OBJECT (new);
	engine->cursor->offset = 0;
}

/* If the cursor is in a text element, the following function will split it so
   that stuff can be inserted at the right position.  The returned value will
   be TRUE if we should append elements after the current cursor position;
   FALSE if we should prepend them before the current cursor position. */
static gboolean
split_at_cursor (HTMLEngine *engine)
{
	HTMLCursor *cursor;
	HTMLObject *current;

	cursor = engine->cursor;

	/* If we are at the beginning of the element, we can always simply
           prepend stuff to the element itself.  */
	if (cursor->offset == 0)
		return FALSE;

	current = cursor->object;

	if (html_object_is_text (current)) {
		/* If we are at the end of a text element, we just have to
                   append elements.  */
		if (cursor->offset >= HTML_TEXT (current)->text_len)
			return TRUE;

		/* Bad luck, we have to split at the current position.  After
                   calling `split_text()', the cursor will be positioned at the
                   beginning of the second half of the split text, so we just
                   have to prepend the new elements there.  */

		split_text (engine);
		return FALSE;
	}

	/* We are not in a text element, so no splitting is needed.  If we got
           here, offset is certainly positive, so we must append.  */
	return TRUE;
}

/* This splits the paragraph in which the cursor is in, at the cursor position,
   with properties given by those of @clue.  */
static void
split_first_clueflow_at_cursor (HTMLEngine *engine,
				HTMLClueFlow *clue)
{
	HTMLObject *new_clueflow;
	HTMLObject *curr;
	HTMLObject *curr_clue;
	HTMLObject *p, *pnext;

	curr = engine->cursor->object;
	curr_clue = curr->parent;

	/* If there is nothing else before this element in the
           paragraph, add an empty text element, as HTMLClueFlows must
           never be empty.  */
	if (curr->prev == NULL) {
		/* FIXME color */
		GdkColor black = { 0, 0, 0, 0 };
		HTMLObject *new_textmaster;

		new_textmaster = html_text_master_new (g_strdup (""), GTK_HTML_FONT_STYLE_DEFAULT,
						       &black);
		html_clue_prepend (HTML_CLUE (curr_clue), new_textmaster);

		engine->cursor->object = new_textmaster;
		engine->cursor->offset = 0;
	} else {
		engine->cursor->object = curr->prev;

		if (html_object_is_text (curr->prev))
			engine->cursor->offset = HTML_TEXT (curr->prev)->text_len;
		else
			engine->cursor->offset = 1;
	}

	new_clueflow = html_object_dup (HTML_OBJECT (clue));

	if (curr_clue->prev == NULL)
		html_clue_prepend (HTML_CLUE (curr_clue->parent), new_clueflow);
	else
		html_clue_append_after (HTML_CLUE (curr_clue->parent), new_clueflow, curr_clue->prev);

	/* Move the stuff until the cursor position into the new HTMLClueFlow.  */

	for (p = HTML_CLUE (curr->parent)->head; p != NULL && p != curr; p = pnext) {
		pnext = p->next;

		html_clue_remove (HTML_CLUE (p->parent), p);

		if (HTML_OBJECT_TYPE (p) == HTML_TYPE_TEXTSLAVE)
			html_object_destroy (p);
		else
			html_clue_append (HTML_CLUE (new_clueflow), p);
	}
}

/* Add a new HTMLClueFlow after `where', and return a pointer to it.  */
static HTMLObject *
add_new_clueflow (HTMLEngine *engine,
		  HTMLClueFlow *clue,
		  HTMLObject *where)
{
	HTMLObject *new_clue;

	new_clue = html_object_dup (HTML_OBJECT (clue));
	html_clue_append_after (HTML_CLUE (where->parent), new_clue, where);

	return new_clue;
}

/* Add all the necessary HTMLClueFlows to paste the current selection.  */
static gboolean
prepare_clueflows (HTMLEngine *engine)
{
	HTMLObject *clue;
	HTMLObject *curr;
	GList *p;
	gboolean first;
	gboolean retval;

	curr = engine->cursor->object;
	g_return_val_if_fail (curr->parent != NULL, FALSE);
	g_return_val_if_fail (HTML_OBJECT_TYPE (curr->parent) == HTML_TYPE_CLUEFLOW, FALSE);

	clue = curr->parent;
	first = TRUE;
	retval = FALSE;
	for (p = engine->cut_buffer; p != NULL; p = p->next) {
		HTMLObject *obj;

		obj = (HTMLObject *) p->data;
		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_CLUEFLOW)
			continue;

		if (first) {
			first = FALSE;
			split_first_clueflow_at_cursor (engine, HTML_CLUEFLOW (obj));
			retval = TRUE;
		} else {
			clue = add_new_clueflow (engine, HTML_CLUEFLOW (obj), clue);
		}
	}

	return retval;
}


/* FIXME this is kind of evil.  */
static void
skip (HTMLEngine *engine)
{
	HTMLCursor *cursor;
	HTMLObject *curr;

	cursor = engine->cursor;
	curr = cursor->object;

	if (html_object_is_text (curr))
		cursor->relative_position += HTML_TEXT (curr)->text_len;
	else
		cursor->relative_position ++;

	cursor->object = curr->next;
}

static void
append_object (HTMLEngine *engine,
	       HTMLObject *object)
{
	HTMLCursor *cursor;
	HTMLObject *curr;
	HTMLObject *parent;

	cursor = engine->cursor;
	curr = cursor->object;
	parent = curr->parent;

	html_clue_append_after (HTML_CLUE (parent), object, curr);

	skip (engine);
}

static void
prepend_object (HTMLEngine *engine,
		HTMLObject *object)
{
	HTMLCursor *cursor;
	HTMLObject *curr;
	HTMLObject *parent;

	cursor = engine->cursor;
	curr = cursor->object;
	parent = curr->parent;

	if (curr->prev != NULL)
		html_clue_append_after (HTML_CLUE (parent), object, curr->prev);
	else
		html_clue_prepend (HTML_CLUE (parent), object);

	cursor->object = object;
	skip (engine);
}


void
html_engine_paste (HTMLEngine *engine)
{
	gboolean append;
	GList *p;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->clue != NULL);

	if (engine->cut_buffer == NULL)
		return;

	g_warning ("Paste!");

	/* 1. Freeze the engine.  */

	html_engine_freeze (engine);

	g_print ("\n**** Tree before pasting:\n\n");
	gtk_html_debug_dump_tree (engine->clue, 2);

	/* 2. Split the first paragraph at the cursor position, to allow insertion
              of the elements.  */

	append = split_at_cursor (engine);

	g_print ("\n**** Tree after splitting first para:\n\n");
	gtk_html_debug_dump_tree (engine->clue, 2);

	/* 3. Prepare the HTMLClueFlows to hold the elements we want to paste.  */

	if (prepare_clueflows (engine))
		append = TRUE;

	g_print ("\n**** Tree after clueflow preparation:\n\n");
	gtk_html_debug_dump_tree (engine->clue, 2);

	/* 4. Duplicate the objects in the cut buffer, one by one, and insert
              them into the document.  */

	for (p = engine->cut_buffer; p != NULL; p = p->next) {
		HTMLObject *obj;

		obj = (HTMLObject *) p->data;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_CLUEFLOW) {
			if (html_object_is_text (obj))
				g_print ("*** Pasting `%s'\n", HTML_TEXT (obj)->text);
			if (append)
				append_object (engine, html_object_dup (obj));
			else
				prepend_object (engine, html_object_dup (obj));
		} else {
			HTMLObject *next_clueflow;
			HTMLObject *obj_copy;

			g_print ("*** Pasting HTMLClueFlow\n");

			/* This is an HTMLClueFlow, which we have already added
                           to the tree: move to the next element in the cut
                           buffer, and insert it into the next HTMLClueFlow
                           (which is the one we created).  */

			/* This should never give us another ClueFlow, because
                           ClueFlows are never empty.  */
			p = p->next;
			if (p == NULL) {
				g_print ("    next is NULL: bailing out\n");
				break;
			}

			obj = (HTMLObject *) p->data;

			if (HTML_OBJECT_TYPE (obj) == HTML_TYPE_CLUEFLOW) {
				g_warning ("Internal error: pasting empty HTMLClueFlows");
				continue;
			}

			if (! append)
				skip (engine);

			append = TRUE;

			next_clueflow = engine->cursor->object->parent->next;
			g_assert (next_clueflow != NULL);
			g_assert (HTML_OBJECT_TYPE (next_clueflow) == HTML_TYPE_CLUEFLOW);

			g_print ("*** Appending %s to %p\n", html_type_name (HTML_OBJECT_TYPE (obj)), next_clueflow);

			obj_copy = html_object_dup (obj);
			html_clue_prepend (HTML_CLUE (next_clueflow), obj_copy);

			/* Kind of evil, isn't it?  */
			engine->cursor->object = obj_copy;
			engine->cursor->offset = 0;
			engine->cursor->relative_position++;
		}
	}

	if (append)
		skip (engine);

	g_print ("\n**** Tree after pasting:\n\n");
	gtk_html_debug_dump_tree (engine->clue, 2);

	/* Thaw the engine so that things are re-laid out again.  FIXME: this
           might be a bit inefficient for cut & paste.  */

	html_engine_thaw (engine);
}
