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

#include "htmlengine-cutbuffer.h"
#include "htmlengine-edit-cut.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-delete.h"

#include "gtkhtmldebug.h"

#include "htmlengine-edit-paste.h"

/* #define PARANOID_DEBUG */


/* This function adds an empty HTMLTextMaster object to @flow; it is
   used to make sure no empty HTMLClueFlow object is created.  */
static void
add_empty_text_master_to_clueflow (HTMLClueFlow *flow)
{
	static GdkColor black = { 0, 0, 0, 0 };	/* FIXME */
	HTMLObject *new_textmaster;

	new_textmaster = html_text_master_new ("",
					       GTK_HTML_FONT_STYLE_DEFAULT,
					       &black);
	html_clue_prepend (HTML_CLUE (flow), new_textmaster);
}

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

	/* We are not in a text element, so no splitting is needed.
           If we are at the end of the non-texte element, we must
           append; otherwise, we must prepend.  */
	if (cursor->offset == 0)
		return FALSE;
	else
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
		add_empty_text_master_to_clueflow (HTML_CLUEFLOW (curr_clue));
		engine->cursor->object = HTML_CLUE (curr_clue)->head;
		engine->cursor->offset = 0;
	} else {
		engine->cursor->object = curr->prev;

		if (html_object_is_text (curr->prev))
			engine->cursor->offset = HTML_TEXT (curr->prev)->text_len;
		else
			engine->cursor->offset = 1;
	}

	new_clueflow = html_object_dup (HTML_OBJECT (clue));
	html_object_change_set (new_clueflow, HTML_CHANGE_ALL);

	if (curr_clue->prev == NULL)
		html_clue_prepend (HTML_CLUE (curr_clue->parent), new_clueflow);
	else
		html_clue_append_after (HTML_CLUE (curr_clue->parent), new_clueflow, curr_clue->prev);

	/* Move the stuff until the cursor position into the new HTMLClueFlow.  */

	for (p = HTML_CLUE (curr_clue)->head; p != NULL && p != curr; p = pnext) {
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
	html_object_change_set (new_clue, HTML_CHANGE_ALL);

	html_clue_append_after (HTML_CLUE (where->parent), new_clue, where);

	return new_clue;
}

/* Add all the necessary HTMLClueFlows to paste the current selection.  */
static gboolean
prepare_clueflows (HTMLEngine *engine,
		   GList *buffer,
		   gboolean append)
{
	HTMLObject *clue;
	HTMLObject *curr;
	GList *p;
	gboolean first;
	gboolean retval;

	curr = engine->cursor->object;
	g_return_val_if_fail (curr->parent != NULL, FALSE);
	g_return_val_if_fail (HTML_OBJECT_TYPE (curr->parent) == HTML_TYPE_CLUEFLOW, FALSE);

	clue = NULL;		/* Make compiler happy.  */

	first = TRUE;
	retval = FALSE;
	for (p = buffer; p != NULL; p = p->next) {
		HTMLObject *obj;

		obj = (HTMLObject *) p->data;
		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_CLUEFLOW)
			continue;

		if (first && ! append) {
			split_first_clueflow_at_cursor (engine, HTML_CLUEFLOW (obj));

			clue = engine->cursor->object->parent;
			g_assert (clue != NULL);

			first = FALSE;
			retval = TRUE;
		} else {
			if (first)
				clue = engine->cursor->object->parent;

			g_assert (clue != NULL);

			clue = add_new_clueflow (engine, HTML_CLUEFLOW (obj), clue);
		}
	}

	return retval;
}


static guint
obj_len (HTMLObject *obj)
{
	return (html_object_is_text (obj)) ? HTML_TEXT (obj)->text_len : 1;
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

	cursor->object    = object;
	cursor->offset    = obj_len (object);
	cursor->position += obj_len (object);
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

	if (curr->prev == NULL)
		html_clue_prepend (HTML_CLUE (parent), object);
	else
		html_clue_append_after (HTML_CLUE (parent), object, curr->prev);

	cursor->position += obj_len (object);
}

static void
merge_possibly (HTMLEngine *engine,
		HTMLObject *a,
		HTMLObject *b)
{
	HTMLCursor *cursor;

	if (a == NULL || b == NULL)
		return;

	if (! html_object_is_text (a) || ! html_object_is_text (b))
		return;

	if (! html_text_check_merge (HTML_TEXT (a), HTML_TEXT (b)))
		return;

	cursor = engine->cursor;

	if (cursor->object == HTML_OBJECT (a))
		cursor->object = HTML_OBJECT (b);
	else if (cursor->object == HTML_OBJECT (b))
		cursor->offset += HTML_TEXT (a)->text_len;

	html_text_merge (HTML_TEXT (b), HTML_TEXT (a), TRUE);

	html_clue_remove (HTML_CLUE (a->parent), HTML_OBJECT (a));
	html_object_destroy (HTML_OBJECT (a));
}

