#ifndef _HTMLTABLECELL_H_
#define _HTMLTABLECELL_H_

#include "htmlcluev.h"

typedef struct _HTMLTableCell HTMLTableCell;

#define HTML_TABLE_CELL(x) ((HTMLTableCell *)(x))

struct _HTMLTableCell {
	HTMLClueV parent;

	gint rspan;
	gint cspan;
	gint padding;
	/* FIXME: Color */
	gint refcount;
};

HTMLObject *html_table_cell_new (gint x, gint y, gint max_width, gint percent, gint rs, gint cs, gint pad);
void        html_table_cell_link (HTMLTableCell *cell);
void        html_table_cell_unlink (HTMLTableCell *cell);
void        html_table_cell_set_width (HTMLTableCell *cell, gint width);

#endif /* _HTMLTABLECELL_H_ */
