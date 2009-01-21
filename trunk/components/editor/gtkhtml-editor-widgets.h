/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-widgets.h
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

#ifndef GTKHTML_EDITOR_WIDGETS_H
#define GTKHTML_EDITOR_WIDGETS_H

#define GTKHTML_EDITOR_WIDGET(editor, name) \
	(gtkhtml_editor_get_widget ((editor), (name)))

/* Cell Properties Window */
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_CELL_RADIO_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-cell-radio-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-color-combo")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_COLUMN_RADIO_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-column-radio-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_COLUMN_SPAN_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-column-span-spin-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_HEADER_STYLE_CHECK_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-header-style-check-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_HORIZONTAL_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-horizontal-combo-box")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_IMAGE_FILE_CHOOSER(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-image-file-chooser")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_ROW_RADIO_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-row-radio-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_ROW_SPAN_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-row-span-spin-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_TABLE_RADIO_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-table-radio-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_VERTICAL_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-vertical-combo-box")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_WIDTH_CHECK_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-width-check-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_WIDTH_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-width-combo-box")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_WIDTH_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-width-spin-button")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-window")
#define GTKHTML_EDITOR_WIDGET_CELL_PROPERTIES_WRAP_TEXT_CHECK_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "cell-properties-wrap-text-check-button")

/* Find Window */
#define GTKHTML_EDITOR_WIDGET_FIND_BACKWARDS(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-backwards")
#define GTKHTML_EDITOR_WIDGET_FIND_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-button")
#define GTKHTML_EDITOR_WIDGET_FIND_CASE_SENSITIVE(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-case-sensitive")
#define GTKHTML_EDITOR_WIDGET_FIND_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-window")
#define GTKHTML_EDITOR_WIDGET_FIND_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-entry")
#define GTKHTML_EDITOR_WIDGET_FIND_REGULAR_EXPRESSION(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "find-regular-expression")

/* Image Properties Window */
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_ALIGNMENT_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-alignment-combo-box")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_BORDER_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-border-spin-button")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_DESCRIPTION_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-description-entry")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_HEIGHT_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-height-combo-box")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_HEIGHT_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-height-spin-button")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_SOURCE_FILE_CHOOSER(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-source-file-chooser")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_URL_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-url-button")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_URL_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-url-entry")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_WIDTH_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-width-combo-box")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_WIDTH_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-width-spin-button")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-window")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_X_PADDING_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-x-padding-spin-button")
#define GTKHTML_EDITOR_WIDGET_IMAGE_PROPERTIES_Y_PADDING_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "image-properties-y-padding-spin-button")

/* Link Properties Window */
#define GTKHTML_EDITOR_WIDGET_LINK_PROPERTIES_DESCRIPTION_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "link-properties-description-entry")
#define GTKHTML_EDITOR_WIDGET_LINK_PROPERTIES_TEST_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "link-properties-test-button")
#define GTKHTML_EDITOR_WIDGET_LINK_PROPERTIES_URL_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "link-properties-url-entry")
#define GTKHTML_EDITOR_WIDGET_LINK_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "link-properties-window")

/* Page Properties Window */
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_BACKGROUND_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-background-color-combo")
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_CUSTOM_FILE_CHOOSER(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-custom-file-chooser")
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_LINK_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-link-color-combo")
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_TEMPLATE_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-template-combo-box")
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_TEXT_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-text-color-combo")
#define GTKHTML_EDITOR_WIDGET_PAGE_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "page-properties-window")

/* Paragraph Properties Window */
#define GTKHTML_EDITOR_WIDGET_PARAGRAPH_PROPERTIES_CENTER_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "paragraph-properties-center-button")
#define GTKHTML_EDITOR_WIDGET_PARAGRAPH_PROPERTIES_LEFT_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "paragraph-properties-left-button")
#define GTKHTML_EDITOR_WIDGET_PARAGRAPH_PROPERTIES_RIGHT_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "paragraph-properties-right-button")
#define GTKHTML_EDITOR_WIDGET_PARAGRAPH_PROPERTIES_STYLE_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "paragraph-properties-style-combo-box")
#define GTKHTML_EDITOR_WIDGET_PARAGRAPH_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "paragraph-properties-window")

/* Replace Confirmation Window */
#define GTKHTML_EDITOR_WIDGET_REPLACE_CONFIRMATION_CLOSE_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-confirmation-close-button")
#define GTKHTML_EDITOR_WIDGET_REPLACE_CONFIRMATION_NEXT_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-confirmation-next-button")
#define GTKHTML_EDITOR_WIDGET_REPLACE_CONFIRMATION_REPLACE_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-confirmation-replace-button")
#define GTKHTML_EDITOR_WIDGET_REPLACE_CONFIRMATION_REPLACE_ALL_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-confirmation-replace-all-button")
#define GTKHTML_EDITOR_WIDGET_REPLACE_CONFIRMATION_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-confirmation-window")

/* Replace Window */
#define GTKHTML_EDITOR_WIDGET_REPLACE_BACKWARDS(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-backwards")
#define GTKHTML_EDITOR_WIDGET_REPLACE_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-button")
#define GTKHTML_EDITOR_WIDGET_REPLACE_CASE_SENSITIVE(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-case-sensitive")
#define GTKHTML_EDITOR_WIDGET_REPLACE_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-entry")
#define GTKHTML_EDITOR_WIDGET_REPLACE_WITH_ENTRY(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-with-entry")
#define GTKHTML_EDITOR_WIDGET_REPLACE_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "replace-window")

/* Rule Properties Window */
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_ALIGNMENT_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-alignment-combo-box")
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_SHADED_CHECK_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-shaded-check-button")
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_SIZE_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-size-spin-button")
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_WIDTH_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-width-combo-box")
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_WIDTH_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-width-spin-button")
#define GTKHTML_EDITOR_WIDGET_RULE_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "rule-properties-window")

/* Spell Check Window */
#define GTKHTML_EDITOR_WIDGET_SPELL_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "spell-window")

/* Table Properties Window */
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_ALIGNMENT_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-alignment-combo-box")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_BORDER_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-border-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-color-combo")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_COLS_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-cols-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_IMAGE_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-image-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_PADDING_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-padding-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_ROWS_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-rows-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_SPACING_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-spacing-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_WIDTH_CHECK_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-width-check-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_WIDTH_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-width-combo-box")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_WIDTH_SPIN_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-width-spin-button")
#define GTKHTML_EDITOR_WIDGET_TABLE_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "table-properties-window")

/* Text Properties Window */
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_BOLD_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-bold-button")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_COLOR_COMBO(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-color-combo")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_ITALIC_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-italic-button")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_SIZE_COMBO_BOX(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-size-combo-box")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_STRIKETHROUGH_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-strikethrough-button")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_UNDERLINE_BUTTON(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-underline-button")
#define GTKHTML_EDITOR_WIDGET_TEXT_PROPERTIES_WINDOW(editor) \
	GTKHTML_EDITOR_WIDGET ((editor), "text-properties-window")

#endif /* GTKHTML_EDITOR_WIDGETS_H */
