#ifndef _HTMLSTACK_H_
#define _HTMLSTACK_H_

typedef struct _HTMLStackElement HTMLStackElement;

#include "htmlengine.h"
#include "htmlobject.h"

typedef void (*HTMLBlockFunc)(HTMLEngine *e, HTMLObject *clue, HTMLStackElement *el);



struct _HTMLStackElement {
	HTMLBlockFunc exitFunc;

	gint id;
	gint level;
	gint miscData1;
	gint miscData2;
	HTMLStackElement *next;
};

HTMLStackElement *html_stack_element_new (gint id, gint level, HTMLBlockFunc exitFunc, gint miscData1, gint miscData2, HTMLStackElement *next);

#endif /* _HTMLSTACK_H_ */
