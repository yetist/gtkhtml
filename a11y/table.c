/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
 *
 *  Author: Radek Doulik
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <atk/atktable.h>

#include "htmltable.h"
#include "htmltablecell.h"
#include "gtkhtml.h"
#include "htmlengine.h"

#include "html.h"
#include "object.h"
#include "table.h"
#include "utils.h"

struct _HTMLA11YTable {
	HTMLA11Y html_a11y_object;
};

static void atk_table_interface_init      (AtkTableIface *iface);

static AtkObject * html_a11y_table_ref_at (AtkTable *table, gint row, gint column);
static gint html_a11y_table_get_index_at (AtkTable *table, gint row, gint column);
static gint html_a11y_table_get_column_at_index (AtkTable *table, gint index);
static gint html_a11y_table_get_row_at_index (AtkTable *table, gint index);
static gint html_a11y_table_get_n_columns (AtkTable *table);
static gint html_a11y_table_get_n_rows (AtkTable *table);
static gint html_a11y_table_get_column_extent_at (AtkTable *table, gint row, gint column);
static gint html_a11y_table_get_row_extent_at (AtkTable *table, gint row, gint column);
static AtkObject * html_a11y_table_get_column_header (AtkTable *table, gint column);
static AtkObject * html_a11y_table_get_row_header (AtkTable *table, gint row);

G_DEFINE_TYPE_EXTENDED (HTMLA11YTable,
                       html_a11y_table,
                       ATK_TYPE_OBJECT,
                       0,
                       G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE,
                         atk_table_interface_init));

static void
atk_table_interface_init (AtkTableIface *iface)
{
	g_return_if_fail (iface != NULL);

	iface->ref_at = html_a11y_table_ref_at;
	iface->get_index_at = html_a11y_table_get_index_at;
	iface->get_column_at_index = html_a11y_table_get_column_at_index;
	iface->get_row_at_index = html_a11y_table_get_row_at_index;
	iface->get_n_columns = html_a11y_table_get_n_columns;
	iface->get_n_rows = html_a11y_table_get_n_rows;
	iface->get_column_extent_at = html_a11y_table_get_column_extent_at;
	iface->get_row_extent_at = html_a11y_table_get_row_extent_at;
	iface->get_column_header = html_a11y_table_get_column_header;
	iface->get_row_header = html_a11y_table_get_row_header;
}

static void
html_a11y_table_finalize (GObject *obj)
{
	G_OBJECT_CLASS (html_a11y_table_parent_class)->finalize (obj);
}

static void
html_a11y_table_initialize (AtkObject *obj,
                            gpointer data)
{
	/* printf ("html_a11y_table_initialize\n"); */

	if (ATK_OBJECT_CLASS (html_a11y_table_parent_class)->initialize)
		ATK_OBJECT_CLASS (html_a11y_table_parent_class)->initialize (obj, data);
}

static void
html_a11y_table_class_init (HTMLA11YTableClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	html_a11y_table_parent_class = g_type_class_peek_parent (klass);

	atk_class->initialize = html_a11y_table_initialize;
	gobject_class->finalize = html_a11y_table_finalize;
}

static void
html_a11y_table_init (HTMLA11YTable *a11y_table)
{
}

AtkObject *
html_a11y_table_new (HTMLObject *html_obj)
{
	GObject *object;
	AtkObject *accessible;

	g_return_val_if_fail (HTML_IS_TABLE (html_obj), NULL);

	object = g_object_new (G_TYPE_HTML_A11Y_TABLE, NULL);

	accessible = ATK_OBJECT (object);
	atk_object_initialize (accessible, html_obj);

	accessible->role = ATK_ROLE_TABLE;

	/* printf ("created new html accessible table object\n"); */

	return accessible;
}

static gboolean
is_valid (AtkObject *table)
{
	GtkHTMLA11Y * htmla11y = html_a11y_get_gtkhtml_parent (HTML_A11Y (table));
	GtkHTML *html = GTK_HTML_A11Y_GTKHTML (htmla11y);
	AtkStateSet *ss;

	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));
	if (!htmla11y)
		return FALSE;

	if (!to)
		return FALSE;

	if (html->engine->parsing)
		return FALSE;

	ss = atk_object_ref_state_set (ATK_OBJECT (htmla11y));
	if (atk_state_set_contains_state (ss, ATK_STATE_DEFUNCT)) {
		g_object_unref (ss);
		return FALSE;
	}
	g_object_unref (ss);

	return TRUE;
}

/*
 * AtkTable interface
 */

static AtkObject *
html_a11y_table_ref_at (AtkTable *table,
                        gint row,
                        gint column)
{
	AtkObject *accessible = NULL;
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));
	HTMLTableCell *cell;

	if (!is_valid (ATK_OBJECT (table)))
		return NULL;

	g_return_val_if_fail (row < to->totalRows, NULL);
	g_return_val_if_fail (column < to->totalCols, NULL);

	cell = to->cells[row][column];

	if (cell) {
		accessible = html_utils_get_accessible (HTML_OBJECT (cell), ATK_OBJECT (table));
		if (accessible)
			g_object_ref (accessible);
	}

	return accessible;
}

