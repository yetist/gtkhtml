/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This library is free software; you can redistribute it and/or
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
#include "htmltext.h"
#include "htmltextmaster.h"
#include "htmltextslave.h"
#include "htmltable.h"
#include "htmlclue.h"
#include "htmltype.h"
#include "debug.h"

/* Various debugging routines */

void
debug_dump_table (HTMLObject *o, gint level)
{
	gint c, r;
	HTMLTable *table = HTML_TABLE (o);

	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {
			debug_dump_tree (HTML_OBJECT (table->cells[r][c]), level + 1);
		}
	}

}

void
debug_dump_tree (HTMLObject *o, gint level)
{
	HTMLObject *obj;
	gint i;

	obj = o;
	while (obj) {
		for (i = 0; i < level; i++)
			g_print (" ");

		g_print ("Obj: %p, Parent: %p  Prev: %p Next: %p ObjectType: %s\n",
			 obj, obj->parent, obj->prev, obj->next, html_type_name (HTML_OBJECT_TYPE (obj)));

		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_TABLE:
			debug_dump_table (obj, level + 1);
			break;
		case HTML_TYPE_TEXT:
		case HTML_TYPE_TEXTMASTER:
			for (i = 0; i < level; i++)
				g_print (" ");
			g_print ("Text: %s\n", HTML_TEXT (obj)->text);
			break;
		case HTML_TYPE_CLUEH:
		case HTML_TYPE_CLUEFLOW:
		case HTML_TYPE_CLUEV:
		case HTML_TYPE_CLUEALIGNED:
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
		case HTML_TYPE_TABLECELL:
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
			
		default:
			break;
		}

		obj = obj->next;
	}
}
