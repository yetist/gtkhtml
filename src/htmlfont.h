#ifndef _HTMLFONT_H_
#define _HTMLFONT_H_

#include <gdk/gdk.h>

#define MAXFONTSIZES 7

typedef enum {
	Normal,
	Bold
} HTMLFontWeight;

typedef struct _HTMLFont HTMLFont;
typedef struct _HTMLFontStack HTMLFontStack;

struct _HTMLFont {
	gchar *family;
	gint size;
	gint pointSize;
	HTMLFontWeight weight;
	gboolean italic;
	gboolean underline;

	GdkColor *textColor;
	GdkFont *gdk_font;
  
};

struct _HTMLFontStack {
	GList *list;
};

HTMLFont *html_font_new (gchar *family, gint size, gint *fontSizes, HTMLFontWeight weight, gboolean italic, gboolean underline);
HTMLFontStack *html_font_stack_new (void);
void html_font_stack_push (HTMLFontStack *fs, HTMLFont *f);
void html_font_stack_clear (HTMLFontStack *fs);
HTMLFont *html_font_stack_pop (HTMLFontStack *fs);
HTMLFont *html_font_stack_top (HTMLFontStack *fs);
gint html_font_calc_width (HTMLFont *f, gchar *text, gint len);
gint html_font_calc_descent (HTMLFont *f);
gint html_font_calc_ascent (HTMLFont *f);
#endif /* _HTMLFONT_H_ */
