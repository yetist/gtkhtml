#ifndef _HTMLLINKTEXTMASTER_H_
#define _HTMLLINKTEXTMASTER_H_

#include "htmlobject.h"
#include "htmltextmaster.h"

typedef struct _HTMLLinkTextMaster HTMLLinkTextMaster;

#define HTML_LINK_TEXT_MASTER(x) ((HTMLLinkTextMaster *)(x))

struct _HTMLLinkTextMaster {
	HTMLTextMaster parent;

	gchar *url;
	gchar *target;
};

#endif /* _HTMLLINKTEXTMASTER_H_ */