static void
remove_slaves_at_cursor (HTMLEngine *engine)
{
	HTMLCursor *cursor;
	HTMLObject *parent;

	cursor = engine->cursor;
	parent = cursor->object->parent;

	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return;

	html_clueflow_remove_text_slaves (HTML_CLUEFLOW (parent));
}


static guint
do_paste (HTMLEngine *engine,
	  GList *buffer)
{
	/* Whether we need to append the elements at the cursor
	   position or not.  */
	gboolean append;
	guint oldpos = engine->cursor->position;
	GList *p;

#ifdef PARANOID_DEBUG
	g_print ("\n**** Cut buffer contents:\n\n");
	gtk_html_debug_dump_list_simple (buffer, 1);
#endif

	/* 1. Freeze the engine.  */
	html_engine_freeze (engine);

#ifdef PARANOID_DEBUG
	g_print ("\n**** Tree before pasting:\n\n");
	gtk_html_debug_dump_tree_simple (engine->clue, 1);
#endif

	/* 2. Remove all the HTMLTextSlaves in the current HTMLClueFlow.  */
	remove_slaves_at_cursor (engine);

	/* 3. Split the first paragraph at the cursor position, to
	   allow insertion of the elements.  */
	append = split_at_cursor (engine);

#ifdef PARANOID_DEBUG
	g_print ("\n**** Tree after splitting first para:\n\n");
	gtk_html_debug_dump_tree_simple (engine->clue, 1);
#endif

	/* 4. Prepare the HTMLClueFlows to hold the elements we want
	   to paste.  */
	if (prepare_clueflows (engine, buffer, append))
		append = TRUE;

#ifdef PARANOID_DEBUG
	g_print ("\n**** Tree after clueflow preparation:\n\n");
	gtk_html_debug_dump_tree_simple (engine->clue, 1);
#endif

	g_print ("\n");

	/* 5. Duplicate the objects in the cut buffer, one by one, and
	   insert them into the document.  */

	for (p = buffer; p != NULL; p = p->next) {
		HTMLObject *obj_copy;
		HTMLObject *obj;

		obj = (HTMLObject *) p->data;

		if (HTML_OBJECT_TYPE (obj) != HTML_TYPE_CLUEFLOW) {
#ifdef PARANOID_DEBUG
			if (html_object_is_text (obj))
				g_print ("*** Pasting `%s'\n", HTML_TEXT (obj)->text);
#endif
			obj_copy = html_object_dup (obj);
			html_object_change_set (obj_copy, HTML_CHANGE_ALL);

			if (append)
				append_object (engine, obj_copy);
			else
				prepend_object (engine, obj_copy);
		} else {
			HTMLObject *clueflow;
#ifdef PARANOID_DEBUG
			g_print ("*** Pasting HTMLClueFlow\n");
#endif

			/* This is an HTMLClueFlow, which we have already added to the
                           tree: move to the next element in the cut buffer, and insert it
                           into the next HTMLClueFlow (which is the one we created).  If
                           we get another ClueFlow after this one, we need to fill the
                           empty HTMLClueFlow with an empty text element.  */

			if (!p->next) {
				engine->cursor->object = HTML_CLUE (engine->cursor->object->parent->next)->head;
				engine->cursor->offset = 0;
				engine->cursor->position++;
#ifdef PARANOID_DEBUG
				g_print ("    next is NULL: bailing out\n");
#endif
				break;
			}
			p = p->next;

			obj = HTML_OBJECT (p->data);

			clueflow = engine->cursor->object->parent->next;
			g_assert (clueflow != NULL);
			g_assert (HTML_OBJECT_TYPE (clueflow) == HTML_TYPE_CLUEFLOW);

			while (HTML_OBJECT_TYPE (obj) == HTML_TYPE_CLUEFLOW) {
				add_empty_text_master_to_clueflow (HTML_CLUEFLOW (clueflow));
				engine->cursor->position += 2;
				/* engine->cursor->object    = HTML_CLUE (clueflow)->head;
				   engine->cursor->offset    = 0; */
				clueflow = clueflow->next;

				p = p->next;
				if (p == NULL)
					break;
				obj = HTML_OBJECT (p->data);
			}

			if (p == NULL)
				break;

			obj_copy = html_object_dup (obj);
			html_object_change_set (obj_copy, HTML_CHANGE_ALL);

			html_clue_prepend (HTML_CLUE (clueflow), obj_copy);

			engine->cursor->object    = obj_copy;
			engine->cursor->offset    = obj_len (obj_copy);
			engine->cursor->position += obj_len (obj_copy);

			append = TRUE;
#ifdef PARANOID_DEBUG
			g_print ("*** Appending %s to %p\n",
				 html_type_name (HTML_OBJECT_TYPE (obj)), clueflow);
#endif
		}

		/* Merge the first element with the previous one if possible.  */
		if (p->prev == NULL)
			merge_possibly (engine, obj_copy->prev, obj_copy);
		/* Merge the last element with the following one if possible. */
		if (p->next == NULL)
			merge_possibly (engine, obj_copy, obj_copy->next);
	}

#ifdef PARANOID_DEBUG
	g_print ("\n**** Tree after pasting:\n\n");
	gtk_html_debug_dump_tree_simple (engine->clue, 1);
#endif

	/* 8. Thaw the engine so that things are re-laid out again.  FIXME: this is
           inefficient.  */
	html_engine_thaw (engine);

	/* 9. Normalize the cursor pointer.  */
	html_cursor_normalize (engine->cursor);

	/* Return the number of character elements pasted, to make undo/redo possible.  */
	return engine->cursor->position - oldpos;
}


