/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-signals.h
 *
 * Copyright (C) 2008 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This file is for widget signal handlers, primarily for property dialogs.
 * Most of these handlers will be auto-connected by libglade.  GtkAction
 * callbacks belong in gtkhtml-editor-actions.c. */

#ifndef GTKHTML_EDITOR_SIGNALS_H
#define GTKHTML_EDITOR_SIGNALS_H

#include "gtkhtml-editor-common.h"

G_BEGIN_DECLS

/*****************************************************************************
 * Find Window
 *****************************************************************************/

void	gtkhtml_editor_find_backwards_toggled_cb
						(GtkWidget *window,
						 GtkToggleButton *button);
void	gtkhtml_editor_find_case_sensitive_toggled_cb
						(GtkWidget *window,
						 GtkToggleButton *button);
void	gtkhtml_editor_find_entry_activate_cb	(GtkWidget *window,
						 GtkEntry *entry);
void	gtkhtml_editor_find_entry_changed_cb	(GtkWidget *window,
						 GtkEntry *entry);
void	gtkhtml_editor_find_regular_expression_toggled_cb
						(GtkWidget *window,
						 GtkToggleButton *button);

/*****************************************************************************
 * Insert Link Window
 *****************************************************************************/

void	gtkhtml_editor_insert_link_description_changed_cb
						(GtkWidget *window);
void	gtkhtml_editor_insert_link_url_changed_cb
						(GtkWidget *window);
void	gtkhtml_editor_insert_link_show_window_cb
						(GtkWidget *window);

/*****************************************************************************
 * Insert Rule Window
 *****************************************************************************/

void	gtkhtml_editor_insert_rule_alignment_changed_cb
						(GtkWidget *window,
						 GtkComboBox *combo_box);
void	gtkhtml_editor_insert_rule_shaded_toggled_cb
						(GtkWidget *window,
						 GtkToggleButton *button);
void	gtkhtml_editor_insert_rule_size_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_rule_show_window_cb
						(GtkWidget *window);
void	gtkhtml_editor_insert_rule_width_changed_cb
						(GtkWidget *window);

/*****************************************************************************
 * Insert Table Window
 *****************************************************************************/

void	gtkhtml_editor_insert_table_alignment_changed_cb
						(GtkWidget *window,
						 GtkComboBox *combo_box);
void	gtkhtml_editor_insert_table_border_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_table_cols_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_table_image_changed_cb
						(GtkWidget *window,
						 GtkFileChooser *file_chooser);
void	gtkhtml_editor_insert_table_padding_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_table_spacing_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_table_rows_changed_cb
						(GtkWidget *window,
						 GtkSpinButton *button);
void	gtkhtml_editor_insert_table_show_window_cb
						(GtkWidget *window);
void	gtkhtml_editor_insert_table_width_changed_cb
						(GtkWidget *window);

/*****************************************************************************
 * Replace Confirmation Window
 *****************************************************************************/

gboolean gtkhtml_editor_replace_confirmation_delete_event_cb
						(GtkWidget *window,
						 GdkEvent *event);

G_END_DECLS

#endif /* GTKHTML_EDITOR_SIGNALS_H */
