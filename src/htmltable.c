/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
	      (C) 1999 Anders Carlsson (andersca@gnu.org)

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
#include "htmlobject.h"
#include "htmltable.h"


#define a_colinfo(x) (((ColInfo_t *)(table->colInfo)->data)[x])
#define a_coltype(x) (((ColType *)(table->colType)->data)[x])
#define a_columnpos(x) (((gint *)(table->columnPos)->data)[x])
#define a_columnprefpos(x) (((gint *)(table->columnPrefPos)->data)[x])
#define a_colspan(x) (((gint *)(table->colSpan)->data)[x])
#define a_columnopt(x) (((gint *)(table->columnOpt)->data)[x])
#define a_rowheights(x) (((gint *)(table->rowHeights)->data)[x])


HTMLTableClass html_table_class;


static void
add_row_info (HTMLTable *table, gint row, gint colInfoIndex)
{
	table->rowInfo[row].entry[table->rowInfo[row].nrEntries++] = colInfoIndex;
}

static void
add_columns (HTMLTable *table, gint num)
{
	HTMLTableCell **newCells;
	gint r;

	g_print ("adding columns: %d\n", num);

	for (r = 0; r < table->allocRows; r++) {
		newCells = g_malloc (sizeof (HTMLTableCell *) * (table->totalCols + num));
		memcpy (newCells, table->cells[r], table->totalCols * sizeof (HTMLTableCell *));
		memset (newCells + table->totalCols, 0, num * sizeof (HTMLTableCell *));
		/* FIXME: Do destroy instead of free? */
		g_free (table->cells[r]);
		table->cells[r] = newCells;
	}

	table->totalCols += num;
	
}

static void
add_rows (HTMLTable *table, gint num)
{
	gint r;
	HTMLTableCell ***newRows = g_malloc (sizeof (HTMLTableCell **) * (table->allocRows + num));
	memcpy (newRows, table->cells, table->allocRows * sizeof (HTMLTableCell **));
	g_print ("adding rows\n");
	/* FIXME: do destroy instead of free? */
	g_free (table->cells);
	table->cells = newRows;

	for (r = table->allocRows; r < table->allocRows + num; r++) {
		table->cells[r] = g_malloc (sizeof (HTMLTableCell *) * table->totalCols);
		memset (table->cells[r], 0, table->totalCols * sizeof (HTMLTableCell *));
	}

	table->allocRows += num;
}

