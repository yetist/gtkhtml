/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor-private.h
 *
 * Copyright (C) 2008 Novell, Inc.
 * Copyright (C) 2025 yetist <yetist@gmail.com>
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

#ifndef GTKHTML_EDITOR_PRIVATE_H
#define GTKHTML_EDITOR_PRIVATE_H

#include <gtkhtml-editor.h>

#include <glib/gi18n-lib.h>
#include <stdlib.h>

#ifdef HAVE_XFREE
#include <X11/XF86keysym.h>
#endif

/* Custom Widgets */
#include "gtkhtml-color-combo.h"
#include "gtkhtml-color-palette.h"
#include "gtkhtml-color-swatch.h"
#include "gtkhtml-combo-box.h"
#include "gtkhtml-face-action.h"
#include "gtkhtml-face-chooser.h"
#include "gtkhtml-image-chooser-dialog.h"
#include "gtkhtml-spell-dialog.h"

/* GtkHTML internals */
#include "gtkhtml-private.h"
#include "gtkhtmlfontstyle.h"
#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-movement.h"
#include "htmlengine-edit-rule.h"
#include "htmlengine-edit-table.h"
#include "htmlengine-edit-tablecell.h"
#include "htmlengine-edit-text.h"
#include "htmlengine-search.h"
#include "htmlimage.h"
#include "htmlobject.h"
#include "htmlpainter.h"
#include "htmlgdkpainter.h"
#include "htmlplainpainter.h"
#include "htmlrule.h"
#include "htmlselection.h"
#include "htmlsettings.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltext.h"
#include "htmltype.h"
#include "htmlundo.h"

/* Marshalling */
#include "gtkhtml-editor-marshal.h"

/* Shorthand, requires a variable named "editor". */
#define ACTION(name)	(GTKHTML_EDITOR_ACTION_##name (editor))
#define WIDGET(name)	(GTKHTML_EDITOR_WIDGET_##name (editor))

/* For use in dispose() methods. */
#define DISPOSE(obj) \
	G_STMT_START { \
	if ((obj) != NULL) { g_object_unref (obj); (obj) = NULL; } \
	} G_STMT_END

/* Regular expressions */
#define CTL	"\\x00-\\x1f\\x7f"
#define SEP	"\\x09\\x20\\(\\)<>@,;:\\\\\"/\\[\\]\\?=\\{\\}"
#define TOKEN	"[^" CTL SEP "]+"

/* Fatal error message given when a data file is not found. */
#define DATA_FILE_NOT_FOUND_MESSAGE \
"\n*** FATAL ERROR ***\n\n" \
PACKAGE " could not find the required file \"%s\" in any of\n" \
"the system-wide data directories given by the XDG_DATA_DIRS environment\n" \
"variable.  " PACKAGE " looked for:\n\n"

#define MORE_INFORMATION_MESSAGE \
"\nSee http://www.freedesktop.org/Standards/basedir-spec for more\n" \
"information about standard base directories.\n\n"

/* This controls how spelling suggestions are divided between the primary
 * context menu and a secondary menu.  The idea is to prevent the primary
 * menu from growing too long.
 *
 * The constants below are used as follows:
 *
 * if TOTAL_SUGGESTIONS <= MAX_LEVEL1_SUGGETIONS:
 *     LEVEL1_SUGGESTIONS = TOTAL_SUGGESTIONS
 * elif TOTAL_SUGGESTIONS - MAX_LEVEL1_SUGGESTIONS < MIN_LEVEL2_SUGGESTIONS:
 *     LEVEL1_SUGGESTIONS = TOTAL_SUGGESTIONS
 * else
 *     LEVEL1_SUGGESTIONS = MAX_LEVEL1_SUGGETIONS
 *
 * LEVEL2_SUGGESTIONS = TOTAL_SUGGESTIONS - LEVEL1_SUGGESTIONS
 *
 * Note that MAX_LEVEL1_SUGGETIONS is not a hard maximum.
 */
#define MAX_LEVEL1_SUGGESTIONS	4
#define MIN_LEVEL2_SUGGESTIONS	3

G_BEGIN_DECLS

typedef struct _GtkhtmlEditorRequest GtkhtmlEditorRequest;

