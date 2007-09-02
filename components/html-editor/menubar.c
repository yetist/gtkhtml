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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

#include <config.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtkfilechooser.h>
#include <gtk/gtkfilechooserdialog.h>
#include <bonobo.h>

#include "htmlengine.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-edit-rule.h"
#include "htmlengine-edit-table.h"
#include "htmlimage.h"
#include "htmlrule.h"
#include "htmltable.h"
#include "htmlselection.h"
#include "gtkhtml-private.h"

#include "menubar.h"
#include "gtkhtml.h"
#include "body.h"
#include "control-data.h"
#include "properties.h"
#include "image.h"
#include "text.h"
#include "paragraph.h"
#include "spellchecker.h"
#include "table.h"
#include "template.h"

static void smiley_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname);
static void font_style_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname);
static void command_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname);

static gboolean get_file_charset (const gchar * filename, gchar *charset, gint len);

static void
paste_quotation_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_paste (cd->html, TRUE);
}

static void
search_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search (cd);
}

static void
search_next_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search_next (cd);
}

static void
replace_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	replace (cd);
}

static void
insert_image_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	GtkWidget *filesel;
	HTMLObject *img;

	filesel = gtk_file_chooser_dialog_new (_("Insert image"),
					       NULL,
					       GTK_FILE_CHOOSER_ACTION_OPEN,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					       NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (filesel), GTK_RESPONSE_OK);
	if (filesel) {
		if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_OK) {
			char *url = NULL;

			url = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filesel));
			img = html_image_new (html_engine_get_image_factory (cd->html->engine), url,
					      NULL, NULL, 0, 0, 0, 0, 0, NULL, HTML_VALIGN_NONE, FALSE);
			html_engine_paste_object (cd->html->engine, img, 1);
			g_free (url);
		}
		gtk_widget_destroy (filesel);
	}
}

static void
insert_link_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Insert"), "stock_insert-url");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_LINK, _("Link"),
						   link_insert,
						   link_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, GTK_HTML_EDIT_PROPERTY_LINK);
}

static void
insert_rule_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	html_engine_insert_rule (cd->html->engine, 0, 100, 2, FALSE, HTML_HALIGN_LEFT);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Insert"), "stock_insert-rule");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_RULE, _("Rule"),
						   rule_properties,
						   rule_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

void
insert_table (GtkHTMLControlData *cd)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	if (cd->html->engine && cd->html->engine->selection)
		html_engine_unselect_all (cd->html->engine);

	html_engine_insert_table_1_1 (cd->html->engine);
	if (html_engine_get_table (cd->html->engine)) {
		html_engine_table_set_cols (cd->html->engine, 3);
		html_engine_table_set_rows (cd->html->engine, 3);
	}
	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Insert"), "stock_insert-table");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_TABLE, _("Table"),
						   table_properties,
						   table_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
insert_table_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	insert_table (cd);
}

static void
insert_template_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Insert"), "stock_insert_graphic");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_TABLE, _("Template"),
						   template_insert,
						   template_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
file_dialog_ok (GtkWidget *w, GtkHTMLControlData *cd)
{
	const gchar *filename;
	GIOChannel *io = NULL;
	GError *error = NULL;
	gchar *data = NULL;
	gsize len = 0;
	gchar charset[32];

	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (cd->file_dialog));
	io = g_io_channel_new_file (filename, "r", &error);

	if (error || !io)
		goto end;
		
	/* try reading as utf-8 */
	g_io_channel_read_to_end (io, &data, &len, &error);

	/* If we get a charset error try reading as the locale charset */
	if (error && g_error_matches (error, G_CONVERT_ERROR, G_CONVERT_ERROR_ILLEGAL_SEQUENCE)) {

		/* 
		 * Close the io channel since we can't set the 
		 * charset once we've begun reading.
		 */
		g_io_channel_unref (io);
		io = NULL;

		if (get_file_charset (filename, charset, 32)) {

			g_error_free (error);
			error = NULL;

			io = g_io_channel_new_file (filename, "r", &error);
		
			if (error || !io)
				goto end;

			g_io_channel_set_encoding (io, charset, NULL);
			g_io_channel_read_to_end (io, &data, &len, &error);
		}
	}
	
	if (error)
		goto end;
	
	if (cd->file_html) {
		GtkHTML *tmp;
		GtkHTMLStream *stream;
		
		tmp = GTK_HTML (gtk_html_new ());
		stream = gtk_html_begin_content (tmp, "text/html; charset=utf-8");
		gtk_html_write (tmp, stream, data, len);
		gtk_html_end (tmp, stream, error ? GTK_HTML_STREAM_OK : GTK_HTML_STREAM_ERROR);
		gtk_html_insert_gtk_html (cd->html, tmp);
	} else {
		html_engine_paste_text (cd->html->engine, data, g_utf8_strlen (data, -1));
	}
	g_free (data);

 end:
	if (io)
		g_io_channel_unref (io);

	if (error) {
		GtkWidget *toplevel;
		
		toplevel = gtk_widget_get_toplevel (GTK_WIDGET (cd->html));
		
		if (GTK_WIDGET_TOPLEVEL (toplevel)) {
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_CLOSE,
							 _("Error loading file '%s': %s"),
							 filename, error->message);
		
			/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
			g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
						  G_CALLBACK (gtk_widget_destroy),
						  GTK_OBJECT (dialog));

			gtk_widget_show (dialog);
		} else {
			g_warning ("Error loading file '%s': %s", filename, error->message);
		}
		g_error_free (error);
	}
}

