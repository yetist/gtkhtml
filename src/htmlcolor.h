#ifndef _HTMLCOLOR_H_
#define _HTMLCOLOR_H_

typedef struct _HTMLColorStack HTMLColorStack;

struct _HTMLColorStack {
	GList *list;
};

HTMLColorStack *html_color_stack_new (void);
void            html_color_stack_push (HTMLColorStack *cs, GdkColor *c);
void            html_color_stack_clear (HTMLColorStack *cs);
GdkColor       *html_color_stack_top (HTMLColorStack *cs);
GdkColor       *html_color_stack_pop (HTMLColorStack *cs);
#endif /* _HTMLCOLOR_H_ */
