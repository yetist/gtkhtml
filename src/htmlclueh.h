#ifndef _HTMLCLUEH_H_
#define _HTMLCLUEH_H_

#include "htmlobject.h"
#include "htmlclue.h"

typedef struct _HTMLClueH HTMLClueH;

#define HTML_CLUEH(x) ((HTMLClueH *)(x))

struct _HTMLClueH {
	HTMLClue parent;

	gshort indent;
};

HTMLObject *html_clueh_new (gint x, gint y, gint max_width);

#endif /* _HTMLCLUEH_H_ */