static void
insert_file_dialog (GtkHTMLControlData *cd, gboolean html)
{
	cd->file_html = html;
	if (cd->file_dialog != NULL) {
		gdk_window_show (GTK_WIDGET (cd->file_dialog)->window);
		return;
	}

	cd->file_dialog = gtk_file_chooser_dialog_new (html ? _("Insert HTML File") : _("Insert Text File"),
						       NULL,
						       GTK_FILE_CHOOSER_ACTION_OPEN,
						       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						       GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						       NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (cd->file_dialog), GTK_RESPONSE_OK);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (cd->file_dialog), g_get_home_dir ());

	if (cd->file_dialog) {
		if (gtk_dialog_run (GTK_DIALOG (cd->file_dialog)) == GTK_RESPONSE_OK) {
			file_dialog_ok (cd->file_dialog, cd);
		}
		gtk_widget_destroy (cd->file_dialog);
		cd->file_dialog = NULL;
	}
}

static void
insert_text_file_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	insert_file_dialog (cd, FALSE);
}

static void
insert_html_file_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	insert_file_dialog (cd, TRUE);
}

static void 
spell_check_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	spell_check_dialog (cd, TRUE);
}

static void
format_page_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Format"), "gtk-properties");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_BODY, _("Page"),
						   body_properties,
						   body_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, GTK_HTML_EDIT_PROPERTY_BODY);
}

static void
format_text_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Format"), "gtk-properties");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_BODY, _("Text"),
						   text_properties,
						   text_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, GTK_HTML_EDIT_PROPERTY_TEXT);
}

static void
format_paragraph_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, _("Format"), "gtk-properties");

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_BODY, _("Paragraph"),
						   paragraph_properties,
						   paragraph_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, GTK_HTML_EDIT_PROPERTY_PARAGRAPH);
}

