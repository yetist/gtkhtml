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

#include <gnome.h>
#include <bonobo.h>

#include "toolbar.h"
#include "utils.h"
#include "gtk-combo-box.h"
#include "htmlcolor.h"
#include "htmlengine-edit-fontstyle.h"


#define EDITOR_TOOLBAR_PATH "/HTMLEditor"


/* Paragraph style option menu.  */

static struct {
	GtkHTMLParagraphStyle style;
	const gchar *description;
} paragraph_style_items[] = {
	{ GTK_HTML_PARAGRAPH_STYLE_NORMAL, _("Normal") },
	{ GTK_HTML_PARAGRAPH_STYLE_H1, _("Header 1") },
	{ GTK_HTML_PARAGRAPH_STYLE_H2, _("Header 2") },
	{ GTK_HTML_PARAGRAPH_STYLE_H3, _("Header 3") },
	{ GTK_HTML_PARAGRAPH_STYLE_H4, _("Header 4") },
	{ GTK_HTML_PARAGRAPH_STYLE_H5, _("Header 5") },
	{ GTK_HTML_PARAGRAPH_STYLE_H6, _("Header 6") },
	{ GTK_HTML_PARAGRAPH_STYLE_ADDRESS, _("Address") },
	{ GTK_HTML_PARAGRAPH_STYLE_PRE, _("Pre") },
	{ GTK_HTML_PARAGRAPH_STYLE_ITEMDIGIT, _("List item (digit)") },
	{ GTK_HTML_PARAGRAPH_STYLE_ITEMDOTTED, _("List item (unnumbered)") },
	{ GTK_HTML_PARAGRAPH_STYLE_ITEMROMAN, _("List item (roman)") },
	{ GTK_HTML_PARAGRAPH_STYLE_NORMAL, NULL },
};

static void
paragraph_style_changed_cb (GtkHTML *html,
			    GtkHTMLParagraphStyle style,
			    gpointer data)
{
	GtkOptionMenu *option_menu;
	guint i;

	option_menu = GTK_OPTION_MENU (data);

	for (i = 0; paragraph_style_items[i].description != NULL; i++) {
		if (paragraph_style_items[i].style == style) {
			gtk_option_menu_set_history (option_menu, i);
			return;
		}
	}

	g_warning ("Editor component toolbar: unknown paragraph style %d", style);
}

static void
paragraph_style_menu_item_activated_cb (GtkWidget *widget,
					gpointer data)
{
	GtkHTMLParagraphStyle style;
	GtkHTML *html;

	html = GTK_HTML (data);
	style = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), "paragraph_style_value"));

	g_warning ("Setting paragraph style to %d.", style);

	gtk_html_set_paragraph_style (html, style);
}

static GtkWidget *
setup_paragraph_style_option_menu (GtkHTML *html)
{
	GtkWidget *option_menu;
	GtkWidget *menu;
	guint i;

	option_menu = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	for (i = 0; paragraph_style_items[i].description != NULL; i++) {
		GtkWidget *menu_item;

		menu_item = gtk_menu_item_new_with_label (paragraph_style_items[i].description);
		gtk_widget_show (menu_item);

		gtk_menu_append (GTK_MENU (menu), menu_item);

		gtk_object_set_data (GTK_OBJECT (menu_item), "paragraph_style_value",
				     GINT_TO_POINTER (paragraph_style_items[i].style));
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
				    GTK_SIGNAL_FUNC (paragraph_style_menu_item_activated_cb),
				    html);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

	gtk_signal_connect (GTK_OBJECT (html), "current_paragraph_style_changed",
			    GTK_SIGNAL_FUNC (paragraph_style_changed_cb), option_menu);

	gtk_widget_show (option_menu);

	return option_menu;
}

static void
set_color (GtkWidget *w, gushort r, gushort g, gushort b, gushort a, GtkHTMLControlData *cd)
{
	HTMLColor *color = html_color_new_from_rgb (r, g, b);

	html_engine_set_color (cd->html->engine, color);
	html_color_unref (color);
}

static void
color_button_clicked (GtkWidget *w, GtkHTMLControlData *cd)
{
	gdouble r, g, b, rn, gn, bn;
	gtk_combo_box_popup_hide (GTK_COMBO_BOX (cd->combo));

	gnome_color_picker_get_d (GNOME_COLOR_PICKER (cd->cpicker), &r, &g, &b, NULL);

	rn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].red)  /0xffff;
	gn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].green)/0xffff;
	bn = ((gdouble) w->style->bg [GTK_STATE_NORMAL].blue) /0xffff;

	if (r != rn || g != gn || b != bn) {
		gnome_color_picker_set_d (GNOME_COLOR_PICKER (cd->cpicker), rn, gn, bn, 1.0);
		set_color (cd->cpicker, rn*0xffff, gn*0xffff, bn*0xffff, 0xffff, cd);
	}
}

