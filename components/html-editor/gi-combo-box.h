/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gtk-combo-box.h - a customizable combobox
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Miguel de Icaza <miguel@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef GI_COMBO_BOX_H
#define GI_COMBO_BOX_H

#include <gtk/gtkhbox.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GI_COMBO_BOX_TYPE          (gi_combo_box_get_type())
#define GI_COMBO_BOX(obj)	    G_TYPE_CHECK_INSTANCE_CAST (obj, gi_combo_box_get_type (), GiComboBox)
#define GI_COMBO_BOX_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gi_combo_box_get_type (), GiComboBoxClass)
#define GI_IS_COMBO_BOX(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gi_combo_box_get_type ())

typedef struct _GiComboBox	   GiComboBox;
typedef struct _GiComboBoxPrivate GiComboBoxPrivate;
typedef struct _GiComboBoxClass   GiComboBoxClass;

struct _GiComboBox {
	GtkHBox hbox;
	GiComboBoxPrivate *priv;
};

struct _GiComboBoxClass {
	GtkHBoxClass parent_class;

	GtkWidget *(*pop_down_widget) (GiComboBox *cbox);

	/*
	 * invoked when the popup has been hidden, if the signal
	 * returns TRUE, it means it should be killed from the
	 */ 
	gboolean  *(*pop_down_done)   (GiComboBox *cbox, GtkWidget *);

	/*
	 * Notification signals.
	 */
	void      (*pre_pop_down)     (GiComboBox *cbox);
	void      (*post_pop_hide)    (GiComboBox *cbox);
};

GtkType    gi_combo_box_get_type    (void);
void       gi_combo_box_construct   (GiComboBox *combo_box,
				      GtkWidget   *display_widget,
				      GtkWidget   *optional_pop_down_widget);
void       gi_combo_box_get_pos     (GiComboBox *combo_box, int *x, int *y);

GtkWidget *gi_combo_box_new         (GtkWidget *display_widget,
				      GtkWidget *optional_pop_down_widget);
void       gi_combo_box_popup_hide  (GiComboBox *combo_box);

void       gi_combo_box_set_display (GiComboBox *combo_box,
				      GtkWidget *display_widget);

void       gi_combo_box_set_title   (GiComboBox *combo,
				      const gchar *title);

void       gi_combo_box_set_tearable        (GiComboBox *combo,
					      gboolean tearable);
void       gi_combo_box_set_arrow_sensitive (GiComboBox *combo,
					      gboolean sensitive);
void       gi_combo_box_set_arrow_relief    (GiComboBox *cc,
					      GtkReliefStyle relief);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