static BonoboUIVerb editor_verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("EditUndo", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditRedo", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditCut", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditCopy", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditPaste", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditPasteQuotation", paste_quotation_cb),
	BONOBO_UI_UNSAFE_VERB ("EditFind", search_cb),
	BONOBO_UI_UNSAFE_VERB ("EditFindAgain", search_next_cb),
	BONOBO_UI_UNSAFE_VERB ("EditReplace", replace_cb),
	BONOBO_UI_UNSAFE_VERB ("EditSelectAll", command_cb),
	BONOBO_UI_UNSAFE_VERB ("EditSpellCheck", spell_check_cb),

	BONOBO_UI_UNSAFE_VERB ("InsertImage", insert_image_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertLink",  insert_link_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertRule",  insert_rule_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertTable", insert_table_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertTemplate", insert_template_cb),

	BONOBO_UI_UNSAFE_VERB ("InsertTextFile", insert_text_file_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertHTMLFile", insert_html_file_cb),

	BONOBO_UI_UNSAFE_VERB ("InsertSmiley1", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley2", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley3", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley4", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley5", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley6", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley8", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley9", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley10", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley11", smiley_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertSmiley26", smiley_cb),

	BONOBO_UI_UNSAFE_VERB ("IndentMore", command_cb),
	BONOBO_UI_UNSAFE_VERB ("IndentLess", command_cb),
	BONOBO_UI_UNSAFE_VERB ("WrapLines", command_cb),

	BONOBO_UI_UNSAFE_VERB ("FormatText", format_text_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatParagraph", format_paragraph_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatPage", format_page_cb),

	BONOBO_UI_UNSAFE_VERB ("HeadingNormal", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingPreformat", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH1", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH2", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH3", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH4", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH5", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH6", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingH1", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingAddress", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingBulletedList", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingRomanList", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingNumberedList", command_cb),
	BONOBO_UI_UNSAFE_VERB ("HeadingAlphabeticalList", command_cb),

	BONOBO_UI_UNSAFE_VERB ("FontSizeNegTwo", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeNegOne", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeZero", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeOne", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeTwo", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeThree", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeFour", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatBold", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatItalic", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatUnderline", command_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatStrikeout", command_cb),

	BONOBO_UI_UNSAFE_VERB ("FormatFixed", font_style_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatSubscript", font_style_cb),
	BONOBO_UI_UNSAFE_VERB ("FormatSuperscript", font_style_cb),

	BONOBO_UI_UNSAFE_VERB ("AlignLeft", command_cb),
	BONOBO_UI_UNSAFE_VERB ("AlignCenter", command_cb),
	BONOBO_UI_UNSAFE_VERB ("AlignRight", command_cb),

	BONOBO_UI_VERB_END
};

static struct {
	const gchar *command;
	const gchar *verb;
} command_assoc[] = {
	{"size-minus-2", "FontSizeNegTwo"},
	{"size-minus-1", "FontSizeNegOne"},
	{"size-plus-0", "FontSizeZero"},
	{"size-plus-1", "FontSizeOne"},
	{"size-plus-2", "FontSizeTwo"},
	{"size-plus-3", "FontSizeThree"},
	{"size-plus-4", "FontSizeFour"},
	{"bold-toggle",    "FormatBold"},
	{"italic-toggle", "FormatItalic"},
	{"underline-toggle", "FormatUnderline"},
	{"strikeout-toggle", "FormatStrikeout"},
	{"indent-more", "IndentMore"},
	{"indent-less", "IndentLess"},
	{"indent-paragraph", "WrapLines"},
	{"align-left", "AlignLeft"},
	{"align-right", "AlignRight"},
	{"align-center", "AlignCenter"},
	{"select-all", "EditSelectAll"},
	{"undo", "EditUndo"},
	{"redo", "EditRedo"},
	{"cut", "EditCut"},
	{"copy", "EditCopy"},
	{"paste", "EditPaste"},
	{"style-normal", "HeadingNormal"},
	{"style-header1", "HeadingH1"},
	{"style-header2", "HeadingH2"},
	{"style-header3", "HeadingH3"},
	{"style-header4", "HeadingH4"},
	{"style-header5", "HeadingH5"},
	{"style-header6", "HeadingH6"},
	{"style-address", "HeadingAddress"},
	{"style-pre", "HeadingPreformat"},
	{"style-itemdot", "HeadingBulletedList"},
	{"style-itemroman", "HeadingRomanList"},
	{"style-itemdigit", "HeadingNumberedList"},
	{"style-itemalpha", "HeadingAlphabeticalList"},
	{NULL, NULL}
};

static struct {
	GtkHTMLFontStyle style;
	const gchar *verb;
} font_style_assoc[] = {	
	{GTK_HTML_FONT_STYLE_FIXED, "FormatFixed"},
	{GTK_HTML_FONT_STYLE_SUBSCRIPT, "FormatSubscript"},
	{GTK_HTML_FONT_STYLE_SUBSCRIPT, "FormatSuperscript"},
	{0, NULL}
};

static void
font_style_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
        int i;
                                                                              
        /* g_warning ("wowee %s :: %s", path, state); */
        for (i = 0; font_style_assoc[i].verb != NULL; i++) {
                if (!strcmp (cname, font_style_assoc[i].verb))
			gtk_html_toggle_font_style (cd->html, font_style_assoc[i].style);
        }
}

static void 
command_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	int i;
	
	for (i = 0; command_assoc[i].verb != NULL; i++) {
		if (!strcmp (cname, command_assoc[i].verb)) {
			gtk_html_command (cd->html, command_assoc[i].command); 
			return;
		}
	}
}

