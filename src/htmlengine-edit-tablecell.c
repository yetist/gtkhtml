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
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlobject.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmlundo.h"

HTMLTableCell *
html_engine_get_table_cell (HTMLEngine *e)
{
	g_assert (HTML_IS_ENGINE (e));

	if (!e->cursor->object->parent || !e->cursor->object->parent->parent
	    || !HTML_IS_TABLE_CELL (e->cursor->object->parent->parent))
		return NULL;

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
	gboolean no_wrap;
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
table_cell_set_bg_color_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
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
						    HTML_UNDO_DATA (undo),
						    html_cursor_get_position (e->cursor),
						    html_cursor_get_position (e->cursor)), dir);

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
table_cell_set_bg_pixmap_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_bg_pixmap (e, html_engine_get_table_cell (e), data->attr.pixmap, html_undo_direction_reverse (dir));
}

static void
table_cell_set_bg_pixmap (HTMLEngine *e, HTMLTableCell *cell, gchar *url, HTMLUndoDirection dir)
{
	HTMLImagePointer *iptr;
	HTMLTableCellSetAttrUndo *undo;

	undo = attr_undo_new (HTML_TABLE_CELL_BGPIXMAP);
	undo->attr.pixmap = cell->have_bgPixmap ? g_strdup (cell->bgPixmap->url) : NULL;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set cell background pixmap", table_cell_set_bg_pixmap_undo_action,
						    HTML_UNDO_DATA (undo),
						    html_cursor_get_position (e->cursor),
						    html_cursor_get_position (e->cursor)), dir);

	iptr = cell->bgPixmap;
	cell->bgPixmap = url ? html_image_factory_register (e->image_factory, NULL, url, TRUE) : NULL;
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

static void table_cell_set_halign (HTMLEngine *e, HTMLTableCell *cell, HTMLHAlignType halign, HTMLUndoDirection dir);

static void
table_cell_set_halign_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_halign (e, html_engine_get_table_cell (e), data->attr.halign, html_undo_direction_reverse (dir));
}

static void
table_cell_set_halign (HTMLEngine *e, HTMLTableCell *cell, HTMLHAlignType halign, HTMLUndoDirection dir)
{
	HTMLTableCellSetAttrUndo *undo;

	undo = attr_undo_new (HTML_TABLE_CELL_HALIGN);
	undo->attr.halign = HTML_CLUE (cell)->halign;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set cell horizontal align", table_cell_set_halign_undo_action,
						    HTML_UNDO_DATA (undo),
						    html_cursor_get_position (e->cursor),
						    html_cursor_get_position (e->cursor)), dir);

	HTML_CLUE (cell)->halign = halign;
	html_engine_schedule_update (e);
}

void
html_engine_table_cell_set_halign (HTMLEngine *e, HTMLTableCell *cell, HTMLHAlignType halign)
{
	table_cell_set_halign (e, cell, halign, HTML_UNDO_UNDO);
}

/*
 * valign
 *
 */

static void table_cell_set_valign (HTMLEngine *e, HTMLTableCell *cell, HTMLVAlignType valign, HTMLUndoDirection dir);

static void
table_cell_set_valign_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_valign (e, html_engine_get_table_cell (e), data->attr.valign, html_undo_direction_reverse (dir));
}

static void
table_cell_set_valign (HTMLEngine *e, HTMLTableCell *cell, HTMLVAlignType valign, HTMLUndoDirection dir)
{
	HTMLTableCellSetAttrUndo *undo;

	undo = attr_undo_new (HTML_TABLE_CELL_VALIGN);
	undo->attr.valign = HTML_CLUE (cell)->valign;
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Set cell vertical align", table_cell_set_valign_undo_action,
						    HTML_UNDO_DATA (undo),
						    html_cursor_get_position (e->cursor),
						    html_cursor_get_position (e->cursor)), dir);

	HTML_CLUE (cell)->valign = valign;
	html_engine_schedule_update (e);
}

void
html_engine_table_cell_set_valign (HTMLEngine *e, HTMLTableCell *cell, HTMLVAlignType valign)
{
	table_cell_set_valign (e, cell, valign, HTML_UNDO_UNDO);
}