static void
calc_col_info (HTMLTable *table)
{
	gint r, c;
	gint borderExtra = (table->border == 0) ? 0 : 1;

	gint i, j, totalRowInfos;

	/* Allocate some memory for column info */
	g_array_set_size (table->colInfo, table->totalCols * 2);
	table->rowInfo = g_new (RowInfo_t, table->totalRows);
	table->totalColInfos = 0;

	for (r = 0; r < table->totalRows; r++) {
		table->rowInfo[r].entry = (gint *)g_new (gint, table->totalCols);
		table->rowInfo[r].nrEntries = 0;
		for (c = 0; c < table->totalCols; c++) {
			HTMLTableCell *cell = table->cells[r][c];
			gint min_size;
			gint pref_size;
			gint colInfoIndex;
			ColType col_type;

			if (cell == 0)
				continue;
			if ((c > 0) && (table->cells[r][c-1] == cell))
				continue;
			if ((r > 0) && (table->cells[r-1][c] == cell))
				continue;

			/* calculate minimum size */
			min_size = html_object_calc_min_width (HTML_OBJECT (cell));
			min_size += table->padding * 2 + table->spacing + borderExtra;

			/* calculate preferred pos */
			if (HTML_OBJECT (cell)->percent > 0) {
				pref_size = (HTML_OBJECT (table)->max_width * 
					     HTML_OBJECT (cell)->percent / 100) +
					table->padding * 2 + table->spacing + borderExtra;
				col_type = Percent;
			}
			else if (HTML_OBJECT (cell)->flags
				 & HTML_OBJECT_FLAG_FIXEDWIDTH) {
				pref_size = HTML_OBJECT (cell)->width + 
					table->padding * 2 + table->spacing + borderExtra;
				col_type = Fixed;
			}
			else {
				pref_size = html_object_calc_preferred_width (HTML_OBJECT (cell)) + table->padding * 2+ table->spacing + borderExtra;
				col_type = Variable;
			}

			colInfoIndex = html_table_add_col_info (table, c, cell->cspan,
								min_size, pref_size,
								HTML_OBJECT (table)->max_width, col_type);
			add_row_info (table, r, colInfoIndex);
		}
					
	}

	/* Remove redundant rows */
	totalRowInfos = 1;
	for (i = 1; i < table->totalRows; i++) {
		gboolean unique = TRUE;
		for (j = 0; (j < totalRowInfos) && (unique == TRUE); j++) {
			gint k;
			if (table->rowInfo[i].nrEntries == table->rowInfo[j].nrEntries)
				unique = FALSE;
			else {
				gboolean match = TRUE;
				k = table->rowInfo[i].nrEntries;
				while (k--) {
					if (table->rowInfo[i].entry[k] != 
					    table->rowInfo[j].entry[k]) {
						match = FALSE;
						break;
					}
				}
				
				if (match)
					unique = FALSE;
			}
		}
		if (!unique) {
			g_free (table->rowInfo[i].entry);
		} else {
			if (totalRowInfos != i) {
				table->rowInfo[totalRowInfos].entry = table->rowInfo[i].entry;
				table->rowInfo[totalRowInfos].nrEntries = table->rowInfo[i].nrEntries;
			}
			totalRowInfos++;
		}
	}

	/* Calculate pref width and min width for each row */
	table->_minWidth = 0;
	table->_prefWidth = 0;
	for (i = 0; i < totalRowInfos; i++) {
		gint min = 0;
		gint pref = 0;
		gint j;
		for (j = 0; j < table->rowInfo[i].nrEntries; j++) {
			gint index = table->rowInfo[i].entry[j];
			min += a_colinfo (index).minSize;
			pref += a_colinfo (index).prefSize;
		}
		table->rowInfo[i].minSize = min;
		table->rowInfo[i].prefSize = pref;

		if (table->_minWidth < min) {
			table->_minWidth = min;
		}
		if (table->_prefWidth < pref) {
			table->_prefWidth = pref;
		}
	       
	}

	if (HTML_OBJECT (table)->flags & HTML_OBJECT_FLAG_FIXEDWIDTH) {
		/* Our minimum width is at least our fixed width */
		if (HTML_OBJECT (table)->width > table->_minWidth)
			table->_minWidth = HTML_OBJECT(table)->width;

		/* And our actual width is at least our minimum width */
		if (HTML_OBJECT (table)->width < table->_minWidth)
			HTML_OBJECT (table)->width = table->_minWidth;
	}
}

static gint
scale_selected_columns (HTMLTable *table, gint c_start, gint c_end,
				   gint tooAdd, gboolean *selected)
{
	gint c, c1;
	gint numSelected = 0;
	gint addSize, left;

	if (tooAdd <= 0)
		return tooAdd;
	
	for (c = c_start; c <= c_end; c++) {
		if (selected[c])
			numSelected++;
	}
	if (numSelected < 1) 
		return tooAdd;

	addSize = tooAdd / numSelected;
	left = tooAdd - addSize * numSelected;
	
	for (c = c_start; c <= c_end; c++) {
		if (!selected[c])
			continue;
		tooAdd -= addSize;
		for (c1 = c + 1; c1 <= (gint)table->totalCols; c1++) {
			a_columnopt (c1) += addSize;
			if (left)
				a_columnopt (c1)++;
		}
		if (left) {
			tooAdd--;
			left--;
		}
	}
	return tooAdd;
}

