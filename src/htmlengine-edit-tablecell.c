/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2001 Ximian, Inc.
    Authors: Radek Doulik

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
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlobject.h"
#include "htmltablecell.h"
#include "htmlundo.h"

HTMLTableCell *
html_engine_get_table_cell (HTMLEngine *e)
{
	g_assert (HTML_IS_ENGINE (e));

	g_return_val_if_fail (e->cursor->object->parent && e->cursor->object->parent->parent
			      && HTML_IS_TABLE_CELL (e->cursor->object->parent->parent), NULL);

	return HTML_TABLE_CELL (e->cursor->object->parent->parent);
}

typedef enum {
	HTML_TABLE_CELL_BGCOLOR,
	HTML_TABLE_CELL_BGPIXMAP,
	HTML_TABLE_CELL_NOWRAP,
	HTML_TABLE_CELL_HEADING,
	HTML_TABLE_CELL_HALIGN,
	HTML_TABLE_CELL_VALIGN,
	HTML_TABLE_CELL_WIDTH,
} HTMLTableCellAttrType;

union _HTMLTableCellUndoAttr {
	gboolean nowrap;
	gboolean heading;

	gchar *pixmap;

	struct {
		gint width;
		gboolean percent;
	} width;

	struct {
		GdkColor color;
		gboolean has_bg_color;
	} color;

	HTMLHAlignType halign;
	HTMLHAlignType valign;
};
typedef union _HTMLTableCellUndoAttr HTMLTableCellUndoAttr;

struct _HTMLTableCellSetAttrUndo {
	HTMLUndoData data;

	HTMLTableCellUndoAttr attr;
	HTMLTableCellAttrType type;
};
typedef struct _HTMLTableCellSetAttrUndo HTMLTableCellSetAttrUndo;

static void
attr_destroy (HTMLUndoData *undo_data)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	switch (data->type) {
	case HTML_TABLE_CELL_BGPIXMAP:
		g_free (data->attr.pixmap);
		break;
	default:
		;
	}
}

static HTMLTableCellSetAttrUndo *
attr_undo_new (HTMLTableCellAttrType type)
{
	HTMLTableCellSetAttrUndo *undo = g_new (HTMLTableCellSetAttrUndo, 1);

	html_undo_data_init (HTML_UNDO_DATA (undo));
	undo->data.destroy = attr_destroy;
	undo->type         = type;

	return undo;
}

/*
 * bg color
 *
 */

static void table_cell_set_bg_color (HTMLEngine *e, HTMLTableCell *cell, GdkColor *c, HTMLUndoDirection dir);

static void
table_cell_set_bg_color_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_bg_color (e, html_engine_get_table_cell (e), data->attr.color.has_bg_color
				 ? &data->attr.color.color : NULL, html_undo_direction_reverse (dir));
}

static void
table_cell_set_bg_color (HTMLEngine *e, HTMLTableCell *cell, GdkColor *c, HTMLUndoDirection dir)
{
	HTMLTableCellSetAttrUndo *undo;

	undo = attr_undo_new (HTML_TABLE_CELL_BGCOLOR);
	undo->attr.color.color        = cell->bg;
	undo->attr.color.has_bg_color = cell->have_bg;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set cell background color", table_cell_set_bg_color_undo_action,
						    HTML_UNDO_DATA (undo), html_cursor_get_position (e->cursor)), dir);

	html_object_set_bg_color (HTML_OBJECT (cell), c);
	html_engine_queue_draw (e, HTML_OBJECT (cell));
}

void
html_engine_table_cell_set_bg_color (HTMLEngine *e, HTMLTableCell *cell, GdkColor *c)
{
	table_cell_set_bg_color (e, cell, c, HTML_UNDO_UNDO);
}

/*
 * bg pixmap
 *
 */

static void table_cell_set_bg_pixmap (HTMLEngine *e, HTMLTableCell *cell, gchar *url, HTMLUndoDirection dir);

static void
table_cell_set_bg_pixmap_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_bg_pixmap (e, html_engine_get_table_cell (e), data->attr.pixmap, html_undo_direction_reverse (dir));
}

static void
table_cell_set_bg_pixmap (HTMLEngine *e, HTMLTableCell *cell, gchar *url, HTMLUndoDirection dir)
{
	HTMLImagePointer *iptr;
	HTMLTableCellSetAttrUndo *undo;

	undo = attr_undo_new (HTML_TABLE_CELL_BGCOLOR);
	undo->attr.pixmap = cell->have_bgPixmap ? g_strdup (cell->bgPixmap->url) : NULL;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set cell background pixmap", table_cell_set_bg_pixmap_undo_action,
						    HTML_UNDO_DATA (undo), html_cursor_get_position (e->cursor)), dir);

	iptr = cell->bgPixmap;
	cell->bgPixmap = url ? html_image_factory_register (e->image_factory, NULL, url) : NULL;
	if (cell->have_bgPixmap && iptr)
		html_image_factory_unregister (e->image_factory, iptr, NULL);
	cell->have_bgPixmap = url ? TRUE : FALSE;
	html_engine_queue_draw (e, HTML_OBJECT (cell));
}

void
html_engine_table_cell_set_bg_pixmap (HTMLEngine *e, HTMLTableCell *cell, gchar *url)
{
	table_cell_set_bg_pixmap (e, cell, url, HTML_UNDO_UNDO);
}

/*
 * halign
 *
 */

void
html_engine_table_cell_set_halign (HTMLEngine *e, HTMLTableCell *cell, HTMLHAlignType halign)
{
	HTML_CLUE (cell)->halign = halign;
	html_engine_schedule_update (e);
}

void
html_engine_table_cell_set_valign (HTMLEngine *e, HTMLTableCell *cell, HTMLVAlignType valign)
{
	HTML_CLUE (cell)->valign = valign;
	html_engine_schedule_update (e);
}

void
html_engine_table_cell_set_no_wrap (HTMLEngine *e, HTMLTableCell *cell, gboolean no_wrap)
{
	if (cell->no_wrap != no_wrap) {
		cell->no_wrap = no_wrap;
		html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
		html_engine_schedule_update (e);
	}
}

void
html_engine_table_cell_set_heading (HTMLEngine *e, HTMLTableCell *cell, gboolean heading)
{
	if (cell->heading != heading) {
		cell->heading = heading;
		html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
		html_object_change_set_down (HTML_OBJECT (cell), HTML_CHANGE_ALL);
		html_engine_schedule_update (e);
	}
}

void
html_engine_table_cell_set_width (HTMLEngine *e, HTMLTableCell *cell, gint width, gboolean percent)
{
	cell->percent_width = percent;
	cell->fixed_width = width;
	if (width && !percent)
		HTML_OBJECT (cell)->flags |= HTML_OBJECT_FLAG_FIXEDWIDTH;
	else
		HTML_OBJECT (cell)->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
	html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
	html_engine_schedule_update (e);
}