/*
 * nowrap
 *
 */

static void table_cell_set_no_wrap (HTMLEngine *e, HTMLTableCell *cell, gboolean no_wrap, HTMLUndoDirection dir);

static void
table_cell_set_no_wrap_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_no_wrap (e, html_engine_get_table_cell (e), data->attr.no_wrap, html_undo_direction_reverse (dir));
}

static void
table_cell_set_no_wrap (HTMLEngine *e, HTMLTableCell *cell, gboolean no_wrap, HTMLUndoDirection dir)
{
	if (cell->no_wrap != no_wrap) {
		HTMLTableCellSetAttrUndo *undo;

		undo = attr_undo_new (HTML_TABLE_CELL_NOWRAP);
		undo->attr.no_wrap = cell->no_wrap;
		html_undo_add_action (e->undo,
				      html_undo_action_new ("Set cell wrapping", table_cell_set_no_wrap_undo_action,
							    HTML_UNDO_DATA (undo),
							    html_cursor_get_position (e->cursor),
							    html_cursor_get_position (e->cursor)), dir);
		cell->no_wrap = no_wrap;
		html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
		html_engine_schedule_update (e);
	}
}

void
html_engine_table_cell_set_no_wrap (HTMLEngine *e, HTMLTableCell *cell, gboolean no_wrap)
{
	table_cell_set_no_wrap (e, cell, no_wrap, HTML_UNDO_UNDO);
}

/*
 * heading
 *
 */

static void table_cell_set_heading (HTMLEngine *e, HTMLTableCell *cell, gboolean heading, HTMLUndoDirection dir);

static void
table_cell_set_heading_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_heading (e, html_engine_get_table_cell (e), data->attr.heading, html_undo_direction_reverse (dir));
}

static void
table_cell_set_heading (HTMLEngine *e, HTMLTableCell *cell, gboolean heading, HTMLUndoDirection dir)
{
	if (cell->heading != heading) {
		HTMLTableCellSetAttrUndo *undo;

		undo = attr_undo_new (HTML_TABLE_CELL_HEADING);
		undo->attr.heading = cell->heading;
		html_undo_add_action (e->undo,
				      html_undo_action_new ("Set cell style", table_cell_set_heading_undo_action,
							    HTML_UNDO_DATA (undo),
							    html_cursor_get_position (e->cursor),
							    html_cursor_get_position (e->cursor)), dir);

		cell->heading = heading;
		html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
		html_object_change_set_down (HTML_OBJECT (cell), HTML_CHANGE_ALL);
		html_engine_schedule_update (e);
	}
}

void
html_engine_table_cell_set_heading (HTMLEngine *e, HTMLTableCell *cell, gboolean heading)
{
	table_cell_set_heading (e, cell, heading, HTML_UNDO_UNDO);
}

/*
 * width
 *
 */

static void table_cell_set_width (HTMLEngine *e, HTMLTableCell *cell, gint width, gboolean percent, HTMLUndoDirection dir);
static void
table_cell_set_width_undo_action (HTMLEngine *e, HTMLUndoData *undo_data, HTMLUndoDirection dir, guint position_after)
{
	HTMLTableCellSetAttrUndo *data = (HTMLTableCellSetAttrUndo *) undo_data;

	table_cell_set_width (e, html_engine_get_table_cell (e),
			      data->attr.width.width, data->attr.width.percent, html_undo_direction_reverse (dir));
}

static void
table_cell_set_width (HTMLEngine *e, HTMLTableCell *cell, gint width, gboolean percent, HTMLUndoDirection dir)
{
	if (cell->percent_width != percent || cell->fixed_width != width) {
		HTMLTableCellSetAttrUndo *undo;

		undo = attr_undo_new (HTML_TABLE_CELL_WIDTH);
		undo->attr.width.width = cell->fixed_width;
		undo->attr.width.percent = cell->percent_width;
		html_undo_add_action (e->undo,
				      html_undo_action_new ("Set cell style", table_cell_set_width_undo_action,
							    HTML_UNDO_DATA (undo),
							    html_cursor_get_position (e->cursor),
							    html_cursor_get_position (e->cursor)), dir);

		cell->percent_width = percent;
		cell->fixed_width = width;
		if (width && !percent)
			HTML_OBJECT (cell)->flags |= HTML_OBJECT_FLAG_FIXEDWIDTH;
		else
			HTML_OBJECT (cell)->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
		html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL_CALC);
		html_engine_schedule_update (e);
	}
}

