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

    Author: Radek Doulik <rodo@helixcode.com>
*/

#include "control-data.h"

GtkHTMLControlData *
gtk_html_control_data_new (GtkHTML *html, GtkWidget *vbox)
{
	GtkHTMLControlData * ncd = g_new0 (GtkHTMLControlData, 1);

	ncd->html = html;
	ncd->vbox = vbox;

	return ncd;
}

void
gtk_html_control_data_destroy (GtkHTMLControlData *cd)
{
	if (cd->search_dialog) {
		gtk_html_search_dialog_destroy (cd->search_dialog);
	}
	if (cd->replace_dialog) {
		gtk_html_replace_dialog_destroy (cd->replace_dialog);
	}
	if (cd->image_dialog) {
		gtk_html_image_dialog_destroy (cd->image_dialog);
	}
	if (cd->link_dialog) {
		gtk_html_link_dialog_destroy (cd->link_dialog);
	}

	g_free (cd);
}
