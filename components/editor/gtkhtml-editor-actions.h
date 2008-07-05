/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-actions.h
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

#ifndef GTKHTML_EDITOR_ACTIONS_H
#define GTKHTML_EDITOR_ACTIONS_H

#define GTKHTML_EDITOR_ACTION(editor, name) \
	(gtkhtml_editor_get_action (GTKHTML_EDITOR (editor), (name)))

#define GTKHTML_EDITOR_ACTION_BOLD(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "bold")
#define GTKHTML_EDITOR_ACTION_CONFIRM_REPLACE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "confirm-replace")
#define GTKHTML_EDITOR_ACTION_CONFIRM_REPLACE_ALL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "confirm-replace-all")
#define GTKHTML_EDITOR_ACTION_CONFIRM_REPLACE_CANCEL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "confirm-replace-cancel")
#define GTKHTML_EDITOR_ACTION_CONFIRM_REPLACE_NEXT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "confirm-replace-next")
#define GTKHTML_EDITOR_ACTION_CONTEXT_DELETE_CELL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-delete-cell")
#define GTKHTML_EDITOR_ACTION_CONTEXT_DELETE_COLUMN(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-delete-column")
#define GTKHTML_EDITOR_ACTION_CONTEXT_DELETE_ROW(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-delete-row")
#define GTKHTML_EDITOR_ACTION_CONTEXT_DELETE_TABLE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-delete-table")
#define GTKHTML_EDITOR_ACTION_CONTEXT_INSERT_COLUMN_AFTER(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-insert-column-after")
#define GTKHTML_EDITOR_ACTION_CONTEXT_INSERT_COLUMN_BEFORE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-insert-column-before")
#define GTKHTML_EDITOR_ACTION_CONTEXT_INSERT_ROW_ABOVE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-insert-row-above")
#define GTKHTML_EDITOR_ACTION_CONTEXT_INSERT_ROW_BELOW(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-insert-row-below")
#define GTKHTML_EDITOR_ACTION_CONTEXT_INSERT_TABLE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-insert-table")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_CELL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-cell")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_IMAGE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-image")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_LINK(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-link")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_PARAGRAPH(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-paragraph")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_RULE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-rule")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_TABLE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-table")
#define GTKHTML_EDITOR_ACTION_CONTEXT_PROPERTIES_TEXT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-properties-text")
#define GTKHTML_EDITOR_ACTION_CONTEXT_REMOVE_LINK(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-remove-link")
#define GTKHTML_EDITOR_ACTION_CONTEXT_SPELL_ADD(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-spell-add")
#define GTKHTML_EDITOR_ACTION_CONTEXT_SPELL_ADD_MENU(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-spell-add-menu")
#define GTKHTML_EDITOR_ACTION_CONTEXT_SPELL_IGNORE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "context-spell-ignore")
#define GTKHTML_EDITOR_ACTION_COPY(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "copy")
#define GTKHTML_EDITOR_ACTION_CUT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "cut")
#define GTKHTML_EDITOR_ACTION_EDIT_MENU(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "edit-menu")
#define GTKHTML_EDITOR_ACTION_FIND(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "find")
#define GTKHTML_EDITOR_ACTION_FIND_AND_REPLACE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "find-and-replace")
#define GTKHTML_EDITOR_ACTION_FORMAT_MENU(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "format-menu")
#define GTKHTML_EDITOR_ACTION_FORMAT_TEXT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "format-text")
#define GTKHTML_EDITOR_ACTION_INSERT_IMAGE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "insert-image")
#define GTKHTML_EDITOR_ACTION_INSERT_LINK(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "insert-link")
#define GTKHTML_EDITOR_ACTION_INSERT_MENU(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "insert-menu")
#define GTKHTML_EDITOR_ACTION_INSERT_RULE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "insert-rule")
#define GTKHTML_EDITOR_ACTION_INSERT_TABLE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "insert-table")
#define GTKHTML_EDITOR_ACTION_ITALIC(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "italic")
#define GTKHTML_EDITOR_ACTION_JUSTIFY_CENTER(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "justify-center")
#define GTKHTML_EDITOR_ACTION_JUSTIFY_LEFT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "justify-left")
#define GTKHTML_EDITOR_ACTION_JUSTIFY_RIGHT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "justify-right")
#define GTKHTML_EDITOR_ACTION_MODE_HTML(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "mode-html")
#define GTKHTML_EDITOR_ACTION_MODE_PLAIN(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "mode-plain")
#define GTKHTML_EDITOR_ACTION_MONOSPACED(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "monospaced")
#define GTKHTML_EDITOR_ACTION_PROPERTIES_RULE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "properties-rule")
#define GTKHTML_EDITOR_ACTION_PROPERTIES_TABLE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "properties-table")
#define GTKHTML_EDITOR_ACTION_SHOW_FIND(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "show-find")
#define GTKHTML_EDITOR_ACTION_SHOW_REPLACE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "show-replace")
#define GTKHTML_EDITOR_ACTION_SIZE_PLUS_ZERO(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "size-plus-zero")
#define GTKHTML_EDITOR_ACTION_SPELL_CHECK(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "spell-check")
#define GTKHTML_EDITOR_ACTION_STRIKETHROUGH(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "strikethrough")
#define GTKHTML_EDITOR_ACTION_STYLE_ADDRESS(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-address")
#define GTKHTML_EDITOR_ACTION_STYLE_H1(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h1")
#define GTKHTML_EDITOR_ACTION_STYLE_H2(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h2")
#define GTKHTML_EDITOR_ACTION_STYLE_H3(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h3")
#define GTKHTML_EDITOR_ACTION_STYLE_H4(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h4")
#define GTKHTML_EDITOR_ACTION_STYLE_H5(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h5")
#define GTKHTML_EDITOR_ACTION_STYLE_H6(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-h6")
#define GTKHTML_EDITOR_ACTION_STYLE_NORMAL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "style-normal")
#define GTKHTML_EDITOR_ACTION_TEST_URL(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "test-url")
#define GTKHTML_EDITOR_ACTION_UNDERLINE(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "underline")
#define GTKHTML_EDITOR_ACTION_UNINDENT(editor) \
	GTKHTML_EDITOR_ACTION ((editor), "unindent")

#endif /* GTKHTML_EDITOR_ACTIONS_H */