static void
default_clicked (GtkWidget *w, GtkHTMLControlData *cd)
{
	gtk_combo_box_popup_hide (GTK_COMBO_BOX (cd->combo));
	gnome_color_picker_set_d (GNOME_COLOR_PICKER (cd->cpicker), 0.0, 0.0, 0.0, 1.0);
	html_engine_set_color (cd->html->engine,
			       html_colorset_get_color (cd->html->engine->settings->color_set, HTMLTextColor));
	
}

static GtkWidget *
setup_color_option_menu (GtkHTMLControlData *cd)
{
       	GtkWidget *vbox, *table, *button;

	cd->cpicker = gnome_color_picker_new ();
	GTK_WIDGET_UNSET_FLAGS (cd->cpicker, GTK_CAN_FOCUS);
	vbox = gtk_vbox_new (FALSE, 2);
	button = gtk_button_new_with_label (_("Default"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked", default_clicked, cd);
	gtk_signal_connect (GTK_OBJECT (cd->cpicker), "color_set", GTK_SIGNAL_FUNC (set_color), cd);
	table = color_table_new (color_button_clicked, cd);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), button);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), table);
	cd->combo = gtk_combo_box_new (cd->cpicker, vbox);
	if (!gnome_preferences_get_toolbar_relief_btn ()) {
		gtk_combo_box_set_arrow_relief (GTK_COMBO_BOX (cd->combo), GTK_RELIEF_NONE);
		gtk_button_set_relief (GTK_BUTTON (cd->cpicker), GTK_RELIEF_NONE);
	}

	gtk_widget_show_all (cd->combo);
	return cd->combo;
}


/* Data for the toolbar button callbacks.  */
/* (GnomeUIInfo sucks BTW.)  */

struct _ToolbarData {
	guint font_style_changed_connection_id;

	GtkWidget *html;

	GtkWidget *bold_button;
	GtkWidget *italic_button;
	GtkWidget *underline_button;
	GtkWidget *strikeout_button;

	GtkWidget *left_align_button;
	GtkWidget *center_button;
	GtkWidget *right_align_button;
};
typedef struct _ToolbarData ToolbarData;


/* Clipboard group.  */

static void
editor_toolbar_cut_cb (GtkWidget *widget,
		       gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

	gtk_html_cut (GTK_HTML (toolbar_data->html));
}

static void
editor_toolbar_copy_cb (GtkWidget *widget,
			gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

	gtk_html_copy (GTK_HTML (toolbar_data->html));
}

static void
editor_toolbar_paste_cb (GtkWidget *widget,
			 gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

	gtk_html_paste (GTK_HTML (toolbar_data->html));
}


/* Font style group.  */

static void
editor_toolbar_bold_cb (GtkWidget *widget,
			gpointer data)
{
	ToolbarData *toolbar_data;
	gboolean active;

	toolbar_data = (ToolbarData *) data;
	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	if (active)
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html),
					 GTK_HTML_FONT_STYLE_MAX,
					 GTK_HTML_FONT_STYLE_BOLD);
	else
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html), ~GTK_HTML_FONT_STYLE_BOLD, 0);
}

static void
editor_toolbar_italic_cb (GtkWidget *widget,
			  gpointer data)
{
	ToolbarData *toolbar_data;
	gboolean active;

	toolbar_data = (ToolbarData *) data;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	if (active)
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html),
					 GTK_HTML_FONT_STYLE_MAX,
					 GTK_HTML_FONT_STYLE_ITALIC);
	else
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html), ~GTK_HTML_FONT_STYLE_ITALIC, 0);
}

static void
editor_toolbar_underline_cb (GtkWidget *widget,
			     gpointer data)
{
	ToolbarData *toolbar_data;
	gboolean active;

	toolbar_data = (ToolbarData *) data;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	if (active)
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html),
					 GTK_HTML_FONT_STYLE_MAX,
					 GTK_HTML_FONT_STYLE_UNDERLINE);
	else
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html), ~GTK_HTML_FONT_STYLE_UNDERLINE, 0);
}

static void
editor_toolbar_strikeout_cb (GtkWidget *widget,
			     gpointer data)
{
	ToolbarData *toolbar_data;
	gboolean active;

	toolbar_data = (ToolbarData *) data;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	if (active)
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html),
					 GTK_HTML_FONT_STYLE_MAX,
					 GTK_HTML_FONT_STYLE_STRIKEOUT);
	else
		gtk_html_set_font_style (GTK_HTML (toolbar_data->html), ~GTK_HTML_FONT_STYLE_STRIKEOUT, 0);
}

