#ifndef _HTMLRULE_H_
#define _HTMLRULE_H_

#include "htmlobject.h"

typedef struct _HTMLRule HTMLRule;

#define HTML_RULE(x) ((HTMLRule *)(x))

struct _HTMLRule {
	HTMLObject parent;

	gint shade;
};

HTMLObject *html_rule_new (gint max_width, gint percent, gint size, gboolean shade);
gint        html_rule_calc_min_width (HTMLObject *o);
void        html_rule_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);
void        html_rule_set_max_width (HTMLObject *o, gint max_width);

#endif /* _HTMLRULE_H_ */
