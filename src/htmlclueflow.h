#ifndef _HTMLCLUEFLOW_H_
#define _HTMLCLUEFLOW_H_

#include "htmlobject.h"

typedef struct _HTMLClueFlow HTMLClueFlow;

#define HTML_CLUEFLOW(x) ((HTMLClueFlow *)(x))

struct _HTMLClueFlow {
	HTMLClue parent;

	gshort indent;
};

void        html_clueflow_set_max_width (HTMLObject *o, gint max_width);
HTMLObject *html_clueflow_new (int x, int y, int max_width);

#endif /* _HTMLCLUEFLOW_H_ */
