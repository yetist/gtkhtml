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

#include <config.h>
#include "htmlclueflow.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmltable.h"
#include "htmltablepriv.h"
#include "htmltablecell.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-table.h"

static HTMLTableCell *
new_cell (HTMLEngine *e)
{
	HTMLObject    *cell;
	HTMLObject    *text;
	HTMLObject    *flow;
	
	cell  = html_table_cell_new (0, 1, 1, 1);
	flow  = html_clueflow_new (HTML_CLUEFLOW_STYLE_NORMAL, 0);
	text  = html_engine_new_text_empty (e);

	html_clue_append (HTML_CLUE (flow), text);
	html_clue_append (HTML_CLUE (cell), flow);

	return HTML_TABLE_CELL (cell);
}

void
html_engine_insert_table_1_1 (HTMLEngine *e)
{
	HTMLObject    *flow;
	HTMLObject    *table;

	table = html_table_new (0, 0, 1, 2, 1);

	html_table_add_cell (HTML_TABLE (table), new_cell (e));

	flow  = html_clueflow_new (HTML_CLUEFLOW_STYLE_NORMAL, 0);
	html_clue_append (HTML_CLUE (flow), table);

	html_engine_append_object (e, flow, 2, 2);
}

void
html_engine_insert_table_column (HTMLEngine *e, gboolean after)
{
	HTMLTable *t;
	gint r, c, nc, col, delta = 0;

	t = HTML_TABLE (html_object_nth_parent (e->cursor->object, 3));
	if (!t)
		return;

	html_engine_freeze (e);

	col = HTML_TABLE_CELL (html_object_nth_parent (e->cursor->object, 2))->col + (after ? 1 : 0);
	nc  = t->totalCols + 1;
	html_table_alloc_cell (t, 0, t->totalCols);

	for (r = 0; r < t->totalRows; r ++) {
		for (c = nc - 1; c > col; c --) {
			HTMLTableCell *cell = t->cells [r][c - 1];

			if (cell->col >= col) {
				if (cell->row == r && cell->col == c - 1)
					html_table_cell_set_position (cell, r, c);
				t->cells [r][c] = cell;
				t->cells [r][c - 1] = NULL;
			}
		}
		if (!t->cells [r][col]) {
			html_table_set_cell (t, r, col, new_cell (e));
			html_table_cell_set_position (t->cells [r][col], r, col);
			delta ++;
		}
	}
	if (!after)
		e->cursor->position += delta;
	html_object_change_set (HTML_OBJECT (t), HTML_CHANGE_ALL);

	// insert_column_setup_undo ();

	html_engine_thaw (e);
}

void
html_engine_delete_table_column (HTMLEngine *e)
{
	HTMLTable *t;
	HTMLTableCell *cell;
	HTMLTableCell **column;
	HTMLObject *co;
	gint r, c, col, delta = 0;

	t = HTML_TABLE (html_object_nth_parent (e->cursor->object, 3));
	if (!t ||  t->totalCols < 2)
		return;

	html_engine_freeze (e);

	cell   = HTML_TABLE_CELL (html_object_nth_parent (e->cursor->object, 2));
	col    = cell->col;
	column = g_new0 (HTMLTableCell *, t->totalRows);

	/* move cursor after/before this column (always keep it in table!) */
	do {
		if (col != t->totalCols - 1)
			html_cursor_forward (e->cursor, e);
		else
			html_cursor_backward (e->cursor, e);
		co = html_object_nth_parent (e->cursor->object, 2);
	} while (co && co->parent == HTML_OBJECT (t)
		 && HTML_OBJECT_TYPE (co) == HTML_TYPE_TABLECELL && HTML_TABLE_CELL (co)->col == col);

	for (r = 0; r < t->totalRows; r ++) {
		cell = t->cells [r][col];

		/* remove & keep old one */
		if (cell && cell->col == col) {
			HTML_OBJECT (cell)->parent = NULL;
			column [r] = cell;
			t->cells [r][col] = NULL;
		}

		for (c = col + 1; c < t->totalCols; c ++) {
			cell = t->cells [r][c];
			if (cell && cell->col != col) {
				if (cell->row == r && cell->col == c)
					html_table_cell_set_position (cell, r, c - 1);
				t->cells [r][c - 1] = cell;
				t->cells [r][c]     = NULL;
			}
		}
	}

	t->totalCols --;

	html_object_change_set (HTML_OBJECT (t), HTML_CHANGE_ALL);

	// delete_column_setup_undo ();

	html_engine_thaw (e);
}
