#ifndef _HTMLCLUEV_H_
#define _HTMLCLUEV_H_

#include "htmlobject.h"
#include "htmlclue.h"

typedef struct _HTMLClueV HTMLClueV;

#define HTML_CLUEV(x) ((HTMLClueV *)(x))

struct _HTMLClueV {
	HTMLClue parent;
	
	/* fixme: htmlcluealigned */
	HTMLObject *alignLeftList;
	HTMLObject *alignRightList;
	gushort padding;
};

void       html_cluev_find_free_area (HTMLClue *clue, gint y, gint width, gint height, gint indent, gint *y_pos, gint *_lmargin, gint *_rmargin);
HTMLObject *html_cluev_new (int x, int y, int max_width, int percent);

void        html_cluev_set_max_width (HTMLObject *o, gint max_width);
void        html_cluev_reset (HTMLObject *clue);
void        html_cluev_calc_size (HTMLObject *clue, HTMLObject *parent);
gint        html_cluev_get_left_margin (HTMLClue *clue, gint y);
gint        html_cluev_get_right_margin (HTMLClue *o, gint y);
void        html_cluev_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);

#endif /* _HTMLCLUEV_H_ */
