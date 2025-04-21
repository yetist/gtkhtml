/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
 *
 *  Author: Radek Doulik
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "htmlclueflow.h"
#include "paragraph.h"

struct _HTMLA11YParagraph {
	HTMLA11Y html_a11y_object;
};

G_DEFINE_TYPE (HTMLA11YParagraph, html_a11y_paragraph, G_TYPE_HTML_A11Y);

static void
html_a11y_paragraph_finalize (GObject *obj)
{
	G_OBJECT_CLASS (html_a11y_paragraph_parent_class)->finalize (obj);
}

static void
html_a11y_paragraph_initialize (AtkObject *obj,
                                gpointer data)
{
	/* printf ("html_a11y_paragraph_initialize\n"); */

	if (ATK_OBJECT_CLASS (html_a11y_paragraph_parent_class)->initialize)
		ATK_OBJECT_CLASS (html_a11y_paragraph_parent_class)->initialize (obj, data);
}

static void
html_a11y_paragraph_class_init (HTMLA11YParagraphClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

	html_a11y_paragraph_parent_class = g_type_class_peek_parent (klass);

	atk_class->initialize = html_a11y_paragraph_initialize;
	gobject_class->finalize = html_a11y_paragraph_finalize;
}

static void
html_a11y_paragraph_init (HTMLA11YParagraph *a11y_paragraph)
{
}

AtkObject *
html_a11y_paragraph_new (HTMLObject *html_obj)
{
	GObject *object;
	AtkObject *accessible;

	g_return_val_if_fail (HTML_IS_CLUEFLOW (html_obj), NULL);

	object = g_object_new (G_TYPE_HTML_A11Y_PARAGRAPH, NULL);

	accessible = ATK_OBJECT (object);
	atk_object_initialize (accessible, html_obj);

	accessible->role = ATK_ROLE_PANEL; /* FIX2 fixme PARAGRAPH */

	/* printf ("created new html accessible paragraph object\n"); */

	return accessible;
}
