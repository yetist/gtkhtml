/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

   Copyright (C) 2000 Helix Code, Inc.
   Authors:           Radek Doulik (rodo@helixcode.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHcANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>
#include "gtkhtml.h"
#include "gtkhtml-properties.h"

#define DEFAULT_FONT_SIZE   12
#define DEFAULT_FONT_SIZE_S "12"

#define STRINGIZE(x) #x

GtkHTMLClassProperties *
gtk_html_class_properties_new (void)
{
	GtkHTMLClassProperties *p = g_new (GtkHTMLClassProperties, 1);

	/* default values */
	p->magic_links             = TRUE;
	p->keybindings_theme       = g_strdup ("emacs");
	p->font_var         = g_strdup ("-*-helvetica-*-*-*-*-12-*-*-*-*-*-*-*");
	p->font_fix         = g_strdup ("-*-courier-*-*-*-*-12-*-*-*-*-*-*-*");
	p->font_var_size           = DEFAULT_FONT_SIZE;
	p->font_fix_size           = DEFAULT_FONT_SIZE;
	p->font_var_print   = g_strdup ("-*-helvetica-*-*-*-*-12-*-*-*-*-*-*-*");
	p->font_fix_print   = g_strdup ("-*-courier-*-*-*-*-12-*-*-*-*-*-*-*");
	p->font_var_size_print     = DEFAULT_FONT_SIZE;
	p->font_fix_size_print     = DEFAULT_FONT_SIZE;
	p->animations              = TRUE;

	p->live_spell_check        = TRUE;
	p->spell_error_color.red   = 0xffff;
	p->spell_error_color.green = 0;
	p->spell_error_color.blue  = 0;
	p->language                = g_strdup ("en");

	return p;
}

void
gtk_html_class_properties_destroy (GtkHTMLClassProperties *p)
{
	g_free (p->keybindings_theme);
	g_free (p);
}

#ifdef GTKHTML_HAVE_GCONF
#define GET(t,x,prop,f,c) \
        key = g_strconcat (GTK_HTML_GCONF_DIR, x, NULL); \
        val = gconf_client_get_without_default (client, key, NULL); \
        if (val) { f; p->prop = c (gconf_value_get_ ## t (val)); \
        gconf_value_free (val); } \
        g_free (key);

void
gtk_html_class_properties_load (GtkHTMLClassProperties *p, GConfClient *client)
{
	GConfValue *val;
	gchar *key;

	g_assert (client);

	GET (bool, "/magic_links", magic_links,,);
	GET (bool, "/animations", animations,,);
	GET (string, "/keybindings_theme", keybindings_theme,
	     g_free (p->keybindings_theme), g_strdup);
	GET (string, "/font_variable", font_var,
	     g_free (p->font_var), g_strdup);
	GET (string, "/font_fixed", font_fix,
	     g_free (p->font_fix), g_strdup);
	GET (int, "/font_variable_size", font_var_size,,);
	GET (int, "/font_fixed_size", font_fix_size,,);
	GET (string, "/font_variable_print", font_var_print,
	     g_free (p->font_var_print), g_strdup);
	GET (string, "/font_fixed_print", font_fix_print,
	     g_free (p->font_fix_print), g_strdup);
	GET (int, "/font_variable_size_print", font_var_size_print,,);
	GET (int, "/font_fixed_size_print", font_fix_size_print,,);

	GET (bool, "/live_spell_check", live_spell_check,,);
	GET (int, "/spell_error_color_red",   spell_error_color.red,,);
	GET (int, "/spell_error_color_green", spell_error_color.green,,);
	GET (int, "/spell_error_color_blue",  spell_error_color.blue,,);
	GET (string, "/language", language,
	     g_free (p->language), g_strdup);
}

#define SET(t,x,prop) \
        { key = g_strconcat (GTK_HTML_GCONF_DIR, x, NULL); \
        gconf_client_set_ ## t (client, key, p->prop, NULL); \
        g_free (key); }


void
gtk_html_class_properties_update (GtkHTMLClassProperties *p, GConfClient *client, GtkHTMLClassProperties *old)
{
	gchar *key;

	if (p->animations != old->animations)
		SET (bool, "/animations", animations);
	if (p->magic_links != old->magic_links)
		SET (bool, "/magic_links", magic_links);
	SET (string, "/keybindings_theme", keybindings_theme);
	if (strcmp (p->font_var, old->font_var))
		SET (string, "/font_variable", font_var);
	if (strcmp (p->font_fix, old->font_fix))
		SET (string, "/font_fixed", font_fix);
	if (p->font_var_size != old->font_var_size)
		SET (int, "/font_variable_size", font_var_size);
	if (p->font_fix_size != old->font_fix_size)
		SET (int, "/font_fixed_size", font_fix_size);
	if (strcmp (p->font_var_print, old->font_var_print))
		SET (string, "/font_variable_print", font_var_print);
	if (strcmp (p->font_fix_print, old->font_fix_print))
		SET (string, "/font_fixed_print", font_fix_print);
	if (p->font_var_size_print != old->font_var_size_print)
		SET (int, "/font_variable_size_print", font_var_size_print);
	if (p->font_fix_size_print != old->font_fix_size_print)
		SET (int, "/font_fixed_size_print", font_fix_size_print);

	if (p->live_spell_check != old->live_spell_check)
		SET (bool, "/live_spell_check", live_spell_check);
	if (!gdk_color_equal (&p->spell_error_color, &old->spell_error_color)) {
		SET (int, "/spell_error_color_red",   spell_error_color.red);
		SET (int, "/spell_error_color_green", spell_error_color.green);
		SET (int, "/spell_error_color_blue",  spell_error_color.blue);
	}
	if (strcmp (p->language, old->language))
		SET (string, "/language", language);
}

#else

#undef GET
#define GET(t,v,s) \
	p->v = gnome_config_get_ ## t (s)
#define GETS(v,s) \
        g_free (p->v); \
        GET(string,v,s)


void
gtk_html_class_properties_load (GtkHTMLClassProperties *p)
{
	char *s;

	gnome_config_push_prefix (GTK_HTML_GNOME_CONFIG_PREFIX);
	GET  (bool, magic_links, "magic_links=true");
	GET  (bool, animations, "animations=true");
	GETS (keybindings_theme, "keybindings_theme=ms");
	GETS (font_var, "font_variable=-*-helvetica-*-*-*-*-12-*-*-*-*-*-*-*");
	GETS (font_fix, "font_fixed=-*-courier-*-*-*-*-12-*-*-*-*-*-*-*");
	GETS (font_var_print, "font_variable_print=-*-helvetica-*-*-*-*-12-*-*-*-*-*-*-*");
	GETS (font_fix_print, "font_fixed_print=-*-courier-*-*-*-*-12-*-*-*-*-*-*-*");

	s = g_strdup_printf ("font_variable_size=%d", DEFAULT_FONT_SIZE);
	GET  (int, font_var_size, s);
	g_free (s);

	s = g_strdup_printf ("font_fixed_size=%d", DEFAULT_FONT_SIZE);
	GET  (int, font_fix_size, s);
	g_free (s);

	s = g_strdup_printf ("font_variable_size_print=%d", DEFAULT_FONT_SIZE);
	GET  (int, font_var_size_print, s);
	g_free (s);

	s = g_strdup_printf ("font_fixed_size_print=%d", DEFAULT_FONT_SIZE);
	GET  (int, font_fix_size_print, s);
	g_free (s);

	GET  (bool, live_spell_check, "live_spell_check=true");
	GET  (int, spell_error_color.red,   "spell_error_color_red=0xffff");
	GET  (int, spell_error_color.green, "spell_error_color_green=0");
	GET  (int, spell_error_color.blue,  "spell_error_color_blue=0");
	GETS (language, "language=en");

	gnome_config_pop_prefix ();
}

void
gtk_html_class_properties_save (GtkHTMLClassProperties *p)
{
	gnome_config_push_prefix (GTK_HTML_GNOME_CONFIG_PREFIX);
	gnome_config_set_bool ("magic_links", p->magic_links);
	gnome_config_set_bool ("animations", p->animations);
	gnome_config_set_string ("keybindings_theme", p->keybindings_theme);
	gnome_config_set_string ("font_variable", p->font_var);
	gnome_config_set_string ("font_fixed", p->font_fix);
	gnome_config_set_int ("font_variable_size", p->font_var_size);
	gnome_config_set_int ("font_fixed_size", p->font_fix_size);
	gnome_config_set_string ("font_variable_print", p->font_var_print);
	gnome_config_set_string ("font_fixed_print", p->font_fix_print);
	gnome_config_set_int ("font_variable_size_print", p->font_var_size_print);
	gnome_config_set_int ("font_fixed_size_print", p->font_fix_size_print);

	gnome_config_set_bool ("live_spell_check", p->live_spell_check);
	gnome_config_set_int  ("spell_error_color_red",   p->spell_error_color.red);
	gnome_config_set_int  ("spell_error_color_green", p->spell_error_color.green);
	gnome_config_set_int  ("spell_error_color_blue",  p->spell_error_color.blue);
	gnome_config_set_string ("language", p->language);

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}
#endif

#define COPYS(v) \
        g_free (p1->v); \
        p1->v = g_strdup (p2->v);
#define COPY(v) \
        p1->v = p2->v;

void
gtk_html_class_properties_copy (GtkHTMLClassProperties *p1,
				GtkHTMLClassProperties *p2)
{
	COPY  (magic_links);
	COPYS (keybindings_theme);
	COPYS (font_var);
	COPYS (font_fix);
	COPY  (font_var_size);
	COPY  (font_fix_size);
	COPYS (font_var_print);
	COPYS (font_fix_print);
	COPY  (font_var_size_print);
	COPY  (font_fix_size_print);

	COPY  (live_spell_check);
	COPY  (spell_error_color);
	COPYS (language);
}

/* enums */

static GtkEnumValue _gtk_html_cursor_skip_values[] = {
  { GTK_HTML_CURSOR_SKIP_ONE,  "GTK_HTML_CURSOR_SKIP_ONE",  "one" },
  { GTK_HTML_CURSOR_SKIP_WORD, "GTK_HTML_CURSOR_SKIP_WORD", "word" },
  { GTK_HTML_CURSOR_SKIP_PAGE, "GTK_HTML_CURSOR_SKIP_WORD", "page" },
  { GTK_HTML_CURSOR_SKIP_ALL,  "GTK_HTML_CURSOR_SKIP_ALL",  "all" },
  { 0, NULL, NULL }
};

GtkType
gtk_html_cursor_skip_get_type ()
{
	static GtkType cursor_skip_type = 0;

	if (!cursor_skip_type)
		cursor_skip_type = gtk_type_register_enum ("GTK_HTML_CURSOR_SKIP", _gtk_html_cursor_skip_values);

	return cursor_skip_type;
}

static GtkEnumValue _gtk_html_command_values[] = {
  { GTK_HTML_COMMAND_UNDO,  "GTK_HTML_COMMAND_UNDO",  "undo" },
  { GTK_HTML_COMMAND_REDO,  "GTK_HTML_COMMAND_REDO",  "redo" },
  { GTK_HTML_COMMAND_COPY,  "GTK_HTML_COMMAND_COPY",  "copy" },
  { GTK_HTML_COMMAND_CUT,   "GTK_HTML_COMMAND_CUT",   "cut" },
  { GTK_HTML_COMMAND_PASTE, "GTK_HTML_COMMAND_PASTE", "paste" },
  { GTK_HTML_COMMAND_CUT_LINE, "GTK_HTML_COMMAND_CUT_LINE", "cut-line" },

  { GTK_HTML_COMMAND_INSERT_RULE, "GTK_HTML_COMMAND_INSERT_RULE", "insert-rule" },
  { GTK_HTML_COMMAND_INSERT_PARAGRAPH, "GTK_HTML_COMMAND_INSERT_PARAGRAPH", "insert-paragraph" },
  { GTK_HTML_COMMAND_DELETE, "GTK_HTML_COMMAND_DELETE", "delete" },
  { GTK_HTML_COMMAND_DELETE_BACK, "GTK_HTML_COMMAND_DELETE_BACK", "delete-back" },
  { GTK_HTML_COMMAND_DELETE_BACK_OR_INDENT_DEC, "GTK_HTML_COMMAND_DELETE_BACK_OR_INDENT_DEC", "delete-back-or-indent-dec" },
  { GTK_HTML_COMMAND_SET_MARK, "GTK_HTML_COMMAND_SET_MARK", "set-mark" },
  { GTK_HTML_COMMAND_DISABLE_SELECTION, "GTK_HTML_COMMAND_DISABLE_SELECTION", "disable-selection" },
  { GTK_HTML_COMMAND_BOLD_ON, "GTK_HTML_COMMAND_BOLD_ON", "bold-on" },
  { GTK_HTML_COMMAND_BOLD_OFF, "GTK_HTML_COMMAND_BOLD_OFF", "bold-off" },
  { GTK_HTML_COMMAND_BOLD_TOGGLE, "GTK_HTML_COMMAND_BOLD_TOGGLE", "bold-toggle" },
  { GTK_HTML_COMMAND_ITALIC_ON, "GTK_HTML_COMMAND_ITALIC_ON", "italic-on" },
  { GTK_HTML_COMMAND_ITALIC_OFF, "GTK_HTML_COMMAND_ITALIC_OFF", "italic-off" },
  { GTK_HTML_COMMAND_ITALIC_TOGGLE, "GTK_HTML_COMMAND_ITALIC_TOGGLE", "italic-toggle" },
  { GTK_HTML_COMMAND_UNDERLINE_ON, "GTK_HTML_COMMAND_UNDERLINE_ON", "underline-on" },
  { GTK_HTML_COMMAND_UNDERLINE_OFF, "GTK_HTML_COMMAND_UNDERLINE_OFF", "underline-off" },
  { GTK_HTML_COMMAND_UNDERLINE_TOGGLE, "GTK_HTML_COMMAND_UNDERLINE_TOGGLE", "underline-toggle" },
  { GTK_HTML_COMMAND_STRIKEOUT_ON, "GTK_HTML_COMMAND_STRIKEOUT_ON", "strikeout-on" },
  { GTK_HTML_COMMAND_STRIKEOUT_OFF, "GTK_HTML_COMMAND_STRIKEOUT_OFF", "strikeout-off" },
  { GTK_HTML_COMMAND_STRIKEOUT_TOGGLE, "GTK_HTML_COMMAND_STRIKEOUT_TOGGLE", "strikeout-toggle" },
  { GTK_HTML_COMMAND_SIZE_MINUS_2, "GTK_HTML_COMMAND_SIZE_MINUS_2", "size-minus-2" },
  { GTK_HTML_COMMAND_SIZE_MINUS_1, "GTK_HTML_COMMAND_SIZE_MINUS_1", "size-minus-1" },
  { GTK_HTML_COMMAND_SIZE_PLUS_0, "GTK_HTML_COMMAND_SIZE_PLUS_0", "size-plus-0" },
  { GTK_HTML_COMMAND_SIZE_PLUS_1, "GTK_HTML_COMMAND_SIZE_PLUS_1", "size-plus-1" },
  { GTK_HTML_COMMAND_SIZE_PLUS_2, "GTK_HTML_COMMAND_SIZE_PLUS_2", "size-plus-2" },
  { GTK_HTML_COMMAND_SIZE_PLUS_3, "GTK_HTML_COMMAND_SIZE_PLUS_3", "size-plus-3" },
  { GTK_HTML_COMMAND_SIZE_PLUS_4, "GTK_HTML_COMMAND_SIZE_PLUS_4", "size-plus-4" },
  { GTK_HTML_COMMAND_SIZE_INCREASE, "GTK_HTML_COMMAND_SIZE_INCREASE", "size-inc" },
  { GTK_HTML_COMMAND_SIZE_DECREASE, "GTK_HTML_COMMAND_SIZE_DECREASE", "size-dec" },
  { GTK_HTML_COMMAND_ALIGN_LEFT, "GTK_HTML_COMMAND_ALIGN_LEFT", "align-left" },
  { GTK_HTML_COMMAND_ALIGN_CENTER, "GTK_HTML_COMMAND_ALIGN_CENTER", "align-center" },
  { GTK_HTML_COMMAND_ALIGN_RIGHT, "GTK_HTML_COMMAND_ALIGN_RIGHT", "align-right" },
  { GTK_HTML_COMMAND_INDENT_ZERO, "GTK_HTML_COMMAND_INDENT_ZERO", "indent-zero" },
  { GTK_HTML_COMMAND_INDENT_INC, "GTK_HTML_COMMAND_INDENT_INC", "indent-more" },
  { GTK_HTML_COMMAND_INDENT_DEC, "GTK_HTML_COMMAND_INDENT_DEC", "indent-less" },
  { GTK_HTML_COMMAND_INDENT_PARAGRAPH, "GTK_HTML_COMMAND_INDENT_PARAGRAPH", "indent-paragraph" },
  { GTK_HTML_COMMAND_BREAK_AND_FILL_LINE, "GTK_HTML_COMMAND_BREAK_AND_FILL_LINE", "break-and-fill" },
  { GTK_HTML_COMMAND_SPACE_AND_FILL_LINE, "GTK_HTML_COMMAND_SPACE_AND_FILL_LINE", "space-and-fill" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_NORMAL, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_NORMAL", "style-normal" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H1, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H1", "style-header1" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H2, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H2", "style-header2" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H3, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H3", "style-header3" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H4, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H4", "style-header4" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H5, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H5", "style-header5" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_H6, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_H6", "style-header6" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_ADDRESS, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_ADDRESS", "style-address" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_PRE, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_PRE", "style-pre" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDOTTED, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDOTTED", "style-itemdot" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMROMAN, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMROMAN", "style-itemroman" },
  { GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDIGIT, "GTK_HTML_COMMAND_PARAGRAPH_STYLE_ITEMDIGIT", "style-itemdigit" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_UP, "GTK_HTML_COMMAND_MODIFY_SELECTION_UP", "selection-move-up" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_DOWN, "GTK_HTML_COMMAND_MODIFY_SELECTION_DOWN", "selection-move-down" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_LEFT, "GTK_HTML_COMMAND_MODIFY_SELECTION_LEFT", "selection-move-left" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_RIGHT, "GTK_HTML_COMMAND_MODIFY_SELECTION_RIGHT", "selection-move-right" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_BOL, "GTK_HTML_COMMAND_MODIFY_SELECTION_BOL", "selection-move-bol" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_EOL, "GTK_HTML_COMMAND_MODIFY_SELECTION_EOL", "selection-move-eol" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_BOD, "GTK_HTML_COMMAND_MODIFY_SELECTION_BOD", "selection-move-bod" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_EOD, "GTK_HTML_COMMAND_MODIFY_SELECTION_EOD", "selection-move-eod" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEUP, "GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEUP", "selection-move-pageup" },
  { GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEDOWN, "GTK_HTML_COMMAND_MODIFY_SELECTION_PAGEDOWN", "selection-move-pagedown" },
  { GTK_HTML_COMMAND_SELECT_WORD, "GTK_HTML_COMMAND_SELECT_WORD", "select-word" },
  { GTK_HTML_COMMAND_SELECT_LINE, "GTK_HTML_COMMAND_SELECT_LINE", "select-line" },
  { GTK_HTML_COMMAND_SELECT_PARAGRAPH, "GTK_HTML_COMMAND_SELECT_PARAGRAPH", "select-paragraph" },
  { GTK_HTML_COMMAND_SELECT_PARAGRAPH_EXTENDED, "GTK_HTML_COMMAND_SELECT_PARAGRAPH_EXTENDED", "select-paragraph-extended" },
  { GTK_HTML_COMMAND_SELECT_ALL, "GTK_HTML_COMMAND_SELECT_ALL", "select-all" },
  { GTK_HTML_COMMAND_CURSOR_POSITION_SAVE, "GTK_HTML_COMMAND_CURSOR_POSITION_SAVE", "cursor-position-save" },
  { GTK_HTML_COMMAND_CURSOR_POSITION_RESTORE, "GTK_HTML_COMMAND_CURSOR_POSITION_RESTORE", "cursor-position-restore" },
  { GTK_HTML_COMMAND_CAPITALIZE_WORD, "GTK_HTML_COMMAND_CAPITALIZE_WORD", "capitalize-word" },
  { GTK_HTML_COMMAND_UPCASE_WORD, "GTK_HTML_COMMAND_UPCASE_WORD", "upcase-word" },
  { GTK_HTML_COMMAND_DOWNCASE_WORD, "GTK_HTML_COMMAND_DOWNCASE_WORD", "downcase-word" },
  { GTK_HTML_COMMAND_SPELL_SUGGEST, "GTK_HTML_COMMAND_SPELL_SUGGEST", "spell-suggest" },
  { GTK_HTML_COMMAND_SPELL_PERSONAL_DICTIONARY_ADD, "GTK_HTML_COMMAND_SPELL_PERSONAL_DICTIONARY_ADD", "spell-personal-add" },
  { GTK_HTML_COMMAND_SPELL_SESSION_DICTIONARY_ADD, "GTK_HTML_COMMAND_SPELL_SESSION_DICTIONARY_ADD", "spell-session-add" },
  { GTK_HTML_COMMAND_SEARCH_INCREMENTAL_FORWARD, "GTK_HTML_COMMAND_SEARCH_INCREMENTAL_FORWARD", "isearch-forward" },
  { GTK_HTML_COMMAND_SEARCH_INCREMENTAL_BACKWARD, "GTK_HTML_COMMAND_SEARCH_INCREMENTAL_BACKWARD", "isearch-backward" },
  { GTK_HTML_COMMAND_SEARCH, "GTK_HTML_COMMAND_SEARCH", "search" },
  { GTK_HTML_COMMAND_SEARCH_REGEX, "GTK_HTML_COMMAND_SEARCH_REGEX", "search-regex" },
  { GTK_HTML_COMMAND_FOCUS_FORWARD, "GTK_HTML_COMMAND_FOCUS_FORWARD", "focus-forward" },
  { GTK_HTML_COMMAND_FOCUS_BACKWARD, "GTK_HTML_COMMAND_FOCUS_BACKWARD", "focus-backward" },
  { GTK_HTML_COMMAND_POPUP_MENU, "GTK_HTML_COMMAND_POPUP_MENU", "popup-menu" },
  { GTK_HTML_COMMAND_PROPERTIES_DIALOG, "GTK_HTML_COMMAND_PROPERTIES_DIALOG", "property-dialog" },
  { GTK_HTML_COMMAND_CURSOR_FORWARD, "GTK_HTML_COMMAND_CURSOR_FORWARD", "cursor-forward" },
  { GTK_HTML_COMMAND_CURSOR_BACKWARD, "GTK_HTML_COMMAND_CURSOR_BACKWARD", "cursor-backward" },
  { GTK_HTML_COMMAND_INSERT_TABLE_1_1, "GTK_HTML_COMMAND_INSERT_TABLE_1_1", "insert-table-1-1" },
  { GTK_HTML_COMMAND_TABLE_INSERT_COL_AFTER, "GTK_HTML_COMMAND_TABLE_INSERT_COL_AFTER", "insert-col-after" },
  { GTK_HTML_COMMAND_TABLE_INSERT_COL_BEFORE, "GTK_HTML_COMMAND_TABLE_INSERT_COL_BEFORE", "insert-col-before" },
  { GTK_HTML_COMMAND_TABLE_INSERT_ROW_AFTER, "GTK_HTML_COMMAND_TABLE_INSERT_ROW_AFTER", "insert-row-after" },
  { GTK_HTML_COMMAND_TABLE_INSERT_ROW_BEFORE, "GTK_HTML_COMMAND_TABLE_INSERT_ROW_BEFORE", "insert-row-before" },
  { GTK_HTML_COMMAND_TABLE_DELETE_COL, "GTK_HTML_COMMAND_TABLE_DELETE_COL", "delete-col" },
  { GTK_HTML_COMMAND_TABLE_DELETE_ROW, "GTK_HTML_COMMAND_TABLE_DELETE_ROW", "delete-row" },
  { GTK_HTML_COMMAND_TABLE_CELL_INC_CSPAN, "GTK_HTML_COMMAND_TABLE_CELL_INC_CSPAN", "inc-cspan" },
  { GTK_HTML_COMMAND_TABLE_CELL_DEC_CSPAN, "GTK_HTML_COMMAND_TABLE_CELL_DEC_CSPAN", "dec-cspan" },
  { GTK_HTML_COMMAND_TABLE_CELL_INC_RSPAN, "GTK_HTML_COMMAND_TABLE_CELL_INC_RSPAN", "inc-rspan" },
  { GTK_HTML_COMMAND_TABLE_CELL_DEC_RSPAN, "GTK_HTML_COMMAND_TABLE_CELL_DEC_RSPAN", "dec-rspan" },
  { GTK_HTML_COMMAND_TABLE_CELL_JOIN_LEFT, "GTK_HTML_COMMAND_TABLE_CELL_JOIN_LEFT", "cell-join-left" },
  { GTK_HTML_COMMAND_TABLE_CELL_JOIN_RIGHT, "GTK_HTML_COMMAND_TABLE_CELL_JOIN_RIGHT", "cell-join-right" },
  { GTK_HTML_COMMAND_TABLE_CELL_JOIN_UP, "GTK_HTML_COMMAND_TABLE_CELL_JOIN_UP", "cell-join-up" },
  { GTK_HTML_COMMAND_TABLE_CELL_JOIN_DOWN, "GTK_HTML_COMMAND_TABLE_CELL_JOIN_DOWN", "cell-join-down" },
  { GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_INC, "GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_INC", "inc-border" },
  { GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_DEC, "GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_DEC", "dec-border" },
  { GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_ZERO, "GTK_HTML_COMMAND_TABLE_BORDER_WIDTH_ZERO", "zero-border" },
  { GTK_HTML_COMMAND_TEXT_SET_DEFAULT_COLOR, "GTK_HTML_COMMAND_TEXT_SET_DEFAULT_COLOR", "text-default-color" },
  { GTK_HTML_COMMAND_CURSOR_BOD, "GTK_HTML_COMMAND_CURSOR_BOD", "cursor-bod" },
  { GTK_HTML_COMMAND_CURSOR_EOD, "GTK_HTML_COMMAND_CURSOR_EOD", "cursor-eod" },
  { 0, NULL, NULL }
};

GtkType
gtk_html_command_get_type ()
{
	static GtkType command_type = 0;

	if (!command_type)
		command_type = gtk_type_register_enum ("GTK_HTML_COMMAND", _gtk_html_command_values);

	return command_type;
}