static void
scale_columns (HTMLTable *table, gint c_start, gint c_end, gint tooAdd)
{
	gint r, c;
	gint colspan;
	gint addSize;
	gint minWidth, prefWidth;
	gint totalAllowed, totalRequested;
	gint borderExtra = (table->border == 0) ? 0 : 1;
	gint tableWidth = HTML_OBJECT (table)->width - table->border;

	gint *prefColumnWidth;
	gboolean *fixedCol;
	gboolean *percentCol;
	gboolean *variableCol;

	/* Satisfy fixed width cells */
	for (colspan = 0; colspan <= 1; colspan++) {
		for (r = 0; r < table->totalRows; r++) {
			for (c = c_start; c <= c_end; c++) {
				HTMLTableCell *cell = table->cells[r][c];

				if (cell == 0)
					continue;

				if (r < table->totalRows - 1 && 
				    table->cells[r + 1][c] == cell)
					continue;

				/* Fixed cells only */
				if (!(HTML_OBJECT(cell)->flags
				      & HTML_OBJECT_FLAG_FIXEDWIDTH))
					continue;

				if (colspan == 0) {
					/* colSpan == 1 */
					if (cell->cspan != 1)
						continue;
				}
				else {
					/* colSpan > 1 */
					if (cell->cspan <= 1)
						continue;
					if (c < table->totalCols - 1 &&
					    table->cells[r][c + 1] == cell)
						continue;
				}

				minWidth = a_columnopt (c + 1) - a_columnopt (c + 1 - cell->cspan);
				prefWidth = HTML_OBJECT (cell)->width + table->padding * 2 +
					table->spacing + borderExtra;

				if (prefWidth <= minWidth)
					continue;
				
				addSize = (prefWidth - minWidth);
				
				tooAdd -= addSize;

				if (colspan == 0) {
					gint c1;
					/* Just add this to the column size */
					for (c1 = c + 1; c1 <= table->totalCols; c1++)
						a_columnopt (c1) += addSize;
				}
				else {
					gint c_b = c + 1 - cell->cspan;

					/* Some end-conditions are required here to prevent looping */
					if (c_b < c_start)
						continue;
					if ((c_b == c_start) && (c == c_end))
						continue;

					/* Scale the columns covered by 'cell' first */
					scale_columns (table, c_b, c, addSize);
				}
			}
		}
	}

	/* Satisfy percentage width cells */
	for (r = 0; r < table->totalRows; r++) {
		totalRequested = 0;
		if (tooAdd <= 0) /* No space left! */
			return;
		
		/* first calculate how much we would like to add in this row */
		for (c = c_start; c <= c_end; c++) {
			HTMLTableCell *cell = table->cells[r][c];
			
			if (cell == 0)
				continue;
			if (r < table->totalRows - 1 &&
			    table->cells[r + 1][c] == cell)
				continue;
			if (c < table->totalCols - 1 &&
			    table->cells[r][c + 1] == cell)
				continue;

			/* Percentage cells only */
			if (HTML_OBJECT (cell)->percent <=0)
				continue;

			/* Only cells with a colspan which fits within
			   c_begin .. c_start */
			if (cell->cspan > 1) {
				gint c_b = c + 1 - cell->cspan;

				if (c_b < c_start)
					continue;
				if ((c_b == c_start) && (c == c_end))
					continue;
			}

			minWidth = a_columnopt (c + 1) - 
				a_columnopt (c + 1 - cell->cspan);
			prefWidth = tableWidth * HTML_OBJECT (cell)->percent /
				100 + table->padding * 2 + table->spacing +
				borderExtra;

			if (prefWidth <= minWidth)
				continue;

			totalRequested += (prefWidth - minWidth);
		}

		if (totalRequested == 0) /* Nothing to do */
			continue;

		totalAllowed = tooAdd;

		/* Do the actual adjusting of the percentage cells */
		for (colspan = 0; colspan <= 1; colspan ++) {
			for (c = c_start; c <= c_end; c++) {
				HTMLTableCell *cell = table->cells[r][c];

				if (cell == 0)
					continue;
				if (c < table->totalCols - 1 &&
				    table->cells[r][c + 1] == cell)
					continue;
				if (r < table->totalRows - 1 &&
				    table->cells[r + 1][c] == cell)
					continue;

				/* Percentage cells only */
				if (HTML_OBJECT (cell)->percent <= 0)
					continue;

				/* Only cells with a colspan which fits
				   within c_begin .. c_start */
				if (cell->cspan > 1) {
					gint c_b = c + 1 - cell->cspan;

					if (colspan == 0)
						continue;

					if (c_b < c_start)
						continue;
					if ((c_b == c_start) && (c == c_end))
						continue;
				}
				else {
					if (colspan != 0)
						continue;
				}

				minWidth = a_columnopt (c + 1) - 
					a_columnopt (c + 1 - cell->cspan);
				prefWidth = tableWidth * HTML_OBJECT (cell)->percent / 100 + 
					table->padding * 2 + table->spacing +
					borderExtra;

				if (prefWidth <= minWidth)
					continue;

				addSize = (prefWidth - minWidth);

				if (totalRequested > totalAllowed) { 
					/* We can't honour the request, scale it */
					addSize = addSize * totalAllowed / totalRequested;
					totalRequested -= (prefWidth - minWidth);
					totalAllowed -= addSize;
				}

				tooAdd -= addSize;

				if (colspan == 0) {
					gint c1;

					/* Just add this to the column size */
					for (c1 = c + 1; c1 <= table->totalCols; c1++)
						a_columnopt (c1) += addSize;
				}
				else {
					gint c_b = c + 1 - cell->cspan;

					/* Some end-conditions are required here
					   to prevent looping */
					if (c_b < c_start)
						continue;
					if ((c_b == c_start) && (c == c_end))
						continue;

					/* Scale the columns covered by 'cell'
                                           first */
					scale_columns (table, c_b, c, addSize);
				}
			}
		}
	}

	totalRequested = 0;

	if (tooAdd <= 0) {
		return;
	}

	prefColumnWidth = g_new0 (int, table->totalCols);
	fixedCol = g_new0 (gboolean, table->totalCols);
	percentCol = g_new0 (gboolean, table->totalCols);
	variableCol = g_new0 (gboolean, table->totalCols);

	/* first calculate how much we would like to add in each column */
	for (c = c_start; c <= c_end; c++) {
		minWidth = a_columnopt (c + 1) - a_columnopt (c);
		prefColumnWidth [c] = minWidth;
		fixedCol[c] = FALSE;
		percentCol[c] = FALSE;
		variableCol[c] = TRUE;
		for (r = 0; r < table->totalRows; r++) {
			gint prefCellWidth;
			HTMLTableCell *cell = table->cells[r][c];

			if (cell == 0)
				continue;
			if (r < table->totalRows && table->cells[r + 1][c] == cell)
				continue;

			if (HTML_OBJECT(cell)->flags
			    & HTML_OBJECT_FLAG_FIXEDWIDTH) {
				/* fixed width */
				prefCellWidth = HTML_OBJECT (cell)->width + table->padding * 2 +
					table->spacing + borderExtra;
				fixedCol[c] = TRUE;
				variableCol[c] = FALSE;

			}
			else if (HTML_OBJECT (cell)->percent > 0) {
				/* percentage width */
				prefCellWidth = tableWidth * HTML_OBJECT (cell)->percent / 100 +
					table->padding * 2 + table->spacing +
					borderExtra;
				percentCol[c] = TRUE;
				variableCol[c] = FALSE;

			}
			else {
				/* variable width */
				prefCellWidth = html_object_calc_preferred_width (HTML_OBJECT (cell)) + table->padding * 2 + table->spacing + borderExtra;
			}
			
			prefCellWidth = prefCellWidth / cell->cspan;

			if (prefCellWidth > prefColumnWidth [c])
				prefColumnWidth [c] = prefCellWidth;
		}

		if (prefColumnWidth [c] > minWidth) {
			totalRequested += (prefColumnWidth [c] - minWidth);
		}
		else {
			prefColumnWidth [c] = 0;
		}
	}

	if (totalRequested > 0) { /* Nothing to do */
		totalAllowed = tooAdd;

		/* Do the actual adjusting of the variable width cells */

		for (c = c_start; c <= c_end; c++) {
			gint c1;

			minWidth = a_columnopt (c + 1) - a_columnopt (c);
			prefWidth = prefColumnWidth [c];

			if (prefWidth <= minWidth || fixedCol[c] || percentCol[c])
				continue;

			addSize = (prefWidth - minWidth);

			if (totalRequested > totalAllowed) {/* We can't honour the request, scale it */
				addSize = addSize * totalAllowed / totalRequested;
				totalRequested -= (prefWidth - minWidth);
				totalAllowed -= addSize;
			}

			tooAdd -= addSize;

			/* Just add this to the column size */
			for (c1 = c + 1; c1 <= table->totalCols; c1++)
				a_columnopt (c1) += addSize;
		}
		
	}
	g_free (prefColumnWidth);

	/* Spread the remaining space equally across all variable columns */
	if (tooAdd > 0)
		tooAdd = scale_selected_columns (table, c_start, c_end,
						 tooAdd, variableCol);

	/* Spread the remaining space equally across all percentage columns */
	if (tooAdd > 0)
		tooAdd = scale_selected_columns (table, c_start, c_end,
						 tooAdd, percentCol);

	/* Still something left ... Change fixed columns */
	if (tooAdd > 0)
		tooAdd = scale_selected_columns (table, c_start, c_end,
						 tooAdd, fixedCol);

	g_free (fixedCol);
	g_free (percentCol);
	g_free (variableCol);
}