void
html_engine_table_cell_set_width (HTMLEngine *e, HTMLTableCell *cell, gint width, gboolean percent)
{
	table_cell_set_width (e, cell, width, percent, HTML_UNDO_UNDO);
}

void
html_engine_delete_table_cell_contents (HTMLEngine *e)
{
	HTMLTableCell *cell;

	cell = html_engine_get_table_cell (e);
	if (!cell)
		return;

	html_engine_prev_cell (e);
	html_cursor_forward (e->cursor, e);
	html_engine_set_mark (e);
	html_engine_next_cell (e, FALSE);
	html_cursor_backward (e->cursor, e);
	html_engine_delete (e);
}

static void
move_cell_rd (HTMLTable *t, HTMLTableCell *cell, gint rs, gint cs)
{
	gint r, c;

	g_assert ((rs == 0 && cs > 0) || (cs == 0 && rs > 0));
	/* printf ("move %dx%d --> %dx%d\n", cell->row, cell->col, cell->row + rs, cell->col + cs); */
	for (r = cell->row + cell->rspan - 1; r >= cell->row; r --)
		for (c = cell->col + cell->cspan - 1; c >= cell->col; c --) {
			if (r > cell->row + cell->rspan - 1 - rs || c > cell->col + cell->cspan - 1 - cs) {
				gint nr = rs + r - (rs ? cell->rspan : 0), nc = cs + c - (cs ? cell->cspan : 0);

				/* printf ("exchange: %dx%d <--> %dx%d (%p)\n", rs + r, cs + c, nr, nc, t->cells [rs][nc]); */
				t->cells [nr][nc] = t->cells [rs + r][cs + c];
				if (t->cells [nr][nc])
					html_table_cell_set_position (t->cells [nr][nc], nr, nc);
				t->cells [rs + r][cs + c] = cell;
			} else {
				if (r >= cell->row + rs && c >= cell->col + cs) {
					if (t->cells [rs + r][cs + c] && t->cells [rs + r][cs + c]->col == cs + c && t->cells [rs + r][cs + c]->row == rs + r) {
						/* printf ("move destroy: %dx%d\n", rs + r, cs + c); */
						html_object_destroy (HTML_OBJECT (t->cells [rs + r][cs + c]));
					}
					t->cells [r][c] = NULL;
				}
				t->cells [rs + r][cs + c] = cell;
			}
			/* printf ("cell %dx%d <--\n", rs + r, cs + c); */
		}
	/* printf ("set  %dx%d --> %dx%d\n", cell->row, cell->col, cell->row + rs, cell->col + cs); */
	html_table_cell_set_position (cell, cell->row + rs, cell->col + cs);
}

static void
expand_cspan (HTMLEngine *e, HTMLTableCell *cell, gint cspan, HTMLUndoDirection dir)
{
	HTMLTable *table = HTML_TABLE (HTML_OBJECT (cell)->parent);
	gint r, c, *move_rows, max_move, add_cols;

	move_rows = g_new0 (gint, cell->rspan);
	for (r = cell->row; r < cell->row + cell->rspan; r ++)
		for (c = cell->col + cell->cspan; c < MIN (cell->col + cspan, table->totalCols); c ++)
			if (table->cells [r][c] && !html_table_cell_is_empty (table->cells [r][c]) && move_rows [r - cell->row] == 0)
				move_rows [r - cell->row] = cspan - (c - cell->col);

	max_move = 0;
	for (r = 0; r < cell->rspan; r ++)
		if (move_rows [r] > max_move)
			max_move = move_rows [r];

	add_cols = MAX (max_move, cspan - (table->totalCols - cell->col));
	/* printf ("max move: %d add: %d\n", max_move, add_cols); */
	for (c = 0; c < add_cols; c ++)
		html_table_insert_column (table, e, table->totalCols, NULL, dir);

	if (max_move > 0) {
		for (c = table->totalCols - max_move - 1; c >= cell->col + cspan - max_move; c --)
			for (r = cell->row; r < cell->row + cell->rspan; r ++) {
				HTMLTableCell *ccell = table->cells [r][c];

				if (ccell && ccell->col == c) {
					move_cell_rd (table, ccell, 0, max_move);
					r += ccell->rspan - 1;
				}
			}
	}

	g_warning ("TODO: keep old content");

	cell->cspan = cspan;
	for (r = cell->row; r < cell->row + cell->rspan; r ++)
		for (c = cell->col; c < cell->col + cell->cspan; c ++)
			table->cells [r][c] = cell;

	html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL);
}

