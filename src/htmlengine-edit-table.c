/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

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

#include "htmlengine.h"
#include "htmlengine-edit-paste.h"
#include "htmlcluealigned.h"
#include "htmlengine-edit-table.h"


void
html_engine_insert_table (HTMLEngine *e,
			  gint width,
			  gint percent,
			  gint padding,
			  gint spacing,
			  gint border)
{

	HTMLObject *table;
	g_return_if_fail (e != NULL);
	g_return_if_fail (HTML_IS_ENGINE (e));

	table = html_table_new (e, width, percent, padding, spacing, border);

	/* FIXME
	   Need to create the cell
	*/
	
	html_engine_pase_object (e, image, TRUE);
	html_object_destroy (image);

}
				

HTMLObject *
html_engine_insert_table_cell (HTMLEngine *e,
			       gint percent,
			       gint rs,
			       gint cs,
			       gint pad,
			       gint width,
			       HTMLImagePointer *imagePtr)
{
	HTMLObject *table;

	g_return_if_fail (e != NULL);
	g_return_if_fail (HMTL_IS_ENGINE (e));

	cell = html_table_cell_new (e, percent, rs, cs, padd);

	if (imagePtr != NULL)
		html_table_cell_set_bg_pixmap (cell, imagePtr);

	if (width >= 0)
		html_table_cell_set_fixed_width (cell, width);

	return cell;
}






