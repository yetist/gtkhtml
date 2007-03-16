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

/* FIXME: Should use BonoboUIHandler.  */

#include <config.h>
#ifdef GNOME_GTKHTML_EDITOR_SHLIB
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include <libgnomeui/gnome-app-helper.h>
#include <bonobo.h>

#include "gi-color-combo.h"
#include "paragraph-style.h"
#include "toolbar.h"
#include "utils.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlengine-edit-fontstyle.h"
#include "htmlsettings.h"
#include "gtkhtml-private.h"

#define EDITOR_TOOLBAR_PATH "/HTMLEditor"

static void
active_font_size_changed (GtkComboBox *combo_box,
                          GtkHTMLControlData *cd)
{
	GtkHTMLFontStyle style = GTK_HTML_FONT_STYLE_SIZE_1;

	style += gtk_combo_box_get_active (combo_box);

	if (!cd->block_font_style_change)
		gtk_html_set_font_style (cd->html,
			GTK_HTML_FONT_STYLE_MAX &
			~GTK_HTML_FONT_STYLE_SIZE_MASK,
			style);
}

static void
font_size_changed (GtkHTML *html,
                   GtkHTMLFontStyle style,
                   GtkHTMLControlData *cd)
{
	if (style == GTK_HTML_FONT_STYLE_DEFAULT)
		style = GTK_HTML_FONT_STYLE_SIZE_3;
	else
		style &= GTK_HTML_FONT_STYLE_SIZE_MASK;

	cd->block_font_style_change++;

	gtk_combo_box_set_active (
		GTK_COMBO_BOX (cd->font_size_menu),
		style - GTK_HTML_FONT_STYLE_SIZE_1);

	cd->block_font_style_change--;
}

static GtkWidget *
setup_font_size_combo_box (GtkHTMLControlData *cd)
{
	GtkWidget *combo_box;

	combo_box = gtk_combo_box_new_text ();

	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "-2");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "-1");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "+0");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "+1");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "+2");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "+3");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), "+4");

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 2);

	g_signal_connect (
		combo_box, "changed",
		G_CALLBACK (active_font_size_changed), cd);

	g_signal_connect (
		cd->html, "insertion_font_style_changed",
		G_CALLBACK (font_size_changed), cd);

	gtk_widget_show (combo_box);

	return combo_box;
}

static void
apply_color (GdkColor *gdk_color, GtkHTMLControlData *cd)
{
	HTMLColor *color;
	
	color = gdk_color
		&& gdk_color != &html_colorset_get_color (cd->html->engine->settings->color_set, HTMLTextColor)->color
		? html_color_new_from_gdk_color (gdk_color) : NULL;

	gtk_html_set_color (cd->html, color);
	if (color)
		html_color_unref (color);
}

void
toolbar_apply_color (GtkHTMLControlData *cd)
{
	GdkColor *color;
	gboolean default_color;

	color = gi_color_combo_get_color (GI_COLOR_COMBO (cd->combo), &default_color);
	apply_color (color, cd);
	if (color)
		gdk_color_free (color);
}

static void
color_changed (GtkWidget *w, GdkColor *gdk_color, gboolean custom, gboolean by_user, gboolean is_default,
	       GtkHTMLControlData *cd)
{
	/* If the color was changed programatically there's not need to set things */
	if (!by_user)
		return;
	apply_color (gdk_color, cd);
}

static void
unset_focus (GtkWidget *w, gpointer data)
{
	GTK_WIDGET_UNSET_FLAGS (w, GTK_CAN_FOCUS);
	if (GTK_IS_CONTAINER (w))
		gtk_container_forall (GTK_CONTAINER (w), unset_focus, NULL);
}

static void
set_gi_color_combo (GtkHTML *html, GtkHTMLControlData *cd)
{
	gi_color_combo_set_color (GI_COLOR_COMBO (cd->combo),
			       &html_colorset_get_color_allocated (html->engine->settings->color_set,
								   html->engine->painter, HTMLTextColor)->color);
}

