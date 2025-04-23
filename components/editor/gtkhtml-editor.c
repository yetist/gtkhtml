/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gtkhtml-editor.c
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "gtkhtml-editor.h"
#include "gtkhtml-editor-private.h"

enum {
	PROP_0,
	PROP_FILENAME,
	PROP_HTML,
	PROP_HTML_MODE,
	PROP_INLINE_SPELLING,
	PROP_MAGIC_LINKS,
	PROP_MAGIC_SMILEYS,
	PROP_CHANGED
};

enum {
	COMMAND_AFTER,
	COMMAND_BEFORE,
	IMAGE_URI,
	LINK_CLICKED,
	OBJECT_DELETED,
	URI_REQUESTED,
	SPELL_LANGUAGES_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE (GtkhtmlEditor, gtkhtml_editor, GTK_TYPE_WINDOW);

static void
editor_alignment_changed_cb (GtkhtmlEditor *editor,
                             GtkHTMLParagraphAlignment alignment)
{
	gtk_radio_action_set_current_value (
		GTK_RADIO_ACTION (ACTION (JUSTIFY_CENTER)), alignment);
}

static void
editor_show_popup_menu (GtkhtmlEditor *editor,
                        GdkEventButton *event,
                        HTMLObject *object,
                        guint offset)
{
	GtkHTML *html;
	GtkWidget *menu;
	gboolean in_selection;

	html = gtkhtml_editor_get_html (editor);
	menu = gtkhtml_editor_get_managed_widget (editor, "/context-menu");

	gtk_widget_grab_focus (GTK_WIDGET (html));

	/* Did we right-click in a selection? */
	in_selection =
		html_engine_is_selection_active (html->engine) &&
		html_engine_point_in_selection (html->engine, object, offset);

	if (!in_selection) {
		html_engine_disable_selection (html->engine);
		if (event != NULL)
			html_engine_jump_at (
				html->engine, event->x, event->y);
	}

	gtkhtml_editor_update_context (editor);

	if (event != NULL)
		gtk_menu_popup (
			GTK_MENU (menu), NULL, NULL, NULL, NULL,
			event->button, event->time);
	else
		gtk_menu_popup (
			GTK_MENU (menu), NULL, NULL, NULL, NULL,
			0, gtk_get_current_event_time ());
}

static gboolean
editor_button_press_event_cb (GtkhtmlEditor *editor,
                              GdkEventButton *event)
{
	GtkHTML *html;
	HTMLObject *object;
	guint offset;

	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	if (event->button != 3)
		return FALSE;

	html = gtkhtml_editor_get_html (editor);

	object = html_engine_get_object_at (
		html->engine, event->x, event->y, &offset, FALSE);

	editor_show_popup_menu (editor, event, object, offset);

	return TRUE;
}

static void
editor_font_style_changed_cb (GtkhtmlEditor *editor,
                              GtkHTMLFontStyle style)
{
	GtkHTMLFontStyle size;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	priv->ignore_style_change++;

	gtk_toggle_action_set_active (
		GTK_TOGGLE_ACTION (ACTION (BOLD)),
		style & GTK_HTML_FONT_STYLE_BOLD);

	gtk_toggle_action_set_active (
		GTK_TOGGLE_ACTION (ACTION (MONOSPACED)),
		style & GTK_HTML_FONT_STYLE_FIXED);

	gtk_toggle_action_set_active (
		GTK_TOGGLE_ACTION (ACTION (ITALIC)),
		style & GTK_HTML_FONT_STYLE_ITALIC);

	gtk_toggle_action_set_active (
		GTK_TOGGLE_ACTION (ACTION (STRIKETHROUGH)),
		style & GTK_HTML_FONT_STYLE_STRIKEOUT);

	gtk_toggle_action_set_active (
		GTK_TOGGLE_ACTION (ACTION (UNDERLINE)),
		style & GTK_HTML_FONT_STYLE_UNDERLINE);

	size = style & GTK_HTML_FONT_STYLE_SIZE_MASK;
	if (size == GTK_HTML_FONT_STYLE_DEFAULT)
		size = GTK_HTML_FONT_STYLE_SIZE_3;

	gtk_radio_action_set_current_value (
		GTK_RADIO_ACTION (ACTION (SIZE_PLUS_ZERO)), size);

	priv->ignore_style_change--;
}

static void
editor_indentation_changed_cb (GtkhtmlEditor *editor,
                               guint indentation_level)
{
	gtk_action_set_sensitive (ACTION (UNINDENT), indentation_level > 0);
}

static void
editor_paragraph_style_changed_cb (GtkhtmlEditor *editor,
                                   GtkHTMLParagraphStyle style)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	priv->ignore_style_change++;

	gtk_radio_action_set_current_value (
		GTK_RADIO_ACTION (ACTION (STYLE_NORMAL)), style);

	priv->ignore_style_change--;
}

static gboolean
editor_popup_menu_cb (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	HTMLObject *object;

	html = gtkhtml_editor_get_html (editor);
	object = html->engine->cursor->object;

	editor_show_popup_menu (editor, NULL, object, html->engine->cursor->offset);

	return TRUE;
}

static void
editor_text_color_changed_cb (GtkhtmlEditor *editor)
{
	GtkhtmlColorState *state;
	GdkColor gdk_color;
	GtkHTML *html;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);

	state = priv->text_color;
	html = gtkhtml_editor_get_html (editor);

	if (gtkhtml_color_state_get_current_color (state, &gdk_color)) {
		HTMLColor *color;

		color = html_color_new_from_gdk_color (&gdk_color);
		gtk_html_set_color (html, color);
		html_color_unref (color);
	} else
		gtk_html_set_color (html, NULL);
}

static void
editor_url_requested_cb (GtkhtmlEditor *editor,
                         const gchar *url,
                         GtkHTMLStream *stream)
{
	g_signal_emit (editor, signals[URI_REQUESTED], 0, url, stream);
}

static void
engine_undo_changed_cb (GtkhtmlEditor *editor,
                        HTMLEngine *engine)
{
	g_object_notify (G_OBJECT (editor), "changed");
}

static void
editor_command_popup_menu (GtkhtmlEditor *editor)
{
	g_warning ("GtkHTML command \"popup-menu\" not implemented");
}

static void
editor_command_properties_dialog (GtkhtmlEditor *editor)
{
	g_warning ("GtkHTML command \"property-dialog\" not implemented");
}

static void
editor_command_text_color_apply (GtkhtmlEditor *editor)
{
	GObject *object;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);

	object = G_OBJECT (priv->text_color);
	g_object_notify (object, "current-color");
}

static gboolean
editor_method_check_word (GtkHTML *html,
                          const gchar *word,
                          gpointer user_data)
{
	GtkhtmlEditor *editor = user_data;
	gboolean correct = FALSE;
	GList *list;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);

	list = priv->active_spell_checkers;

	/* If no spell checkers are active, assume the word is correct. */
	if (list == NULL)
		return TRUE;

	/* The word is correct if ANY active spell checker can verify it. */
	while (list != NULL && !correct) {
		GtkhtmlSpellChecker *checker = list->data;

		correct = gtkhtml_spell_checker_check_word (checker, word, -1);
		list = g_list_next (list);
	}

	return correct;
}

static void
editor_method_suggestion_request (GtkHTML *html,
                                  gpointer user_data)
{
	g_warning ("GtkHTML suggestion_request() method not implemented");
}

static void
editor_method_add_to_session (GtkHTML *html,
                              const gchar *word,
                              gpointer user_data)
{
	g_warning ("GtkHTML add_to_session() method not implemented");
}

static void
editor_method_add_to_personal (GtkHTML *html,
                               const gchar *word,
                               const gchar *language_code,
                               gpointer user_data)
{
	GtkhtmlEditor *editor = user_data;
	const GtkhtmlSpellLanguage *language;
	GtkhtmlSpellChecker *checker;
	GHashTable *hash_table;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	language = gtkhtml_spell_language_lookup (language_code);
	g_return_if_fail (language != NULL);

	hash_table = priv->available_spell_checkers;
	checker = g_hash_table_lookup (hash_table, language);
	g_return_if_fail (checker != NULL);

	gtkhtml_spell_checker_add_word (checker, word, -1);
}

static gboolean
editor_method_command (GtkHTML *html,
                       GtkHTMLCommandType command,
                       gpointer user_data)
{
	GtkhtmlEditor *editor = user_data;
	gboolean handled = TRUE;

	switch (command) {
		case GTK_HTML_COMMAND_POPUP_MENU:
			editor_command_popup_menu (editor);
			break;

		case GTK_HTML_COMMAND_PROPERTIES_DIALOG:
			editor_command_properties_dialog (editor);
			break;

		case GTK_HTML_COMMAND_TEXT_COLOR_APPLY:
			editor_command_text_color_apply (editor);
			break;

		default:
			handled = FALSE;
			break;
	}

	return handled;
}

