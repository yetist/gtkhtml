#include "htmltext.h"

HTMLObject *
html_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter)
{
	HTMLText *htmltext = g_new0 (HTMLText, 1);
	HTMLObject *object = HTML_OBJECT (htmltext);
	html_object_init (object, Text);

	/* Functions */
	object->draw = html_text_draw;

	object->width = html_font_calc_width (font, text, -1);
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	htmltext->text = text;
	htmltext->font = font;

	return object;
}

void
html_text_draw (HTMLObject *o, HTMLPainter *p,
		gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLText *htmltext = HTML_TEXT (o);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	html_painter_set_font (p, htmltext->font);
	html_painter_set_pen (p, htmltext->font->textColor);
	html_painter_draw_text (p, o->x + tx, o->y + ty, htmltext->text, -1);
}