static gint
html_a11y_table_get_index_at (AtkTable *table,
                              gint row,
                              gint column)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	g_return_val_if_fail (row < to->totalRows, -1);
	g_return_val_if_fail (column < to->totalCols, -1);
	g_return_val_if_fail (to->cells[row][column], -1);

	return html_object_get_child_index (HTML_OBJECT (to), HTML_OBJECT (to->cells[row][column]));
}

static gint
html_a11y_table_get_column_at_index (AtkTable *table,
                                     gint index)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));
	HTMLTableCell *cell;

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	cell = HTML_TABLE_CELL (html_object_get_child (HTML_OBJECT (to), index));

	return cell ? cell->col : -1;
}

static gint
html_a11y_table_get_row_at_index (AtkTable *table,
                                  gint index)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));
	HTMLTableCell *cell;

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	cell = HTML_TABLE_CELL (html_object_get_child (HTML_OBJECT (to), index));

	return cell ? cell->row : -1;
}

static gint
html_a11y_table_get_n_columns (AtkTable *table)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	return to->totalCols;
}

static gint
html_a11y_table_get_n_rows (AtkTable *table)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	return to->totalRows;
}

static gint
html_a11y_table_get_column_extent_at (AtkTable *table,
                                      gint row,
                                      gint column)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	g_return_val_if_fail (row < to->totalRows, -1);
	g_return_val_if_fail (column < to->totalCols, -1);
	g_return_val_if_fail (to->cells[row][column], -1);

	return to->cells[row][column]->cspan;
}

static gint
html_a11y_table_get_row_extent_at (AtkTable *table,
                                   gint row,
                                   gint column)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return -1;

	g_return_val_if_fail (row < to->totalRows, -1);
	g_return_val_if_fail (column < to->totalCols, -1);
	g_return_val_if_fail (to->cells[row][column], -1);

	return to->cells[row][column]->rspan;
}

static AtkObject *
html_a11y_table_get_column_header (AtkTable *table,
                                   gint column)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return NULL;

	g_return_val_if_fail (column < to->totalCols, NULL);
	g_return_val_if_fail (to->cells[0][column], NULL);

	return to->cells[0][column]->heading
		? html_utils_get_accessible (HTML_OBJECT (to->cells[0][column]), ATK_OBJECT (table)) : NULL;
}

static AtkObject *
html_a11y_table_get_row_header (AtkTable *table,
                                gint row)
{
	HTMLTable *to = HTML_TABLE (HTML_A11Y_HTML (table));

	if (!is_valid (ATK_OBJECT (table)))
		return NULL;

	g_return_val_if_fail (row < to->totalRows, NULL);
	g_return_val_if_fail (to->cells[row][0], NULL);

	return to->cells[row][0]->heading
		? html_utils_get_accessible (HTML_OBJECT (to->cells[row][0]), ATK_OBJECT (table)) : NULL;
}

/* unsupported calls
 * 
 *   AtkObject*
 *                     (* get_caption)              (AtkTable      *table);
 *   const gchar *     (* get_column_description)   (AtkTable      *table,
 *                                                   gint          column);
 *   const gchar *     (* get_row_description)      (AtkTable      *table,
 *                                                   gint          row);
 *   AtkObject*        (* get_summary)              (AtkTable      *table);
 *   void              (* set_caption)              (AtkTable      *table,
 *                                                   AtkObject     *caption);
 *   void              (* set_column_description)   (AtkTable      *table,
 *                                                   gint          column,
 *                                                   const gchar   *description);
 *   void              (* set_column_header)        (AtkTable      *table,
 *                                                   gint          column,
 *                                                   AtkObject     *header);
 *   void              (* set_row_description)      (AtkTable      *table,
 *                                                   gint          row,
 *                                                   const gchar   *description);
 *   void              (* set_row_header)           (AtkTable      *table,
 *                                                   gint          row,
 *                                                   AtkObject     *header);
 *   void              (* set_summary)              (AtkTable      *table,
 *                                                   AtkObject     *accessible);
 *   gint              (* get_selected_columns)     (AtkTable      *table,
 *                                                   gint          **selected);
 *   gint              (* get_selected_rows)        (AtkTable      *table,
 *                                                   gint          **selected);
 *   gboolean          (* is_column_selected)       (AtkTable      *table,
 *                                                   gint          column);
 *   gboolean          (* is_row_selected)          (AtkTable      *table,
 *                                                   gint          row);
 *   gboolean          (* is_selected)              (AtkTable      *table,
 *                                                   gint          row,
 *                                                   gint          column);
 *   gboolean          (* add_row_selection)        (AtkTable      *table,
 *                                                   gint          row);
 *   gboolean          (* remove_row_selection)     (AtkTable      *table,
 *                                                   gint          row);
 *   gboolean          (* add_column_selection)     (AtkTable      *table,
 *                                                   gint          column);
 *   gboolean          (* remove_column_selection)  (AtkTable      *table,
 *                                                   gint          column);
 */
