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

#include "htmlengine-edit-clueflowstyle.h"

/* FIXME undo/redo is incomplete/broken here.  */


/* Undo/redo data for setting the style of a single paragraph.  */

struct _OriginalClueFlowProps {
	HTMLClueFlowStyle style;
	guint8 level;
	HTMLHAlignType alignment;
};
typedef struct _OriginalClueFlowProps OriginalClueFlowProps;

static OriginalClueFlowProps *
get_props (HTMLClueFlow *flow)
{
	OriginalClueFlowProps *props;

	props = g_new (OriginalClueFlowProps, 1);

	html_clueflow_get_properties (flow, &props->style, &props->level, &props->alignment);

	return props;
}

static void
set_props (HTMLClueFlow *flow,
		HTMLEngine *engine,
		const OriginalClueFlowProps *props)
{
	html_clueflow_set_properties (flow,
				      engine,
				      props->style,
				      props->level,
				      props->alignment);
}

static void
free_props (OriginalClueFlowProps *props)
{
	g_free (props);
}


/* Helper function to retrieve the current paragraph.  */

static HTMLClueFlow *
get_current_para (HTMLEngine *engine)
{
	HTMLObject *current;
	HTMLObject *parent;

	current = engine->cursor->object;
	if (current == NULL)
		return NULL;

	parent = current->parent;
	if (parent == NULL)
		return NULL;

	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return NULL;

	return HTML_CLUEFLOW (parent);
}


/* Retrieving the current paragraph style to show in the toolbar.  */

HTMLClueFlowStyle
html_engine_get_current_clueflow_style (HTMLEngine *engine)
{
	HTMLClueFlow *para;

	/* FIXME TODO region */

	g_return_val_if_fail (engine != NULL, HTML_CLUEFLOW_STYLE_NORMAL);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), HTML_CLUEFLOW_STYLE_NORMAL);

	para = get_current_para (engine);
	if (para == NULL)
		return HTML_CLUEFLOW_STYLE_NORMAL;

	return para->style;
}
 

/* Retrieving the current paragraph indentation to show in the toolbar.  */

guint
html_engine_get_current_clueflow_indentation (HTMLEngine *engine)
{
	HTMLClueFlow *para;

	/* FIXME TODO region */

	g_return_val_if_fail (engine != NULL, 0);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), 0);

	para = get_current_para (engine);
	if (para == NULL)
		return 0;

	return para->level;
}


/* Retrieving the current paragraph alignment to show in the toolbar.  */

HTMLHAlignType
html_engine_get_current_clueflow_alignment (HTMLEngine *engine)
{
	HTMLClueFlow *para;

	/* FIXME TODO region */

	g_return_val_if_fail (engine != NULL, HTML_HALIGN_LEFT);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), HTML_HALIGN_LEFT);

	para = get_current_para (engine);
	if (para == NULL)
		return 0;

	return HTML_CLUE (para)->halign;
}


static void set_clueflow_style_undo (HTMLEngine *engine, gpointer closure);
static void set_clueflow_style_undo_closure_destroy (gpointer closure);

/* Setting style of a single paragraph -- redo.  */

static void
set_clueflow_style_redo_closure_destroy (gpointer closure)
{
	free_props ((OriginalClueFlowProps *) closure);
}

static void
set_clueflow_style_redo (HTMLEngine *engine,
			 gpointer closure)
{
	HTMLClueFlow *para;
	OriginalClueFlowProps *undo_props;
	HTMLUndoAction *undo_action;

	para = get_current_para (engine);

	undo_props = get_props (para);
	set_props (para, engine, (OriginalClueFlowProps *) closure);

	html_engine_make_cursor_visible (engine);

	undo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_undo,
					    set_clueflow_style_undo_closure_destroy,
					    undo_props,
					    html_cursor_get_relative (engine->cursor));
	html_undo_add_undo_action (engine->undo, undo_action);

	html_cursor_reset_relative (engine->cursor);
}


/* Setting style of a single paragraph -- undo.  */

static void
set_clueflow_style_undo_closure_destroy (gpointer closure)
{
	free_props ((OriginalClueFlowProps *) closure);
}

static void
set_clueflow_style_undo (HTMLEngine *engine,
			 gpointer closure)
{
	HTMLClueFlow *para;
	HTMLUndoAction *redo_action;
	OriginalClueFlowProps *redo_props;

	para = get_current_para (engine);
	g_return_if_fail (para != NULL);

	redo_props = get_props (para);

	set_props (para,  engine, (OriginalClueFlowProps *) closure);

	html_engine_make_cursor_visible (engine);

	redo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_redo,
					    set_clueflow_style_redo_closure_destroy,
					    redo_props,
					    html_cursor_get_relative (engine->cursor));

	html_undo_add_redo_action (engine->undo, redo_action);
	html_cursor_reset_relative (engine->cursor);
}