/* Both the minimum and preferred column sizes are calculated here.
   The hard part is choosing the actual sizes based on these two. */
static void
calc_column_widths (HTMLTable *table)
{
	gint r, c, i;
	gint indx, borderExtra = (table->border == 0) ? 0 : 1;

	g_array_set_size (table->colType, table->totalCols + 1);
	for (i=0; i < table->colType->len; i++) {
		a_coltype (i) = Variable;
	}

	g_array_set_size (table->columnPos, table->totalCols + 1);
	a_columnpos (0) = table->border + table->spacing;
	
	g_array_set_size (table->columnPrefPos, table->totalCols + 1);
	a_columnprefpos (0) = table->border + table->spacing;

	g_array_set_size (table->colSpan, table->totalCols + 1);
	for (i = 0; i < table->colSpan->len; i++)
		a_colspan (i) = 1;

	for (c = 0; c < table->totalCols; c++) {
		a_columnpos (c + 1) = 0;
		a_columnprefpos (c + 1) = 0;

		for (r = 0; r < table->totalRows; r++) {
			HTMLTableCell *cell = table->cells[r][c];
			gint colPos;

			if (cell == 0)
				continue;

			if (c < table->totalCols - 1 && 
			    table->cells[r][c + 1] == cell)
				continue;
			if (r < table->totalRows - 1 && 
			    table->cells[r + 1][c] == cell)
				continue;

			if ((indx = c - cell->cspan + 1) < 0)
				indx = 0;

			/* calculate minimum pos */
			colPos = (a_columnpos (indx)
				  + html_object_calc_min_width (HTML_OBJECT (cell))
				  + table->padding * 2
				  + table->padding
				  + borderExtra);
			
			if (a_columnpos (c + 1) < colPos)
				a_columnpos (c + 1) = colPos;

			if (a_coltype (c + 1) != Variable) {
				continue;
			}

			/* Calculate preferred pos */
			if (HTML_OBJECT (cell)->percent > 0) {
				colPos = a_columnprefpos (indx) + (HTML_OBJECT (table)->max_width * HTML_OBJECT (cell)->percent / 100) + table->padding * 2 + table->spacing + borderExtra;
				a_coltype (c + 1) = Percent;
				a_colspan (c + 1) = cell->cspan;
				a_columnprefpos (c + 1) = colPos;
			}
			else if (HTML_OBJECT (cell)->flags
				 & HTML_OBJECT_FLAG_FIXEDWIDTH) {
				colPos = a_columnprefpos (indx) + 
					HTML_OBJECT (cell)->width + 
					table->padding * 2 + table->spacing + borderExtra;
				a_coltype (c + 1) = Fixed;
				a_colspan (c + 1) = cell->cspan;
				a_columnprefpos (c + 1) = colPos;
			}
			else {
				colPos = html_object_calc_preferred_width (HTML_OBJECT (cell));
				colPos += a_columnprefpos (indx) + table->padding * 2 + table->spacing + borderExtra;
				if (a_columnprefpos (c + 1) < colPos)
					a_columnprefpos (c + 1) = colPos;

			}
			
			if (a_columnprefpos (c + 1) < a_columnpos (c + 1))
				a_columnprefpos (c + 1) = a_columnpos (c + 1);
		}

		if (a_columnpos (c + 1) <= a_columnpos (c))
			a_columnpos (c + 1) = a_columnpos (c);

		if (a_columnprefpos (c + 1) <= a_columnprefpos (c))
			a_columnprefpos (c + 1) = a_columnprefpos (c) + 1;

	}
}