static GValue *
editor_method_event (GtkHTML *html,
                     GtkHTMLEditorEventType event,
                     GValue *args,
                     gpointer user_data)
{
	GtkhtmlEditor *editor = user_data;
	GValue *return_value = NULL;
	gchar *return_string = NULL;
	const gchar *string = NULL;
	guint signal_id;

	/* GtkHTML event arguments are either NULL or a single string. */
	if (args != NULL && G_VALUE_HOLDS (args, G_TYPE_STRING))
		string = g_value_get_string (args);

	switch (event) {
		case GTK_HTML_EDITOR_EVENT_COMMAND_BEFORE:
			/* Signal argument is the command name. */
			signal_id = signals[COMMAND_BEFORE];
			g_return_val_if_fail (string != NULL, NULL);
			g_signal_emit (editor, signal_id, 0, string);
			break;

		case GTK_HTML_EDITOR_EVENT_COMMAND_AFTER:
			/* Signal argument is the command name. */
			signal_id = signals[COMMAND_AFTER];
			g_return_val_if_fail (string != NULL, NULL);
			g_signal_emit (editor, signal_id, 0, string);
			break;

		case GTK_HTML_EDITOR_EVENT_IMAGE_URL:
			/* Signal argument is the image URL. */
			/* XXX Returns a string. */
			signal_id = signals[IMAGE_URI];
			g_return_val_if_fail (string != NULL, NULL);
			g_signal_emit (
				editor, signal_id, 0, string, &return_string);
			return_value = g_new0 (GValue, 1);
			g_value_init (return_value, G_TYPE_STRING);
			g_value_take_string (return_value, return_string);
			break;

		case GTK_HTML_EDITOR_EVENT_DELETE:
			/* No signal arguments. */
			signal_id = signals[OBJECT_DELETED];
			g_return_val_if_fail (string == NULL, NULL);
			g_signal_emit (editor, signal_id, 0);
			break;

		case GTK_HTML_EDITOR_EVENT_LINK_CLICKED:
			/* Signal argument is the link URL. */
			signal_id = signals[LINK_CLICKED];
			g_return_val_if_fail (string != NULL, NULL);
			g_signal_emit (editor, signal_id, 0, string);
			break;
	}

	return return_value;
}

static GtkWidget *
editor_method_create_input_line (GtkHTML *html,
                                 gpointer user_data)
{
	g_warning ("GtkHTML create_input_line() method not implemented");

	return NULL;
}

static void
editor_method_set_language (GtkHTML *html,
                            const gchar *language,
                            gpointer user_data)
{
	GtkhtmlEditor *editor = user_data;
	GtkActionGroup *action_group;
	GtkAction *action;
	gchar *action_name;
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	action_group = priv->language_actions;
	action_name = g_strdelimit (g_strdup (language), "-", '_');
	action = gtk_action_group_get_action (action_group, action_name);
	g_free (action_name);

	/* silently ignore when such language doesn't exist, it might mean that
	 * user doesn't have installed language for locale the editor is run in
	*/
	if (action != NULL)
		gtk_action_activate (action);
}

static GtkHTMLEditorAPI editor_api = {
	editor_method_check_word,
	editor_method_suggestion_request,
	editor_method_add_to_session,
	editor_method_add_to_personal,
	editor_method_command,
	editor_method_event,
	editor_method_create_input_line,
	editor_method_set_language
};

static void
editor_set_html (GtkhtmlEditor *editor,
                 GtkHTML *html)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	g_return_if_fail (priv->edit_area == NULL);

	if (html == NULL)
		html = (GtkHTML *) gtk_html_new ();
	else
		g_return_if_fail (GTK_IS_HTML (html));

	gtk_html_load_empty (html);
	gtk_html_set_editable (html, TRUE);

	priv->edit_area = (GtkWidget*) g_object_ref_sink (html);
}