static void
insertion_font_style_changed_cb (GtkHTML *widget,
				 GtkHTMLFontStyle font_style,
				 gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

#define BLOCK_SIGNAL(w)									\
	gtk_signal_handler_block_by_func (GTK_OBJECT (toolbar_data->w##_button),	\
					  editor_toolbar_##w##_cb, data)

	BLOCK_SIGNAL (bold);
	BLOCK_SIGNAL (italic);
	BLOCK_SIGNAL (underline);
	BLOCK_SIGNAL (strikeout);

#undef BLOCK_SIGNAL

	if (font_style & GTK_HTML_FONT_STYLE_BOLD)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->bold_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->bold_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_ITALIC)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->italic_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->italic_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_UNDERLINE)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->underline_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->underline_button), FALSE);

	if (font_style & GTK_HTML_FONT_STYLE_STRIKEOUT)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->strikeout_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_data->strikeout_button), FALSE);

#define UNBLOCK_SIGNAL(w)								\
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (toolbar_data->w##_button),	\
					    editor_toolbar_##w##_cb, data)

	UNBLOCK_SIGNAL (bold);
	UNBLOCK_SIGNAL (italic);
	UNBLOCK_SIGNAL (underline);
	UNBLOCK_SIGNAL (strikeout);

#undef UNBLOCK_SIGNAL
}

static void
color_changed_cb (GtkHTML *widget,
		  HTMLColor *color,
		  GtkHTMLControlData *cd)
{
	gnome_color_picker_set_d (GNOME_COLOR_PICKER (cd->cpicker),
				  ((gdouble) color->color.red)   / 0xffff,
				  ((gdouble) color->color.green) / 0xffff,
				  ((gdouble) color->color.blue)  / 0xffff,
				  1.0);
}


/* Alignment group.  */

static void
editor_toolbar_left_align_cb (GtkWidget *widget,
			      gpointer data)
{
	ToolbarData *toolbar_data;

	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	toolbar_data = (ToolbarData *) data;
	gtk_html_set_paragraph_alignment (GTK_HTML (toolbar_data->html),
					  GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT);
}

static void
editor_toolbar_center_cb (GtkWidget *widget,
			  gpointer data)
{
	ToolbarData *toolbar_data;

	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	toolbar_data = (ToolbarData *) data;
	gtk_html_set_paragraph_alignment (GTK_HTML (toolbar_data->html),
					  GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER);
}

static void
editor_toolbar_right_align_cb (GtkWidget *widget,
			       gpointer data)
{
	ToolbarData *toolbar_data;

	/* If the button is not active at this point, it means that the user clicked on
           some other button in the radio group.  */
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		return;

	toolbar_data = (ToolbarData *) data;
	gtk_html_set_paragraph_alignment (GTK_HTML (toolbar_data->html),
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

	gtk_signal_handler_block_by_data (object, data);
	gtk_toggle_button_set_active (toggle_button, TRUE);
	gtk_signal_handler_unblock_by_data (object, data);
}

static void
paragraph_alignment_changed_cb (GtkHTML *widget,
				GtkHTMLParagraphAlignment alignment,
				gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

	switch (alignment) {
	case GTK_HTML_PARAGRAPH_ALIGNMENT_LEFT:
		safe_set_active (toolbar_data->left_align_button, data);
		break;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_CENTER:
		safe_set_active (toolbar_data->center_button, data);
		break;
	case GTK_HTML_PARAGRAPH_ALIGNMENT_RIGHT:
		safe_set_active (toolbar_data->right_align_button, data);
		break;
	default:
		g_warning ("Unknown GtkHTMLParagraphAlignment %d.", alignment);
	}
}


/* Indentation group.  */

static void
editor_toolbar_indent_cb (GtkWidget *widget,
			  gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;
	gtk_html_indent (GTK_HTML (toolbar_data->html), +1);
}

static void
editor_toolbar_unindent_cb (GtkWidget *widget,
			    gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;
	gtk_html_indent (GTK_HTML (toolbar_data->html), -1);
}


/* Editor toolbar.  */

static GnomeUIInfo editor_toolbar_alignment_group[] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Left align"), N_("Left justify paragraphs"),
				editor_toolbar_left_align_cb, GNOME_STOCK_PIXMAP_ALIGN_LEFT),
	GNOMEUIINFO_ITEM_STOCK (N_("Center"), N_("Centers paragraphs"),
				editor_toolbar_center_cb, GNOME_STOCK_PIXMAP_ALIGN_CENTER),
	GNOMEUIINFO_ITEM_STOCK (N_("Right align"), N_("Right justify paragraphs"),
				editor_toolbar_right_align_cb, GNOME_STOCK_PIXMAP_ALIGN_RIGHT),
	GNOMEUIINFO_END
};

