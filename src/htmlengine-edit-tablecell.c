/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2001 Ximian, Inc.
    Authors: Radek Doulik

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

#include <config.h>
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlobject.h"
#include "htmltablecell.h"

void
html_engine_table_cell_set_bg_color (HTMLEngine *e, HTMLTableCell *cell, GdkColor *c)
{
	html_object_set_bg_color (HTML_OBJECT (cell), c);
	html_engine_queue_draw (e, HTML_OBJECT (cell));
}

void
html_engine_table_cell_set_bg_pixmap (HTMLEngine *e, HTMLTableCell *cell, gchar *url)
{
	HTMLImagePointer *iptr;

	iptr = cell->bgPixmap;
	cell->bgPixmap = url ? html_image_factory_register (e->image_factory, NULL, url) : NULL;
	if (cell->have_bgPixmap && iptr)
		html_image_factory_unregister (e->image_factory, iptr, NULL);
	cell->have_bgPixmap = url ? TRUE : FALSE;
	html_engine_queue_draw (e, HTML_OBJECT (cell));
}

HTMLTableCell *
html_engine_get_table_cell (HTMLEngine *e)
{
	g_assert (HTML_IS_ENGINE (e));

	g_return_val_if_fail (e->cursor->object->parent && e->cursor->object->parent->parent
			      && HTML_IS_TABLE_CELL (e->cursor->object->parent->parent), NULL);

	return HTML_TABLE_CELL (e->cursor->object->parent->parent);
}