static GObject *
editor_constructor (GType type,
                    guint n_construct_properties,
                    GObjectConstructParam *construct_properties)
{
	GtkHTML *html;
	GtkhtmlEditor *editor;
	GObject *object;
	GtkhtmlEditorPrivate *priv;


	/* Chain up to parent's constructor() method. */
	object = G_OBJECT_CLASS (gtkhtml_editor_parent_class)->constructor (
		type, n_construct_properties, construct_properties);

	editor = GTKHTML_EDITOR (object);
	gtkhtml_editor_private_constructed (editor);

	html = gtkhtml_editor_get_html (editor);
	gtk_html_set_editor_api (html, &editor_api, editor);
	priv = gtkhtml_editor_get_instance_private (editor);

	gtk_container_add (GTK_CONTAINER (object), priv->vbox);
	gtk_window_set_default_size (GTK_WINDOW (object), 600, 440);

	/* Listen for events from core and update the UI accordingly. */

	g_signal_connect_swapped (
		html, "button_press_event",
		G_CALLBACK (editor_button_press_event_cb), editor);

	g_signal_connect_swapped (
		html, "current_paragraph_alignment_changed",
		G_CALLBACK (editor_alignment_changed_cb), editor);

	g_signal_connect_swapped (
		html, "current_paragraph_indentation_changed",
		G_CALLBACK (editor_indentation_changed_cb), editor);

	g_signal_connect_swapped (
		html, "current_paragraph_style_changed",
		G_CALLBACK (editor_paragraph_style_changed_cb), editor);

	g_signal_connect_swapped (
		html, "insertion_font_style_changed",
		G_CALLBACK (editor_font_style_changed_cb), editor);

	g_signal_connect_swapped (
		html, "popup-menu",
		G_CALLBACK (editor_popup_menu_cb), editor);

	g_signal_connect_swapped (
		html, "url_requested",
		G_CALLBACK (editor_url_requested_cb), editor);

	g_signal_connect_swapped (
		html->engine, "undo-changed",
		G_CALLBACK (engine_undo_changed_cb), editor);

	/* Connect property dialog widgets to actions. */

	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (TEXT_PROPERTIES_BOLD_BUTTON)),
		ACTION (BOLD));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (REPLACE_CONFIRMATION_REPLACE_BUTTON)),
		ACTION (CONFIRM_REPLACE));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (REPLACE_CONFIRMATION_REPLACE_ALL_BUTTON)),
		ACTION (CONFIRM_REPLACE_ALL));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (REPLACE_CONFIRMATION_CLOSE_BUTTON)),
		ACTION (CONFIRM_REPLACE_CANCEL));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (REPLACE_CONFIRMATION_NEXT_BUTTON)),
		ACTION (CONFIRM_REPLACE_NEXT));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (FIND_BUTTON)),
		ACTION (FIND));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (REPLACE_BUTTON)),
		ACTION (FIND_AND_REPLACE));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (TEXT_PROPERTIES_ITALIC_BUTTON)),
		ACTION (ITALIC));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (PARAGRAPH_PROPERTIES_CENTER_BUTTON)),
		ACTION (JUSTIFY_CENTER));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (PARAGRAPH_PROPERTIES_LEFT_BUTTON)),
		ACTION (JUSTIFY_LEFT));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (PARAGRAPH_PROPERTIES_RIGHT_BUTTON)),
		ACTION (JUSTIFY_RIGHT));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (TEXT_PROPERTIES_STRIKETHROUGH_BUTTON)),
		ACTION (STRIKETHROUGH));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (LINK_PROPERTIES_TEST_BUTTON)),
		ACTION (TEST_URL));
	gtk_activatable_set_related_action (
		GTK_ACTIVATABLE (WIDGET (TEXT_PROPERTIES_UNDERLINE_BUTTON)),
		ACTION (UNDERLINE));

	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (CELL_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (FIND_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (IMAGE_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (LINK_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (PAGE_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (PARAGRAPH_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (REPLACE_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (REPLACE_CONFIRMATION_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (RULE_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (TABLE_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));
	gtk_window_set_transient_for (
		GTK_WINDOW (WIDGET (TEXT_PROPERTIES_WINDOW)),
		GTK_WINDOW (object));

	/* Initialize various GtkhtmlComboBox instances. */
	g_object_set (
		WIDGET (PARAGRAPH_PROPERTIES_STYLE_COMBO_BOX),
		"action", ACTION (STYLE_NORMAL), NULL);
	g_object_set (
		WIDGET (TEXT_PROPERTIES_SIZE_COMBO_BOX),
		"action", ACTION (SIZE_PLUS_ZERO), NULL);

	g_signal_connect_swapped (
		priv->text_color, "notify::current-color",
		G_CALLBACK (editor_text_color_changed_cb), editor);

	gtkhtml_editor_set_html_mode (editor, TRUE);

	return object;
}

static void
editor_set_property (GObject *object,
                     guint property_id,
                     const GValue *value,
                     GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_FILENAME:
			gtkhtml_editor_set_filename (
				GTKHTML_EDITOR (object),
				g_value_get_string (value));
			return;

		case PROP_HTML:
			editor_set_html (
				GTKHTML_EDITOR (object),
				g_value_get_object (value));
			return;

		case PROP_HTML_MODE:
			gtkhtml_editor_set_html_mode (
				GTKHTML_EDITOR (object),
				g_value_get_boolean (value));
			return;

		case PROP_INLINE_SPELLING:
			gtkhtml_editor_set_inline_spelling (
				GTKHTML_EDITOR (object),
				g_value_get_boolean (value));
			return;

		case PROP_MAGIC_LINKS:
			gtkhtml_editor_set_magic_links (
				GTKHTML_EDITOR (object),
				g_value_get_boolean (value));
			return;

		case PROP_MAGIC_SMILEYS:
			gtkhtml_editor_set_magic_smileys (
				GTKHTML_EDITOR (object),
				g_value_get_boolean (value));
			return;
		case PROP_CHANGED:
			gtkhtml_editor_set_changed (
				GTKHTML_EDITOR (object),
				g_value_get_boolean (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
editor_get_property (GObject *object,
                     guint property_id,
                     GValue *value,
                     GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_FILENAME:
			g_value_set_string (
				value, gtkhtml_editor_get_filename (
				GTKHTML_EDITOR (object)));
			return;

		case PROP_HTML:
			g_value_set_object (
				value, gtkhtml_editor_get_html (
				GTKHTML_EDITOR (object)));
			return;

		case PROP_HTML_MODE:
			g_value_set_boolean (
				value, gtkhtml_editor_get_html_mode (
				GTKHTML_EDITOR (object)));
			return;

		case PROP_INLINE_SPELLING:
			g_value_set_boolean (
				value, gtkhtml_editor_get_inline_spelling (
				GTKHTML_EDITOR (object)));
			return;

		case PROP_MAGIC_LINKS:
			g_value_set_boolean (
				value, gtkhtml_editor_get_magic_links (
				GTKHTML_EDITOR (object)));
			return;

		case PROP_MAGIC_SMILEYS:
			g_value_set_boolean (
				value, gtkhtml_editor_get_magic_smileys (
				GTKHTML_EDITOR (object)));
			return;
		case PROP_CHANGED:
			g_value_set_boolean (
				value, gtkhtml_editor_get_changed (
				GTKHTML_EDITOR (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
editor_dispose (GObject *object)
{
	GtkhtmlEditorPrivate *priv;
	GtkhtmlEditor *editor;

	editor = GTKHTML_EDITOR (object);
	priv = gtkhtml_editor_get_instance_private (editor);

	DISPOSE (priv->vbox);

	gtkhtml_editor_private_dispose (editor);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (gtkhtml_editor_parent_class)->dispose (object);
}

static void
editor_finalize (GObject *object)
{
	GtkhtmlEditor *editor = GTKHTML_EDITOR (object);

	gtkhtml_editor_private_finalize (editor);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (gtkhtml_editor_parent_class)->finalize (object);
}

static gboolean
editor_key_press_event (GtkWidget *widget,
                        GdkEventKey *event)
{
#ifdef HAVE_XFREE
	GtkhtmlEditor *editor = GTKHTML_EDITOR (widget);

	if (event->keyval == XF86XK_Spell) {
		gtk_action_activate (ACTION (SPELL_CHECK));
		return TRUE;
	}
#endif

	/* Chain up to parent's key_press_event() method. */
	return GTK_WIDGET_CLASS (gtkhtml_editor_parent_class)->key_press_event (widget, event);
}

static void
editor_cut_clipboard (GtkhtmlEditor *editor)
{
	gtk_html_command (gtkhtml_editor_get_html (editor), "cut");
}

static void
editor_copy_clipboard (GtkhtmlEditor *editor)
{
	gtk_html_command (gtkhtml_editor_get_html (editor), "copy");
}

static void
editor_paste_clipboard (GtkhtmlEditor *editor)
{
	gtk_html_command (gtkhtml_editor_get_html (editor), "paste");
}

static void
editor_select_all (GtkhtmlEditor *editor)
{
	gtk_html_command (gtkhtml_editor_get_html (editor), "select-all");
}

static void
gtkhtml_editor_class_init (GtkhtmlEditorClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->constructor = editor_constructor;
	object_class->set_property = editor_set_property;
	object_class->get_property = editor_get_property;
	object_class->dispose = editor_dispose;
	object_class->finalize = editor_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->key_press_event = editor_key_press_event;

	class->cut_clipboard = editor_cut_clipboard;
	class->copy_clipboard = editor_copy_clipboard;
	class->paste_clipboard = editor_paste_clipboard;
	class->select_all = editor_select_all;

	g_object_class_install_property (
		object_class,
		PROP_FILENAME,
		g_param_spec_string (
			"filename",
			"Filename",
			"The filename to use when saving",
			NULL,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_HTML,
		g_param_spec_object (
			"html",
			"HTML Editing Widget",
			"The main HTML editing widget",
			GTK_TYPE_HTML,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_HTML_MODE,
		g_param_spec_boolean (
			"html-mode",
			"HTML Mode",
			"Edit HTML or plain text",
			TRUE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_INLINE_SPELLING,
		g_param_spec_boolean (
			"inline-spelling",
			"Inline Spelling",
			"Check your spelling as you type",
			TRUE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_MAGIC_LINKS,
		g_param_spec_boolean (
			"magic-links",
			"Magic Links",
			"Make URIs clickable as you type",
			TRUE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_MAGIC_SMILEYS,
		g_param_spec_boolean (
			"magic-smileys",
			"Magic Smileys",
			"Convert emoticons to images as you type",
			TRUE,
			G_PARAM_READWRITE |
			G_PARAM_CONSTRUCT));

	g_object_class_install_property (
		object_class,
		PROP_CHANGED,
		g_param_spec_boolean (
			"changed",
			_("Changed property"),
			_("Whether editor changed"),
			FALSE,
			G_PARAM_READWRITE));

	signals[COMMAND_AFTER] = g_signal_new (
		"command-after",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, command_after),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE, 1,
		G_TYPE_STRING);

	signals[COMMAND_BEFORE] = g_signal_new (
		"command-before",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, command_before),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE, 1,
		G_TYPE_STRING);

	signals[IMAGE_URI] = g_signal_new (
		"image-uri",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, image_uri),
		NULL, NULL,
		gtkhtml_editor_marshal_STRING__STRING,
		G_TYPE_STRING, 1,
		G_TYPE_STRING);

	signals[LINK_CLICKED] = g_signal_new (
		"link-clicked",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, link_clicked),
		NULL, NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE, 1,
		G_TYPE_STRING);

	signals[OBJECT_DELETED] = g_signal_new (
		"object-deleted",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, object_deleted),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	signals[URI_REQUESTED] = g_signal_new (
		"uri-requested",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GtkhtmlEditorClass, uri_requested),
		NULL, NULL,
		gtkhtml_editor_marshal_VOID__STRING_POINTER,
		G_TYPE_NONE, 2,
		G_TYPE_STRING,
		G_TYPE_POINTER);

	signals[SPELL_LANGUAGES_CHANGED] = g_signal_new (
		"spell-languages-changed",
		G_OBJECT_CLASS_TYPE (class),
		G_SIGNAL_RUN_LAST,
		0,
		NULL, NULL,
		g_cclosure_marshal_VOID__POINTER,
		G_TYPE_NONE, 1,
		G_TYPE_POINTER);
}

static void
gtkhtml_editor_init (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	priv->vbox = g_object_ref_sink (gtk_vbox_new (FALSE, 0));
	gtk_widget_show (priv->vbox);

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
				   "/gtkhtml/icons/");
//	editor->priv = priv;
	gtkhtml_editor_private_init (editor);
}

GtkWidget *
gtkhtml_editor_new (void)
{
	return g_object_new (GTKHTML_TYPE_EDITOR, NULL);
}

GtkHTML *
gtkhtml_editor_get_html (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	priv = gtkhtml_editor_get_instance_private (editor);
	return GTK_HTML (priv->edit_area);
}

GtkBuilder *
gtkhtml_editor_get_builder (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;
	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	priv = gtkhtml_editor_get_instance_private (editor);

	return priv->builder;
}

GtkUIManager *
gtkhtml_editor_get_ui_manager (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;
	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	priv = gtkhtml_editor_get_instance_private (editor);
	return priv->manager;
}

GtkAction *
gtkhtml_editor_get_action (GtkhtmlEditor *editor,
                           const gchar *action_name)
{
	GtkUIManager *manager;
	GtkAction *action = NULL;
	GList *iter;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (action_name != NULL, NULL);

	manager = gtkhtml_editor_get_ui_manager (editor);
	iter = gtk_ui_manager_get_action_groups (manager);

	while (iter != NULL && action == NULL) {
		GtkActionGroup *action_group = iter->data;

		action = gtk_action_group_get_action (
			action_group, action_name);
		iter = g_list_next (iter);
	}

	g_return_val_if_fail (action != NULL, NULL);

	return action;
}

GtkActionGroup *
gtkhtml_editor_get_action_group (GtkhtmlEditor *editor,
                                 const gchar *group_name)
{
	GtkUIManager *manager;
	GList *iter;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (group_name != NULL, NULL);

	manager = gtkhtml_editor_get_ui_manager (editor);
	iter = gtk_ui_manager_get_action_groups (manager);

	while (iter != NULL) {
		GtkActionGroup *action_group = iter->data;
		const gchar *name;

		name = gtk_action_group_get_name (action_group);
		if (strcmp (name, group_name) == 0)
			return action_group;

		iter = g_list_next (iter);
	}

	return NULL;
}

GtkWidget *
gtkhtml_editor_get_widget (GtkhtmlEditor *editor,
                           const gchar *widget_name)
{
	GtkBuilder *builder;
	GObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (widget_name != NULL, NULL);

	builder = gtkhtml_editor_get_builder (editor);
	object = gtk_builder_get_object (builder, widget_name);
	g_return_val_if_fail (GTK_IS_WIDGET (object), NULL);

	return GTK_WIDGET (object);
}

GtkWidget *
gtkhtml_editor_get_managed_widget (GtkhtmlEditor *editor,
                                   const gchar *widget_path)
{
	GtkUIManager *manager;
	GtkWidget *widget;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (widget_path != NULL, NULL);

	manager = gtkhtml_editor_get_ui_manager (editor);
	widget = gtk_ui_manager_get_widget (manager, widget_path);

	g_return_val_if_fail (widget != NULL, NULL);

	return widget;
}

gboolean
gtkhtml_editor_get_changed (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	GtkhtmlEditorPrivate *priv;

	/* XXX GtkHTML does not notify us when its internal "saved" state
	 *     changes, so we can't have a "changed" property because its
	 *     notifications would be unreliable. */

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	priv = gtkhtml_editor_get_instance_private (editor);
	html = gtkhtml_editor_get_html (editor);

	return priv->changed || !html_engine_is_saved (html->engine);
}

void
gtkhtml_editor_set_changed (GtkhtmlEditor *editor,
                            gboolean changed)
{
	GtkHTML *html;
	GtkhtmlEditorPrivate *priv;

	/* XXX GtkHTML does not notify us when its internal "saved" state
	 *     changes, so we can't have a "changed" property because its
	 *     notifications would be unreliable. */

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	if (!changed) {
		html = gtkhtml_editor_get_html (editor);

		/* XXX The NULL check was added to deal with a race in
		 *     Evolution when saving a message to a remote Drafts
		 *     folder (which is asynchronous) and then closing the
		 *     editor (which is immediate).  The GtkHTML object
		 *     may already be disposed by the time the Save Draft
		 *     callback gets here.  See bug #553995 for details. */
		if (html)
			html_engine_saved (html->engine);
	}
	priv = gtkhtml_editor_get_instance_private (editor);
	priv->changed = changed;

	g_object_notify (G_OBJECT (editor), "changed");
}

const gchar *
gtkhtml_editor_get_filename (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;
	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	priv = gtkhtml_editor_get_instance_private (editor);
	return priv->filename;
}

void
gtkhtml_editor_set_filename (GtkhtmlEditor *editor,
                             const gchar *filename)
{
	GtkhtmlEditorPrivate *priv;
	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	priv = gtkhtml_editor_get_instance_private (editor);

	g_free (priv->filename);
	priv->filename = g_strdup (filename);

	g_object_notify (G_OBJECT (editor), "filename");
}

gboolean
gtkhtml_editor_get_html_mode (GtkhtmlEditor *editor)
{
	GtkRadioAction *action;
	EditorMode mode;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	action = GTK_RADIO_ACTION (ACTION (MODE_HTML));
	mode = gtk_radio_action_get_current_value (action);

	return (mode == EDITOR_MODE_HTML);
}

void
gtkhtml_editor_set_html_mode (GtkhtmlEditor *editor,
                              gboolean html_mode)
{
	GtkRadioAction *action;
	EditorMode mode;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	action = GTK_RADIO_ACTION (ACTION (MODE_HTML));
	mode = html_mode ? EDITOR_MODE_HTML : EDITOR_MODE_TEXT;
	gtk_radio_action_set_current_value (action, mode);

	/* We emit "notify::html-mode" in the action handler. */
}

gboolean
gtkhtml_editor_get_inline_spelling (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	return gtk_html_get_inline_spelling (html);
}

void
gtkhtml_editor_set_inline_spelling (GtkhtmlEditor *editor,
                                    gboolean inline_spelling)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);
	gtk_html_set_inline_spelling (html, inline_spelling);

	g_object_notify (G_OBJECT (editor), "inline-spelling");
}

gboolean
gtkhtml_editor_get_magic_links (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	return gtk_html_get_magic_links (html);
}

void
gtkhtml_editor_set_magic_links (GtkhtmlEditor *editor,
                                gboolean magic_links)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);
	gtk_html_set_magic_links (html, magic_links);

	g_object_notify (G_OBJECT (editor), "magic-links");
}

gboolean
gtkhtml_editor_get_magic_smileys (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	return gtk_html_get_magic_smileys (html);
}

void
gtkhtml_editor_set_magic_smileys (GtkhtmlEditor *editor,
                                  gboolean magic_smileys)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);
	gtk_html_set_magic_smileys (html, magic_smileys);

	g_object_notify (G_OBJECT (editor), "magic-smileys");
}

GList *
gtkhtml_editor_get_spell_languages (GtkhtmlEditor *editor)
{
	GList *spell_languages = NULL;
	GtkActionGroup *action_group;
	GList *list;
	GtkhtmlEditorPrivate *priv;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	priv = gtkhtml_editor_get_instance_private (editor);
	action_group = priv->language_actions;
	list = gtk_action_group_list_actions (action_group);

	while (list != NULL) {
		GtkToggleAction *action = list->data;
		const GtkhtmlSpellLanguage *language;
		const gchar *language_code;

		list = g_list_delete_link (list, list);
		if (!gtk_toggle_action_get_active (action))
			continue;

		language_code = gtk_action_get_name (GTK_ACTION (action));
		language = gtkhtml_spell_language_lookup (language_code);
		if (language == NULL)
			continue;

		spell_languages = g_list_prepend (
			spell_languages, (gpointer) language);
	}

	return g_list_reverse (spell_languages);
}

void
gtkhtml_editor_set_spell_languages (GtkhtmlEditor *editor,
                                    GList *spell_languages)
{
	GtkActionGroup *action_group;
	GList *list;
	GtkhtmlEditorPrivate *priv;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));
	priv = gtkhtml_editor_get_instance_private (editor);

	action_group = priv->language_actions;
	list = gtk_action_group_list_actions (action_group);

	while (list != NULL) {
		GtkToggleAction *action = list->data;
		const GtkhtmlSpellLanguage *language;
		const gchar *language_code;
		gboolean active;

		list = g_list_delete_link (list, list);
		language_code = gtk_action_get_name (GTK_ACTION (action));
		language = gtkhtml_spell_language_lookup (language_code);
		active = (g_list_find (spell_languages, language) != NULL);

		gtk_toggle_action_set_active (action, active);
	}
}

/* Helper for gtkhtml_editor_get_text_[html/plain]() */
static gboolean
editor_save_receiver (HTMLEngine *engine,
                      const gchar *data,
                      gsize length,
                      GString *contents)
{
	g_string_append_len (contents, data, length);

	return TRUE;
}

gchar *
gtkhtml_editor_get_text_html (GtkhtmlEditor *editor,
                              gsize *length)
{
	GString *string;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	string = g_string_sized_new (4096);

	gtk_html_export (
		gtkhtml_editor_get_html (editor), "text/html",
		(GtkHTMLSaveReceiverFn) editor_save_receiver, string);

	if (length != NULL)
		*length = string->len;

	return g_string_free (string, FALSE);
}

gchar *
gtkhtml_editor_get_text_plain (GtkhtmlEditor *editor,
                               gsize *length)
{
	GString *string;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	string = g_string_sized_new (4096);

	gtk_html_export (
		gtkhtml_editor_get_html (editor), "text/plain",
		(GtkHTMLSaveReceiverFn) editor_save_receiver, string);

	if (length != NULL)
		*length = string->len;

	return g_string_free (string, FALSE);
}

void
gtkhtml_editor_set_text_html (GtkhtmlEditor *editor,
                              const gchar *text,
                              gssize length)
{
	GtkHTML *html;
	GtkHTMLStream *stream;
	gboolean editable;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));
	g_return_if_fail (text != NULL);

	/* XXX gtk_html_write() should really do this. */
	if (length < 0)
		length = strlen (text);

	html = gtkhtml_editor_get_html (editor);
	editable = gtk_html_get_editable (html);
	gtk_html_set_editable (html, FALSE);

	stream = gtk_html_begin_content (html, "text/html; charset=utf-8");

	/* XXX gtk_html_write() requires length > 0, which is
	 *     unnecessary.  Zero length should be a no-op. */
	if (length > 0)
		gtk_html_write (html, stream, text, length);

	gtk_html_end (html, stream, GTK_HTML_STREAM_OK);

	gtk_html_set_editable (html, editable);
}

gboolean
gtkhtml_editor_save (GtkhtmlEditor *editor,
                     const gchar *filename,
                     gboolean as_html,
                     GError **error)
{
	gchar *contents;
	gsize length;
	gboolean success;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (as_html)
		contents = gtkhtml_editor_get_text_html (editor, &length);
	else
		contents = gtkhtml_editor_get_text_plain (editor, &length);

	success = g_file_set_contents (filename, contents, length, error);

	g_free (contents);

	if (success) {
		GtkHTML *html;

		html = gtkhtml_editor_get_html (editor);
		html->engine->saved_step_count =
			html_undo_get_step_count (html->engine->undo);
		gtkhtml_editor_run_command (editor, "saved");
	}

	return success;
}

const gchar *
gtkhtml_editor_get_paragraph_data (GtkhtmlEditor *editor,
                                   const gchar *key)
{
	GtkHTML *html;
	HTMLObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	html = gtkhtml_editor_get_html (editor);

	object = html->engine->cursor->object;
	if (object == NULL)
		return NULL;

	object = object->parent;
	if (object == NULL)
		return NULL;

	if (!HTML_IS_CLUEFLOW (object))
		return NULL;

	return html_object_get_data (object, key);
}

gboolean
gtkhtml_editor_set_paragraph_data (GtkhtmlEditor *editor,
                                   const gchar *key,
                                   const gchar *value)
{
	GtkHTML *html;
	HTMLObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	html = gtkhtml_editor_get_html (editor);

	object = html->engine->cursor->object;
	if (object == NULL)
		return FALSE;

	object = object->parent;
	if (object == NULL)
		return FALSE;

	if (HTML_OBJECT_TYPE (object) != HTML_TYPE_CLUEFLOW)
		return FALSE;

	html_object_set_data (object, key, value);

	return TRUE;
}

gboolean
gtkhtml_editor_run_command (GtkhtmlEditor *editor,
                            const gchar *command)
{
	GtkHTML *html;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);
	g_return_val_if_fail (command != NULL, FALSE);

	html = gtkhtml_editor_get_html (editor);

	return gtk_html_command (html, command);
}

gboolean
gtkhtml_editor_is_paragraph_empty (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	HTMLObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	object = html->engine->cursor->object;
	if (object == NULL)
		return FALSE;

	object = object->parent;
	if (object == NULL)
		return FALSE;

	if (HTML_OBJECT_TYPE (object) != HTML_TYPE_CLUEFLOW)
		return FALSE;

	return html_clueflow_is_empty (HTML_CLUEFLOW (object));
}

gboolean
gtkhtml_editor_is_previous_paragraph_empty (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	HTMLObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	object = html->engine->cursor->object;
	if (object == NULL)
		return FALSE;

	object = object->parent;
	if (object == NULL)
		return FALSE;

	object = object->prev;
	if (object == NULL)
		return FALSE;

	if (HTML_OBJECT_TYPE (object) != HTML_TYPE_CLUEFLOW)
		return FALSE;

	return html_clueflow_is_empty (HTML_CLUEFLOW (object));
}

void
gtkhtml_editor_insert_html (GtkhtmlEditor *editor,
                            const gchar *html_text)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	gtk_html_insert_html (html, html_text);
}

void
gtkhtml_editor_insert_image (GtkhtmlEditor *editor,
                             const gchar *image_uri)
{
	GtkHTML *html;
	HTMLObject *image;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));
	g_return_if_fail (image_uri != NULL);

	html = gtkhtml_editor_get_html (editor);

	image = html_image_new (
		html_engine_get_image_factory (html->engine), image_uri,
		NULL, NULL, 0, 0, 0, 0, 0, NULL, HTML_VALIGN_NONE, FALSE);

	html_engine_paste_object (html->engine, image, 1);
}

