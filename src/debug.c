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
	HTMLObject *obj = o;
	gint i;

	while (obj) {
		for (i=0;i<level;i++) g_print (" ");

		g_print ("Obj: %d, Object Type: ", obj);
		debug_dump_object_type (obj);

		switch (obj->ObjectType) {
		case TableType:
			debug_dump_table (obj, level + 1);
			break;
		case Text:
		case TextMaster:
			for (i=0;i<level;i++) g_print (" ");
			g_print ("Text: %s\n", HTML_TEXT (obj)->text);
			break;
		case ClueH:
		case ClueFlow:
		case ClueV:
		case ClueAligned:
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
		case TableCell:
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
			
		default:
			break;
		}

		obj = obj->nextObj;
	}
}

void
debug_dump_object_type (HTMLObject *o)
{
	switch (o->ObjectType) {

	case TableCell:
		g_print ("HTMLTableCell\n");
		break;
	case TableType:
		g_print ("HTMLTable\n");
		break;
	case Bullet:
		g_print ("HTMLBullet\n");
		break;
	case ClueH:
		g_print ("HTMLClueH\n");
		break;
	case ClueV:
		g_print ("HTMLClueV\n");
		break;
	case ClueFlow:
		g_print ("HTMLClueFlow\n");
		break;
	case TextMaster:
		g_print ("HTMLTextMaster\n");
		break;
	case TextSlave:
		g_print ("HTMLTextSlave\n");
		break;
	case HSpace:
		g_print ("HTMLHSpace\n");
		break;
	case Text:
		g_print ("HTMLText\n");
		break;
	case VSpace:
		g_print ("HTMLVspace\n");
		break;
	case Image:
		g_print ("HTMLImage\n");
		break;
	case ClueAligned:
		g_print ("HTMLClueAligned\n");
		break;
        default:
		g_print ("Unknown: %d\n", o->ObjectType);
	}
}

