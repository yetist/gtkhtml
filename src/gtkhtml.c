#include <gtk/gtk.h>
#include "htmlpainter.h"
#include "gtkhtml.h"

guint           gtk_html_get_type (void);
static void     gtk_html_class_init (GtkHTMLClass *klass);
static void     gtk_html_init (GtkHTML* html);

static void     gtk_html_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint     gtk_html_expose (GtkWidget *widget, GdkEventExpose *event);
static void     gtk_html_realize (GtkWidget *widget);
static void     gtk_html_draw (GtkWidget *widget, GdkRectangle *area);
void     gtk_html_calc_scrollbars (GtkHTML *htm);
static void     gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data);

static GtkLayoutClass *parent_class = NULL;

guint html_signals [LAST_SIGNAL] = { 0 };

guint
gtk_html_get_type (void)
{
	static guint html_type = 0;

	if (!html_type) {
		static const GtkTypeInfo html_info = {
			"GtkHTML",
			sizeof (GtkHTML),
			sizeof (GtkHTMLClass),
			(GtkClassInitFunc) gtk_html_class_init,
			(GtkObjectInitFunc) gtk_html_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};
		
		html_type = gtk_type_unique (GTK_TYPE_LAYOUT, &html_info);
	}

	return html_type;
}

static void
gtk_html_class_init (GtkHTMLClass *klass)
{
	GtkHTMLClass *html_class;
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	html_class = (GtkHTMLClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	object_class = (GtkObjectClass *)klass;

	parent_class = gtk_type_class (GTK_TYPE_LAYOUT);

	html_signals [TITLE_CHANGED] = 
		gtk_signal_new ("title_changed",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkHTMLClass, title_changed),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, html_signals, LAST_SIGNAL);

	widget_class->realize = gtk_html_realize;
	widget_class->draw = gtk_html_draw;
	widget_class->expose_event  = gtk_html_expose;
	widget_class->size_allocate = gtk_html_size_allocate;
}

static void
gtk_html_init (GtkHTML* html)
{
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (html), GTK_CAN_FOCUS);

	gtk_layout_set_hadjustment (GTK_LAYOUT (html), NULL);
	gtk_layout_set_vadjustment (GTK_LAYOUT (html), NULL);

	/* Create vertical scrollbar */
	html->vsbadjust = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	html->vscrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (html->vsbadjust));
	gtk_layout_put (GTK_LAYOUT (html), html->vscrollbar,
			0, 0);
	gtk_signal_connect (GTK_OBJECT (html->vsbadjust), "value_changed",
			    GTK_SIGNAL_FUNC (gtk_html_vertical_scroll), (gpointer)html);
}

GtkWidget *
gtk_html_new (void)
{
	GtkHTML *html;

	html = gtk_type_new (gtk_html_get_type ());

	html->engine = html_engine_new ();
	html->engine->widget = html;

	return GTK_WIDGET (html);
}

void
gtk_html_parse (GtkHTML *html)
{
	html_engine_parse (html->engine);
}

void
gtk_html_begin (GtkHTML *html, gchar *url)
{
	html->displayVScroll = FALSE;
	gtk_widget_hide (html->vscrollbar);
	GTK_ADJUSTMENT (html->vsbadjust)->step_increment = 12;

	html_engine_begin (html->engine, url);
}

void
gtk_html_write (GtkHTML *html, gchar *buffer)
{
	html_engine_write (html->engine, buffer);
}

void
gtk_html_end (GtkHTML *html)
{
	html_engine_end (html->engine);
}

static void
gtk_html_realize (GtkWidget *widget)
{
	GtkHTML *html;

	g_message ("realize");
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_HTML (widget));

	html = GTK_HTML (widget);

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	gdk_window_set_events (html->layout.bin_window,
			       (gdk_window_get_events (html->layout.bin_window)
				| GDK_EXPOSURE_MASK));

	/* Create our painter dc */
	html->engine->painter->gc = gdk_gc_new (html->layout.bin_window);

	/* Set our painter window */
	html->engine->painter->window = html->layout.bin_window;
}

static gint
gtk_html_expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (GTK_HTML (widget)->engine->settings->bgcolor)
		html_painter_set_background_color (GTK_HTML (widget)->engine->painter,
						   GTK_HTML (widget)->engine->settings->bgcolor);

	html_engine_draw (GTK_HTML (widget)->engine,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);
	return FALSE;
}

static void
gtk_html_draw (GtkWidget *widget, GdkRectangle *area)
{
	html_painter_set_background_color (GTK_HTML (widget)->engine->painter,
					   GTK_HTML (widget)->engine->settings->bgcolor);
	gdk_window_clear (GTK_HTML (widget)->engine->painter->window);


	html_engine_draw (GTK_HTML (widget)->engine,
			  area->x, area->y,
			  area->width, area->height);
}

static void
gtk_html_set_geometry (GtkWidget *widget, gint x, gint y, gint width, gint height)
{
	GtkAllocation allocation;

	allocation.x = x;
	allocation.y = y;
	allocation.width = width;
	allocation.height = height;

	gtk_widget_size_allocate (widget, &allocation);
}

static void
gtk_html_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkHTML *html;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_HTML (widget));
	g_return_if_fail (allocation != NULL);
	
	html = GTK_HTML (widget);

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		( *GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	html->engine->width = allocation->width;
	html->engine->height = allocation->height;

	html_engine_calc_size (html->engine);

	gtk_html_calc_scrollbars (html);

}


void
gtk_html_calc_scrollbars (GtkHTML *html)
{
	if ((html_engine_get_doc_height (html->engine) > html->engine->height)) {
		html->displayVScroll = TRUE;
	}
	else {
		html->displayVScroll = FALSE;
	}

	if (html->displayVScroll) {
		GTK_ADJUSTMENT (html->vsbadjust)->lower = 0;
		GTK_ADJUSTMENT (html->vsbadjust)->upper = html_engine_get_doc_height (html->engine);
		GTK_ADJUSTMENT (html->vsbadjust)->page_size = html->engine->height;
	}

	if (!html->displayVScroll) {
		gtk_widget_hide (html->vscrollbar);
		GTK_ADJUSTMENT (html->vsbadjust)->upper = GTK_ADJUSTMENT (html->vsbadjust)->page_size = 0;
	}
	else {
		gtk_html_set_geometry (html->vscrollbar,
				       GTK_WIDGET (html)->allocation.width - html->vscrollbar->allocation.width, 
				       0,
				       html->vscrollbar->allocation.width,
				       GTK_WIDGET (html)->allocation.height);
		gtk_widget_show (html->vscrollbar);
	}
	
	
}

static void
gtk_html_vertical_scroll (GtkAdjustment *adjustment, gpointer data)
{
	GtkHTML *html = GTK_HTML (data);

	html->engine->y_offset = (gint)adjustment->value;
	gtk_widget_draw (GTK_WIDGET (html), NULL);
}