void
menubar_update_format (GtkHTMLControlData *cd)
{
	CORBA_Environment ev;
	BonoboUIComponent *uic;
	gchar *sensitive;

	uic = bonobo_control_get_ui_component (cd->control);

	if ((uic != CORBA_OBJECT_NIL) && (bonobo_ui_component_get_container (uic) != CORBA_OBJECT_NIL)) {

		sensitive = (cd->format_html ? "1" : "0");

		CORBA_exception_init (&ev);
		bonobo_ui_component_freeze (uic, &ev);

		bonobo_ui_component_set_prop (uic, "/commands/InsertImage",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatPage",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatText",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeNegTwo",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeNegOne",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeZero",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeOne",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeTwo",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeThree",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FontSizeFour",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/InsertLink",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/InsertRule",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/InsertTable",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/InsertTemplate",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/InsertTemplate",
					      "sensitive", sensitive, &ev);

		bonobo_ui_component_set_prop (uic, "/commands/FormatBold",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatItalic",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatUnderline",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatStrikeout",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/FormatFixed",
					      "sensitive", sensitive, &ev);

		bonobo_ui_component_set_prop (uic, "/commands/HeadingH1",
					      "sensitive", sensitive, &ev);	
		bonobo_ui_component_set_prop (uic, "/commands/HeadingH2",
					      "sensitive", sensitive, &ev);	
		bonobo_ui_component_set_prop (uic, "/commands/HeadingH3",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/HeadingH4",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/HeadingH5",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/HeadingH6",
					      "sensitive", sensitive, &ev);
		bonobo_ui_component_set_prop (uic, "/commands/HeadingAddress",
					      "sensitive", sensitive, &ev);

		bonobo_ui_component_thaw (uic, &ev);	

		CORBA_exception_free (&ev);
	}
}

void
menubar_set_languages (GtkHTMLControlData *cd)
{
	GString *str;
	gboolean enabled;
	gint i;
	gint no_of_lang=0;

	if (!cd->languages) {
		bonobo_ui_component_set_prop (cd->uic, "/commands/EditSpellCheck", "sensitive", "0", NULL);
		return;
	}

	str = g_string_new (NULL);
	cd->block_language_changes = TRUE;
	for (i = 0; i < cd->languages->_length; i ++) {
		enabled = cd->language && strstr (cd->language, cd->languages->_buffer [i].abbreviation) != NULL;
		if (enabled)
			no_of_lang++;
		g_string_printf (str, "/commands/SpellLanguage%d", i + 1);
		bonobo_ui_component_set_prop (cd->uic, str->str, "state", enabled ? "1" : "0", NULL);
	}
	if (no_of_lang > 0)
		bonobo_ui_component_set_prop (cd->uic, "/commands/EditSpellCheck", "sensitive", "1", NULL);
	else
		bonobo_ui_component_set_prop (cd->uic, "/commands/EditSpellCheck", "sensitive", "0", NULL);
	g_string_free (str, TRUE);
	cd->block_language_changes = FALSE;
}

static void
smiley_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	const gchar *emoticon;
	const gchar *filename;
	GtkIconInfo *icon_info;
	gchar *icon_name;
	gchar *html;
	gint smiley;

	g_return_if_fail (cname != NULL);

	/* cname is "InsertSmileyNN" */
	smiley = atoi (cname + 12);
	icon_name = g_strdup_printf ("stock_smiley-%d", smiley);
	icon_info = gtk_icon_theme_lookup_icon (
		gtk_icon_theme_get_default (), icon_name, 16, 0);
	g_assert (icon_info != NULL);
	filename = gtk_icon_info_get_filename (icon_info);
	g_assert (filename != NULL);
	g_free (icon_name);

	switch (smiley) {
		case 1: /* smile */
			emoticon = ":)";
			break;
		case 2: /* surprised */
			emoticon = "=)";
			break;
		case 3: /* wink */
			emoticon = ";)";
			break;
		case 4: /* frown */
			emoticon = ":(";
			break;
		case 5: /* oh no! */
			emoticon = ":O";
			break;
		case 6: /* laughing */
			emoticon = ":D";
			break;
		case 8: /* indifferent */
			emoticon = ":-|";
			break;
		case 9: /* undecided */
			emoticon = ":-/";
			break;
		case 10: /* tongue */
			emoticon = ":-P";
			break;
		case 11: /* crying */
			emoticon = ":~(";
			break;
		case 26: /* sick */
			emoticon = ":-Q";
			break;
		default:
			g_assert_not_reached ();
	}

	html = g_strdup_printf ("<IMG ALT=\"%s\" SRC=\"%s\">",
		emoticon, filename);
	gtk_html_insert_html (cd->html, html);
	gtk_icon_info_free (icon_info);
	g_free (html);
}

