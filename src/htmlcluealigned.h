#ifndef _HTML_CLUEALIGNED_H_
#define _HTML_CLUEALIGNED_H_

typedef struct _HTMLClueAligned HTMLClueAligned;

#include "htmlobject.h"
#include "htmlclue.h"

#define HTML_CLUEALIGNED(x) ((HTMLClueAligned *)(x))

struct _HTMLClueAligned {
	HTMLClue parent;

	HTMLClue *prnt;
	HTMLClueAligned *nextAligned;
};

HTMLObject *html_cluealigned_new (HTMLClue *parent, gint x, gint y, gint max_width, gint percent);

#endif /* _HTML_CLUEALIGNED_H_ */