static void
calc_row_heights (HTMLTable *table)
{
	gint r, c;
	gint rowPos, indx, borderExtra = table->border ? 1: 0;
	HTMLTableCell *cell;

	g_array_set_size (table->rowHeights, table->totalRows + 1);
	a_rowheights (0) = table->border + table->spacing;

	for (r = 0; r < table->totalRows; r++) {
		a_rowheights (r + 1) = 0;
		for (c = 0; c < table->totalCols; c++) {
			if ((cell = table->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 &&
			    cell == table->cells[r][c + 1])
				continue;
			if (r < table->totalRows - 1 &&
			    table->cells[r + 1][c] == cell)
				continue;

			if ((indx = r - cell->rspan + 1) < 0)
				indx = 0;

			rowPos = a_rowheights (indx) + 
				HTML_OBJECT (cell)->ascent +
				HTML_OBJECT (cell)->descent +
				table->padding * 2 + 
				table->spacing +
				borderExtra;

			if (rowPos > a_rowheights (r + 1))
				a_rowheights (r + 1) = rowPos;
		}
		
		if (a_rowheights (r + 1) < a_rowheights (r))
			a_rowheights (r + 1) = a_rowheights (r);
	}
}

static void
optimize_cell_width (HTMLTable *table)
{
	gint tableWidth = HTML_OBJECT (table)->width - table->border;
	gint addSize = 0;
	gint i;

	g_array_set_size (table->columnOpt, table->totalCols + 1);
	for (i = 0; i < table->columnOpt->len; i++) 
		a_columnopt (i) = a_columnpos (i);

	if (tableWidth > a_columnpos (table->totalCols)) {
		/* We have some space to spare */
		addSize = tableWidth - a_columnpos (table->totalCols);

		if ((HTML_OBJECT (table)->percent <= 0) && 
		    (!(HTML_OBJECT (table)->flags
		       & HTML_OBJECT_FLAG_FIXEDWIDTH))) {

			/* Variable width table */
			if (a_columnprefpos (table->totalCols) < tableWidth) {

				/* Don't scale beyond preferred width */
				addSize = a_columnprefpos (table->totalCols) -
					a_columnpos (table->totalCols);
			}
		}
	}
	
	if (addSize > 0)
		scale_columns (table, 0, table->totalCols - 1, addSize);
}

static void
set_cells (HTMLTable *table, gint r, gint c, HTMLTableCell *cell)
{
	gint endRow = r + cell->rspan;
	gint endCol = c + cell->cspan;
	gint tc;
	
	if (endCol > table->totalCols)
		add_columns (table, endCol - table->totalCols);

	if (endRow >= table->allocRows)
		add_rows (table, endRow - table->allocRows + 10);

	if (endRow > table->totalRows)
		table->totalRows = endRow;

	for (; r < endRow; r++) {
		for (tc = c; tc < endCol ; tc++) {
			if (table->cells[r][tc])
				html_table_cell_unlink (table->cells[r][tc]);
			table->cells[r][tc] = cell;
			html_table_cell_link (cell);
		}
	}
}


/* HTMLObject methods.  */

static void
calc_size (HTMLObject *o, HTMLObject *parent)
{
	gint r, c;
	gint indx, w;
	HTMLTableCell *cell;
	HTMLTable *table = HTML_TABLE (o);

	/* Recalculate min/max widths */
	calc_column_widths (table);

	/* If it doesn't fit... MAKE IT FIT!! */
	for (c = 0; c < table->totalCols; c++) {
		if (a_columnpos (c + 1) > o->max_width - table->border)
			a_columnpos (c + 1) = o->max_width - table->border;
	}

	/* Attempt to get sensible cell widths */
	optimize_cell_width (table);

	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {
			if ((cell = table->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 &&
			    cell == table->cells [r][c + 1])
				continue;
			if (r < table->totalRows - 1 &&
			    cell == table->cells [r + 1][c])
				continue;

			if ((indx = c - cell->cspan + 1) < 0)
				indx = 0;

			w = a_columnopt (c + 1) - a_columnopt (indx) - 
				table->spacing - table->padding * 2;

			html_table_cell_set_width (cell, w);
			html_object_calc_size (HTML_OBJECT (cell), NULL);
		}
	}

	if (table->caption) {
		g_print ("FIXME: Caption support\n");
	}

	/* We have all the cell sizes now, so calculate the vertical positions */
	calc_row_heights (table);

	/* Set cell positions */
	for (r = 0; r < table->totalRows; r++) {
		gint cellHeight;
		
		HTML_OBJECT (table)->ascent = a_rowheights (r + 1) - table->padding - table->spacing;
		if (table->caption && table->capAlign == HTML_VALIGN_TOP) {
			g_print ("FIXME: caption support\n");
		}

		for (c = 0; c < table->totalCols; c++) {
			if ((cell = table->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 &&
			    cell == table->cells[r][c + 1])
				continue;
			if (r < table->totalRows - 1 &&
			    cell == table->cells[r + 1][c])
				continue;

			if ((indx = c - cell->cspan + 1) < 0)
				indx = 0;

			HTML_OBJECT (cell)->x = a_columnopt (indx) + table->padding;
			HTML_OBJECT (cell)->y = HTML_OBJECT (table)->ascent - HTML_OBJECT (cell)->descent;
			if ((indx = r - cell->rspan + 1) < 0) 
				indx = 0;
			
			cellHeight = a_rowheights (r + 1) - a_rowheights (indx) -
				table->padding * 2 - table->spacing;
			html_object_set_max_ascent (HTML_OBJECT (cell),
						    cellHeight);
		}

	}

	if (table->caption && table->capAlign == HTML_VALIGN_BOTTOM) {
		g_print ("FIXME: Caption support\n");
	}

	HTML_OBJECT (table)->width = a_columnopt (table->totalCols) + table->border;
	HTML_OBJECT (table)->ascent = a_rowheights (table->totalRows) + table->border;
	
	if (table->caption) {
		g_print ("FIXME: Caption support\n");
	}
		
}

static void
draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLTableCell *cell;
	HTMLTable *table = HTML_TABLE (o);
	gint cindx, rindx;
	gint r, c;

	if (y + height < o->y - o->ascent || y > o->y + o->descent) {
		g_print ("nonono\n");
		return;
	}
	
	tx += o->x;
	ty += o->y - o->ascent;

	/* Draw the cells */
	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {
			if ((cell = table->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 && 
			    cell == table->cells[r][c + 1])
				continue;
			if (c < table->totalRows - 1 &&
			    table->cells[r + 1][c] == cell)
				continue;
			html_object_draw (HTML_OBJECT (cell), p,
					  x - o->x, y - (o->y - o->ascent),
					  width, height, tx, ty);
			/* FIXME: ColsDone */
		}
	}
#if 0
	/* Draw the border */
	{
		gint capOffset = 0;
		if (table->caption && table->capAlign == Top) {
			g_print ("fIXME: Support captions\n");
		}

		/* Draw borders around each cell */
		for (r = 0; r < table->totalRows; r++) {
			for (c = 0; c < table->totalCols; c++) {
				if ((cell = table->cells[r][c]) == 0)
					continue;
				if (c < table->totalCols - 1 &&
				    cell == table->cells[r][c + 1])
					continue;
				if (r < table->totalRows - 1 &&
				    table->cells[r + 1][c] == cell)
					continue;

				if ((cindx = c - cell->cspan + 1) < 0)
					cindx = 0;
				if ((rindx = r - cell->rspan + 1) < 0)
					rindx = 0;
				
				/* FIXME: Shaded */
				html_painter_draw_rect (p,
							tx + a_columnopt(cindx),
							ty + a_rowheights(rindx) + capOffset,
							a_columnopt (c + 1) - a_columnopt (cindx) - table->spacing,
							a_rowheights (r + 1) - a_rowheights (rindx) - table->spacing);
						      
			}
		}
	}
#endif
}


static gint
calc_min_width (HTMLObject *o) {
	return HTML_TABLE(o)->_minWidth;
}

static gint
calc_preferred_width (HTMLObject *o) {
	return HTML_TABLE (o)->_prefWidth;
}

static void
set_max_width (HTMLObject *o, gint max_width)
{
	HTMLTable *table = HTML_TABLE (o);
	
	o->max_width = max_width;
	
	if (!(o->flags & HTML_OBJECT_FLAG_FIXEDWIDTH)) {
		if (o->percent > 0)
			o->width = o->max_width * o->percent / 100;
		else
			o->width = o->max_width;
		calc_column_widths (table);
	}
}

static void
calc_absolute_pos (HTMLObject *o, gint x, gint y)
{
	gint lx = x + o->x;
	gint ly = y + o->y - o->ascent;
	HTMLTable *table = HTML_TABLE (o);
	HTMLTableCell *cell;

	guint r, c;

	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {
			if ((cell = HTML_TABLE (o)->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 &&
			    cell == HTML_TABLE (o)->cells[r][c + 1])
				continue;
			if (r < table->totalRows - 1 &&
			    cell == HTML_TABLE (o)->cells[r + 1][c])
				continue;
			
			html_object_calc_absolute_pos (HTML_OBJECT (cell),
						       lx, ly);
		}
	}
}

static void
reset (HTMLObject *o) {
	HTMLTable *table = HTML_TABLE (o);
	HTMLTableCell *cell;
	guint r, c;

	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {

			if ((cell = table->cells[r][c]) == 0)
				continue;
			if (c < table->totalCols - 1 &&
			    cell == table->cells[r][c + 1])
				continue;
			if (r < table->totalRows - 1 &&
			    cell == table->cells[r + 1][c])
				continue;
			
			html_object_reset (HTML_OBJECT (cell));
		}
	}

	calc_col_info (table);

}


