#include "htmltext.h"
#include "htmllinktext.h"

HTMLObject *
html_link_text_new (gchar *text, HTMLFont *font, HTMLPainter *painter,
		    gchar *url, gchar *target)
{
	HTMLLinkText *htmllinktext = g_new0 (HTMLLinkText, 1);
	HTMLText *htmltext = HTML_TEXT (htmllinktext);
	HTMLObject *object = HTML_OBJECT (htmllinktext);
	html_object_init (object, LinkText);

	
	object->width = html_font_calc_width (font, text, -1);
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	htmltext->text = text;
	htmltext->font = font;

	htmllinktext->url = url;
	htmllinktext->target = target;

	return object;
}