void
gtkhtml_editor_insert_text (GtkhtmlEditor *editor,
                            const gchar *plain_text)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	html_engine_paste_text (html->engine, plain_text, -1);
}

gboolean
gtkhtml_editor_search_by_data (GtkhtmlEditor *editor,
                               glong level,
                               const gchar *klass,
                               const gchar *key,
                               const gchar *value)
{
	GtkHTML *html;
	HTMLObject *last_object = NULL;
	HTMLObject *object;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	html = gtkhtml_editor_get_html (editor);

	do {
		if (html->engine->cursor->object != last_object) {
			object = html_object_nth_parent (
				html->engine->cursor->object, level);
			if (object != NULL) {
				const gchar *object_value;

				object_value =
					html_object_get_data (object, key);
				if (object_value != NULL &&
					strcmp (object_value, value) == 0)
					return TRUE;
			}
		}
		last_object = html->engine->cursor->object;
	} while (html_cursor_forward (html->engine->cursor, html->engine));

	return FALSE;
}

void
gtkhtml_editor_freeze (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	html_engine_freeze (html->engine);
}

void
gtkhtml_editor_thaw (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	html_engine_thaw (html->engine);
}

void
gtkhtml_editor_undo_begin (GtkhtmlEditor *editor,
                           const gchar *undo_name,
                           const gchar *redo_name)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));
	g_return_if_fail (undo_name != NULL);
	g_return_if_fail (redo_name != NULL);

	html = gtkhtml_editor_get_html (editor);

	html_undo_level_begin (html->engine->undo, undo_name, redo_name);
}

