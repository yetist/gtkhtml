#ifndef _HTMLVSPACE_H_
#define _HTMLVSPACE_H_

#include "htmlobject.h"

typedef struct _HTMLVSpace HTMLVSpace;

#define HTML_VSPACE(x) ((HTMLVSpace *)(x))

struct _HTMLVSpace {
	HTMLObject parent;
	ClearType clear;
};

HTMLObject *html_vspace_new (gint space, ClearType clear);

#endif /* _HTMLVSPACE_H_ */
