#ifndef _HTMLTEXT_H_
#define _HTMLTEXT_H_

#include "htmlobject.h"
#include "htmlfont.h"

typedef struct _HTMLText HTMLText;

#define HTML_TEXT(x) ((HTMLText *)(x))

struct _HTMLText {
	HTMLObject parent;
	
	gchar *text;
	HTMLFont *font;
};

HTMLObject *html_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter);
void        html_text_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

#endif /* _HTMLTEXT_H_ */
