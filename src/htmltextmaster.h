#ifndef _HTMLTEXTMASTER_H_
#define _HTMLTEXTMASTER_H_

#include "htmlobject.h"
#include "htmltext.h"

typedef struct _HTMLTextMaster HTMLTextMaster;

#define HTML_TEXT_MASTER(x) ((HTMLTextMaster *)(x))

struct _HTMLTextMaster {
	HTMLText parent;

	gint minWidth;
	gint prefWidth;
	gint strLen;
};

HTMLFitType html_text_master_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft);
HTMLObject *html_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter);


#endif /* _HTMLTEXTMASTER_H_ */
