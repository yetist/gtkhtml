#ifndef _HTMLTABLE_H_
#define _HTMLTABLE_H_

#include <glib.h>
#include "htmlobject.h"
#include "htmlcluev.h"
#include "htmltablecell.h"

typedef struct _HTMLTable HTMLTable;
#define HTML_TABLE(x) ((HTMLTable *)(x))

typedef enum { Fixed, Percent, Variable } ColType;

typedef struct _ColInfo {
	gint startCol;
	gint colSpan;
	gint minSize;
	gint prefSize;
	gint maxSize;
	ColType colType;
} ColInfo_t;

typedef struct _RowInfo {
	gint *entry;
	gint nrEntries;
	gint minSize;
	gint prefSize;
} RowInfo_t;

struct _HTMLTable {
	HTMLObject parent;

	gint _minWidth;
	gint _prefWidth;
	HTMLTableCell ***cells;
	guint totalColInfos;
	guint col, totalCols;
	guint row, totalRows, allocRows;
	gint spacing;
	gint padding;
	gint border;
	HTMLClueV *caption;
	VAlignType capAlign;
	
	GArray *colInfo; /* ColInfo_t array */
	GArray *colType; /* ColType array */
	GArray *columnPos; /* integer array */
	GArray *columnPrefPos; /* integer array */
	GArray *columnOpt; /* integer array */
	GArray *colSpan; /* integer array */
	GArray *rowHeights; /* integer array */

	RowInfo_t *rowInfo;
};

HTMLObject *html_table_new (gint x, gint y, gint max_width, gint width, gint percent, gint padding, gint spacing, gint border);
void        html_table_end_row (HTMLTable *table);
void        html_table_start_row (HTMLTable *table);
void        html_table_add_cell (HTMLTable *table, HTMLTableCell *cell);
void        html_table_end_table (HTMLTable *table);
gint        html_table_add_col_info (HTMLTable *table, gint startCol, gint colSpan, gint minSize, gint prefSize, gint maxSize, ColType coltype);
#endif /* _HTMLTABLE_H_ */
