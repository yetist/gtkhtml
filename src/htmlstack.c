#include "htmlengine.h"
#include "htmlstack.h"


HTMLStackElement*
html_stack_element_new (gint id, gint level, HTMLBlockFunc exitFunc, 
			gint miscData1, gint miscData2, HTMLStackElement *next)
{
	HTMLStackElement *se;

	se = g_new0 (HTMLStackElement, 1);
	se->id = id;
	se->level = level;
	se->miscData1 = miscData1;
	se->miscData2 = miscData2;
	se->next = next;
	se->exitFunc = exitFunc;
	return se;
}