void
gtkhtml_editor_undo_end (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	html_undo_level_end (html->engine->undo, html->engine);
}

gboolean
gtkhtml_editor_has_undo (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	return gtk_html_has_undo (html);
}

void
gtkhtml_editor_drop_undo (GtkhtmlEditor *editor)
{
	GtkHTML *html;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	html = gtkhtml_editor_get_html (editor);

	gtk_html_drop_undo (html);
}

void
gtkthtml_editor_emit_spell_languages_changed (GtkhtmlEditor *editor)
{
	GList *languages, *iter;
	GtkhtmlEditorPrivate *priv;

	g_return_if_fail (editor != NULL);

	languages = NULL;
	priv = gtkhtml_editor_get_instance_private (editor);
	for (iter = priv->active_spell_checkers; iter; iter = g_list_next (iter)) {
		const GtkhtmlSpellLanguage *language;
		GtkhtmlSpellChecker *checker = iter->data;

		if (!checker)
			continue;

		language = gtkhtml_spell_checker_get_language (checker);
		if (!language)
			continue;

		languages = g_list_prepend (languages, (gpointer) language);
	}

	languages = g_list_reverse (languages);

	g_signal_emit (editor, signals[SPELL_LANGUAGES_CHANGED], 0, languages);

	g_list_free (languages);
}

////////////////////////////////

/************************ Begin Spell Dialog Callbacks ***********************/

static void
editor_set_word (GtkhtmlEditor *editor,
                 GtkhtmlSpellDialog *dialog)
{
	GtkHTML *html;
	gchar *word;

	html = gtkhtml_editor_get_html (editor);

	html_engine_select_spell_word_editable (html->engine);

	word = html_engine_get_spell_word (html->engine);
	gtkhtml_spell_dialog_set_word (dialog, word);
	g_free (word);
}

static void
editor_next_word_cb (GtkhtmlEditor *editor,
                     GtkhtmlSpellDialog *dialog)
{
	if (gtkhtml_editor_next_spell_error (editor))
		editor_set_word (editor, dialog);
	else
		gtkhtml_spell_dialog_close (dialog);
}

static void
editor_prev_word_cb (GtkhtmlEditor *editor,
                     GtkhtmlSpellDialog *dialog)
{
	if (gtkhtml_editor_prev_spell_error (editor))
		editor_set_word (editor, dialog);
	else
		gtkhtml_spell_dialog_close (dialog);
}

static void
editor_recheck_cb (GtkhtmlEditor *editor,
                   GtkhtmlSpellDialog *dialog)
{
	GtkHTML *html;

	html = gtkhtml_editor_get_html (editor);
	html_engine_spell_check (html->engine);
}

static void
editor_replace_cb (GtkhtmlEditor *editor,
                   const gchar *correction,
                   GtkhtmlSpellDialog *dialog)
{
	GtkHTML *html;

	html = gtkhtml_editor_get_html (editor);

	html_engine_replace_spell_word_with (html->engine, correction);
	gtkhtml_spell_dialog_next_word (dialog);
}

static void
editor_replace_all_cb (GtkhtmlEditor *editor,
                       const gchar *correction,
                       GtkhtmlSpellDialog *dialog)
{
	GtkHTML *html;
	gchar *misspelled;

	html = gtkhtml_editor_get_html (editor);

	/* Replace this occurrence. */
	misspelled = html_engine_get_spell_word (html->engine);
	html_engine_replace_spell_word_with (html->engine, correction);

	/* Replace all subsequent occurrences. */
	while (gtkhtml_editor_next_spell_error (editor)) {
		gchar *word;

		word = html_engine_get_spell_word (html->engine);
		if (g_str_equal (word, misspelled))
			html_engine_replace_spell_word_with (
				html->engine, correction);
		g_free (word);
	}

	/* Jump to beginning of document (XXX is this right?) */
	html_engine_beginning_of_document (html->engine);
	gtkhtml_spell_dialog_next_word (dialog);

	g_free (misspelled);
}

/************************* End Spell Dialog Callbacks ************************/

