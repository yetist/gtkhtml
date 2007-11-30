/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2007 Novell, Inc.

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <config.h>

#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "gtkhtml.h"
#include "paragraph-style.h"

static struct {
	GtkHTMLParagraphStyle style;
	const gchar *description;
	gboolean sensitive_html;
	gboolean sensitive_plain;
} paragraph_style_data[] = {

        { GTK_HTML_PARAGRAPH_STYLE_NORMAL,
          N_("Normal"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_PRE,
          N_("Preformat"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED,
          N_("Bulleted List"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT,
          N_("Numbered List"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN,
          N_("Roman List"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_ITEMALPHA,
          N_("Alphabetical List"), TRUE, TRUE },

        { GTK_HTML_PARAGRAPH_STYLE_H1,
          N_("Header 1"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_H2,
          N_("Header 2"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_H3,
          N_("Header 3"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_H4,
          N_("Header 4"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_H5,
          N_("Header 5"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_H6,
          N_("Header 6"), TRUE, FALSE },

        { GTK_HTML_PARAGRAPH_STYLE_ADDRESS,
          N_("Address"), TRUE, FALSE }
};

enum {
	PARAGRAPH_STYLE_COLUMN_TEXT,
	PARAGRAPH_STYLE_COLUMN_SENSITIVE
};

static GtkListStore *
paragraph_style_get_store (GtkHTMLControlData *cd)
{
	GtkTreeIter iter;
	gint ii;

	if (cd->paragraph_style_store != NULL)
		goto exit;

	cd->paragraph_style_store =
		gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

	for (ii = 0; ii < G_N_ELEMENTS (paragraph_style_data); ii++) {
		gtk_list_store_append (cd->paragraph_style_store, &iter);
		gtk_list_store_set (
			cd->paragraph_style_store, &iter,
			PARAGRAPH_STYLE_COLUMN_TEXT,
			_(paragraph_style_data[ii].description),
			PARAGRAPH_STYLE_COLUMN_SENSITIVE,
			cd->format_html ?
				paragraph_style_data[ii].sensitive_html :
				paragraph_style_data[ii].sensitive_plain,
			-1);
	}

exit:
	return cd->paragraph_style_store;
}

static gint
paragraph_style_lookup (GtkHTMLParagraphStyle style)
{
	gint ii;

	for (ii = 0; ii < G_N_ELEMENTS (paragraph_style_data); ii++)
		if (paragraph_style_data[ii].style == style)
			return ii;

	g_assert_not_reached ();
}

static void
active_paragraph_style_changed_cb (GtkComboBox *combo_box,
                                   GtkHTMLControlData *cd)
{
	GtkHTMLParagraphStyle style;
	gint index;

	index = gtk_combo_box_get_active (combo_box);
	style = paragraph_style_data[index].style;
	gtk_html_set_paragraph_style (cd->html, style);
}

static void
current_paragraph_style_changed_cb (GtkHTML *html,
                                    GtkHTMLParagraphStyle style,
                                    GtkComboBox *combo_box)
{
	gint index;

	index = paragraph_style_lookup (style);
	if (index != gtk_combo_box_get_active (combo_box))
		gtk_combo_box_set_active (combo_box, index);
}

GtkWidget *
paragraph_style_combo_box_new (GtkHTMLControlData *cd)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkWidget *combo_box;

	g_return_val_if_fail (cd != NULL, NULL);

	store = paragraph_style_get_store (cd);
	renderer = gtk_cell_renderer_text_new ();

	combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));

	gtk_cell_layout_pack_start (
		GTK_CELL_LAYOUT (combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes (
		GTK_CELL_LAYOUT (combo_box), renderer,
		"text", PARAGRAPH_STYLE_COLUMN_TEXT,
		"sensitive", PARAGRAPH_STYLE_COLUMN_SENSITIVE,
		NULL);

	/* activate the current paragraph style */
	current_paragraph_style_changed_cb (
		cd->html,
		gtk_html_get_paragraph_style (cd->html),
		GTK_COMBO_BOX (combo_box));

	g_signal_connect (
		combo_box, "changed",
		G_CALLBACK (active_paragraph_style_changed_cb), cd);

	g_signal_connect (
		cd->html, "current_paragraph_style_changed",
		G_CALLBACK (current_paragraph_style_changed_cb), combo_box);

	gtk_widget_show (combo_box);

	return combo_box;
}

void
paragraph_style_update_store (GtkHTMLControlData *cd)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean iterating;
	gint ii = 0;

	g_return_if_fail (cd != NULL);

	store = paragraph_style_get_store (cd);

	iterating = gtk_tree_model_get_iter_first (
		GTK_TREE_MODEL (store), &iter);

	while (iterating) {
		gtk_list_store_set (
			store, &iter,
			PARAGRAPH_STYLE_COLUMN_SENSITIVE,
			cd->format_html ?
				paragraph_style_data[ii++].sensitive_html :
				paragraph_style_data[ii++].sensitive_plain,
			-1);
		iterating = gtk_tree_model_iter_next (
			GTK_TREE_MODEL (store), &iter);
	}
}