/* Setting style of a single paragraph -- do.  */

static gboolean
set_clueflow_style (HTMLEngine *engine,
		    HTMLClueFlowStyle style,
		    HTMLHAlignType alignment,
		    gint indentation,
		    HTMLEngineSetClueFlowStyleMask mask)
{
	HTMLClueFlow *para;
	HTMLUndoAction *undo_action;
	OriginalClueFlowProps *undo_props;

	para = get_current_para (engine);
	g_return_val_if_fail (para != NULL, FALSE);

	undo_props = get_props (para);

	if (mask & HTML_ENGINE_SET_CLUEFLOW_STYLE)
		html_clueflow_set_style (para, engine, style);

	if (mask & HTML_ENGINE_SET_CLUEFLOW_ALIGNMENT)
		html_clueflow_set_halignment (para, engine, alignment);

	if (mask & HTML_ENGINE_SET_CLUEFLOW_INDENTATION)
		html_clueflow_indent (para, engine, indentation);

	html_engine_make_cursor_visible (engine);

	undo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_undo,
					    set_clueflow_style_undo_closure_destroy,
					    undo_props,
					    html_cursor_get_relative (engine->cursor));
	html_undo_add_undo_action (engine->undo, undo_action);
	html_cursor_reset_relative (engine->cursor);

	return TRUE;
}


/* Setting paragraph style in a region -- redo/undo data.  */
/* We need to declare all this in advance as redo needs to setup undo and vice
   versa.  */

/* Undo data.  */
struct _SetClueFlowStyleUndoData {
	/* This contains the original styles of the various HTMLClueFlows we
           find in the region.  */
	GList *original_styles;			/* HTMLClueFlowStyle */

	/* This is the style that the "do" operation has set.  It is used to set
           up the redo operation. */
	HTMLClueFlowStyle style_set;
};
typedef struct _SetClueFlowStyleUndoData SetClueFlowStyleUndoData;

static void set_clueflow_style_in_region_redo (HTMLEngine *engine, gpointer closure);
static void set_clueflow_style_in_region_redo_closure_destroy (gpointer closure);

/* Redo data.  */
struct _SetClueFlowStyleRedoData {
	/* Number of paragraphs to restore, total.  */
	guint total;

	/* Style to set.  */
	HTMLClueFlowStyle style;
};
typedef struct _SetClueFlowStyleRedoData SetClueFlowStyleRedoData;

static void set_clueflow_style_in_region_undo (HTMLEngine *engine, gpointer closure);
static void set_clueflow_style_in_region_undo_closure_destroy (gpointer closure);


/* Setting paragraph style in a region -- redo.  */

/* This is the data used in the redo "forall" cycle.  */
struct _SetClueFlowStyleRedoForallData {
	/* Redo data.  */
	SetClueFlowStyleRedoData *redo_data;

	/* Engine.  */
	HTMLEngine *engine;

	/* Whether we should be setting the style for this paragraph or not.  */
	gboolean do_set_style;

	/* Number of paragraphs restored so far.  */
	guint count;

	/* Last paragraph of which we have changed the style.  */
	HTMLObject *previous_clueflow;

	/* This contains the original styles of the various HTMLClueFlows we
           modify in the region, in reverse order.  It is used to undo the
           redone operation.  */
	GList *original_styles;			/* HTMLClueFlowStyle */
};
typedef struct _SetClueFlowStyleRedoForallData SetClueFlowStyleRedoForallData;

static void
set_clueflow_style_in_region_redo_closure_destroy (gpointer closure)
{
	SetClueFlowStyleRedoData *redo_data;

	redo_data = (SetClueFlowStyleRedoData *) closure;

	/* (Nothing to free in the structure.)  */

	g_free (redo_data);
}

static void
set_clueflow_style_in_region_redo_forall (HTMLObject *self,
					  gpointer closure)
{
	SetClueFlowStyleRedoForallData *forall_data;
	SetClueFlowStyleRedoData *redo_data;
	HTMLObject *parent;

	forall_data = (SetClueFlowStyleRedoForallData *) closure;
	redo_data = forall_data->redo_data;

	if (self == forall_data->engine->cursor->object)
		forall_data->do_set_style = TRUE;
	else if (! forall_data->do_set_style)
		return;

	parent = self->parent;
	if (parent == NULL)
		return;
	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return;
	if (parent == forall_data->previous_clueflow)
		return;

	forall_data->original_styles = g_list_prepend (forall_data->original_styles,
						       GINT_TO_POINTER (HTML_CLUEFLOW (parent)->style));
	html_clueflow_set_style (HTML_CLUEFLOW (parent), forall_data->engine, redo_data->style);

	forall_data->previous_clueflow = parent;
	forall_data->count++;

	if (forall_data->count == redo_data->total)
		forall_data->do_set_style = FALSE;
}

