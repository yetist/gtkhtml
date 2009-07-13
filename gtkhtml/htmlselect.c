/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.

    Copyright (C) 2000 Jonas Borgström <jonas_b@bitsmart.com>.
    Copyright (C) 2000, 2001, 2002 Ximian, Inc.

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

#include "htmlselect.h"
#include <string.h>

HTMLSelectClass html_select_class;
static HTMLEmbeddedClass *parent_class = NULL;

static void
clear_paths (HTMLSelect *select)
{
	g_list_foreach (select->paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (select->paths);
	select->paths = NULL;
}

static void
destroy (HTMLObject *o)
{
	clear_paths (HTML_SELECT (o));

	HTML_OBJECT_CLASS (parent_class)->destroy (o);
}

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	/* FIXME TODO */
	HTMLSelect *s = HTML_SELECT (self);
	HTMLSelect *d = HTML_SELECT (dest);

	(* HTML_OBJECT_CLASS (parent_class)->copy) (self,dest);

	/* FIXME g_warning ("HTMLSelect::copy() is not complete."); */
	d->size =    s->size;
	d->multi =   s->multi;

	d->paths = NULL;

	d->view = NULL;
}

static void
reset (HTMLEmbedded *e)
{
	HTMLSelect *s = HTML_SELECT(e);

	if (s->multi || s->size > 1) {
		GtkTreeView *tree_view;
		GtkTreeSelection *selection;
		GList *iter;

		tree_view = GTK_TREE_VIEW (s->view);
		selection = gtk_tree_view_get_selection (tree_view);
		gtk_tree_selection_unselect_all (selection);
		for (iter = s->paths; iter != NULL; iter = iter->next) {
			GtkTreePath *path = iter->data;
			gtk_tree_selection_select_path (selection, path);
		}

	} else if (s->paths != NULL) {
		GtkTreePath *path = s->paths->data;
		GtkComboBox *combo_box;
		GtkTreeIter iter;

		combo_box = GTK_COMBO_BOX (e->widget);
		if (gtk_tree_model_get_iter (s->model, &iter, path))
			gtk_combo_box_set_active_iter (combo_box, &iter);
	}
}

struct EmbeddedSelectionInfo {
	HTMLEmbedded *embedded;
	GString *string;
};

static void
add_selected (GtkTreeModel *model,
              GtkTreePath *path,
              GtkTreeIter *iter,
              struct EmbeddedSelectionInfo *info,
              const gchar * codepage)
{
	gchar *value, *encoded;

	gtk_tree_model_get (model, iter, 0, &value, -1);

	if (info->string->len)
		g_string_append_c (info->string, '&');

	encoded = html_embedded_encode_string (info->embedded->name, codepage);
	g_string_append (info->string, encoded);
	g_free (encoded);

	g_string_append_c (info->string, '=');

	encoded = html_embedded_encode_string (value, codepage);
	g_string_append (info->string, encoded);
	g_free (encoded);

	g_free (value);
}

static gchar *
encode (HTMLEmbedded *e, const gchar *codepage)
{
	struct EmbeddedSelectionInfo info;
	HTMLSelect *s = HTML_SELECT(e);
	GtkTreeIter iter;

	info.embedded = e;
	info.string = g_string_sized_new (128);

	if (e->name != NULL && *e->name != '\0') {
		if (s->size > 1) {
			gtk_tree_selection_selected_foreach (
				gtk_tree_view_get_selection (
				GTK_TREE_VIEW (s->view)),
				(GtkTreeSelectionForeachFunc)
				add_selected, &info);
		} else {
			GtkComboBox *combo_box;

			combo_box = GTK_COMBO_BOX (e->widget);
			if (gtk_combo_box_get_active_iter (combo_box, &iter))
				add_selected (s->model, NULL, &iter, &info, codepage);
		}
	}

	return g_string_free (info.string, FALSE);
}

void
html_select_type_init (void)
{
	html_select_class_init (
		&html_select_class, HTML_TYPE_SELECT, sizeof (HTMLSelect));
}

void
html_select_class_init (HTMLSelectClass *klass,
                        HTMLType type,
                        guint object_size)
{
	HTMLEmbeddedClass *element_class;
	HTMLObjectClass *object_class;

	element_class = HTML_EMBEDDED_CLASS (klass);
	object_class = HTML_OBJECT_CLASS (klass);

	html_embedded_class_init (element_class, type, object_size);

	/* HTMLEmbedded methods.   */
	element_class->reset = reset;
	element_class->encode = encode;

	/* HTMLObject methods.   */
	object_class->destroy = destroy;
	object_class->copy = copy;

	parent_class = &html_embedded_class;
}