void
html_table_type_init (void)
{
	html_table_class_init (&html_table_class, HTML_TYPE_TABLE);
}

void
html_table_class_init (HTMLTableClass *klass,
		       HTMLType type)
{
	HTMLObjectClass *object_class;

	object_class = HTML_OBJECT_CLASS (klass);

	html_object_class_init (object_class, type);

	/* FIXME destroy!!! */

	object_class->calc_size = calc_size;
	object_class->draw = draw;
	object_class->calc_min_width = calc_min_width;
	object_class->calc_preferred_width = calc_preferred_width;
	object_class->set_max_width = set_max_width;
	object_class->calc_absolute_pos = calc_absolute_pos;
	object_class->reset = reset;
}

void
html_table_init (HTMLTable *table,
		 HTMLTableClass *klass,
		 gint x, gint y, gint max_width, gint width, gint percent,
		 gint padding, gint spacing, gint border)
{
	HTMLObject *object;
	gint r;

	object = HTML_OBJECT (table);

	html_object_init (object, HTML_OBJECT_CLASS (klass));

	object->x = x;
	object->y = y;
	object->width = width;
	object->max_width = max_width;
	object->percent = percent;

	table->padding = padding;
	table->spacing = spacing;
	table->border = border;
	table->caption = 0;

	object->flags &= ~ HTML_OBJECT_FLAG_FIXEDWIDTH;
	table->row = 0;
	table->col = 0;

	table->totalCols = 1; /* this should be expanded to the maximum number
				 of cols by the first row parsed */
	table->totalRows = 1;
	table->allocRows = 5; /* allocate five rows initially */

	table->cells = g_new0 (HTMLTableCell **, table->allocRows);

	for (r = 0; r < table->allocRows; r++) {
		table->cells[r] = g_new0 (HTMLTableCell *, table->totalCols);;
	}
	
	if (percent > 0)
		object->width = max_width * percent / 100;
	else if (width == 0)
		object->width = max_width;
	else {
		object->max_width = object->width;
		object->flags |= HTML_OBJECT_FLAG_FIXEDWIDTH;
	}

	/* Set up arrays */
	table->colInfo = g_array_new (FALSE, TRUE, sizeof (ColInfo_t));
	table->colType = g_array_new (FALSE, FALSE, sizeof (ColType));
	table->columnPos = g_array_new (FALSE, FALSE, sizeof (gint));
	table->columnPrefPos = g_array_new (FALSE, FALSE, sizeof (gint));
	table->colSpan = g_array_new (FALSE, FALSE, sizeof (gint));
	table->columnOpt = g_array_new (FALSE, FALSE, sizeof (gint));
	table->rowHeights = g_array_new (FALSE, FALSE, sizeof (gint));
}

