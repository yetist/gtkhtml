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

    Author: Ettore Perazzoli <ettore@helixcode.com>
*/

#include <config.h>
#include <gnome.h>
#include <bonobo.h>

#include "menubar.h"
#include "gtkhtml.h"
#include "control-data.h"
#include "properties.h"
#include "image.h"
#include "text.h"
#include "link.h"
#include "spell.h"
#include "table.h"
#include "template.h"

static void
undo_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_undo (cd->html);
}

static void
redo_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_redo (cd->html);
}

static void
cut_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_cut (cd->html);
}

static void
copy_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_copy (cd->html);
}

static void
paste_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_paste (cd->html);
}

static void
search_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search (cd, FALSE);
}

static void
search_regex_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search (cd, TRUE);
}

static void
search_next_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	search_next (cd);
}

static void
select_all_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_command (cd->html, "select-all");
}

static void
replace_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	replace (cd);
}

static void
insert_image_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);
	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, TRUE, _("Insert"));
	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_IMAGE, _("Image"),
						   image_insertion,
						   image_insert_cb,
						   image_close_cb);
	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_LINK, _("Link"),
						   link_properties,
						   link_apply_cb,
						   link_close_cb);
	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
insert_link_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, FALSE, _("Insert"));

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_TEXT, _("Text"),
						   text_properties,
						   text_apply_cb,
						   text_close_cb);
	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_LINK, _("Link"),
						   link_properties,
						   link_apply_cb,
						   link_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
	gtk_html_edit_properties_dialog_set_page (cd->properties_dialog, GTK_HTML_EDIT_PROPERTY_LINK);
}

static void
insert_rule_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, TRUE, _("Insert"));

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_RULE, _("Rule"),
						   rule_insert,
						   rule_insert_cb,
						   rule_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

void
insert_table (GtkHTMLControlData *cd)
{
	if (cd->properties_dialog)
		gtk_html_edit_properties_dialog_close (cd->properties_dialog);

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, TRUE, _("Insert"));

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_TABLE, _("Table"),
						   table_insert,
						   table_insert_cb,
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

	cd->properties_dialog = gtk_html_edit_properties_dialog_new (cd, TRUE, _("Insert"));

	gtk_html_edit_properties_dialog_add_entry (cd->properties_dialog,
						   GTK_HTML_EDIT_PROPERTY_TABLE, _("Template"),
						   template_insert,
						   template_insert_cb,
						   template_close_cb);

	gtk_html_edit_properties_dialog_show (cd->properties_dialog);
}

static void
properties_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gchar *argv[2] = {"gtkhtml-properties-capplet", NULL};
	if (gnome_execute_async (NULL, 1, argv) < 0)
		gnome_error_dialog (_("Cannot execute gtkhtml properties"));
}

static void 
menubar_update_font_size (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	GtkHTMLFontStyle style;
	g_warning  ("cname = %s", cname);
	
	style = GTK_HTML_FONT_STYLE_SIZE_1;
	
	if (!cd->block_font_style_change)
		gtk_html_set_font_style (cd->html, GTK_HTML_FONT_STYLE_MAX & ~GTK_HTML_FONT_STYLE_SIZE_MASK, style);
}

static void 
indent_more_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_modify_indent_by_delta (GTK_HTML (cd->html), +1);
}

static void 
indent_less_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	gtk_html_modify_indent_by_delta (GTK_HTML (cd->html), -1);
}

static void 
spell_check_cb (BonoboUIComponent *uic, GtkHTMLControlData *cd, const char *cname)
{
	spell_check_document (cd);
}

