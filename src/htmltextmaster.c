#include "htmlobject.h"
#include "htmltextmaster.h"
#include "htmltextslave.h"

static gint html_text_master_calc_min_width (HTMLObject *o);
static gint html_text_master_calc_preferred_width (HTMLObject *o);

HTMLObject *
html_text_master_new (gchar *text, HTMLFont *font, HTMLPainter *painter)
{
	gint runWidth = 0;
	gchar *textPtr = text;

	HTMLTextMaster* textmaster = g_new0 (HTMLTextMaster, 1);
	HTMLText* htmltext = HTML_TEXT (textmaster);
	HTMLObject *object = HTML_OBJECT (textmaster);
	html_object_init (object, TextMaster);

	/* Functions */
	object->fit_line = html_text_master_fit_line;
	object->calc_min_width = html_text_master_calc_min_width;
	object->calc_preferred_width = html_text_master_calc_preferred_width;

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

	return object;
}

static gint
html_text_master_calc_min_width (HTMLObject *o)
{
	return HTML_TEXT_MASTER (o)->minWidth;
}

HTMLFitType
html_text_master_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun, gint widthLeft) 
{
	HTMLTextMaster *textmaster = HTML_TEXT_MASTER (o);

	HTMLObject *next_obj;
	HTMLObject *text_slave;

	if (o->flags & NewLine)
		return HTMLCompleteFit;
	
	/* Remove existing slaves */
	next_obj = o->nextObj;
	while (next_obj && (next_obj->ObjectType == TextSlave)) {
		o->nextObj = next_obj->nextObj;
		g_free (next_obj);
		next_obj = o->nextObj;
	}
	
	/* Turn all text over to our slaves */
	text_slave = html_text_slave_new (textmaster, 0, textmaster->strLen);

	text_slave->nextObj = o->nextObj;
	o->nextObj = text_slave;
	
	return HTMLCompleteFit;

}

static gint
html_text_master_calc_preferred_width (HTMLObject *o)
{
	return HTML_TEXT_MASTER (o)->prefWidth;
}