gchar *
gtkhtml_editor_find_data_file (const gchar *basename)
{
	const gchar * const *datadirs;
	gchar *filename;

	g_return_val_if_fail (basename != NULL, NULL);

	/* Support running directly from the source tree. */
	filename = g_build_filename (".", basename, NULL);
	if (g_file_test (filename, G_FILE_TEST_EXISTS))
		return filename;
	g_free (filename);

	/* Check our own installation prefix. */
	filename = g_build_filename (
		DATADIR, GTKHTML_RELEASE_STRING, basename, NULL);
	if (g_file_test (filename, G_FILE_TEST_EXISTS))
		return filename;
	g_free (filename);

	/* Check the standard system data directories. */
	datadirs = g_get_system_data_dirs ();
	while (*datadirs != NULL) {
		filename = g_build_filename (
			*datadirs++, GTKHTML_RELEASE_STRING, basename, NULL);
		if (g_file_test (filename, G_FILE_TEST_EXISTS))
			return filename;
		g_free (filename);
	}

	/* Print a helpful message and die. */
	g_printerr (DATA_FILE_NOT_FOUND_MESSAGE, basename);
	filename = g_build_filename (
		DATADIR, GTKHTML_RELEASE_STRING, basename, NULL);
	g_printerr ("\t%s\n", filename);
	g_free (filename);
	datadirs = g_get_system_data_dirs ();
	while (*datadirs != NULL) {
		filename = g_build_filename (
			*datadirs++, GTKHTML_RELEASE_STRING, basename, NULL);
		g_printerr ("\t%s\n", filename);
		g_free (filename);
	}
	g_printerr (MORE_INFORMATION_MESSAGE);
	abort ();

	return NULL;  /* never gets here */
}

GFile *
gtkhtml_editor_run_open_dialog (GtkhtmlEditor *editor,
                                const gchar *title,
                                GtkCallback customize_func,
                                gpointer customize_data)
{
	GtkFileChooser *file_chooser;
	GFile *chosen_file = NULL;
	GtkWidget *dialog;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), NULL);

	dialog = gtk_file_chooser_dialog_new (
		title, GTK_WINDOW (editor),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	file_chooser = GTK_FILE_CHOOSER (dialog);

	gtk_dialog_set_default_response (
		GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	gtk_file_chooser_set_local_only (file_chooser, FALSE);

	/* Allow further customizations before running the dialog. */
	if (customize_func != NULL)
		customize_func (dialog, customize_data);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		chosen_file = gtk_file_chooser_get_file (file_chooser);

	gtk_widget_destroy (dialog);

	return chosen_file;
}

void
gtkhtml_editor_show_uri (GtkWindow *parent,
                         const gchar *uri)
{
	GtkWidget *dialog;
	GdkScreen *screen = NULL;
	GError *error = NULL;
	guint32 timestamp;

	g_return_if_fail (uri != NULL);

	timestamp = gtk_get_current_event_time ();

	if (parent != NULL)
		screen = gtk_widget_get_screen (GTK_WIDGET (parent));

	if (gtk_show_uri (screen, uri, timestamp, &error))
		return;

	dialog = gtk_message_dialog_new_with_markup (
		parent, GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		"<big><b>%s</b></big>",
		_("Could not open the link."));

	gtk_message_dialog_format_secondary_text (
		GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
	g_error_free (error);
}


gboolean
gtkhtml_editor_next_spell_error (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	gboolean found = FALSE;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	html_engine_disable_selection (html->engine);

	while (!found && html_engine_forward_word (html->engine))
		found = !html_engine_spell_word_is_valid (html->engine);

	return found;
}

gboolean
gtkhtml_editor_prev_spell_error (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	gboolean found = FALSE;

	g_return_val_if_fail (GTKHTML_IS_EDITOR (editor), FALSE);

	html = gtkhtml_editor_get_html (editor);

	html_engine_disable_selection (html->engine);
	html_engine_backward_word (html->engine);

	while (!found && html_engine_backward_word (html->engine))
		found = !html_engine_spell_word_is_valid (html->engine);

	return found;
}

/* Action callback for context menu spelling suggestions.
 * XXX This should really be in gtkhtml-editor-actions.c */
static void
action_context_spell_suggest_cb (GtkAction *action,
                                 GtkhtmlEditor *editor)
{
	GtkHTML *html;
	const gchar *word;

	html = gtkhtml_editor_get_html (editor);

	word = g_object_get_data (G_OBJECT (action), "word");
	g_return_if_fail (word != NULL);

	html_engine_replace_spell_word_with (html->engine, word);
}

void
gtkhtml_editor_private_init (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;
	gchar *filename;
	GError *error = NULL;

	priv = gtkhtml_editor_get_instance_private (editor);
	priv->manager = gtk_ui_manager_new ();
	priv->core_actions = gtk_action_group_new ("core");
	priv->html_actions = gtk_action_group_new ("html");
	priv->context_actions = gtk_action_group_new ("core-context");
	priv->html_context_actions = gtk_action_group_new ("html-context");
	priv->language_actions = gtk_action_group_new ("language");
	priv->spell_check_actions = gtk_action_group_new ("spell-check");
	priv->suggestion_actions = gtk_action_group_new ("suggestion");

	/* GtkhtmlSpellLanguage -> GtkhtmlSpellChecker */
	priv->available_spell_checkers = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		(GDestroyNotify) NULL,
		(GDestroyNotify) g_object_unref);

	/* GtkhtmlSpellLanguage -> UI Merge ID */
	priv->spell_suggestion_menus =
		g_hash_table_new (g_direct_hash, g_direct_equal);

	filename = gtkhtml_editor_find_data_file ("gtkhtml-editor-manager.ui");

	if (!gtk_ui_manager_add_ui_from_file (priv->manager, filename, &error)) {
		g_critical ("Couldn't load builder file: %s\n", error->message);
		g_clear_error (&error);
	}

	g_free (filename);

	filename = gtkhtml_editor_find_data_file ("gtkhtml-editor-builder.ui");

	priv->builder = gtk_builder_new ();
	/* To keep translated strings in subclasses */
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);
	if (!gtk_builder_add_from_file (priv->builder, filename, &error)) {
		g_critical ("Couldn't load builder file: %s\n", error->message);
		g_clear_error (&error);
	}

	g_free (filename);

	gtkhtml_editor_actions_init (editor);

	gtk_window_add_accel_group (
		GTK_WINDOW (editor),
		gtk_ui_manager_get_accel_group (priv->manager));

	gtk_builder_connect_signals (priv->builder, NULL);

	/* Wait to construct the main window widgets
	 * until the 'html' property is initialized. */
}

