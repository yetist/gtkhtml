#include "htmlcluealigned.h"

HTMLObject *
html_cluealigned_new (HTMLClue *parent, gint x, gint y, gint max_width, gint percent)
{
	HTMLClueAligned *aclue = g_new0 (HTMLClueAligned, 1);
	HTMLClue *clue = HTML_CLUE (aclue);
	HTMLObject *object = HTML_OBJECT (aclue);
	html_clue_init (clue, ClueAligned);
	
	object->x = x;
	object->y = y;
	object->max_width = max_width;
	object->percent = percent;
	
	clue->valign = Bottom;
	clue->halign = Left;
	clue->head = clue->tail = clue->curr = 0;

	if (percent > 0) {
		object->width = max_width * percent / 100;
		object->flags &= ~FixedWidth;
	}
	else if (percent < 0) {
		object->width = max_width;
		object->flags |= ~FixedWidth;
	}
	else 
		object->width = max_width;

	aclue->prnt = parent;
	aclue->nextAligned = 0;
	object->flags |= Aligned;

	return object;
}
