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
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-table.h"

void
html_engine_insert_table_1_1 (HTMLEngine *e)
{
	HTMLObject    *table;
	HTMLObject    *cell;
	HTMLObject    *text;
	HTMLObject    *flow;

	table = html_table_new (0, 0, 1, 2, 1);
	cell  = html_table_cell_new (0, 1, 1, 1);
	flow  = html_clueflow_new (HTML_CLUEFLOW_STYLE_NORMAL, 0);
	text  = html_engine_new_text_empty (e);

	html_clue_append (HTML_CLUE (flow), text);
	html_clue_append (HTML_CLUE (cell), flow);
	html_table_add_cell (HTML_TABLE (table), HTML_TABLE_CELL (cell));

	flow  = html_clueflow_new (HTML_CLUEFLOW_STYLE_NORMAL, 0);
	html_clue_append (HTML_CLUE (flow), table);

	html_engine_append_object (e, flow, 2, 2);
}
