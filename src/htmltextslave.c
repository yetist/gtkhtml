#include <string.h>
#include "htmltextslave.h"

static gint html_text_slave_calc_min_width (HTMLObject *o);
static gint html_text_slave_calc_perferred_width (HTMLObject *o);

HTMLFitType
html_text_slave_fit_line (HTMLObject *o, gboolean startOfLine, gboolean firstRun,
			  gint widthLeft)
{
	gint newLen;
	gint newWidth;
	gchar *splitPtr;
	HTMLTextSlave *textslave = HTML_TEXT_SLAVE (o);
	HTMLTextMaster *textmaster = HTML_TEXT_MASTER (textslave->owner);
	HTMLText *ownertext = HTML_TEXT (textmaster);
	
	gchar *text = ownertext->text;

	/* Remove existing slaves */
	HTMLObject *next_obj = o->nextObj;

	g_print ("fit line: %d %d %d\n", startOfLine, firstRun, widthLeft);


	if (next_obj && (next_obj->ObjectType == TextSlave)) {
		do {
			o->nextObj = next_obj->nextObj;
			g_free (next_obj);
			next_obj = o->nextObj;
		}
		while (next_obj && (next_obj->ObjectType == TextSlave));
		textslave->posLen = textslave->owner->strLen - textslave->posStart;
	}
	
	if (startOfLine && (text[textslave->posStart] == ' ') && (widthLeft >= 0)) {
		/* Skip leading space */
		textslave->posStart++;
		textslave->posLen--;
	}
	text += textslave->posStart;
	
	o->width = html_font_calc_width (ownertext->font, text, textslave->posLen);
	if ((o->width <= widthLeft) || (textslave->posLen <= 1) || (widthLeft < 0)) {
		/* Text fits completely */
		if (!o->nextObj || (o->nextObj->flags & Separator) || (o->nextObj->flags & NewLine))
			return HTMLCompleteFit;

		/* Text is followed by more text...break it before the last word */
		splitPtr = rindex (text + 1, ' ');
		if (!splitPtr)
			return HTMLCompleteFit;
	}
	else {
		splitPtr = index (text + 1, ' ');
	}
	
	if (splitPtr) {
		newLen = splitPtr - text;
		newWidth = html_font_calc_width (ownertext->font, text, newLen);
		if (newWidth > widthLeft) {
			/* Splitting doesn't make it fit */
			splitPtr = 0;
		}
		else {
			gint extraLen;
			gint extraWidth;

			for (;;) {
				gchar *splitPtr2 = index (splitPtr + 1, ' ');
				if (!splitPtr2)
					break;
				extraLen = splitPtr2 - splitPtr;
				extraWidth = html_font_calc_width (ownertext->font, splitPtr, extraLen);
				if (extraWidth + newWidth <= widthLeft) {
					/* We can break on the next separator cause it still fits */
					newLen += extraLen;
					newWidth += extraWidth;
					splitPtr = splitPtr2;
				}
				else {
					/* Using this separator would over-do it */
					break;
				}
			}
		}
	}
	else {
		newLen = textslave->posLen;
		newWidth = o->width;
	}
	
	if (!splitPtr) {
		/* No separator available */
		if (firstRun == FALSE) {
			/* Text does not fit, wait for next line */
			return HTMLNoFit;
		}

		/* Text doesn't fit, too bad. 
		   newLen & newWidth are valid */
	}

	if (textslave->posLen - newLen > 0) {
		/* Move remaining text to our text-slave */
		HTMLObject *textSlave = html_text_slave_new (textmaster, textslave->posStart + newLen, textslave->posLen - newLen);

		textSlave->nextObj = o->nextObj;
		o->nextObj = textSlave;
	}

	textslave->posLen = newLen;
	o->width = newWidth;

	return HTMLPartialFit;
}

void
html_text_slave_draw (HTMLObject *o, HTMLPainter *p,
		      gint x, gint y, gint width, gint height, gint tx, gint ty)
{
	HTMLTextSlave *textslave = HTML_TEXT_SLAVE (o);
	HTMLText *ownertext = HTML_TEXT (textslave->owner);

	if (y + height < o->y - o->ascent || y > o->y + o->descent)
		return;

	html_painter_set_pen (p, ownertext->font->textColor);
	html_painter_set_font (p, ownertext->font);

	html_painter_draw_text (p, o->x + tx, o->y + ty, 
				&(ownertext->text[textslave->posStart]), 
				textslave->posLen);
}

static gint
html_text_slave_calc_min_width (HTMLObject *o)
{
	return 0;
}

HTMLObject *
html_text_slave_new (HTMLTextMaster *owner, gint posStart, gint posLen)
{
	HTMLText *ownertext = HTML_TEXT (owner);
	HTMLTextSlave *textslave = g_new0 (HTMLTextSlave, 1);
	HTMLObject *object = HTML_OBJECT (textslave);
	html_object_init (object, TextSlave);

	/* HTMLObject functions */
	object->draw = html_text_slave_draw;
	object->fit_line = html_text_slave_fit_line;
	object->calc_min_width = html_text_slave_calc_min_width;
	object->calc_preferred_width = html_text_slave_calc_perferred_width;

	object->ascent = HTML_OBJECT (owner)->ascent;
	object->descent = HTML_OBJECT (owner)->descent;

	textslave->posStart = posStart;
	textslave->posLen = posLen;
	textslave->owner = owner;
	
	object->width = html_font_calc_width (ownertext->font, &(ownertext->text[posStart]), posLen);

	return object;
}

static gint
html_text_slave_calc_perferred_width (HTMLObject *o)
{
	return 0;
}
