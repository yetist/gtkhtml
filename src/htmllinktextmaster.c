#include "htmlobject.h"
#include "htmllinktextmaster.h"

HTMLObject *
html_link_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter,
			   gchar *url, gchar *target)
{
	gint runWidth = 0;
	gchar *textPtr = text;

	HTMLLinkTextMaster* linktextmaster = g_new0 (HTMLLinkTextMaster, 1);
	HTMLTextMaster *textmaster = HTML_TEXT_MASTER (linktextmaster);
	HTMLText *htmltext = HTML_TEXT (linktextmaster);
	HTMLObject *object = HTML_OBJECT (linktextmaster);
	html_object_init (object, LinkTextMaster);

	object->width = 0;
	object->ascent = html_font_calc_ascent (font);
	object->descent = html_font_calc_descent (font);

	htmltext->font = font;
	htmltext->text = text;
	textmaster->prefWidth = html_font_calc_width (font, text, -1);
	textmaster->minWidth = 0;
	
	while (*textPtr) {
		if (*textPtr != ' ') {
			runWidth += html_font_calc_width (font, textPtr, 1);
		}
		else {
			if (runWidth > textmaster->minWidth) {
				textmaster->minWidth = runWidth;
			}
			runWidth = 0;
		}
		textPtr++;
	}

	if (runWidth > textmaster->minWidth) {
		textmaster->minWidth = runWidth;
	}

	textmaster->strLen = strlen (text);
}

