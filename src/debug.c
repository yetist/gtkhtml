#include "htmlobject.h"
#include "htmltext.h"
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

		g_print ("Object Type: ");
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
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
		case TableCell:
			debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
			
		default:
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
		g_print ("HTMLTable blabla\n");
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
        default:
		g_print ("Unknown: %d\n", o->ObjectType);
	}
}

