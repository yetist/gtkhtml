#ifndef _HTMLTEXTSLAVE_H_
#define _HTMLTEXTSLAVE_H_

#include "htmlobject.h"
#include "htmltextmaster.h"

typedef struct _HTMLTextSlave HTMLTextSlave;

#define HTML_TEXT_SLAVE(x) ((HTMLTextSlave *)(x))

struct _HTMLTextSlave {
	HTMLObject parent;

	HTMLTextMaster *owner;
	gshort posStart;
	gshort posLen;
};

HTMLObject *html_text_slave_new (HTMLTextMaster *owner, gint posStart, gint posLen);
HTMLFitType html_text_slave_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft);
void        html_text_slave_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

#endif /* _HTMLTEXTSLAVE_H_ */