static void
realize_engine (GtkHTML *html, GtkHTMLControlData *cd)
{
	set_gi_color_combo (html, cd);
	g_signal_handlers_disconnect_matched (html, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL, G_CALLBACK (realize_engine), cd);
}

static void
load_done (GtkHTML *html, GtkHTMLControlData *cd)
{
	if (GTK_WIDGET_REALIZED (cd->html))
		set_gi_color_combo (html, cd);
	else
		g_signal_connect (cd->html, "realize", G_CALLBACK (realize_engine), cd);
}

static void
insertion_color_changed_cb (GtkHTML *widget, GdkColor *color, GtkHTMLControlData *cd)
{
	gi_color_combo_set_color ((GiColorCombo *) cd->combo, color);
}


static GtkWidget *
setup_gi_color_combo (GtkHTMLControlData *cd)
{
	HTMLColor *color;

	color = html_colorset_get_color (cd->html->engine->settings->color_set, HTMLTextColor);
	if (GTK_WIDGET_REALIZED (cd->html))
		html_color_alloc (color, cd->html->engine->painter);
	else
		g_signal_connect (cd->html, "realize", G_CALLBACK (realize_engine), cd);
        g_signal_connect (cd->html, "load_done", G_CALLBACK (load_done), cd);

	cd->combo = gi_color_combo_new (NULL, _("Automatic"), &color->color, color_group_fetch ("toolbar_text", cd));
	g_signal_connect (cd->combo, "color_changed", G_CALLBACK (color_changed), cd);
	g_signal_connect (cd->html, "insertion_color_changed", G_CALLBACK (insertion_color_changed_cb), cd);

	gtk_widget_show_all (cd->combo);
	return cd->combo;
}


/* Font style group.  */

static void
editor_toolbar_tt_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	if (!cd->block_font_style_change) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			gtk_html_set_font_style (GTK_HTML (cd->html),
						 GTK_HTML_FONT_STYLE_MAX,
						 GTK_HTML_FONT_STYLE_FIXED);
		else
			gtk_html_set_font_style (GTK_HTML (cd->html), ~GTK_HTML_FONT_STYLE_FIXED, 0);
	}
}

static void
editor_toolbar_bold_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	if (!cd->block_font_style_change) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			gtk_html_set_font_style (GTK_HTML (cd->html),
						 GTK_HTML_FONT_STYLE_MAX,
						 GTK_HTML_FONT_STYLE_BOLD);
		else
			gtk_html_set_font_style (GTK_HTML (cd->html), ~GTK_HTML_FONT_STYLE_BOLD, 0);
	}
}

static void
editor_toolbar_italic_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	if (!cd->block_font_style_change) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			gtk_html_set_font_style (GTK_HTML (cd->html),
						 GTK_HTML_FONT_STYLE_MAX,
						 GTK_HTML_FONT_STYLE_ITALIC);
		else
			gtk_html_set_font_style (GTK_HTML (cd->html), ~GTK_HTML_FONT_STYLE_ITALIC, 0);
	}
}

static void
editor_toolbar_underline_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	if (!cd->block_font_style_change) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			gtk_html_set_font_style (GTK_HTML (cd->html),
						 GTK_HTML_FONT_STYLE_MAX,
						 GTK_HTML_FONT_STYLE_UNDERLINE);
		else
			gtk_html_set_font_style (GTK_HTML (cd->html), ~GTK_HTML_FONT_STYLE_UNDERLINE, 0);
	}
}

static void
editor_toolbar_strikeout_cb (GtkWidget *widget, GtkHTMLControlData *cd)
{
	if (!cd->block_font_style_change) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
			gtk_html_set_font_style (GTK_HTML (cd->html),
						 GTK_HTML_FONT_STYLE_MAX,
						 GTK_HTML_FONT_STYLE_STRIKEOUT);
		else
			gtk_html_set_font_style (GTK_HTML (cd->html), ~GTK_HTML_FONT_STYLE_STRIKEOUT, 0);
	}
}