HTMLObject *
html_table_new (gint x, gint y, gint max_width, gint width, gint percent,
		gint padding, gint spacing, gint border)
{
	HTMLTable *table;

	table = g_new (HTMLTable, 1);
	html_table_init (table, &html_table_class, x, y, max_width, width,
			 percent, padding, spacing, border);

	return HTML_OBJECT (table);
}


void
html_table_add_cell (HTMLTable *table, HTMLTableCell *cell)
{
	while (table->col < table->totalCols && 
	       table->cells[table->row][table->col] != 0)
		table->col++;

	set_cells (table, table->row, table->col, cell);
}

void
html_table_start_row (HTMLTable *table)
{
	table->col = 0;
}

void
html_table_end_row (HTMLTable *table)
{
	while (table->col < table->totalCols && 
	       table->cells [table->row][table->col] != 0)
		table->col++;
	if (table->col)
		table->row++;
}

void
html_table_end_table (HTMLTable *table)
{
	calc_col_info (table);
	calc_column_widths (table);
}

gint
html_table_add_col_info (HTMLTable *table, gint startCol, gint colSpan,
			 gint minSize, gint prefSize, gint maxSize,
			 ColType coltype)
{
	gint indx;
	
	/* Is there already some info present? */
	for (indx = 0; indx < table->totalColInfos; indx++) {
		if ((a_colinfo (indx).startCol == startCol) &&
		    (a_colinfo (indx).colSpan == colSpan))
			break;
	}
	if (indx == table->totalColInfos) {
		/* No colInfo present, allocate some */
		table->totalColInfos++;
		if (table->totalColInfos >= table->colInfo->len) {
			g_array_set_size (table->colInfo, table->colInfo->len + table->totalCols);
		}
		a_colinfo (indx).startCol = startCol;
		a_colinfo (indx).colSpan = colSpan;
		a_colinfo (indx).minSize = minSize;
		a_colinfo (indx).prefSize = prefSize;
		a_colinfo (indx).maxSize = maxSize;
		a_colinfo (indx).colType = coltype;

	}
	else {
		if (minSize > (a_colinfo (indx).minSize))
			a_colinfo (indx).minSize = minSize;

		/* Fixed < Percent < Variable */
		if (coltype < a_colinfo (indx).colType) {
			a_colinfo (indx).prefSize = prefSize;
		}
		else if (coltype == a_colinfo (indx).colType) {
			if (prefSize > a_colinfo (indx).prefSize)
				a_colinfo (indx).prefSize = prefSize;
		}
	}

	return indx; /* Return the ColInfo Index */
}
