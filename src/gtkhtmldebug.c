/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* Various debugging routines.  */

#include <stdarg.h>

#include "htmlobject.h"
#include "htmltext.h"
#include "htmltextmaster.h"
#include "htmltextslave.h"
#include "htmltable.h"
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmltype.h"

#include "gtkhtmldebug.h"


/**
 * gtk_html_debug_log:
 * @html: A GtkHTML widget
 * @format: A format string, in printf() style
 * 
 * If @html has debugging turned on, print out the message, just like libc
 * printf().  Otherwise, just do nothing.
 **/
void
gtk_html_debug_log (GtkHTML *html,
		    const gchar *format,
		    ...)
{
	va_list ap;

	if (! html->debug)
		return;

	va_start (ap, format);
	vprintf (format, ap);
}


static const gchar *
clueflow_style_to_string (HTMLClueFlowStyle style)
{
	switch (style) {
	case HTML_CLUEFLOW_STYLE_NORMAL:
		return "Normal";
	case HTML_CLUEFLOW_STYLE_H1:
		return "H1";
	case HTML_CLUEFLOW_STYLE_H2:
		return "H2";
	case HTML_CLUEFLOW_STYLE_H3:
		return "H3";
	case HTML_CLUEFLOW_STYLE_H4:
		return "H4";
	case HTML_CLUEFLOW_STYLE_H5:
		return "H5";
	case HTML_CLUEFLOW_STYLE_H6:
		return "H6";
	case HTML_CLUEFLOW_STYLE_ADDRESS:
		return "Address";
	case HTML_CLUEFLOW_STYLE_PRE:
		return "Pre";
	case HTML_CLUEFLOW_STYLE_ITEMDOTTED:
		return "ItemDotted";
	case HTML_CLUEFLOW_STYLE_ITEMROMAN:
		return "ItemRoman";
	case HTML_CLUEFLOW_STYLE_ITEMDIGIT:
		return "ItemDigit";
	default:
		return "UNKNOWN";
	}
}


void
gtk_html_debug_dump_table (HTMLObject *o,
			   gint level)
{
	gint c, r;
	HTMLTable *table = HTML_TABLE (o);

	for (r = 0; r < table->totalRows; r++) {
		for (c = 0; c < table->totalCols; c++) {
			gtk_html_debug_dump_tree (HTML_OBJECT (table->cells[r][c]), level + 1);
		}
	}

}

void
gtk_html_debug_dump_tree (HTMLObject *o,
			  gint level)
{
	HTMLObject *obj;
	gint i;

	obj = o;
	while (obj) {
		for (i = 0; i < level; i++)
			g_print (" ");

		g_print ("Obj: %p, Parent: %p  Prev: %p Next: %p ObjectType: %s Pos: %d, %d,",
			 obj, obj->parent, obj->prev, obj->next, html_type_name (HTML_OBJECT_TYPE (obj)),
			 obj->x, obj->y);

		if (HTML_OBJECT_TYPE (obj) == HTML_TYPE_CLUEFLOW)
			g_print (" [%s, %d, %d]",
				 clueflow_style_to_string (HTML_CLUEFLOW (obj)->style),
				 HTML_CLUEFLOW (obj)->list_level,
				 HTML_CLUEFLOW (obj)->quote_level);
		else if (HTML_OBJECT_TYPE (obj) == HTML_TYPE_TEXTSLAVE)
			g_print ("[start %d end %d]",
				 HTML_TEXT_SLAVE (obj)->posStart,
				 HTML_TEXT_SLAVE (obj)->posStart + HTML_TEXT_SLAVE (obj)->posLen - 1);

		g_print ("\n");

		switch (HTML_OBJECT_TYPE (obj)) {
		case HTML_TYPE_TABLE:
			gtk_html_debug_dump_table (obj, level + 1);
			break;
		case HTML_TYPE_TEXT:
		case HTML_TYPE_TEXTMASTER:
		case HTML_TYPE_LINKTEXT:
		case HTML_TYPE_LINKTEXTMASTER:
			for (i = 0; i < level; i++)
				g_print (" ");
			g_print ("Text: %s\n", HTML_TEXT (obj)->text);
			break;

		case HTML_TYPE_CLUEH:
		case HTML_TYPE_CLUEV:
		case HTML_TYPE_CLUEFLOW:
		case HTML_TYPE_CLUEALIGNED:
			gtk_html_debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;
		case HTML_TYPE_TABLECELL:
			gtk_html_debug_dump_tree (HTML_CLUE (obj)->head, level + 1);
			break;

		default:
			break;
		}

		obj = obj->next;
	}
}