static void
insertion_font_style_changed_cb (GtkHTML *widget, GtkHTMLFontStyle font_style, GtkHTMLControlData *cd)
{
	cd->block_font_style_change++;

	if (font_style & GTK_HTML_FONT_STYLE_FIXED)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->tt_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->tt_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_BOLD)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->bold_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->bold_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->italic_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->italic_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->underline_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->underline_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->strikeout_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cd->strikeout_button), FALSE);

	cd->block_font_style_change--;
}


/* Alignment group.  */

static void
editor_toolbar_left_align_cb (GtkWidget *widget,
			      GtkHTMLControlData *cd)
{
	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	gtk_html_set_paragraph_alignment (GTK_HTML (cd->html),
					  GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT);
}

static void
editor_toolbar_center_cb (GtkWidget *widget,
			  GtkHTMLControlData *cd)
{
	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	gtk_html_set_paragraph_alignment (GTK_HTML (cd->html),
					  GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER);
}

static void
editor_toolbar_right_align_cb (GtkWidget *widget,
			       GtkHTMLControlData *cd)
{
	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	gtk_html_set_paragraph_alignment (GTK_HTML (cd->html),
					  GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT);
}

static void
safe_set_active (GtkWidget *widget,
		 gpointer data)
{
	GtkObject *object;
	GtkToggleButton *toggle_button;

	object = GTK_OBJECT (widget);
	toggle_button = GTK_TOGGLE_BUTTON (widget);

	g_signal_handlers_block_matched (object, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data);
	gtk_toggle_button_set_active (toggle_button, TRUE);
	g_signal_handlers_unblock_matched (object, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, data);
}

static void
paragraph_alignment_changed_cb (GtkHTML *widget,
				GtkHTMLParagraphAlignment alignment,
				GtkHTMLControlData *cd)
{
	switch (alignment) {
	case GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT:
		safe_set_active (cd->left_align_button, cd);
		break;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER:
		safe_set_active (cd->center_button, cd);
		break;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT:
		safe_set_active (cd->right_align_button, cd);
		break;
	default:
		g_warning ("Unknown GtkHTMLParagraphAlignment %d.", alignment);
	}
}


/* Indentation group.  */

static void

editor_toolbar_indent_cb (GtkWidget *widget,
			  GtkHTMLControlData *cd)
{
	gtk_html_indent_push_level (GTK_HTML (cd->html), HTML_LIST_TYPE_BLOCKQUOTE);
}

static void
editor_toolbar_unindent_cb (GtkWidget *widget,
			    GtkHTMLControlData *cd)
{
	gtk_html_indent_pop_level (GTK_HTML (cd->html));
}


/* Editor toolbar.  */

enum EditorAlignmentButtons {
	EDITOR_ALIGNMENT_LEFT,
	EDITOR_ALIGNMENT_CENTER,
	EDITOR_ALIGNMENT_RIGHT
};
static GnomeUIInfo editor_toolbar_alignment_group[] = {
	{ GNOME_APP_UI_ITEM, N_("Left align"), N_("Left justifies the paragraphs"),
	  editor_toolbar_left_align_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_JUSTIFY_LEFT },
	{ GNOME_APP_UI_ITEM, N_("Center"), N_("Center justifies the paragraphs"),
	  editor_toolbar_center_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_JUSTIFY_CENTER },
	{ GNOME_APP_UI_ITEM, N_("Right align"), N_("Right justifies the paragraphs"),
	  editor_toolbar_right_align_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_JUSTIFY_RIGHT },
	GNOMEUIINFO_END
};