typedef enum {
	EDITOR_MODE_HTML,
	EDITOR_MODE_TEXT
} EditorMode;

typedef enum {
	TABLE_CELL_SCOPE_CELL,
	TABLE_CELL_SCOPE_ROW,
	TABLE_CELL_SCOPE_COLUMN,
	TABLE_CELL_SCOPE_TABLE
} TableCellScope;

struct _GtkhtmlEditorPrivate {

	/*** UI Management ***/

	GtkUIManager *manager;
	GtkActionGroup *core_actions;
	GtkActionGroup *html_actions;
	GtkActionGroup *context_actions;
	GtkActionGroup *html_context_actions;
	GtkActionGroup *language_actions;
	GtkActionGroup *spell_check_actions;
	GtkActionGroup *suggestion_actions;
	GtkBuilder *builder;

	/*** Rendering ***/

	HTMLPainter *html_painter;
	HTMLPainter *plain_painter;

	/*** Spell Checking ***/

	GHashTable *available_spell_checkers;
	GHashTable *spell_suggestion_menus;
	GList *active_spell_checkers;
	guint spell_suggestions_merge_id;

	/*** Main Window Widgets ***/
	GtkWidget *vbox;
	GtkWidget *main_menu;
	GtkWidget *main_toolbar;
	GtkWidget *edit_toolbar;
	GtkWidget *html_toolbar;
	GtkWidget *edit_area;

	GtkWidget *color_combo_box;
	GtkWidget *mode_combo_box;
	GtkWidget *size_combo_box;
	GtkWidget *style_combo_box;
	GtkWidget *scrolled_window;

	/*** Cell Properties State ***/

	HTMLObject *cell_object;
	HTMLObject *cell_parent;
	TableCellScope cell_scope;

	/*** Image Properties State ***/

	HTMLObject *image_object;

	/*** Link Properties State ***/

	gboolean link_custom_description;

	/*** Rule Properties State ***/

	HTMLObject *rule_object;

	/*** Table Properties State ***/

	HTMLObject *table_object;

	/* Active URI Requests */

	GList *requests;

	/*** Miscellaneous ***/

	/* Note, 'filename' is not used by GtkhtmlEditor itself but is here
	 * for the convenience of extending the editor to support saving to
	 * a file. */

	gchar *filename;
	GtkhtmlColorPalette *palette;
	GtkhtmlColorState *text_color;
	guint ignore_style_change;
	gboolean changed;
};

struct _GtkhtmlEditorRequest {
	GtkhtmlEditor *editor;
	GCancellable *cancellable;
	GSimpleAsyncResult *simple;

	GFile *file;
	GInputStream *input_stream;
	GtkHTMLStream *output_stream;
	gchar buffer[4096];
};

void		gtkhtml_editor_private_init	(GtkhtmlEditor *editor);
void		gtkhtml_editor_private_constructed
						(GtkhtmlEditor *editor);
void		gtkhtml_editor_private_dispose	(GtkhtmlEditor *editor);
void		gtkhtml_editor_private_finalize	(GtkhtmlEditor *editor);
/* Private Utilities */

void		gtkhtml_editor_actions_init	(GtkhtmlEditor *editor);
gchar *		gtkhtml_editor_find_data_file	(const gchar *basename);
GFile *		gtkhtml_editor_run_open_dialog	(GtkhtmlEditor *editor,
						 const gchar *title,
						 GtkCallback customize_func,
						 gpointer customize_data);
void		gtkhtml_editor_show_uri		(GtkWindow *parent,
						 const gchar *uri);
void		gtkhtml_editor_spell_check	(GtkhtmlEditor *editor,
						 gboolean whole_document);
gboolean	gtkhtml_editor_next_spell_error	(GtkhtmlEditor *editor);
gboolean	gtkhtml_editor_prev_spell_error	(GtkhtmlEditor *editor);
void		gtkhtml_editor_update_context	(GtkhtmlEditor *editor);
void		gtkthtml_editor_emit_spell_languages_changed
						(GtkhtmlEditor *editor);

G_END_DECLS

#endif /* GTKHTML_EDITOR_PRIVATE_H */