static void
menubar_paragraph_style_changed_cb (GtkHTML *html, GtkHTMLParagraphStyle style, GtkHTMLControlData *cd)
{
	bonobo_ui_component_set_prop (bonobo_control_get_ui_component (cd->control), "/commands/WrapLines",
				      "sensitive", style == GTK_HTML_PARAGRAPH_STYLE_PRE ? "1" : "0", NULL);
}

typedef enum
{
	EDITOR_ICON_STOCK = 0,
	EDITOR_ICON_MENU = 16,
	EDITOR_ICON_TOOLBAR = 24,
} EditorIconSize;

typedef struct
{
	const char *path;
	const char *stock_name;
	EditorIconSize size;
} EditorUIPixmap;

static EditorUIPixmap pixmaps_map [] =
{
	{ "/Toolbar/EditUndo", GTK_STOCK_UNDO, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditRedo", GTK_STOCK_REDO, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditCut", GTK_STOCK_CUT, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditCopy", GTK_STOCK_COPY, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditPaste", GTK_STOCK_PASTE, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditFind", GTK_STOCK_FIND, EDITOR_ICON_STOCK },
	{ "/Toolbar/EditReplace", GTK_STOCK_FIND_AND_REPLACE, EDITOR_ICON_STOCK },
	{ "/Toolbar/InsertImage", "stock_insert_image", EDITOR_ICON_TOOLBAR },
	{ "/Toolbar/InsertLink", "stock_insert-url", EDITOR_ICON_TOOLBAR },
 	{ "/Toolbar/InsertRule", "stock_insert-rule", EDITOR_ICON_TOOLBAR },
 	{ "/Toolbar/InsertTable", "stock_insert-table", EDITOR_ICON_TOOLBAR },

	{ "/menu/Edit/EditUndoRedo/EditUndo", GTK_STOCK_UNDO, EDITOR_ICON_STOCK },
	{ "/menu/Edit/EditUndoRedo/EditRedo", GTK_STOCK_REDO, EDITOR_ICON_STOCK },

	{ "/menu/Edit/EditCutCopyPaste/EditCut", GTK_STOCK_CUT, EDITOR_ICON_STOCK },
	{ "/menu/Edit/EditCutCopyPaste/EditCopy", GTK_STOCK_COPY, EDITOR_ICON_STOCK },
	{ "/menu/Edit/EditCutCopyPaste/EditPaste", GTK_STOCK_PASTE, EDITOR_ICON_STOCK },

	{ "/menu/Edit/EditFindReplace/EditFind", GTK_STOCK_FIND, EDITOR_ICON_STOCK },
	{ "/menu/Edit/EditFindReplace/EditReplace", GTK_STOCK_FIND_AND_REPLACE, EDITOR_ICON_STOCK },

	{ "/menu/Edit/EditMisc/EditSpellCheck", GTK_STOCK_SPELL_CHECK, EDITOR_ICON_STOCK },

	{ "/menu/Insert/Component/InsertImage", "stock_insert_image", EDITOR_ICON_MENU },
	{ "/menu/Insert/Component/InsertLink", "stock_insert-url", EDITOR_ICON_MENU },
 	{ "/menu/Insert/Component/InsertRule", "stock_insert-rule", EDITOR_ICON_MENU },
 	{ "/menu/Insert/Component/InsertTable", "stock_insert-table", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley", "stock_smiley-1", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley1", "stock_smiley-1", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley2", "stock_smiley-2", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley3", "stock_smiley-3", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley4", "stock_smiley-4", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley5", "stock_smiley-5", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley6", "stock_smiley-6", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley8", "stock_smiley-8", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley9", "stock_smiley-9", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley10", "stock_smiley-10", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley11", "stock_smiley-11", EDITOR_ICON_MENU },
	{ "/menu/Insert/Smiley/InsertSmiley26", "stock_smiley-26", EDITOR_ICON_MENU },
	{ "/menu/Format/FormatParagraph/FontStyle/FormatFixed", "stock_text-monospaced", EDITOR_ICON_MENU },
	{ "/menu/Format/FormatParagraph/FontStyle/FormatBold", GTK_STOCK_BOLD, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/FontStyle/FormatItalic", GTK_STOCK_ITALIC, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/FontStyle/FormatUnderline", GTK_STOCK_UNDERLINE, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/FontStyle/FormatStrikeout", GTK_STOCK_STRIKETHROUGH, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/Alignment/AlignLeft", GTK_STOCK_JUSTIFY_LEFT, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/Alignment/AlignCenter", GTK_STOCK_JUSTIFY_CENTER, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/Alignment/AlignRight", GTK_STOCK_JUSTIFY_RIGHT, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/IndentMore", GTK_STOCK_INDENT, EDITOR_ICON_STOCK },
	{ "/menu/Format/FormatParagraph/IndentLess", GTK_STOCK_UNINDENT, EDITOR_ICON_STOCK }
};

void
menubar_setup (BonoboUIComponent  *uic,
	       GtkHTMLControlData *cd)
{
	char *domain;
	int i;
	g_return_if_fail (cd->html != NULL);
	g_return_if_fail (GTK_IS_HTML (cd->html));
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (uic));

	/*
	  FIXME

	  we should pass domain to bonobo (once it provides such functionality in its API)
	  now we could "steal" domain from other threads until it's restored back
	  also hopefully no one else is doing this hack so we end with the right domain :(
	*/

	domain = g_strdup (textdomain (NULL));
	textdomain (GETTEXT_PACKAGE);
	bonobo_ui_component_add_verb_list_with_data (uic, editor_verbs, cd);

	if (GTK_HTML_CLASS(G_OBJECT_GET_CLASS (cd->html))->use_emacs_bindings) {
		bonobo_ui_util_set_ui (uic, GTKHTML_DATADIR, "GNOME_GtkHTML_Editor-emacs.xml", "GNOME_GtkHTML_Editor", NULL);
	} else {
		bonobo_ui_util_set_ui (uic, GTKHTML_DATADIR, "GNOME_GtkHTML_Editor.xml", "GNOME_GtkHTML_Editor", NULL);
	}

	for (i = 0; i < G_N_ELEMENTS (pixmaps_map); i++) {
		if (pixmaps_map[i].size == EDITOR_ICON_STOCK) {
			bonobo_ui_component_set_prop (
				uic, pixmaps_map[i].path,
				"pixtype", "stock",
				NULL);
			bonobo_ui_component_set_prop (
				uic, pixmaps_map[i].path,
				"pixname", pixmaps_map[i].stock_name,
				NULL);
		} else {
			GtkIconInfo *icon_info;
			const gchar *filename;

			icon_info = gtk_icon_theme_lookup_icon (
				gtk_icon_theme_get_default (),
				pixmaps_map[i].stock_name,
				pixmaps_map[i].size, 0);
			filename = gtk_icon_info_get_filename (icon_info);
			bonobo_ui_component_set_prop (
				uic, pixmaps_map[i].path,
				"pixtype", "filename",
				NULL);
			bonobo_ui_component_set_prop (
				uic, pixmaps_map[i].path,
				"pixname", filename,
				NULL);
			gtk_icon_info_free (icon_info);
		}
	}

	spell_create_language_menu (cd);
	menubar_set_languages (cd);
	menubar_update_format (cd);

	textdomain (domain);
	g_free (domain);

	menubar_paragraph_style_changed_cb (cd->html, gtk_html_get_paragraph_style (cd->html), cd);
	g_signal_connect (cd->html, "current_paragraph_style_changed", G_CALLBACK (menubar_paragraph_style_changed_cb), cd);

	if (!cd->has_spell_control_set) {
		cd->has_spell_control = spell_has_control ();
		cd->has_spell_control_set = TRUE;
	}

	if (!cd->has_spell_control) {
		cd->has_spell_control = FALSE;
		bonobo_ui_component_set_prop (uic, "/commands/EditSpellCheck", "sensitive", "0", NULL);
	} else {
		cd->has_spell_control = TRUE;
		bonobo_ui_component_set_prop (uic, "/commands/EditSpellCheck", "sensitive", "1", NULL);
	}
}

gboolean 
get_file_charset (const gchar * filename, gchar *charset, gint len)
{
	char data[1024];
	gboolean result = FALSE;
	FILE *fp = fopen (filename, "r");
	
	if (!fp)
		return FALSE;


	while (fgets (data, sizeof(data)-1, fp)) {

		char * pos = strstr (g_ascii_strdown (data, -1), "charset");
		if (pos) {
			gchar * pos2 = strstr (pos, "=");
			if (pos2) {
				pos2++;
				gchar * word = pos2;
				while (*pos2) {
					if (*pos2 != '"')
						pos2++;
					else
					{
						if (pos2 - word < len) {
							g_strlcpy (charset, word, pos2 - word + 1);
							result = TRUE;
						}
						break;
					}
				}
				break;
			}
		}
	}

	fclose (fp);


	return result;
}