static void
set_clueflow_style_in_region_redo (HTMLEngine *engine,
				   gpointer closure)
{
	SetClueFlowStyleRedoData *redo_data;
	SetClueFlowStyleUndoData *undo_data;
	SetClueFlowStyleRedoForallData *forall_data;
	HTMLUndoAction *undo_action;

	redo_data = (SetClueFlowStyleRedoData *) closure;

	forall_data = g_new (SetClueFlowStyleRedoForallData, 1);
	forall_data->redo_data = redo_data;
	forall_data->engine = engine;
	forall_data->do_set_style = FALSE;
	forall_data->count = 0;
	forall_data->previous_clueflow = NULL;
	forall_data->original_styles = NULL;

	html_object_forall (engine->clue, set_clueflow_style_in_region_redo_forall, forall_data);

	undo_data = g_new (SetClueFlowStyleUndoData, 1);
	undo_data->style_set = redo_data->style;
	undo_data->original_styles = g_list_reverse (forall_data->original_styles);

	undo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_in_region_undo,
					    set_clueflow_style_in_region_undo_closure_destroy,
					    undo_data,
					    html_cursor_get_relative (engine->cursor));
	html_undo_add_undo_action (engine->undo, undo_action);
	html_cursor_reset_relative (engine->cursor);

	g_free (forall_data);
}


/* Setting paragraph style in a region -- undo.  */

/* This is the data used in the undo "forall" cycle.  */
struct _SetClueFlowStyleUndoForallData {
	/* The engine.  */
	HTMLEngine *engine;

	/* Pointer to the next style to set.  If NULL, we have not reached the
           first element yet (i.e. the cursor element).  */
	GList *stylep;

	/* The last clueflow for which we have set the style.  */
	HTMLObject *previous_clueflow;

	/* The undo data.  */
	SetClueFlowStyleUndoData *undo_data;
};
typedef struct _SetClueFlowStyleUndoForallData SetClueFlowStyleUndoForallData;

static void
set_clueflow_style_in_region_undo_closure_destroy (gpointer closure)
{
	SetClueFlowStyleUndoData *undo_data;

	undo_data = (SetClueFlowStyleUndoData *) closure;

	g_list_free (undo_data->original_styles);

	g_free (undo_data);
}

static void
set_clueflow_style_in_region_undo_forall (HTMLObject *self,
					  gpointer closure)
{
	SetClueFlowStyleUndoForallData *forall_data;
	SetClueFlowStyleUndoData *undo_data;
	HTMLObject *parent;

	forall_data = (SetClueFlowStyleUndoForallData *) closure;
	undo_data = forall_data->undo_data;

	if (forall_data->engine->cursor->object == self)
		forall_data->stylep = undo_data->original_styles;
	else if (forall_data->stylep == NULL)
		return;

	parent = self->parent;
	if (parent == NULL)
		return;
	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return;
	if (parent == forall_data->previous_clueflow)
		return;

	html_clueflow_set_style (HTML_CLUEFLOW (parent), forall_data->engine,
				 (HTMLClueFlowStyle) forall_data->stylep->data);

	forall_data->stylep = forall_data->stylep->next;
	forall_data->previous_clueflow = parent;
}

static void
set_clueflow_style_in_region_undo (HTMLEngine *engine,
				   gpointer closure)
{
	SetClueFlowStyleUndoData *undo_data;
	SetClueFlowStyleUndoForallData *forall_data;
	SetClueFlowStyleRedoData *redo_data;
	HTMLUndoAction *redo_action;

	undo_data = (SetClueFlowStyleUndoData *) closure;
	forall_data = g_new (SetClueFlowStyleUndoForallData, 1);

	forall_data->engine = engine;
	forall_data->undo_data = undo_data;
	forall_data->stylep = NULL;
	forall_data->previous_clueflow = NULL;

	html_object_forall (engine->clue, set_clueflow_style_in_region_undo_forall, forall_data);

	g_free (forall_data);

	redo_data = g_new (SetClueFlowStyleRedoData, 1);
	redo_data->total = g_list_length (undo_data->original_styles);
	redo_data->style = undo_data->style_set;

	redo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_in_region_redo,
					    set_clueflow_style_in_region_redo_closure_destroy,
					    redo_data,
					    html_cursor_get_relative (engine->cursor));
	html_undo_add_redo_action (engine->undo, redo_action);
	html_cursor_reset_relative (engine->cursor);
}


/* Setting paragraph style in a region -- do.  */

