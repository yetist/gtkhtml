#ifndef __GTK_COMPAT_H__
#define __GTK_COMPAT_H__

#include <gtk/gtk.h>

/* Provide a GTK+ compatibility layer. */

#if !GTK_CHECK_VERSION (2,23,1)
#define GTK_COMBO_BOX_TEXT		GTK_COMBO_BOX
#define gtk_combo_box_text_new		gtk_combo_box_new_text
#define gtk_combo_box_text_append_text	gtk_combo_box_append_text
#endif

#if !GTK_CHECK_VERSION (2,23,1)
#define gdk_window_get_display		gdk_drawable_get_display
#define gdk_window_get_visual		gdk_drawable_get_visual
#endif

/* For use with GTK+ key binding functions. */
#if GTK_CHECK_VERSION (2,91,0)
#define COMPAT_BINDING_TYPE	G_OBJECT
#else
#define COMPAT_BINDING_TYPE	GTK_OBJECT
#endif

#if !GTK_CHECK_VERSION (2,91,0)

#define gtk_widget_get_preferred_size(widget, minimum_size, natural_size) \
	(gtk_widget_size_request ((widget), (minimum_size)))

#define gdk_window_set_background_pattern(window, pattern) \
	(gdk_window_set_back_pixmap ((window), NULL, FALSE))

#endif /* < 2.91.0 */

#if GTK_CHECK_VERSION (2,90,5)

/* Recreate GdkRegion until we drop GTK2 compatibility. */

#define GdkRegion cairo_region_t

#define gdk_region_new()		cairo_region_create()
#define gdk_region_destroy(region)	cairo_region_destroy (region)

#define gdk_region_union_with_rect(region, rect) \
	G_STMT_START { \
		if ((rect)->width > 0 && (rect)->height > 0) \
			cairo_region_union_rectangle ((region), (rect)); \
	} G_STMT_END

#endif /* >= 2.90.5 */

#endif /* __GTK_COMPAT_H__ */