struct _CollapseSpanUndo {
	HTMLUndoData data;

	gint span;
};
typedef struct _CollapseSpanUndo CollapseSpanUndo;
#define COLLAPSE_UNDO(x) ((CollapseSpanUndo *) x)

static HTMLUndoData *
collapse_undo_data_new (gint span)
{
	CollapseSpanUndo *ud = g_new0 (CollapseSpanUndo, 1);

	html_undo_data_init (HTML_UNDO_DATA (ud));
	ud->span = span;

	return HTML_UNDO_DATA (ud);
}

static void
collapse_cspan_undo_action (HTMLEngine *e, HTMLUndoData *data, HTMLUndoDirection dir, guint position_after)
{
	html_engine_freeze (e);
	expand_cspan (e, html_engine_get_table_cell (e), COLLAPSE_UNDO (data)->span, html_undo_direction_reverse (dir));
	html_engine_thaw (e);
}

static void
collapse_cspan_setup_undo (HTMLEngine *e, gint cspan, guint position_before, HTMLUndoDirection dir)
{
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Insert table column", collapse_cspan_undo_action,
						    collapse_undo_data_new (cspan), html_cursor_get_position (e->cursor),
						    position_before),
			      dir);
}

static void
collapse_cspan (HTMLEngine *e, HTMLTableCell *cell, gint cspan, HTMLUndoDirection dir)
{
	HTMLTable *table;
	guint position_before = e->cursor->position;
	gint r, c;

	table = HTML_TABLE (HTML_OBJECT (cell)->parent);
	for (c = cell->col + cspan; c < cell->col + cell->cspan; c ++)
		for (r = cell->row; r < cell->row + cell->rspan; r ++) {
			table->cells [r][c] = NULL;
			html_table_set_cell (table, r, c, html_engine_new_cell (e, table));
			html_table_cell_set_position (table->cells [r][c], r, c);
		}

	collapse_cspan_setup_undo (e, cell->cspan, position_before, dir);
	cell->cspan = cspan;
	html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL);
}

void
html_engine_set_cspan (HTMLEngine *e, gint cspan)
{
	HTMLTableCell *cell = html_engine_get_table_cell (e);

	g_return_if_fail (cspan > 0);
	g_return_if_fail (cell != NULL);

	if (cell->cspan == cspan)
		return;

	html_engine_freeze (e);
	if (cspan > cell->cspan)
		expand_cspan (e, cell, cspan, HTML_UNDO_UNDO);
	else
		collapse_cspan (e, cell, cspan, HTML_UNDO_UNDO);
	html_engine_thaw (e);
}