void
gtkhtml_editor_private_constructed (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;
	GtkHTML *html;
	GtkWidget *widget;
	GtkToolbar *toolbar;
	GtkToolItem *tool_item;

	/* Construct main window widgets. */
	priv = gtkhtml_editor_get_instance_private (editor);
	widget = gtkhtml_editor_get_managed_widget (editor, "/main-menu");
	gtk_box_pack_start (GTK_BOX (priv->vbox), widget, FALSE, FALSE, 0);
	priv->main_menu = g_object_ref (widget);
	gtk_widget_show (widget);

	widget = gtkhtml_editor_get_managed_widget (editor, "/main-toolbar");
	gtk_box_pack_start (GTK_BOX (priv->vbox), widget, FALSE, FALSE, 0);
	priv->main_toolbar = g_object_ref (widget);
	gtk_widget_show (widget);

	gtk_style_context_add_class (
		gtk_widget_get_style_context (widget),
		GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

	widget = gtkhtml_editor_get_managed_widget (editor, "/edit-toolbar");
	gtk_toolbar_set_style (GTK_TOOLBAR (widget), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_box_pack_start (GTK_BOX (priv->vbox), widget, FALSE, FALSE, 0);
	priv->edit_toolbar = g_object_ref (widget);
	gtk_widget_show (widget);

	widget = gtkhtml_editor_get_managed_widget (editor, "/html-toolbar");
	gtk_toolbar_set_style (GTK_TOOLBAR (widget), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_box_pack_start (GTK_BOX (priv->vbox), widget, FALSE, FALSE, 0);
	priv->html_toolbar = g_object_ref (widget);
	gtk_widget_show (widget);

	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (widget),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (priv->vbox), widget, TRUE, TRUE, 0);
	priv->scrolled_window = g_object_ref (widget);
	gtk_widget_show (widget);

	widget = GTK_WIDGET (gtkhtml_editor_get_html (editor));
	gtk_container_add (GTK_CONTAINER (priv->scrolled_window), widget);
	gtk_widget_show (widget);

	/* Add some combo boxes to the "edit" toolbar. */

	toolbar = GTK_TOOLBAR (priv->edit_toolbar);

	tool_item = gtk_tool_item_new ();
	widget = gtkhtml_combo_box_new_with_action (
		GTK_RADIO_ACTION (ACTION (STYLE_NORMAL)));
	gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (widget), FALSE);
	gtk_container_add (GTK_CONTAINER (tool_item), widget);
	gtk_widget_set_tooltip_text (widget, _("Paragraph Style"));
	gtk_toolbar_insert (toolbar, tool_item, 0);
	priv->style_combo_box = g_object_ref (widget);
	gtk_widget_show_all (GTK_WIDGET (tool_item));

	tool_item = gtk_separator_tool_item_new ();
	gtk_toolbar_insert (toolbar, tool_item, 0);
	gtk_widget_show_all (GTK_WIDGET (tool_item));

	tool_item = gtk_tool_item_new ();
	widget = gtkhtml_combo_box_new_with_action (
		GTK_RADIO_ACTION (ACTION (MODE_HTML)));
	gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (widget), FALSE);
	gtk_container_add (GTK_CONTAINER (tool_item), widget);
	gtk_widget_set_tooltip_text (widget, _("Editing Mode"));
	gtk_toolbar_insert (toolbar, tool_item, 0);
	priv->mode_combo_box = g_object_ref (widget);
	gtk_widget_show_all (GTK_WIDGET (tool_item));

	/* Add some combo boxes to the "html" toolbar. */

	toolbar = GTK_TOOLBAR (priv->html_toolbar);

	tool_item = gtk_tool_item_new ();
	widget = gtkhtml_color_combo_new ();
	gtk_container_add (GTK_CONTAINER (tool_item), widget);
	gtk_widget_set_tooltip_text (widget, _("Font Color"));
	gtk_toolbar_insert (toolbar, tool_item, 0);
	priv->color_combo_box = g_object_ref (widget);
	gtk_widget_show_all (GTK_WIDGET (tool_item));

	tool_item = gtk_tool_item_new ();
	widget = gtkhtml_combo_box_new_with_action (
		GTK_RADIO_ACTION (ACTION (SIZE_PLUS_ZERO)));
	gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (widget), FALSE);
	gtk_container_add (GTK_CONTAINER (tool_item), widget);
	gtk_widget_set_tooltip_text (widget, _("Font Size"));
	gtk_toolbar_insert (toolbar, tool_item, 0);
	priv->size_combo_box = g_object_ref (widget);
	gtk_widget_show_all (GTK_WIDGET (tool_item));

	/* Initialize painters (requires "edit_area"). */

	html = gtkhtml_editor_get_html (editor);
	priv->html_painter = g_object_ref (html->engine->painter);
	priv->plain_painter = html_plain_painter_new (priv->edit_area, TRUE);

	/* Add input methods to the context menu. */

	widget = gtkhtml_editor_get_managed_widget (
		editor, "/context-menu/context-input-methods-menu");
	widget = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));
	gtk_im_multicontext_append_menuitems (
		GTK_IM_MULTICONTEXT (html->priv->im_context),
		GTK_MENU_SHELL (widget));

	/* Configure color stuff. */

	priv->palette = gtkhtml_color_palette_new ();
	priv->text_color = gtkhtml_color_state_new ();

	gtkhtml_color_state_set_default_label (
		priv->text_color, _("Automatic"));
	gtkhtml_color_state_set_palette (
		priv->text_color, priv->palette);

	/* Text color widgets share state. */

	widget = priv->color_combo_box;
	gtkhtml_color_combo_set_state (
		GTKHTML_COLOR_COMBO (widget), priv->text_color);

	widget = WIDGET (TEXT_PROPERTIES_COLOR_COMBO);
	gtkhtml_color_combo_set_state (
		GTKHTML_COLOR_COMBO (widget), priv->text_color);

	/* These color widgets share a custom color palette. */

	widget = WIDGET (CELL_PROPERTIES_COLOR_COMBO);
	gtkhtml_color_combo_set_palette (
		GTKHTML_COLOR_COMBO (widget), priv->palette);

	widget = WIDGET (PAGE_PROPERTIES_BACKGROUND_COLOR_COMBO);
	gtkhtml_color_combo_set_palette (
		GTKHTML_COLOR_COMBO (widget), priv->palette);

	widget = WIDGET (PAGE_PROPERTIES_LINK_COLOR_COMBO);
	gtkhtml_color_combo_set_palette (
		GTKHTML_COLOR_COMBO (widget), priv->palette);

	widget = WIDGET (TABLE_PROPERTIES_COLOR_COMBO);
	gtkhtml_color_combo_set_palette (
		GTKHTML_COLOR_COMBO (widget), priv->palette);
}

void
gtkhtml_editor_private_dispose (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);

	/* Disconnect signal handlers from the color
	 * state object since it may live on. */
	if (priv->text_color != NULL) {
		g_signal_handlers_disconnect_matched (
			priv->text_color, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, editor);
	}

	DISPOSE (priv->manager);
	DISPOSE (priv->core_actions);
	DISPOSE (priv->html_actions);
	DISPOSE (priv->context_actions);
	DISPOSE (priv->html_context_actions);
	DISPOSE (priv->language_actions);
	DISPOSE (priv->spell_check_actions);
	DISPOSE (priv->suggestion_actions);
	DISPOSE (priv->builder);

	DISPOSE (priv->html_painter);
	DISPOSE (priv->plain_painter);

	g_hash_table_remove_all (priv->available_spell_checkers);

	g_list_free_full (g_steal_pointer (&priv->active_spell_checkers), g_object_unref);

	g_list_free (priv->active_spell_checkers);
	priv->active_spell_checkers = NULL;

	DISPOSE (priv->main_menu);
	DISPOSE (priv->main_toolbar);
	DISPOSE (priv->edit_toolbar);
	DISPOSE (priv->html_toolbar);
	DISPOSE (priv->edit_area);

	DISPOSE (priv->color_combo_box);
	DISPOSE (priv->mode_combo_box);
	DISPOSE (priv->size_combo_box);
	DISPOSE (priv->style_combo_box);
	DISPOSE (priv->scrolled_window);

	DISPOSE (priv->palette);
	DISPOSE (priv->text_color);
}

void
gtkhtml_editor_private_finalize (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);

	/* All URI requests should be complete or cancelled by now. */
	if (priv->requests != NULL)
		g_warning ("Finalizing GtkhtmlEditor with active URI requests");

	g_hash_table_destroy (priv->available_spell_checkers);
	g_hash_table_destroy (priv->spell_suggestion_menus);

	g_free (priv->filename);
}

void
gtkhtml_editor_spell_check (GtkhtmlEditor *editor,
                            gboolean whole_document)
{
	GtkHTML *html;
	gboolean spelling_errors;
	GtkhtmlEditorPrivate *priv;

	g_return_if_fail (GTKHTML_IS_EDITOR (editor));

	priv = gtkhtml_editor_get_instance_private (editor);
	html = gtkhtml_editor_get_html (editor);

	if (whole_document) {
		html_engine_disable_selection (html->engine);
		html_engine_beginning_of_document (html->engine);
		gtk_html_set_inline_spelling (html, TRUE);
	}

	spelling_errors =
		!html_engine_spell_word_is_valid (html->engine) ||
		gtkhtml_editor_next_spell_error (editor);

	if (spelling_errors) {
		GtkWidget *dialog;

		dialog = gtkhtml_spell_dialog_new (GTK_WINDOW (editor));

		gtkhtml_spell_dialog_set_spell_checkers (
			GTKHTML_SPELL_DIALOG (dialog),
			priv->active_spell_checkers);

		editor_set_word (editor, GTKHTML_SPELL_DIALOG (dialog));

		g_signal_connect_swapped (
			dialog, "added",
			G_CALLBACK (editor_recheck_cb), editor);

		g_signal_connect_swapped (
			dialog, "ignored",
			G_CALLBACK (editor_recheck_cb), editor);

		g_signal_connect_swapped (
			dialog, "next-word",
			G_CALLBACK (editor_next_word_cb), editor);

		g_signal_connect_swapped (
			dialog, "prev-word",
			G_CALLBACK (editor_prev_word_cb), editor);

		g_signal_connect_swapped (
			dialog, "replace",
			G_CALLBACK (editor_replace_cb), editor);

		g_signal_connect_swapped (
			dialog, "replace-all",
			G_CALLBACK (editor_replace_all_cb), editor);

		gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_widget_destroy (dialog);
	}

	/* XXX Restore original inline spelling value. */
}

static void
editor_inline_spelling_suggestions (GtkhtmlEditor *editor,
                                    GtkhtmlSpellChecker *checker)
{
	GtkActionGroup *action_group;
	GtkUIManager *manager;
	GtkHTML *html;
	GList *list;
	const gchar *path;
	gchar *word;
	guint count = 0;
	guint length;
	guint merge_id;
	guint threshold;
	GtkhtmlEditorPrivate *priv;

	html = gtkhtml_editor_get_html (editor);
	word = html_engine_get_spell_word (html->engine);
	list = gtkhtml_spell_checker_get_suggestions (checker, word, -1);
	priv = gtkhtml_editor_get_instance_private (editor);

	path = "/context-menu/context-spell-suggest/";
	manager = gtkhtml_editor_get_ui_manager (editor);
	action_group = priv->suggestion_actions;
	merge_id = priv->spell_suggestions_merge_id;

	/* Calculate how many suggestions to put directly in the
	 * context menu.  The rest will go in a secondary menu. */
	length = g_list_length (list);
	if (length <= MAX_LEVEL1_SUGGESTIONS)
		threshold = length;
	else if (length - MAX_LEVEL1_SUGGESTIONS < MIN_LEVEL2_SUGGESTIONS)
		threshold = length;
	else
		threshold = MAX_LEVEL1_SUGGESTIONS;

	while (list != NULL) {
		gchar *suggestion = list->data;
		gchar *action_name;
		gchar *action_label;
		GtkAction *action;
		GtkWidget *child;
		GSList *proxies;

		/* Once we reach the threshold, put all subsequent
		 * spelling suggestions in a secondary menu. */
		if (count == threshold)
			path = "/context-menu/context-more-suggestions-menu/";

		/* Action name just needs to be unique. */
		action_name = g_strdup_printf ("suggest-%d", count++);

		action_label = g_markup_printf_escaped (
			"<b>%s</b>", suggestion);

		action = gtk_action_new (
			action_name, action_label, NULL, NULL);

		g_object_set_data_full (
			G_OBJECT (action), "word",
			g_strdup (suggestion), g_free);

		g_signal_connect (
			action, "activate", G_CALLBACK (
			action_context_spell_suggest_cb), editor);

		gtk_action_group_add_action (action_group, action);

		gtk_ui_manager_add_ui (
			manager, merge_id, path,
			action_name, action_name,
			GTK_UI_MANAGER_AUTO, FALSE);

		/* XXX GtkAction offers no support for Pango markup,
		 *     so we have to manually set "use-markup" on the
		 *     child of the proxy widget. */
		gtk_ui_manager_ensure_update (manager);
		proxies = gtk_action_get_proxies (action);
		child = gtk_bin_get_child (proxies->data);
		g_object_set (child, "use-markup", TRUE, NULL);

		g_free (suggestion);
		g_free (action_name);
		g_free (action_label);

		list = g_list_delete_link (list, list);
	}

	g_free (word);
}