BonoboUIVerb verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("EditUndo", undo_cb),
	BONOBO_UI_UNSAFE_VERB ("EditRedo", redo_cb),
	BONOBO_UI_UNSAFE_VERB ("EditCut", cut_cb),
	BONOBO_UI_UNSAFE_VERB ("EditCopy", copy_cb),
	BONOBO_UI_UNSAFE_VERB ("EditPaste", paste_cb),
	BONOBO_UI_UNSAFE_VERB ("EditFind", search_cb),
	BONOBO_UI_UNSAFE_VERB ("EditFindRegex", search_regex_cb),
	BONOBO_UI_UNSAFE_VERB ("EditFindAgain", search_next_cb),
	BONOBO_UI_UNSAFE_VERB ("EditReplace", replace_cb),
	BONOBO_UI_UNSAFE_VERB ("EditProperties", properties_cb),
	BONOBO_UI_UNSAFE_VERB ("EditSelectAll", select_all_cb),
	BONOBO_UI_UNSAFE_VERB ("EditSpellCheck", spell_check_cb),

	BONOBO_UI_UNSAFE_VERB ("InsertImage", insert_image_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertLink",  insert_link_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertRule",  insert_rule_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertTable", insert_table_cb),
	BONOBO_UI_UNSAFE_VERB ("InsertTemplate", insert_template_cb),

	BONOBO_UI_UNSAFE_VERB ("IndentMore", indent_more_cb),
	BONOBO_UI_UNSAFE_VERB ("IndentLess", indent_less_cb),

	/*
	BONOBO_UI_UNSAFE_VERB ("FontSizeNegTwo", font_size_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeNegOne", font_size_cb),
	BONOBO_UI_UNSAFE_VERB ("FontSizeZero", font_size_zero_cb),
	*/

	BONOBO_UI_VERB_END
};

void
menubar_update_paragraph_style (GtkHTML *html, GtkHTMLParagraphStyle style, GtkHTMLControlData *cd)
{
	BonoboUIComponent *uic;
	char *path = NULL;

	uic = bonobo_control_get_ui_component (cd->control);

	g_return_if_fail (uic != NULL);

	switch (style) {
	case GTK_HTML_PARAGRAPH_STYLE_NORMAL:
		path = "/commands/HeadingNormal";
		break;
	case GTK_HTML_PARAGRAPH_STYLE_H1:
		path = "/commands/HeadingH1";
		break;
	case GTK_HTML_PARAGRAPH_STYLE_H2:
		path = "/commands/HeadingH2";
		break;
	case GTK_HTML_PARAGRAPH_STYLE_H3:
		path = "/commands/HeadingH3";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_H4:
		path = "/commands/HeadingH4";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_H5:
		path = "/commands/HeadingH5";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_H6:
		path = "/commands/HeadingH6";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_ADDRESS:
		path = "/commands/HeadingAddress";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_PRE:
		path = "/commands/HeadingPreformat";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED:
		path = "/commands/HeadingBulletList";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN:
		path = "/commands/HeadingRomanList";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT:
		path = "/commands/HeadingNumberList";
		break;	
	case GTK_HTML_PARAGRAPH_STYLE_ITEMALPHA:
		path = "/commands/HeadingAlphaList";
		break;	
	default:
		path = NULL;
	}


	if (path) {
		CORBA_Environment ev;

		CORBA_exception_init (&ev);
		bonobo_ui_component_set_prop (uic, path,
					      "state", "1", &ev);
		CORBA_exception_free (&ev);	
	} else {
		g_warning ("Unknown Paragraph Style");
	}
}

void
menubar_update_format (GtkHTMLControlData *cd)
{
	CORBA_Environment ev;
	BonoboUIComponent *uic;
	gchar *sensitive;

	uic = bonobo_control_get_ui_component (cd->control);

	g_return_if_fail (uic != NULL);

	sensitive = (cd->format_html ? "1" : "0");

	CORBA_exception_init (&ev);

	bonobo_ui_component_freeze (uic, &ev);

	bonobo_ui_component_set_prop (uic, "/commands/InsertImage",
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
	
	bonobo_ui_component_set_prop (uic, "/commands/AlignLeft",
				      "sensitive", sensitive, &ev);		
	bonobo_ui_component_set_prop (uic, "/commands/AlignRight",
				      "sensitive", sensitive, &ev);	
	bonobo_ui_component_set_prop (uic, "/commands/AlignCenter",
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

void
menubar_setup (BonoboUIComponent  *uic,
	       GtkHTMLControlData *cd)
{
	g_return_if_fail (cd->html != NULL);
	g_return_if_fail (GTK_IS_HTML (cd->html));
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (uic));

	gtk_signal_connect (GTK_OBJECT (cd->html), "current_paragraph_style_changed",
			    GTK_SIGNAL_FUNC (menubar_update_paragraph_style), cd);

	gtk_signal_connect (GTK_OBJECT (cd->html), "insertion_font_style_changed",
			    GTK_SIGNAL_FUNC (menubar_update_font_size), cd);

	bonobo_ui_component_add_verb_list_with_data (uic, verbs, cd);

	bonobo_ui_util_set_ui (uic, GNOMEDATADIR,
			       "GNOME_GtkHTML_Editor.xml",
			       "GNOME_GtkHTML_Editor");
}
