#include "htmllist.h"

gboolean
html_list_stack_is_empty (HTMLListStack *ls)
{
	return (g_list_length (ls->list) == 0);
}

gint
html_list_stack_count (HTMLListStack *ls)
{
	return g_list_length (ls->list);
}

void
html_list_stack_push (HTMLListStack *ls, HTMLList *l)
{
	ls->list = g_list_prepend (ls->list, (gpointer) l);
}

void
html_list_stack_clear (HTMLListStack *ls)
{
	/* FIXME: Do something */
}

HTMLList *
html_list_stack_top (HTMLListStack *ls)
{
	HTMLList *list;

	list = (HTMLList *)(g_list_first (ls->list))->data;

	return list;
}

HTMLList *
html_list_stack_pop (HTMLListStack *ls)
{
	HTMLList *list;

	list = html_list_stack_top (ls);

	ls->list = g_list_remove (ls->list, list);

	return list;
}

HTMLListStack *
html_list_stack_new (void)
{
	HTMLListStack *ls;

	ls = g_new0 (HTMLListStack, 1);

	return ls;
}

HTMLList *
html_list_new (ListType t, ListNumType nt)
{
	HTMLList *list;
	
	list = g_new0 (HTMLList, 1);
	list->type = t;
	list->numType = nt;
	list->itemNumber = 1;

	return list;
}