/* Undo/redo.  */

struct _ActionData {
	/* Reference count.  This is necessary because we want to share the data between
           undo and redo.  */
	guint ref_count;

	/* Contents of the cut buffer.  */
	GList *buffer;

	/* Number of character elements in the buffer.  */
	guint buffer_count;
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

	data->ref_count --;
	if (data->ref_count > 0)
		return;

	html_engine_cut_buffer_destroy (data->buffer);

	g_free (data);

	return;
}

static void
do_redo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;

	data = (ActionData *) closure;

	do_paste (engine, data->buffer);

	setup_undo (engine, data);
}

static void
setup_redo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *undo_action;

	data->ref_count ++;

	/* FIXME i18n */
	undo_action = html_undo_action_new ("paste",
					    do_redo,
					    closure_destroy,
					    data,
					    html_cursor_get_position (engine->cursor));

	html_undo_add_redo_action (engine->undo, undo_action);
}

static void
do_undo (HTMLEngine *engine,
	 gpointer closure)
{
	ActionData *data;
	gint i;

	data = (ActionData *) closure;

	for (i=0; i<data->buffer_count; i++)
		html_engine_delete (engine, 1, FALSE, TRUE);

	setup_redo (engine, data);
}

static void
setup_undo (HTMLEngine *engine,
	    ActionData *data)
{
	HTMLUndoAction *undo_action;
	gint i;

	data->ref_count ++;

	/* FIXME i18n */
	undo_action = html_undo_action_new ("paste",
					    do_undo,
					    closure_destroy,
					    data,
					    i=html_cursor_get_position (engine->cursor));

	html_undo_add_undo_action (engine->undo, undo_action);
}

static ActionData *
action_data_from_cut_buffer (HTMLEngine *engine,
			     GList *buffer,
			     guint count)
{
	ActionData *data;

	data = g_new (ActionData, 1);
	data->ref_count = 0;
	data->buffer = html_engine_cut_buffer_dup (buffer);
	data->buffer_count = count;

	return data;
}


/**
 * html_engine_paste_buffer:
 * @engine: An HTMLEngine
 * @buffer: A cut buffer list
 * 
 * Paste @buffer at the current cursor position, WITHOUT SAVING UNDO
 * INFORMATION.
 **/
void
html_engine_paste_buffer (HTMLEngine *engine,
			  GList *buffer)
{
	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->clue != NULL);
	g_return_if_fail (buffer != NULL);

	do_paste (engine, buffer);
}

/**
 * html_engine_paste:
 * @engine: An HTMLEngine
 * @do_undo: Whether to save undo information for this operation.
 * 
 * Paste the current cut buffer at the current cursor position.  Save
 * undo information only if @do_undo is true.
 **/
void
html_engine_paste (HTMLEngine *engine,
		   gboolean do_undo)
{
	guint count;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (engine->clue != NULL);

	if (engine->cut_buffer == NULL)
		return;

	if (do_undo) {
		html_undo_discard_redo (engine->undo);
		html_undo_level_begin (engine->undo, "paste");
	}

	/* Cut current selection.  */
	if (engine->active_selection) {
		GList *cut_buffer;

		/* Keep cut buffer.  */
		cut_buffer = engine->cut_buffer;
		engine->cut_buffer = NULL;

		/* Cut current selection.  */
		html_engine_cut (engine, do_undo);

		/* Restore cut buffer.  */
		if (engine->cut_buffer != NULL)
			html_engine_cut_buffer_destroy (engine->cut_buffer);
		engine->cut_buffer = cut_buffer;
	}

	count = do_paste (engine, engine->cut_buffer);

	if (do_undo) {
		setup_undo (engine, action_data_from_cut_buffer (engine, engine->cut_buffer, count));
		html_undo_level_end (engine->undo);
	}
}

/**
 * html_engine_paste_object:
 * @engine: An HTMLEngine
 * @object: Object to be pasted
 * @do_undo: Whether we want to save undo information for this op.
 * 
 * Paste @buffer at the current cursor position.
 **/
void
html_engine_paste_object (HTMLEngine *engine,
			  HTMLObject *object,
			  gboolean do_undo)
{
	GList *tmp_buffer;

	g_return_if_fail (engine != NULL);
	g_return_if_fail (HTML_IS_ENGINE (engine));
	g_return_if_fail (object != NULL);

	tmp_buffer = engine->cut_buffer;
	engine->cut_buffer = g_list_alloc ();
	engine->cut_buffer->data = object;

	html_engine_paste (engine, do_undo);

	engine->cut_buffer = tmp_buffer;

	g_list_free (tmp_buffer);
}
