#include <glib.h>
#include <gdk/gdk.h>
#include "htmlcolor.h"

HTMLColorStack *
html_color_stack_new (void)
{
	HTMLColorStack *cs;

	cs = g_new0 (HTMLColorStack, 1);
	
	return cs;
}

void
html_color_stack_push (HTMLColorStack *cs, GdkColor *c)
{
	cs->list = g_list_prepend (cs->list, (gpointer) c);
}

void
html_color_stack_clear (HTMLColorStack *cs)
{
	/* FIXME: Should unref the colors */
}

GdkColor *
html_color_stack_top (HTMLColorStack *cs) 
{
	GdkColor *c;

	c = (GdkColor *)(g_list_first (cs->list))->data;

	return c;
}

GdkColor *
html_color_stack_pop (HTMLColorStack *cs)
{
	GdkColor *c;

	c = html_color_stack_top (cs);

	cs->list = g_list_remove (cs->list, c);

	return c;
}





