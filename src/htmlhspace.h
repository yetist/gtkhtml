#ifndef _HTMLHSPACE_H_
#define _HTMLHSPACE_H_

#include "htmlobject.h"
#include "htmlfont.h"

typedef struct _HTMLHSpace HTMLHSpace;

#define HTML_HSPACE(x) ((HTMLHSpace *)(x))

struct _HTMLHSpace {
	HTMLObject parent;

	HTMLFont *font;
};

HTMLObject *html_hspace_new (HTMLFont *font, HTMLPainter *painter, gboolean hidden);
void        html_hspace_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

#endif /* _HTMLHSPACE_H_ */