/* Helper for gtkhtml_editor_update_context() */
static void
editor_spell_checkers_foreach (GtkhtmlSpellChecker *checker,
                               GtkhtmlEditor *editor)
{
	const GtkhtmlSpellLanguage *language;
	const gchar *language_code;
	GtkActionGroup *action_group;
	GtkUIManager *manager;
	GtkHTML *html;
	GList *list;
	gchar *path;
	gchar *word;
	gint count = 0;
	guint merge_id;
	GtkhtmlEditorPrivate *priv;

	language = gtkhtml_spell_checker_get_language (checker);
	language_code = gtkhtml_spell_language_get_code (language);

	html = gtkhtml_editor_get_html (editor);
	word = html_engine_get_spell_word (html->engine);
	list = gtkhtml_spell_checker_get_suggestions (checker, word, -1);
	priv = gtkhtml_editor_get_instance_private (editor);

	manager = gtkhtml_editor_get_ui_manager (editor);
	action_group = priv->suggestion_actions;
	merge_id = priv->spell_suggestions_merge_id;

	path = g_strdup_printf (
		"/context-menu/context-spell-suggest/"
		"context-spell-suggest-%s-menu", language_code);

	while (list != NULL) {
		gchar *suggestion = list->data;
		gchar *action_name;
		gchar *action_label;
		GtkAction *action;
		GtkWidget *child;
		GSList *proxies;

		/* Action name just needs to be unique. */
		action_name = g_strdup_printf (
			"suggest-%s-%d", language_code, count++);

		action_label = g_markup_printf_escaped (
			"<b>%s</b>", suggestion);

		action = gtk_action_new (
			action_name, action_label, NULL, NULL);

		g_object_set_data_full (
			G_OBJECT (action), "word",
			g_strdup (suggestion), g_free);

		g_signal_connect (
			action, "activate", G_CALLBACK (
			action_context_spell_suggest_cb), editor);

		gtk_action_group_add_action (action_group, action);

		gtk_ui_manager_add_ui (
			manager, merge_id, path,
			action_name, action_name,
			GTK_UI_MANAGER_AUTO, FALSE);

		/* XXX GtkAction offers no supports for Pango markup,
		 *     so we have to manually set "use-markup" on the
		 *     child of the proxy widget. */
		gtk_ui_manager_ensure_update (manager);
		proxies = gtk_action_get_proxies (action);
		child = gtk_bin_get_child (proxies->data);
		g_object_set (child, "use-markup", TRUE, NULL);

		g_free (suggestion);
		g_free (action_name);
		g_free (action_label);

		list = g_list_delete_link (list, list);
	}

	g_free (path);
	g_free (word);
}

void
gtkhtml_editor_update_context (GtkhtmlEditor *editor)
{
	GtkHTML *html;
	HTMLType type;
	HTMLObject *object;
	GtkUIManager *manager;
	GtkActionGroup *action_group;
	GList *list;
	gboolean visible;
	guint merge_id;
	GtkhtmlEditorPrivate *priv;

	html = gtkhtml_editor_get_html (editor);
	manager = gtkhtml_editor_get_ui_manager (editor);
	gtk_html_update_styles (html);
	priv = gtkhtml_editor_get_instance_private (editor);

	/* Update context menu item visibility. */

	object = html->engine->cursor->object;

	if (object != NULL)
		type = HTML_OBJECT_TYPE (object);
	else
		type = HTML_TYPE_NONE;

	visible = (type == HTML_TYPE_IMAGE);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_IMAGE), visible);

	visible = (type == HTML_TYPE_LINKTEXT);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_LINK), visible);

	visible = (type == HTML_TYPE_RULE);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_RULE), visible);

	visible = (type == HTML_TYPE_TEXT);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_TEXT), visible);

	visible =
		gtk_action_get_visible (ACTION (CONTEXT_PROPERTIES_IMAGE)) ||
		gtk_action_get_visible (ACTION (CONTEXT_PROPERTIES_LINK)) ||
		gtk_action_get_visible (ACTION (CONTEXT_PROPERTIES_TEXT));
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_PARAGRAPH), visible);

	/* Set to visible if any of these are true:
	 *   - Selection is active and contains a link.
	 *   - Cursor is on a link.
	 *   - Cursor is on an image that has a URL or target.
	 */
	visible =
		(html_engine_is_selection_active (html->engine) &&
		 html_engine_selection_contains_link (html->engine)) ||
		(type == HTML_TYPE_LINKTEXT) ||
		(type == HTML_TYPE_IMAGE &&
			(HTML_IMAGE (object)->url != NULL ||
			 HTML_IMAGE (object)->target != NULL));
	gtk_action_set_visible (ACTION (CONTEXT_REMOVE_LINK), visible);

	/* Get the parent object. */
	object = (object != NULL) ? object->parent : NULL;

	/* Get the grandparent object. */
	object = (object != NULL) ? object->parent : NULL;

	if (object != NULL)
		type = HTML_OBJECT_TYPE (object);
	else
		type = HTML_TYPE_NONE;

	visible = (type == HTML_TYPE_TABLECELL);
	gtk_action_set_visible (ACTION (CONTEXT_DELETE_CELL), visible);
	gtk_action_set_visible (ACTION (CONTEXT_DELETE_COLUMN), visible);
	gtk_action_set_visible (ACTION (CONTEXT_DELETE_ROW), visible);
	gtk_action_set_visible (ACTION (CONTEXT_DELETE_TABLE), visible);
	gtk_action_set_visible (ACTION (CONTEXT_INSERT_COLUMN_AFTER), visible);
	gtk_action_set_visible (ACTION (CONTEXT_INSERT_COLUMN_BEFORE), visible);
	gtk_action_set_visible (ACTION (CONTEXT_INSERT_ROW_ABOVE), visible);
	gtk_action_set_visible (ACTION (CONTEXT_INSERT_ROW_BELOW), visible);
	gtk_action_set_visible (ACTION (CONTEXT_INSERT_TABLE), visible);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_CELL), visible);

	/* Get the great grandparent object. */
	object = (object != NULL) ? object->parent : NULL;

	if (object != NULL)
		type = HTML_OBJECT_TYPE (object);
	else
		type = HTML_TYPE_NONE;

	/* Note the |= (cursor must be in a table cell). */
	visible |= (type == HTML_TYPE_TABLE);
	gtk_action_set_visible (ACTION (CONTEXT_PROPERTIES_TABLE), visible);

	/********************** Spell Check Suggestions **********************/

	object = html->engine->cursor->object;
	action_group = priv->suggestion_actions;

	/* Remove the old content from the context menu. */
	merge_id = priv->spell_suggestions_merge_id;
	if (merge_id > 0) {
		gtk_ui_manager_remove_ui (manager, merge_id);
		priv->spell_suggestions_merge_id = 0;
	}

	/* Clear the action group for spelling suggestions. */
	list = gtk_action_group_list_actions (action_group);
	while (list != NULL) {
		GtkAction *action = list->data;

		gtk_action_group_remove_action (action_group, action);
		list = g_list_delete_link (list, list);
	}

	/* Decide if we should show spell checking items. */
	visible =
		!html_engine_is_selection_active (html->engine) &&
		object != NULL && html_object_is_text (object) &&
		!html_engine_spell_word_is_valid (html->engine);
	action_group = priv->spell_check_actions;
	gtk_action_group_set_visible (action_group, visible);

	/* Exit early if spell checking items are invisible. */
	if (!visible)
		return;

	list = priv->active_spell_checkers;
	merge_id = gtk_ui_manager_new_merge_id (manager);
	priv->spell_suggestions_merge_id = merge_id;

	/* Handle a single active language as a special case. */
	if (g_list_length (list) == 1) {
		editor_inline_spelling_suggestions (editor, list->data);
		return;
	}

	/* Add actions and context menu content for active languages. */
	g_list_foreach (list, (GFunc) editor_spell_checkers_foreach, editor);
}

GtkhtmlEditorPrivate*
gtkhtml_editor_get_private (GtkhtmlEditor *editor)
{
	GtkhtmlEditorPrivate *priv;

	priv = gtkhtml_editor_get_instance_private (editor);
	return priv;
}