void
html_select_init (HTMLSelect *select,
                  HTMLSelectClass *klass,
                  GtkWidget *parent,
                  gchar *name,
                  gint size,
                  gboolean multi)
{
	HTMLEmbedded *element = HTML_EMBEDDED (select);
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkWidget *widget;

	html_embedded_init (
		element, HTML_EMBEDDED_CLASS (klass), parent, name, NULL);

	store = gtk_list_store_new (1, G_TYPE_STRING);
	renderer = gtk_cell_renderer_text_new ();
	select->model = GTK_TREE_MODEL (store);

	if (size > 1 || multi) {
		GtkTreeViewColumn *column;
		GtkRequisition req;
		GtkTreeIter iter;

		select->view = gtk_tree_view_new_with_model (select->model);
		gtk_tree_view_set_headers_visible (
			GTK_TREE_VIEW (select->view), FALSE);
		gtk_tree_selection_set_mode (
			gtk_tree_view_get_selection (
			GTK_TREE_VIEW (select->view)),
			multi ? GTK_SELECTION_MULTIPLE :
			GTK_SELECTION_SINGLE);

		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_pack_start (
			column, renderer, FALSE);
		gtk_tree_view_column_add_attribute (
			column, renderer, "text", 0);
		gtk_tree_view_append_column (
			GTK_TREE_VIEW (select->view), column);

		widget = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (
			GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);
		gtk_scrolled_window_set_policy (
			GTK_SCROLLED_WINDOW (widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (widget), select->view);
		gtk_widget_show_all (widget);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "height", -1);
		gtk_widget_size_request (select->view, &req);
		gtk_widget_set_size_request (
			select->view, 120, req.height * size);
		gtk_list_store_remove (store, &iter);
	} else {
		widget = gtk_combo_box_entry_new_with_model (select->model, 0);
		gtk_widget_set_size_request (widget, 120, -1);
	}

	html_embedded_set_widget (element, widget);

	select->size = size;
	select->multi = multi;
	select->longest = 0;
	select->paths = NULL;
}

HTMLObject *
html_select_new (GtkWidget *parent,
                 gchar *name,
                 gint size,
                 gboolean multi)
{
	HTMLSelect *ti;

	ti = g_new0 (HTMLSelect, 1);
	html_select_init (
		ti, &html_select_class, parent, name, size, multi);

	return HTML_OBJECT (ti);
}

void
html_select_add_option (HTMLSelect *select, const gchar *value, gboolean selected)
{
	GtkListStore *store;
	GtkTreeIter iter;

	if (value == NULL)
		value = "";

	store = GTK_LIST_STORE (select->model);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, value, -1);
	select->longest = MAX (select->longest, strlen (value));

	if (select->size > 1 || select->multi) {
		if (selected) {
			GtkTreeSelection *selection;
			GtkTreeView *tree_view;

			clear_paths (select);
			tree_view = GTK_TREE_VIEW (select->view);
			selection = gtk_tree_view_get_selection (tree_view);
			gtk_tree_selection_select_iter (selection, &iter);
			select->paths =
				gtk_tree_selection_get_selected_rows (
				selection, NULL);
		}
	} else {
		GtkComboBox *combo_box;

		combo_box = GTK_COMBO_BOX (HTML_EMBEDDED (select)->widget);
		selected |= (gtk_combo_box_get_active (combo_box) < 0);

		if (selected) {
			GtkTreePath *path;

			clear_paths (select);
			gtk_combo_box_set_active_iter (combo_box, &iter);
			path = gtk_tree_model_get_path (select->model, &iter);
			select->paths = g_list_prepend (NULL, path);
		}
	}
}

void
html_select_set_text (HTMLSelect *select, const gchar *text)
{
	GtkWidget *w = GTK_WIDGET (HTML_EMBEDDED (select)->widget);
	GtkListStore *store;
	GtkTreePath *path;
	GtkTreeIter iter;
	gint n_children;

	if (text == NULL)
		text = "";

	store = GTK_LIST_STORE (select->model);
	n_children = gtk_tree_model_iter_n_children (select->model, NULL);
	if (n_children > 0) {
		path = gtk_tree_path_new_from_indices (n_children - 1, -1);
		gtk_tree_model_get_iter (select->model, &iter, path);
		gtk_tree_path_free (path);
	} else
		gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, text, -1);
	select->longest = MAX (select->longest, strlen (text));

	if (select->size > 1 || select->multi) {
		GtkRequisition req;
		GtkWidget *scrollbar;
		gint width;

		scrollbar = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (w));
		gtk_widget_size_request (select->view, &req);
		width = req.width;

		if (n_children > select->size && scrollbar != NULL) {
			gtk_widget_size_request (scrollbar, &req);
			width += req.width + 8;
		}

		gtk_widget_set_size_request (w, width, -1);
		HTML_OBJECT (select)->width = width;
	} else {
		w = HTML_EMBEDDED (select)->widget;
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (w), &iter);

		gtk_entry_set_width_chars (
			GTK_ENTRY (gtk_bin_get_child (GTK_BIN (w))),
			select->longest);
		gtk_widget_set_size_request (w, -1, -1);
	}
}
