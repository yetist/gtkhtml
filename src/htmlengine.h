#ifndef _HTMLENGINE_H_
#define _HTMLENGINE_H_

typedef struct _HTMLEngine HTMLEngine;
typedef struct _HTMLEngineClass HTMLEngineClass;

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gtkhtml.h"
#include "htmltokenizer.h"
#include "htmlclue.h"
#include "htmlfont.h"
#include "htmllist.h"
#include "htmlcolor.h"
#include "htmlstack.h"
#include "htmlsettings.h"
#include "htmlpainter.h"
#include "stringtokenizer.h"

#define TYPE_HTML_ENGINE                 (html_engine_get_type ())
#define HTML_ENGINE(obj)                  (GTK_CHECK_CAST ((obj), TYPE_HTML_ENGINE, HTMLEngine))
#define HTML_ENGINE_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), TYPE_HTML_ENGINE, HTMLEngineClass))
#define IS_HTML_ENGINE(obj)              (GTK_CHECK_TYPE ((obj), TYPE_HTML_ENGINE))
#define IS_HTML_ENGINE_CLASS(klass)      (GTK_CHECK_CLASS_TYPE ((klass), TYPE_HTML_ENGINE))

#define LEFT_BORDER 10
#define RIGHT_BORDER 20
#define TOP_BORDER 10
#define BOTTOM_BORDER 10

typedef void (*HTMLParseFunc)(HTMLEngine *p, HTMLObject *clue, const gchar *str);

struct _HTMLEngine {

	GtkObject parent;

	gboolean parsing;
	HTMLTokenizer *ht;
	StringTokenizer *st;
	HTMLObject *clue;
	
	HTMLObject *flow;

	gint leftBorder;
	gint rightBorder;
	gint topBorder;
	gint bottomBorder;

	/* Size of current indenting */
	gint indent;

	/* For the widget */
	gint width;
	gint height;

	gboolean vspace_inserted;
	gboolean bodyParsed;

	HAlignType divAlign;

	/* Number of tokens parsed in the current time-slice */
	gint parseCount;
	gint granularity;

	/* Offsets */
	gint x_offset, y_offset;

	gboolean inTitle;

	HTMLFontWeight weight;
	gboolean italic;
	gboolean underline;
 
	gint fontsize;
	
	HTMLFontStack *fs;
	HTMLColorStack *cs;

	HTMLParseFunc parseFuncArray[26];
	HTMLPainter *painter;
	HTMLStackElement *blockStack;
	HTMLSettings *settings;

	/* timer id for parsing routine */
	guint timerId;

	GString *title;

	gboolean writing;

	/* FIXME: Something else than gchar* is probably
	   needed if we want to support anything else than
	   file:/ urls */
	gchar *actualURL;
	gchar *baseURL;

	/* The background pixmap */
	GdkPixbuf *bgPixmap;
	gboolean bgPixmapSet;

	/* Stack of lists currently active */
	HTMLListStack *listStack;

	/* the widget, used for signal emission*/
	GtkHTML *widget;
};


struct _HTMLEngineClass
{
	GtkObjectClass parent_class;
	
	void (*title_changed) (HTMLEngine *engine);
};

HTMLEngine *html_engine_new (void);
void        html_engine_begin (HTMLEngine *p, gchar *url);
void        html_engine_draw_background (HTMLEngine *e, gint xval, gint yval, gint x, gint y, gint w, gint h);
gchar      *html_engine_parse_body (HTMLEngine *p, HTMLObject *clue, const gchar *end[], gboolean toplevel);
void        html_engine_parse_one_token (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse (HTMLEngine *p);
void        html_engine_parse_b (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_f (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_h (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_i (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_l (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_p (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_t (HTMLEngine *p, HTMLObject *clue, const gchar *str);
void        html_engine_parse_u (HTMLEngine *p, HTMLObject *clue, const gchar *str);
const gchar *html_engine_parse_table (HTMLEngine *e, HTMLObject *clue, gint max_width, const gchar *attr);
HTMLFont *  html_engine_get_current_font (HTMLEngine *p);
void        html_engine_select_font (HTMLEngine *e);
void        html_engine_pop_font (HTMLEngine *e);
void        html_engine_block_end_font (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem);
void        html_engine_push_block (HTMLEngine *e, gint id, gint level, HTMLBlockFunc exitFunc, gint miscData1, gint miscData2);
void        html_engine_pop_block (HTMLEngine *e, gint id, HTMLObject *clue);
void        html_engine_insert_text (HTMLEngine *e, gchar *str, HTMLFont *f);
void        html_engine_calc_size (HTMLEngine *p);
void        html_engine_new_flow (HTMLEngine *p, HTMLObject *clue);
void        html_engine_draw (HTMLEngine *e, gint x, gint y, gint width, gint height);
gboolean    html_engine_insert_vspace (HTMLEngine *e, HTMLObject *clue, gboolean vspace_inserted);
void        html_engine_block_end_color_font (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem);
void        html_engine_pop_color (HTMLEngine *e);
void        html_engine_set_named_color (HTMLEngine *p, GdkColor *c, gchar *name);
void        html_painter_set_background_color (HTMLPainter *painter, GdkColor *color);
gint        html_engine_get_doc_height (HTMLEngine *p);
void        html_engine_stop_parser (HTMLEngine *e);
void        html_engine_write (HTMLEngine *e, gchar *buffer);
void        html_engine_end (HTMLEngine *e);
void        html_engine_block_end_list (HTMLEngine *e, HTMLObject *clue, HTMLStackElement *elem);
void        html_engine_free_block (HTMLEngine *e);

#endif /* _HTMLENGINE_H_ */
