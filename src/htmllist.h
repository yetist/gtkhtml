#ifndef _HTMLLIST_H_
#define _HTMLLIST_H_

#include "htmlobject.h"

typedef struct _HTMLList HTMLList;
typedef struct _HTMLListStack HTMLListStack;

typedef enum { Unordered, UnorderedPlain, Ordered, Menu, Dir } ListType;
typedef enum { Numeric = 0, LowAlpha, UpAlpha, LowRoman, UpRoman } ListNumType;

struct _HTMLList {
	ListType type;
	ListNumType numType;
	gint itemNumber;
};

struct _HTMLListStack {
	GList *list;
};

HTMLListStack *html_list_stack_new (void);
gboolean       html_list_stack_is_empty (HTMLListStack *ls);
gint           html_list_stack_count (HTMLListStack *ls);
void           html_list_stack_push (HTMLListStack *ls, HTMLList *l);
void           html_list_stack_clear (HTMLListStack *ls);
HTMLList      *html_list_stack_top (HTMLListStack *ls);
HTMLList      *html_list_stack_pop (HTMLListStack *ls);
HTMLList      *html_list_new (ListType t, ListNumType nt);

#endif /* _HTMLLIST_H_ */