/* This is the data used to execute the operation.  */
struct _SetClueFlowStyleForallData {
	/* The engine to which we belong.  */
	HTMLEngine *engine;

	/* Style to set.  */
	HTMLClueFlowStyle style;

	/* Indentation delta.  */
	gint indentation;

	/* Alignment.  */
	HTMLHAlignType alignment;

	/* Mask of attributes that we really want to change.  */
	HTMLEngineSetClueFlowStyleMask mask;

	/* Previous parent checked.  */
	HTMLObject *previous_clueflow;

	SetClueFlowStyleUndoData *undo_data;
};
typedef struct _SetClueFlowStyleForallData SetClueFlowStyleForallData;

static void
set_clueflow_style_in_region_forall (HTMLObject *object,
				     gpointer data)
{
	SetClueFlowStyleForallData *forall_data;
	SetClueFlowStyleUndoData *undo_data;
	HTMLObject *parent;

	forall_data = (SetClueFlowStyleForallData *) data;
	undo_data = forall_data->undo_data;

	if (! object->selected)
		return;

	parent = object->parent;

	if (parent == NULL)
		return;
	if (HTML_OBJECT_TYPE (parent) != HTML_TYPE_CLUEFLOW)
		return;
	if (parent == forall_data->previous_clueflow)
		return;

	undo_data->original_styles = g_list_prepend (undo_data->original_styles,
						     GINT_TO_POINTER (HTML_CLUEFLOW (parent)->style));

	if (forall_data->mask & HTML_ENGINE_SET_CLUEFLOW_STYLE)
		html_clueflow_set_style (HTML_CLUEFLOW (parent),
					 forall_data->engine, forall_data->style);

	if (forall_data->mask & HTML_ENGINE_SET_CLUEFLOW_ALIGNMENT)
		html_clueflow_set_halignment (HTML_CLUEFLOW (parent),
					      forall_data->engine, forall_data->alignment);

	if (forall_data->mask & HTML_ENGINE_SET_CLUEFLOW_INDENTATION)
		html_clueflow_indent (HTML_CLUEFLOW (parent),
				      forall_data->engine, forall_data->indentation);

	forall_data->previous_clueflow = parent;
}

static gboolean
set_clueflow_style_in_region (HTMLEngine *engine,
			      HTMLClueFlowStyle style,
			      HTMLHAlignType alignment,
			      gint indentation,
			      HTMLEngineSetClueFlowStyleMask mask)
{
	HTMLUndoAction *undo_action;
	SetClueFlowStyleForallData *data;
	SetClueFlowStyleUndoData *undo_data;

	undo_data = g_new (SetClueFlowStyleUndoData, 1);
	undo_data->original_styles = NULL;
	undo_data->style_set = style;

	data = g_new (SetClueFlowStyleForallData, 1);
	data->engine = engine;
	data->style = style;
	data->alignment = alignment;
	data->indentation = indentation;
	data->mask = mask;
	data->previous_clueflow = NULL;
	data->undo_data = undo_data;

	html_object_forall (engine->clue, set_clueflow_style_in_region_forall, data);

	undo_data->original_styles = g_list_reverse (undo_data->original_styles);

	g_free (data);

	/* FIXME i18n */
	undo_action = html_undo_action_new ("paragraph style change",
					    set_clueflow_style_in_region_undo,
					    set_clueflow_style_in_region_undo_closure_destroy,
					    undo_data,
					    html_cursor_get_relative (engine->cursor));
	html_undo_add_undo_action (engine->undo, undo_action);
	html_cursor_reset_relative (engine->cursor);

	return TRUE;
}


/**
 * html_engine_set_clueflow_style:
 * @engine: An HTMLEngine
 * @style: Paragraph style to set
 * @indentation: Indentation delta: if the value is positive, all the paragraphs
 * increase their indentation by the specified amount.  If the value is negative,
 * they decrease their indentation by the specified amount.
 * 
 * Set the paragraph style.  This works as in an editor: if there is a
 * selection, the paragraphs in the whole selection changes style.  If there is
 * no selection, only the current paragraph changes style.
 * 
 * Return value: %TRUE if the style actually changed; %FALSE otherwise.
 **/
gboolean
html_engine_set_clueflow_style (HTMLEngine *engine,
				HTMLClueFlowStyle style,
				HTMLHAlignType alignment,
				gint indentation,
				HTMLEngineSetClueFlowStyleMask mask)
{
	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_ENGINE (engine), FALSE);
	g_return_val_if_fail (engine->clue != NULL, FALSE);

	if (engine->active_selection)
		return set_clueflow_style_in_region (engine, style, alignment, indentation, mask);
	else
		return set_clueflow_style (engine, style, alignment, indentation, mask);
}
