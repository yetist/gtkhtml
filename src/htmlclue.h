#ifndef HTMLCLUE_H
#define HTMLCLUE_H

#include "htmlobject.h"

typedef struct _HTMLClue HTMLClue;

#define HTML_CLUE(x) ((HTMLClue *)(x))

struct _HTMLClue {
	HTMLObject parent;
	
	HTMLObject *head;
	HTMLObject *tail;
	HTMLObject *curr;
	
	VAlignType valign;
	HAlignType halign;

	gint (*get_left_margin) (HTMLClue *clue, gint y);

	gint (*get_right_margin) (HTMLClue *clue, gint y);
  
	void (*find_free_area) (HTMLClue *clue, gint y, gint width, gint height, gint indent, gint *y_pos, gint *lmargin, gint *rmargin);

	void (*append_right_aligned) (HTMLClue *clue, HTMLClue *aclue);
	
	gboolean (*appended) (HTMLClue *clue, HTMLClue *aclue);
};

void        html_clue_init (HTMLClue *clue, objectType ObjectType);
void        html_clue_append (HTMLObject *clue, HTMLObject *o);
void        html_clue_set_next (HTMLObject *clue, HTMLObject *o);
gint        html_clue_get_left_clear (HTMLObject *clue, gint y);
gint        html_clue_get_right_clear (HTMLObject *clue, gint y);
void        html_clue_reset (HTMLObject *clue);
void        html_clue_draw (HTMLObject *o, HTMLPainter *p, gint x, gint y, gint width, gint height, gint tx, gint ty);;
void        html_clue_calc_size (HTMLObject *o, HTMLObject *parent);
void        html_clue_set_max_ascent (HTMLObject *o, gint a);
void        html_clue_find_free_area (HTMLObject *clue, gint y, gint width, gint height, gint indent, gint *y_pos, gint *lmargin, gint *rmargin);
gint        html_clue_calc_preferred_width (HTMLObject *o);

#endif /* HTMLCLUE_H */