static void
expand_rspan (HTMLEngine *e, HTMLTableCell *cell, gint rspan, HTMLUndoDirection dir)
{
	HTMLTable *table = HTML_TABLE (HTML_OBJECT (cell)->parent);
	gint r, c, *move_cols, max_move, add_rows;

	move_cols = g_new0 (gint, cell->cspan);
	for (c = cell->col; c < cell->col + cell->cspan; c ++)
		for (r = cell->row + cell->rspan; r < MIN (cell->row + rspan, table->totalRows); r ++)
			if (table->cells [r][c] && !html_table_cell_is_empty (table->cells [r][c]) && move_cols [c - cell->col] == 0)
				move_cols [c - cell->col] = rspan - (r - cell->row);

	max_move = 0;
	for (c = 0; c < cell->cspan; c ++)
		if (move_cols [c] > max_move)
			max_move = move_cols [c];
	add_rows = MAX (max_move, rspan - (table->totalRows - cell->row));
	/* printf ("max move: %d add: %d\n", max_move, add_rows); */
	for (r = 0; r < add_rows; r ++)
		html_table_insert_row (table, e, table->totalRows, NULL, dir);

	if (max_move > 0) {
		for (r = table->totalRows - max_move - 1; r >= cell->row + rspan - max_move; r --)
			for (c = cell->col; c < cell->col + cell->cspan; c ++) {
				HTMLTableCell *ccell = table->cells [r][c];

				if (ccell && ccell->row == r) {
					move_cell_rd (table, ccell, max_move, 0);
					c += ccell->cspan - 1;
				}
			}
	}

	g_warning ("TODO: keep old content");

	cell->rspan = rspan;
	for (r = cell->row; r < cell->row + cell->rspan; r ++)
		for (c = cell->col; c < cell->col + cell->cspan; c ++)
			table->cells [r][c] = cell;

	html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL);
}

static void
collapse_rspan_undo_action (HTMLEngine *e, HTMLUndoData *data, HTMLUndoDirection dir, guint position_after)
{
	html_engine_freeze (e);
	expand_rspan (e, html_engine_get_table_cell (e), COLLAPSE_UNDO (data)->span, html_undo_direction_reverse (dir));
	html_engine_thaw (e);
}

static void
collapse_rspan_setup_undo (HTMLEngine *e, gint rspan, guint position_before, HTMLUndoDirection dir)
{
	html_undo_add_action (e->undo,
			      html_undo_action_new ("Insert table column", collapse_rspan_undo_action,
						    collapse_undo_data_new (rspan), html_cursor_get_position (e->cursor),
						    position_before),
			      dir);
}

static void
collapse_rspan (HTMLEngine *e, HTMLTableCell *cell, gint rspan, HTMLUndoDirection dir)
{
	HTMLTable *table;
	guint position_before = e->cursor->position;
	gint r, c;

	table = HTML_TABLE (HTML_OBJECT (cell)->parent);
	for (r = cell->row + rspan; r < cell->row + cell->rspan; r ++)
		for (c = cell->col; c < cell->col + cell->cspan; c ++) {
			table->cells [r][c] = NULL;
			html_table_set_cell (table, r, c, html_engine_new_cell (e, table));
			html_table_cell_set_position (table->cells [r][c], r, c);
		}

	collapse_rspan_setup_undo (e, cell->rspan, position_before, dir);
	cell->rspan = rspan;
	html_object_change_set (HTML_OBJECT (cell), HTML_CHANGE_ALL);
}

void
html_engine_set_rspan (HTMLEngine *e, gint rspan)
{
	HTMLTableCell *cell = html_engine_get_table_cell (e);

	g_return_if_fail (rspan > 0);
	g_return_if_fail (cell != NULL);

	if (cell->rspan == rspan)
		return;

	html_engine_freeze (e);
	if (rspan > cell->rspan)
		expand_rspan (e, cell, rspan, HTML_UNDO_UNDO);
	else
		collapse_rspan (e, cell, rspan, HTML_UNDO_UNDO);
	html_engine_thaw (e);
}

gboolean
html_engine_cspan_delta (HTMLEngine *e, gint delta)
{
	HTMLTableCell *cell;

	cell = html_engine_get_table_cell (e);
	if (cell) {
		if (cell->cspan + delta > 0) {
			html_engine_set_cspan (e, cell->cspan + delta);
			return TRUE;
		}
	}

	return FALSE;
}

gboolean
html_engine_rspan_delta (HTMLEngine *e, gint delta)
{
	HTMLTableCell *cell;

	cell = html_engine_get_table_cell (e);
	if (cell) {
		if (cell->rspan + delta > 0) {
			html_engine_set_rspan (e, cell->rspan + delta);
			return TRUE;
		}
	}

	return FALSE;
}