enum EditorToolbarButtons {
	EDITOR_TOOLBAR_TT,
	EDITOR_TOOLBAR_BOLD,
	EDITOR_TOOLBAR_ITALIC,
	EDITOR_TOOLBAR_UNDERLINE,
	EDITOR_TOOLBAR_STRIKEOUT,
	EDITOR_TOOLBAR_SEP1,
	EDITOR_TOOLBAR_ALIGNMENT,
	EDITOR_TOOLBAR_SEP2,
	EDITOR_TOOLBAR_UNINDENT,
	EDITOR_TOOLBAR_INDENT
};

static GnomeUIInfo editor_toolbar_style_uiinfo[] = {

	{ GNOME_APP_UI_TOGGLEITEM, N_("Typewriter"), N_("Toggle typewriter font style"),
	  editor_toolbar_tt_cb, NULL, NULL, GNOME_APP_PIXMAP_FILENAME },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Bold"), N_("Makes the text bold"),
	  editor_toolbar_bold_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_BOLD },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Italic"), N_("Makes the text italic"),
	  editor_toolbar_italic_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_ITALIC },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Underline"), N_("Underlines the text"),
	  editor_toolbar_underline_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_UNDERLINE },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Strikeout"), N_("Strikes out the text"),
	  editor_toolbar_strikeout_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_STRIKETHROUGH },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_RADIOLIST (editor_toolbar_alignment_group),

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Unindent"), N_("Indents the paragraphs less"),
	  editor_toolbar_unindent_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_UNINDENT },
	{ GNOME_APP_UI_ITEM, N_("Indent"), N_("Indents the paragraphs more"),
	  editor_toolbar_indent_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_INDENT },

	GNOMEUIINFO_END
};

/* static void
toolbar_destroy_cb (GtkObject *object,
		    GtkHTMLControlData *cd)
{
	if (cd->html)
		gtk_signal_disconnect (GTK_OBJECT (cd->html),
				       cd->font_style_changed_connection_id);
}

static void
html_destroy_cb (GtkObject *object,
		 GtkHTMLControlData *cd)
{
	cd->html = NULL;
} */

static void
indentation_changed (GtkWidget *w, guint level, GtkHTMLControlData *cd)
{
	gtk_widget_set_sensitive (cd->unindent_button, level != 0);
}

