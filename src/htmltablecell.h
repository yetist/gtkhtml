/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)

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
#ifndef _HTMLTABLECELL_H_
#define _HTMLTABLECELL_H_

#include "htmlcluev.h"


typedef struct _HTMLTableCell HTMLTableCell;
typedef struct _HTMLTableCellClass HTMLTableCellClass;

#define HTML_TABLE_CELL(x) ((HTMLTableCell *)(x))
#define HTML_TABLE_CELL_CLASS(x) ((HTMLTableCellClass *)(x))

struct _HTMLTableCell {
	HTMLClueV cluev;

	gint rspan;
	gint cspan;
	gint padding;
	gint refcount;

	GdkColor bg;
	gboolean have_bg : 1;
};

struct _HTMLTableCellClass {
	HTMLClueVClass cluev_class;
};


extern HTMLTableCellClass html_table_cell_class;


void html_table_cell_type_init (void);
void html_table_cell_class_init (HTMLTableCellClass *klass, HTMLType type);
void html_table_cell_init (HTMLTableCell *cell, HTMLTableCellClass *klass, gint x, gint y, gint max_width, gint percent, gint rs, gint cs, gint pad);
HTMLObject *html_table_cell_new (gint x, gint y, gint max_width, gint percent, gint rs, gint cs, gint pad);

void html_table_cell_link (HTMLTableCell *cell);
void html_table_cell_unlink (HTMLTableCell *cell);
void html_table_cell_set_width (HTMLTableCell *cell, gint width);

#endif /* _HTMLTABLECELL_H_ */