static GnomeUIInfo editor_toolbar_uiinfo[] = {
	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Cut"), N_("Cut the selected region to the clipboard"),
				editor_toolbar_cut_cb, GNOME_STOCK_PIXMAP_CUT),
	GNOMEUIINFO_ITEM_STOCK (N_("Copy"), N_("Copy the selected region to the clipboard"),
				editor_toolbar_copy_cb, GNOME_STOCK_PIXMAP_COPY),
	GNOMEUIINFO_ITEM_STOCK (N_("Paste"), N_("Paste contents of the clipboard"),
				editor_toolbar_paste_cb, GNOME_STOCK_PIXMAP_PASTE),

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_TOGGLEITEM, N_("Bold"), N_("Sets the bold font"),
	  editor_toolbar_bold_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_BOLD },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Italic"), N_("Make the selection italic"),
	  editor_toolbar_italic_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_ITALIC },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Underline"), N_("Make the selection underlined"),
	  editor_toolbar_underline_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_UNDERLINE },
	{ GNOME_APP_UI_TOGGLEITEM, N_("Strikeout"), N_("Make the selection striked out"),
	  editor_toolbar_strikeout_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_RADIOLIST (editor_toolbar_alignment_group),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Unindent"), N_("Indent paragraphs less"),
				editor_toolbar_unindent_cb, GNOME_STOCK_PIXMAP_TEXT_UNINDENT),
	GNOMEUIINFO_ITEM_STOCK (N_("Indent"), N_("Indent paragraphs more"),
				editor_toolbar_indent_cb, GNOME_STOCK_PIXMAP_TEXT_INDENT),

	GNOMEUIINFO_END
};

static void
toolbar_destroy_cb (GtkObject *object,
		    gpointer data)
{
	ToolbarData *toolbar_data;

	toolbar_data = (ToolbarData *) data;

	gtk_signal_disconnect (GTK_OBJECT (toolbar_data->html),
			       toolbar_data->font_style_changed_connection_id);

	g_free (toolbar_data);
}

static GtkWidget *
create_editor_toolbar (GtkHTMLControlData *cd)
{
	ToolbarData *data;
	GtkWidget *toolbar;
	GtkWidget *toolbar_frame;

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
	gtk_widget_show (toolbar);

	toolbar_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (toolbar_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (toolbar_frame), toolbar);
	gtk_widget_show (toolbar_frame);

	gtk_toolbar_prepend_widget (GTK_TOOLBAR (toolbar),
				    setup_paragraph_style_option_menu (cd->html),
				    NULL, NULL);

	data = g_new (ToolbarData, 1);

	gnome_app_fill_toolbar_with_data (GTK_TOOLBAR (toolbar), editor_toolbar_uiinfo, NULL, data);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar),
				   setup_color_option_menu (cd),
				   NULL, NULL);

	data->font_style_changed_connection_id
		= gtk_signal_connect (GTK_OBJECT (cd->html), "insertion_font_style_changed",
				      GTK_SIGNAL_FUNC (insertion_font_style_changed_cb),
				      data);

	/* The following SUCKS!  */

	data->html = GTK_WIDGET (cd->html);
	data->bold_button = editor_toolbar_uiinfo[5].widget;
	data->italic_button = editor_toolbar_uiinfo[6].widget;
	data->underline_button = editor_toolbar_uiinfo[7].widget;
	data->strikeout_button = editor_toolbar_uiinfo[8].widget;
	/* (FIXME TODO: Button for "fixed" style.)  */

	data->left_align_button = editor_toolbar_alignment_group[0].widget;
	data->center_button = editor_toolbar_alignment_group[1].widget;
	data->right_align_button = editor_toolbar_alignment_group[2].widget;

	gtk_signal_connect (GTK_OBJECT (toolbar), "destroy",
			    GTK_SIGNAL_FUNC (toolbar_destroy_cb), data);

	gtk_signal_connect (GTK_OBJECT (cd->html), "current_paragraph_alignment_changed",
			    GTK_SIGNAL_FUNC (paragraph_alignment_changed_cb), data);

	gtk_signal_connect (GTK_OBJECT (cd->html), "insertion_color_changed",
			    GTK_SIGNAL_FUNC (color_changed_cb),
			    cd);

	return toolbar_frame;
}


GtkWidget *
toolbar_setup (BonoboUIHandler *uih,
	       GtkHTMLControlData *cd)
{
	GtkWidget *toolbar;

	g_return_val_if_fail (uih != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_UI_HANDLER (uih), NULL);
	g_return_val_if_fail (cd->html != NULL, NULL);
	g_return_val_if_fail (GTK_IS_HTML (cd->html), NULL);

	toolbar = create_editor_toolbar (cd);

	return toolbar;
}