static GtkWidget *
create_style_toolbar (GtkHTMLControlData *cd)
{
	GtkIconInfo *icon_info;
	GtkWidget *hbox;
	gchar *domain;
	
	hbox = gtk_hbox_new (FALSE, 0);

	cd->toolbar_style = gtk_toolbar_new ();
	gtk_box_pack_start (GTK_BOX (hbox), cd->toolbar_style, TRUE, TRUE, 0);

	cd->paragraph_option = paragraph_style_combo_box_new (cd);
	gtk_toolbar_prepend_space (GTK_TOOLBAR (cd->toolbar_style));
	gtk_toolbar_prepend_widget (GTK_TOOLBAR (cd->toolbar_style),
				    cd->paragraph_option,
				    NULL, NULL);

	cd->font_size_menu = setup_font_size_combo_box (cd);
	gtk_toolbar_prepend_space (GTK_TOOLBAR (cd->toolbar_style));
	gtk_toolbar_prepend_widget (GTK_TOOLBAR (cd->toolbar_style),
				    cd->font_size_menu,
				    NULL, NULL);

	/* 
	 * FIXME: steal textdomain temporarily from main-process,  and set it to 
	 * GETTEXT_PACKAGE, after create the widgets, restore it.
	 */
	domain = g_strdup (textdomain (NULL));
	textdomain (GETTEXT_PACKAGE);
	
	icon_info = gtk_icon_theme_lookup_icon (
		gtk_icon_theme_get_default (),
		"stock_text-monospaced", 24, 0);
	editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_TT].pixmap_info =
		g_strdup (gtk_icon_info_get_filename (icon_info));
	gtk_icon_info_free (icon_info);

	gnome_app_fill_toolbar_with_data (GTK_TOOLBAR (cd->toolbar_style), editor_toolbar_style_uiinfo, NULL, cd);

	/* restore the stolen domain */
	textdomain (domain);
	g_free (domain);

	gtk_toolbar_append_widget (GTK_TOOLBAR (cd->toolbar_style),
				   setup_gi_color_combo (cd),
				   NULL, NULL);

	cd->font_style_changed_connection_id
		= g_signal_connect (GTK_OBJECT (cd->html), "insertion_font_style_changed",
				      G_CALLBACK (insertion_font_style_changed_cb), cd);

	/* The following SUCKS!  */
	cd->tt_button        = editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_TT].widget;
	cd->bold_button      = editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_BOLD].widget;
	cd->italic_button    = editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_ITALIC].widget;
	cd->underline_button = editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_UNDERLINE].widget;
	cd->strikeout_button = editor_toolbar_style_uiinfo [EDITOR_TOOLBAR_STRIKEOUT].widget;

	cd->left_align_button = editor_toolbar_alignment_group[0].widget;
	cd->center_button = editor_toolbar_alignment_group[1].widget;
	cd->right_align_button = editor_toolbar_alignment_group[2].widget;

	cd->unindent_button  = editor_toolbar_style_uiinfo [8].widget;
	gtk_widget_set_sensitive (cd->unindent_button, gtk_html_get_paragraph_indentation (cd->html) != 0);
	g_signal_connect (cd->html, "current_paragraph_indentation_changed",
			  G_CALLBACK (indentation_changed), cd);

	cd->indent_button    = editor_toolbar_style_uiinfo [9].widget;

	/* g_signal_connect (GTK_OBJECT (cd->html), "destroy",
	   G_CALLBACK (html_destroy_cb), cd);

	   g_signal_connect (GTK_OBJECT (cd->toolbar_style), "destroy",
	   G_CALLBACK (toolbar_destroy_cb), cd); */

	g_signal_connect (cd->html, "current_paragraph_alignment_changed",
			  G_CALLBACK (paragraph_alignment_changed_cb), cd);

	gtk_toolbar_set_style (GTK_TOOLBAR (cd->toolbar_style), GTK_TOOLBAR_ICONS);
	gtk_widget_show_all (hbox);

	toolbar_update_format (cd);

	return hbox;
}

static gboolean
toolbar_item_represents (GtkWidget *item, GtkWidget *widget)
{
	GtkWidget *parent;

	if (item == widget)
		return TRUE;

	parent = gtk_widget_get_parent (widget);
	while (parent) {
		if (parent == item)
			return TRUE;
		parent = gtk_widget_get_parent (parent);
	}

	return FALSE;
}

static void
toolbar_item_update_sensitivity (GtkWidget *widget, gpointer data)
{
	GtkHTMLControlData *cd = (GtkHTMLControlData *)data;
	gboolean sensitive;

	if (toolbar_item_represents (widget, cd->unindent_button))
		return;

	sensitive = (cd->format_html
		     || toolbar_item_represents (widget, cd->paragraph_option)
		     || toolbar_item_represents (widget, cd->indent_button)
		     || toolbar_item_represents (widget, cd->left_align_button)
		     || toolbar_item_represents (widget, cd->center_button)
		     || toolbar_item_represents (widget, cd->right_align_button));

	gtk_widget_set_sensitive (widget, sensitive);
}

void
toolbar_update_format (GtkHTMLControlData *cd)
{
	if (cd->toolbar_style)
		gtk_container_foreach (GTK_CONTAINER (cd->toolbar_style), 
		toolbar_item_update_sensitivity, cd);
}


GtkWidget *
toolbar_style (GtkHTMLControlData *cd)
{
	g_return_val_if_fail (cd->html != NULL, NULL);
	g_return_val_if_fail (GTK_IS_HTML (cd->html), NULL);

	return create_style_toolbar (cd);
}
