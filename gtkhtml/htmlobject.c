/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  This file is part of the GtkHTML library.
 *
 *  Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *  Copyright (C) 1997 Torben Weis (weis@kde.org)
 *  Copyright (C) 1999, 2000 Helix Code, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
*/

#include <config.h>
#include <string.h>

#include "htmlclue.h"
#include "htmlclueflow.h"
#include "htmlcluealigned.h"
#include "htmlcluev.h"
#include "htmlcolor.h"
#include "htmlcolorset.h"
#include "htmlcursor.h"
#include "htmlengine.h"
#include "htmlengine-edit.h"
#include "htmlengine-edit-cut-and-paste.h"
#include "htmlengine-save.h"
#include "htmlframe.h"
#include "htmlinterval.h"
#include "htmlobject.h"
#include "htmlpainter.h"
#include "htmltable.h"
#include "htmltablecell.h"
#include "htmltext.h"
#include "htmlrule.h"
#include "htmltype.h"
#include "htmlsettings.h"

#include "gtkhtmldebug.h"


HTMLObjectClass html_object_class;

#define HO_CLASS(x) HTML_OBJECT_CLASS (HTML_OBJECT (x)->klass)

static void remove_child (HTMLObject *self, HTMLObject *child) G_GNUC_NORETURN;

static void
destroy (HTMLObject *self)
{
#define GTKHTML_MEM_DEBUG 1
#if GTKHTML_MEM_DEBUG
	self->parent = HTML_OBJECT (0xdeadbeef);
	self->next = HTML_OBJECT (0xdeadbeef);
	self->prev = HTML_OBJECT (0xdeadbeef);
#else
	self->next = NULL;
	self->prev = NULL;
#endif

	g_datalist_clear (&self->object_data);
	g_datalist_clear (&self->object_data_nocp);

	g_free (self->id);
	self->id = NULL;

	if (self->redraw_pending) {
		self->free_pending = TRUE;
	} else {
		g_free (self);
	}
}

static void
copy (HTMLObject *self,
      HTMLObject *dest)
{
	dest->klass = self->klass;
	dest->parent = NULL;
	dest->prev = NULL;
	dest->next = NULL;
	dest->x = 0;
	dest->y = 0;
	dest->ascent = self->ascent;
	dest->descent = self->descent;
	dest->width = self->width;
	dest->min_width = self->min_width;
	dest->max_width = self->max_width;
	dest->pref_width = self->pref_width;
	dest->percent = self->percent;
	dest->flags = self->flags;
	dest->redraw_pending = FALSE;
	dest->selected = FALSE;
	dest->free_pending = FALSE;
	dest->change = self->change;
	dest->draw_focused = FALSE;
	dest->id = g_strdup (self->id);

	g_datalist_init (&dest->object_data);
	html_object_copy_data_from_object (dest, self);

	g_datalist_init (&dest->object_data_nocp);
}

static HTMLObject *
op_copy (HTMLObject *self,
         HTMLObject *parent,
         HTMLEngine *e,
         GList *from,
         GList *to,
         guint *len)
{
	if ((!from || GPOINTER_TO_INT (from->data) == 0)
	    && (!to || GPOINTER_TO_INT (to->data) == html_object_get_length (self))) {
		*len += html_object_get_recursive_length (self);

		return html_object_dup (self);
	} else
		return html_engine_new_text_empty (e);
}

static HTMLObject *
op_cut (HTMLObject *self,
        HTMLEngine *e,
        GList *from,
        GList *to,
        GList *left,
        GList *right,
        guint *len)
{
	if ((!from || GPOINTER_TO_INT (from->data) == 0)
	    && (!to || GPOINTER_TO_INT (to->data) == html_object_get_length (self))) {
		if (!html_object_could_remove_whole (self, from, to, left, right)) {
			HTMLObject *empty = html_engine_new_text_empty (e);

			if (e->cursor->object == self)
				e->cursor->object = empty;
			html_clue_append_after (HTML_CLUE (self->parent), empty, self);
			html_object_change_set (empty, HTML_CHANGE_ALL_CALC);
			html_object_check_cut_lists (self, empty, left, right);
		} else
			html_object_move_cursor_before_remove (self, e);

		html_object_change_set (self, HTML_CHANGE_ALL_CALC);
		html_object_change_set (self->parent, HTML_CHANGE_ALL_CALC);
		/* force parent redraw */
		self->parent->width = 0;
		html_object_remove_child (self->parent, self);
		*len += html_object_get_recursive_length (self);

		return self;
	} else
		return html_engine_new_text_empty (e);
}

static gboolean
merge (HTMLObject *self,
       HTMLObject *with,
       HTMLEngine *e,
       GList **left,
       GList **right,
       HTMLCursor *cursor)
{
	if (self->parent) {
		html_object_change_set (self->parent, HTML_CHANGE_ALL_CALC);
		self->parent->width = 0;
	}

	return FALSE;
}

static void
remove_child (HTMLObject *self,
              HTMLObject *child)
{
	g_warning ("REMOVE CHILD unimplemented for ");
	gtk_html_debug_dump_object_type (self);
	g_assert_not_reached ();
}

static void
split (HTMLObject *self,
       HTMLEngine *e,
       HTMLObject *child,
       gint offset,
       gint level,
       GList **left,
       GList **right)
{
	if (child || (offset && html_object_get_length (self) != offset)) {
		g_warning ("don't know how to SPLIT ");
		gtk_html_debug_dump_object_type (self);
		return;
	}

	if (offset) {
		if (!self->next) {
			html_clue_append (HTML_CLUE (self->parent), html_engine_new_text_empty (e));
		}
		*left  = g_list_prepend (*left,  self);
		*right = g_list_prepend (*right, self->next);
	} else {
		if (!self->prev) {
			e->cursor->object = html_engine_new_text_empty (e);
			e->cursor->offset = 0;
			html_clue_prepend (HTML_CLUE (self->parent), e->cursor->object);
		}
		*left  = g_list_prepend (*left,  self->prev);
		*right = g_list_prepend (*right, self);
	}
	level--;

	if (level && self->parent)
		html_object_split (self->parent, e, offset ? self->next : self, 0, level, left, right);
}

static void
draw (HTMLObject *o,
      HTMLPainter *p,
      gint x,
      gint y,
      gint width,
      gint height,
      gint tx,
      gint ty)
{
	/* Do nothing by default.  We don't know how to paint ourselves.  */
}

static gboolean
is_transparent (HTMLObject *self)
{
	return TRUE;
}

static HTMLFitType
fit_line (HTMLObject *o,
          HTMLPainter *painter,
          gboolean start_of_line,
          gboolean first_run,
          gboolean next_to_floating,
          gint width_left)
{
	return (o->width <= width_left || (first_run && !next_to_floating)) ? HTML_FIT_COMPLETE : HTML_FIT_NONE;
}

static gboolean
html_object_real_calc_size (HTMLObject *o,
                            HTMLPainter *painter,
                            GList **changed_objs)
{
	return FALSE;
}

static gint
calc_min_width (HTMLObject *o,
                HTMLPainter *painter)
{
	html_object_calc_size (o, painter, NULL);
	return o->width;
}

static gint
calc_preferred_width (HTMLObject *o,
                      HTMLPainter *painter)
{
	html_object_calc_size (o, painter, NULL);
	return o->width;
}

static void
set_max_width (HTMLObject *o,
               HTMLPainter *painter,
               gint max_width)
{
	o->max_width = max_width;
}

static void
set_max_height (HTMLObject *o,
                HTMLPainter *painter,
                gint max_height)
{
}

static gint
get_left_margin (HTMLObject *self,
                 HTMLPainter *painter,
                 gint y,
                 gboolean with_aligned)
{
	return 0;
}

static gint
get_right_margin (HTMLObject *self,
                  HTMLPainter *painter,
                  gint y,
                  gboolean with_aligned)
{
	return MAX (self->max_width, self->width);
}

static void
set_painter (HTMLObject *o,
             HTMLPainter *painter)
{
}

static void
reset (HTMLObject *o)
{
	/* o->width = 0;
	 * o->ascent = 0;
	 * o->descent = 0; */
}

static const gchar *
get_url (HTMLObject *o,
         gint offset)
{
	return NULL;
}

static const gchar *
get_target (HTMLObject *o,
            gint offset)
{
	return NULL;
}

static const gchar *
get_src (HTMLObject *o)
{
	return NULL;
}

static HTMLAnchor *
find_anchor (HTMLObject *o,
             const gchar *name,
             gint *x,
             gint *y)
{
	return NULL;
}

static void
set_bg_color (HTMLObject *o,
              GdkColor *color)
{
}

static GdkColor *
get_bg_color (HTMLObject *o,
              HTMLPainter *p)
{
	if (o->parent)
		return html_object_get_bg_color (o->parent, p);

	if (p->widget && GTK_IS_HTML (p->widget)) {
		HTMLEngine *e = html_object_engine (o, GTK_HTML (p->widget)->engine);
		return &((html_colorset_get_color (e->settings->color_set, HTMLBgColor))->color);
	}

	return NULL;
}

static HTMLObject *
check_point (HTMLObject *self,
             HTMLPainter *painter,
             gint x,
             gint y,
             guint *offset_return,
             gboolean for_cursor)
{
	if (x >= self->x
	    && x < self->x + self->width
	    && y >= self->y - self->ascent
	    && y < self->y + self->descent) {
		if (offset_return != NULL)
			*offset_return = 0;
		return self;
	}

	return NULL;
}

static gboolean
relayout (HTMLObject *self,
          HTMLEngine *engine,
          HTMLObject *child)
{
	/* FIXME gint types of this stuff might change in `htmlobject.h',
	 * remember to sync.  */
	guint prev_width;
	guint prev_ascent, prev_descent;
	gboolean changed;

	if (html_engine_frozen (engine))
		return FALSE;

	prev_width = self->width;
	prev_ascent = self->ascent;
	prev_descent = self->descent;

	/* Notice that this will reset ascent and descent which we
	 * need afterwards.  Yeah, yuck, bleargh.  */
	html_object_reset (self);

	/* Crappy hack to make crappy htmlclueflow.c happy.  */
	if (self->y < self->ascent + self->descent) {
		g_warning ("htmlobject.c:relayout -- Eeek! This should not happen!  "
			   "Y value < height of object!\n");
		self->y = 0;
	} else {
		self->y -= prev_ascent + prev_descent;
	}

	changed = html_object_calc_size (self, engine->painter, NULL);

	if (prev_width == self->width
	    && prev_ascent == self->ascent
	    && prev_descent == self->descent) {
		gtk_html_debug_log (engine->widget,
				    "relayout: %s %p did not change.\n",
				    html_type_name (HTML_OBJECT_TYPE (self)),
				    self);
		if (changed)
			html_engine_queue_draw (engine, self);

		return FALSE;
	}

	gtk_html_debug_log (engine->widget, "relayout: %s %p changed.\n",
			    html_type_name (HTML_OBJECT_TYPE (self)), self);

	if (self->parent == NULL) {
		/* FIXME resize the widget, e.g. scrollbars and such.  */
		html_engine_queue_draw (engine, self);

		/* FIXME extreme ugliness.  */
		self->x = 0;
		self->y = self->ascent;
	} else {
		/* Relayout our parent starting from us.  */
		if (!html_object_relayout (self->parent, engine, self))
			html_engine_queue_draw (engine, self);
	}

	/* If the object has shrunk, we have to clean the areas around
	 * it so that we don't leave garbage on the screen.  FIXME:
	 * this wastes some time if there is an object on the right of
	 * or under this one.  */

	if (prev_ascent + prev_descent > self->ascent + self->descent)
		html_engine_queue_clear (engine,
					 self->x,
					 self->y + self->descent,
					 self->width,
					 (prev_ascent + prev_descent
					  - (self->ascent + self->descent)));

	if (prev_width > self->width)
		html_engine_queue_clear (engine,
					 self->x + self->width,
					 self->y - self->ascent,
					 prev_width - self->width,
					 self->ascent + self->descent);

	return TRUE;
}

static HTMLVAlignType
get_valign (HTMLObject *self)
{
	return HTML_VALIGN_BOTTOM;
}

static gboolean
accepts_cursor (HTMLObject *self)
{
	return FALSE;
}

static void
get_cursor (HTMLObject *self,
            HTMLPainter *painter,
            guint offset,
            gint *x1,
            gint *y1,
            gint *x2,
            gint *y2)
{
	html_object_get_cursor_base (self, painter, offset, x2, y2);

	*x1 = *x2;
	*y1 = *y2 - self->ascent;
	*y2 += self->descent - 1;
}

static void
get_cursor_base (HTMLObject *self,
                 HTMLPainter *painter,
                 guint offset,
                 gint *x,
                 gint *y)
{
	html_object_calc_abs_position (self, x, y);

	if (offset > 0)
		*x += self->width;
}

static guint
get_length (HTMLObject *self)
{
	return html_object_accepts_cursor (self) ? 1 : 0;
}

static guint
get_line_length (HTMLObject *self,
                 HTMLPainter *p,
                 gint line_offset)
{
	return html_object_get_length (self);
}

static guint
get_recursive_length (HTMLObject *self)
{
	return html_object_get_length (self);
}

static gboolean
select_range (HTMLObject *self,
              HTMLEngine *engine,
              guint start,
              gint length,
              gboolean queue_draw)
{
	gboolean selected;
	gboolean changed;

	selected = length > 0 || (length == -1 && start < html_object_get_length (self)) || html_object_is_container (self) ? TRUE : FALSE;
	changed  = (!selected && self->selected) || (selected && !self->selected) ? TRUE : FALSE;

	self->selected = selected;

	return changed;
}

static void
append_selection_string (HTMLObject *self,
                         GString *buffer)
{
}

static void
forall (HTMLObject *self,
        HTMLEngine *e,
        HTMLObjectForallFunc func,
        gpointer data)
{
	(* func) (self, e, data);
}

static HTMLEngine *
get_engine (HTMLObject *self,
            HTMLEngine *e)
{
	return e;
}

static gboolean
is_container (HTMLObject *self)
{
	return FALSE;
}

static gboolean
save (HTMLObject *self,
      HTMLEngineSaveState *state)
{
	return TRUE;
}

static gboolean
save_plain (HTMLObject *self,
            HTMLEngineSaveState *state,
            gint requested_width)
{
	return TRUE;
}

static gint
check_page_split (HTMLObject *self,
                  HTMLPainter *p,
                  gint y)
{
	return 0;
}

static gboolean
search (HTMLObject *self,
        HTMLSearch *info)
{
	/* not found by default */
	return FALSE;
}

static HTMLObject *
next (HTMLObject *self,
      HTMLObject *child)
{
	return child->next;
}

static HTMLObject *
prev (HTMLObject *self,
      HTMLObject *child)
{
	return child->prev;
}

static HTMLObject *
head (HTMLObject *self)
{
	return NULL;
}

static HTMLObject *
tail (HTMLObject *self)
{
	return NULL;
}

static HTMLClearType
get_clear (HTMLObject *self)
{
	return HTML_CLEAR_NONE;
}

static HTMLDirection
html_object_real_get_direction (HTMLObject *o)
{
	if (o->parent)
		return html_object_get_direction (o->parent);

	return HTML_DIRECTION_DERIVED;
}

static gboolean
html_object_real_cursor_forward (HTMLObject *self,
                                 HTMLCursor *cursor,
                                 HTMLEngine *engine)
{
	gint len;

	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	len = html_object_get_length (self);
	if (cursor->offset < len) {
		cursor->offset++;
		cursor->position++;
		return TRUE;
	} else
		return FALSE;
}

static gboolean
html_cursor_allow_zero_offset (HTMLCursor *cursor,
                               HTMLObject *o)
{
	if (cursor->offset == 1) {
		HTMLObject *prev;

		prev = html_object_prev_not_slave (o);
		if (!prev || HTML_IS_CLUEALIGNED (prev))
			return TRUE;
		else {
			while (prev && !html_object_accepts_cursor (prev))
				prev = html_object_prev_not_slave (prev);

			if (!prev)
				return TRUE;
		}
	}

	return FALSE;
}

static gboolean
html_object_real_cursor_backward (HTMLObject *self,
                                  HTMLCursor *cursor,
                                  HTMLEngine *engine)
{
	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	if (cursor->offset > 1 || html_cursor_allow_zero_offset (cursor, self)) {
		cursor->offset--;
		cursor->position--;
		return TRUE;
	}

	return FALSE;
}

static gboolean
html_object_real_cursor_right (HTMLObject *self,
                               HTMLPainter *painter,
                               HTMLCursor *cursor)
{
	HTMLDirection dir = html_object_get_direction (self);

	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	if (dir != HTML_DIRECTION_RTL) {
		gint len;

		len = html_object_get_length (self);

		if (cursor->offset < len) {
			cursor->offset++;
			cursor->position++;
			return TRUE;
		}
	} else {
		if (cursor->offset > 1 || html_cursor_allow_zero_offset (cursor, self)) {
			cursor->offset--;
			cursor->position--;
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
html_object_real_cursor_left (HTMLObject *self,
                              HTMLPainter *painter,
                              HTMLCursor *cursor)
{
	HTMLDirection dir = html_object_get_direction (self);

	g_assert (self);
	g_assert (cursor->object == self);

	if (html_object_is_container (self))
		return FALSE;

	if (dir != HTML_DIRECTION_RTL) {
		if (cursor->offset > 1 || html_cursor_allow_zero_offset (cursor, self)) {
			cursor->offset--;
			cursor->position--;
			return TRUE;
		}
	} else {
		gint len;

		len = html_object_get_length (self);

		if (cursor->offset < len) {
			cursor->offset++;
			cursor->position++;
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
html_object_real_backspace (HTMLObject *self,
                            HTMLCursor *cursor,
                            HTMLEngine *engine)
{
	html_cursor_backward (cursor, engine);
	html_engine_delete (engine);

	return TRUE;
}

static gint
html_object_real_get_right_edge_offset (HTMLObject *o,
                                        HTMLPainter *painter,
                                        gint offset)
{
	return html_object_get_length (o);
}

static gint
html_object_real_get_left_edge_offset (HTMLObject *o,
                                       HTMLPainter *painter,
                                       gint offset)
{
	return 0;
}

/* Class initialization.  */

void
html_object_type_init (void)
{
	html_object_class_init (&html_object_class, HTML_TYPE_OBJECT, sizeof (HTMLObject));
}

void
html_object_class_init (HTMLObjectClass *klass,
                        HTMLType type,
                        guint object_size)
{
	g_return_if_fail (klass != NULL);

	/* Set type.  */
	klass->type = type;
	klass->object_size = object_size;

	/* Install virtual methods.  */
	klass->destroy = destroy;
	klass->copy = copy;
	klass->op_copy = op_copy;
	klass->op_cut = op_cut;
	klass->merge = merge;
	klass->remove_child = remove_child;
	klass->split = split;
	klass->draw = draw;
	klass->is_transparent = is_transparent;
	klass->fit_line = fit_line;
	klass->calc_size = html_object_real_calc_size;
	klass->set_max_width = set_max_width;
	klass->set_max_height = set_max_height;
	klass->get_left_margin = get_left_margin;
	klass->get_right_margin = get_right_margin;
	klass->set_painter = set_painter;
	klass->reset = reset;
	klass->calc_min_width = calc_min_width;
	klass->calc_preferred_width = calc_preferred_width;
	klass->get_url = get_url;
	klass->get_target = get_target;
	klass->get_src = get_src;
	klass->find_anchor = find_anchor;
	klass->set_link = NULL;
	klass->set_bg_color = set_bg_color;
	klass->get_bg_color = get_bg_color;
	klass->check_point = check_point;
	klass->relayout = relayout;
	klass->get_valign = get_valign;
	klass->accepts_cursor = accepts_cursor;
	klass->get_cursor = get_cursor;
	klass->get_cursor_base = get_cursor_base;
	klass->select_range = select_range;
	klass->append_selection_string = append_selection_string;
	klass->forall = forall;
	klass->is_container = is_container;
	klass->save = save;
	klass->save_plain = save_plain;
	klass->check_page_split = check_page_split;
	klass->search = search;
	klass->search_next = search;
	klass->get_length = get_length;
	klass->get_line_length = get_line_length;
	klass->get_recursive_length = get_recursive_length;
	klass->next = next;
	klass->prev = prev;
	klass->head = head;
	klass->tail = tail;
	klass->get_engine = get_engine;
	klass->get_clear = get_clear;
	klass->get_direction = html_object_real_get_direction;
	klass->cursor_forward = html_object_real_cursor_forward;
	klass->cursor_forward_one = html_object_real_cursor_forward;
	klass->cursor_backward = html_object_real_cursor_backward;
	klass->cursor_backward_one = html_object_real_cursor_backward;
	klass->cursor_left = html_object_real_cursor_left;
	klass->cursor_right = html_object_real_cursor_right;
	klass->backspace = html_object_real_backspace;
	klass->get_right_edge_offset = html_object_real_get_right_edge_offset;
	klass->get_left_edge_offset = html_object_real_get_left_edge_offset;
}

void
html_object_init (HTMLObject *o,
                  HTMLObjectClass *klass)
{
	o->klass = klass;

	o->parent = NULL;
	o->prev = NULL;
	o->next = NULL;

	/* we don't have any info cached in the beginning */
	o->change = HTML_CHANGE_ALL;

	o->x = 0;
	o->y = 0;

	o->ascent = 0;
	o->descent = 0;

	o->width = 0;
	o->max_width = 0;
	o->min_width = 0;
	o->pref_width = 0;
	o->percent = 0;

	o->flags = HTML_OBJECT_FLAG_FIXEDWIDTH; /* FIXME Why? */

	o->redraw_pending = FALSE;
	o->free_pending = FALSE;
	o->selected = FALSE;
	o->draw_focused = FALSE;

	g_datalist_init (&o->object_data);
	g_datalist_init (&o->object_data_nocp);

	o->id = NULL;
}

HTMLObject *
html_object_new (HTMLObject *parent)
{
	HTMLObject *o;

	o = g_new0 (HTMLObject, 1);
	html_object_init (o, &html_object_class);

	return o;
}


/* Object duplication.  */

HTMLObject *
html_object_dup (HTMLObject *object)
{
	HTMLObject *new;

	g_return_val_if_fail (object != NULL, NULL);

	new = g_malloc (object->klass->object_size);
	html_object_copy (object, new);

	return new;
}

HTMLObject *
html_object_op_copy (HTMLObject *self,
                     HTMLObject *parent,
                     HTMLEngine *e,
                     GList *from,
                     GList *to,
                     guint *len)
{
	return (* HO_CLASS (self)->op_copy) (self, parent, e, from, to, len);
}

HTMLObject *
html_object_op_cut (HTMLObject *self,
                    HTMLEngine *e,
                    GList *from,
                    GList *to,
                    GList *left,
                    GList *right,
                    guint *len)
{
	return (* HO_CLASS (self)->op_cut) (self, e, from, to, left, right, len);
}

gboolean
html_object_merge (HTMLObject *self,
                   HTMLObject *with,
                   HTMLEngine *e,
                   GList **left,
                   GList **right,
                   HTMLCursor *cursor)
{
	if ((HTML_OBJECT_TYPE (self) == HTML_OBJECT_TYPE (with)
	     /* FIXME */
	     || (HTML_OBJECT_TYPE (self) == HTML_TYPE_TABLECELL && HTML_OBJECT_TYPE (with) == HTML_TYPE_CLUEV)
	     || (HTML_OBJECT_TYPE (with) == HTML_TYPE_TABLECELL && HTML_OBJECT_TYPE (self) == HTML_TYPE_CLUEV))
	    && (* HO_CLASS (self)->merge) (self, with, e, left, right, cursor)) {
		if (with->parent)
			html_object_remove_child (with->parent, with);
		html_object_destroy (with);
		return TRUE;
	}
	return FALSE;
}

void
html_object_remove_child (HTMLObject *self,
                          HTMLObject *child)
{
	g_assert (self);
	g_assert (child);

	(* HO_CLASS (self)->remove_child) (self, child);
}

void
html_object_split (HTMLObject *self,
                   HTMLEngine *e,
                   HTMLObject *child,
                   gint offset,
                   gint level,
                   GList **left,
                   GList **right)
{
	g_assert (self);

	(* HO_CLASS (self)->split) (self, e, child, offset, level, left, right);
}


void
html_object_set_parent (HTMLObject *o,
                        HTMLObject *parent)
{
	o->parent = parent;

	/* parent change requires recalc of everything */
	o->change = HTML_CHANGE_ALL;
}

static void
frame_offset (HTMLObject *o,
              gint *x_return,
              gint *y_return)
{
	if (html_object_is_frame (o)) {
		HTMLEngine *e = html_object_get_engine (o, NULL);
		*x_return -= e->x_offset;
		*y_return -= e->y_offset;
	}
}

void
html_object_calc_abs_position (HTMLObject *o,
                               gint *x_return,
                               gint *y_return)
{
	HTMLObject *p;

	g_return_if_fail (o != NULL);

	*x_return = o->x;
	*y_return = o->y;

	frame_offset (o, x_return, y_return);

	for (p = o->parent; p != NULL; p = p->parent) {
		*x_return += p->x;
		*y_return += p->y - p->ascent;

		frame_offset (p, x_return, y_return);
	}
}

void
html_object_calc_abs_position_in_frame (HTMLObject *o,
                                        gint *x_return,
                                        gint *y_return)
{
	HTMLObject *p;

	g_return_if_fail (o != NULL);

	*x_return = o->x;
	*y_return = o->y;

	frame_offset (o, x_return, y_return);

	for (p = o->parent; p != NULL && !html_object_is_frame (p); p = p->parent) {
		*x_return += p->x;
		*y_return += p->y - p->ascent;

		frame_offset (p, x_return, y_return);
	}
}

GdkRectangle *
html_object_get_bounds (HTMLObject *o,
                        GdkRectangle *bounds)
{
	if (!bounds)
		bounds = g_new (GdkRectangle, 1);

	bounds->x = o->x;
	bounds->y = o->y - o->ascent;
	bounds->width = o->width;
	bounds->height = o->ascent + o->descent;

	return bounds;
}

gboolean
html_object_intersect (HTMLObject *o,
                       GdkRectangle *result,
                       gint x,
                       gint y,
                       gint width,
                       gint height)
{
	GdkRectangle b;
	GdkRectangle a;

	a.x = x;
	a.y = y;
	a.width = width;
	a.height = height;

	return gdk_rectangle_intersect (html_object_get_bounds (o, &b), &a, result);
}


/* Virtual methods.  */

void
html_object_destroy (HTMLObject *self)
{
	(* HO_CLASS (self)->destroy) (self);
}

void
html_object_copy (HTMLObject *self,
                  HTMLObject *dest)
{
	(* HO_CLASS (self)->copy) (self, dest);
}

void
html_object_draw (HTMLObject *o,
                  HTMLPainter *p,
                  gint x,
                  gint y,
                  gint width,
                  gint height,
                  gint tx,
                  gint ty)
{
	(* HO_CLASS (o)->draw) (o, p, x, y, width, height, tx, ty);
}

gboolean
html_object_is_transparent (HTMLObject *self)
{
	g_return_val_if_fail (self != NULL, TRUE);

	return (* HO_CLASS (self)->is_transparent) (self);
}

HTMLFitType
html_object_fit_line (HTMLObject *o,
                      HTMLPainter *painter,
                      gboolean start_of_line,
                      gboolean first_run,
                      gboolean next_to_floating,
                      gint width_left)
{
	return (* HO_CLASS (o)->fit_line) (o, painter, start_of_line, first_run, next_to_floating, width_left);
}

gboolean
html_object_calc_size (HTMLObject *o,
                       HTMLPainter *painter,
                       GList **changed_objs)
{
	gboolean rv;

	rv = (* HO_CLASS (o)->calc_size) (o, painter, changed_objs);
	o->change &= ~HTML_CHANGE_SIZE;

	return rv;
}

void
html_object_set_max_width (HTMLObject *o,
                           HTMLPainter *painter,
                           gint max_width)
{
	(* HO_CLASS (o)->set_max_width) (o, painter, max_width);
}

void
html_object_set_max_height (HTMLObject *o,
                            HTMLPainter *painter,
                            gint max_height)
{
	(* HO_CLASS (o)->set_max_height) (o, painter, max_height);
}

gint
html_object_get_left_margin (HTMLObject *self,
                             HTMLPainter *painter,
                             gint y,
                             gboolean with_aligned)
{
	return (* HO_CLASS (self)->get_left_margin) (self, painter, y, with_aligned);
}

gint
html_object_get_right_margin (HTMLObject *self,
                              HTMLPainter *painter,
                              gint y,
                              gboolean with_aligned)
{
	return (* HO_CLASS (self)->get_right_margin) (self, painter, y, with_aligned);
}

static void
set_painter_forall (HTMLObject *o,
                    HTMLEngine *e,
                    gpointer data)
{
	(* HO_CLASS (o)->set_painter) (o, HTML_PAINTER (data));
}

void
html_object_set_painter (HTMLObject *o,
                         HTMLPainter *painter)
{
	html_object_forall (o, NULL, set_painter_forall, painter);
}

void
html_object_reset (HTMLObject *o)
{
	(* HO_CLASS (o)->reset) (o);
}

gint
html_object_calc_min_width (HTMLObject *o,
                            HTMLPainter *painter)
{
	if (o->change & HTML_CHANGE_MIN_WIDTH) {
		o->min_width = (* HO_CLASS (o)->calc_min_width) (o, painter);
		o->change &= ~HTML_CHANGE_MIN_WIDTH;
	}
	return o->min_width;
}

gint
html_object_calc_preferred_width (HTMLObject *o,
                                  HTMLPainter *painter)
{
	if (o->change & HTML_CHANGE_PREF_WIDTH) {
		o->pref_width = (* HO_CLASS (o)->calc_preferred_width) (o, painter);
		o->change &= ~HTML_CHANGE_PREF_WIDTH;
	}
	return o->pref_width;
}

#if 0
gint
html_object_get_uris (HTMLObject *o,
                      gchar **link,
                      gchar **target,
                      gchar **src)
{
	return TRUE;
}
#endif

const gchar *
html_object_get_url (HTMLObject *o,
                     gint offset)
{
	return (* HO_CLASS (o)->get_url) (o, offset);
}

const gchar *
html_object_get_target (HTMLObject *o,
                        gint offset)
{
	return (* HO_CLASS (o)->get_target) (o, offset);
}

gchar *
html_object_get_complete_url (HTMLObject *o,
                              gint offset)
{
	const gchar *url, *target;

	url = html_object_get_url (o, offset);
	target = html_object_get_target (o, offset);
	return url || target ? g_strconcat (url ? url : "#", url ? (target && *target ? "#" : NULL) : target,
					      url ? target : NULL, NULL) : NULL;
}

const gchar *
html_object_get_src (HTMLObject *o)
{
	return (* HO_CLASS (o)->get_src) (o);
}

HTMLAnchor *
html_object_find_anchor (HTMLObject *o,
                         const gchar *name,
                         gint *x,
                         gint *y)
{
	return (* HO_CLASS (o)->find_anchor) (o, name, x, y);
}

void
html_object_set_bg_color (HTMLObject *o,
                          GdkColor *color)
{
	(* HO_CLASS (o)->set_bg_color) (o, color);
}

GdkColor *
html_object_get_bg_color (HTMLObject *o,
                          HTMLPainter *p)
{
	return (* HO_CLASS (o)->get_bg_color) (o, p);
}

HTMLObject *
html_object_check_point (HTMLObject *self,
                         HTMLPainter *painter,
                         gint x,
                         gint y,
                         guint *offset_return,
                         gboolean for_cursor)
{
	if (self->width == 0 || self->ascent + self->descent == 0)
		return NULL;

	return (* HO_CLASS (self)->check_point) (self, painter, x, y, offset_return, for_cursor);
}

gboolean
html_object_relayout (HTMLObject *self,
                      HTMLEngine *engine,
                      HTMLObject *child)
{
	g_return_val_if_fail (self != NULL, TRUE);
	return (* HO_CLASS (self)->relayout) (self, engine, child);
}

HTMLVAlignType
html_object_get_valign (HTMLObject *self)
{
	g_return_val_if_fail (self != NULL, HTML_VALIGN_BOTTOM);

	return (* HO_CLASS (self)->get_valign) (self);
}

gboolean
html_object_accepts_cursor (HTMLObject *self)
{
	return (* HO_CLASS (self)->accepts_cursor) (self);
}

/* Warning: `calc_size()' must have been called on `self' before this so that
 * this works correctly.  */
void
html_object_get_cursor (HTMLObject *self,
                        HTMLPainter *painter,
                        guint offset,
                        gint *x1,
                        gint *y1,
                        gint *x2,
                        gint *y2)
{
	(* HO_CLASS (self)->get_cursor) (self, painter, offset, x1, y1, x2, y2);

	if (*y2 - *y1 > *y2 - self->ascent)
		*y2 = *y1 + 20;

	if (!html_object_is_text (self) && *y2 - *y1 < 10) {
		gint missing = 10 - (*y2 - *y1);

		*y1 -= (missing >> 1) + ((missing >> 1) & 1);
		*y2 += missing >> 1;
	}
}

/* Warning: `calc_size()' must have been called on `self' before this so that
 * this works correctly.  */
void
html_object_get_cursor_base (HTMLObject *self,
                             HTMLPainter *painter,
                             guint offset,
                             gint *x,
                             gint *y)
{
	(* HO_CLASS (self)->get_cursor_base) (self, painter, offset, x, y);
}


gboolean
html_object_select_range (HTMLObject *self,
                          HTMLEngine *engine,
                          guint start,
                          gint length,
                          gboolean queue_draw)
{
	return (* HO_CLASS (self)->select_range) (self, engine, start, length, queue_draw);
}

void
html_object_append_selection_string (HTMLObject *self,
                                     GString *buffer)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (buffer != NULL);

	(* HO_CLASS (self)->append_selection_string) (self, buffer);
}

HTMLEngine *
html_object_get_engine (HTMLObject *self,
                        HTMLEngine *e)
{
	return (* HO_CLASS (self)->get_engine) (self, e);
}

HTMLEngine *
html_object_engine (HTMLObject *o,
                    HTMLEngine *e)
{
	while (o) {
		e = html_object_get_engine (o, e);
		if (html_object_is_frame (o))
			break;
		o = o->parent;
	}

	return e;
}

void
html_object_forall (HTMLObject *self,
                    HTMLEngine *e,
                    HTMLObjectForallFunc func,
                    gpointer data)
{
	(* HO_CLASS (self)->forall) (self, e, func, data);
}

/* Ugly.  We should have an `is_a' implementation.  */
gboolean
html_object_is_container (HTMLObject *self)
{
	return (* HO_CLASS (self)->is_container) (self);
}


/* Ugly.  We should have an `is_a' implementation.  */

gboolean
html_object_is_text (HTMLObject *object)
{
	HTMLType type;

	g_return_val_if_fail (object != NULL, FALSE);

	type = HTML_OBJECT_TYPE (object);

	return (type == HTML_TYPE_TEXT || type == HTML_TYPE_LINKTEXT);
}

gboolean
html_object_is_clue (HTMLObject *object)
{
	HTMLType type;

	g_return_val_if_fail (object != NULL, FALSE);

	type = HTML_OBJECT_TYPE (object);

	return (type == HTML_TYPE_CLUE || type == HTML_TYPE_CLUEV || type == HTML_TYPE_TABLECELL
		|| type == HTML_TYPE_CLUEFLOW || type == HTML_TYPE_CLUEALIGNED);
}

HTMLObject *
html_object_next_not_type (HTMLObject *object,
                           HTMLType t)
{
	HTMLObject *p;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (object->parent, NULL);

	p = html_object_next (object->parent, object);
	while (p && HTML_OBJECT_TYPE (p) == t)
		p = html_object_next (p->parent, p);

	return p;
}

HTMLObject *
html_object_prev_not_type (HTMLObject *object,
                           HTMLType t)
{
	HTMLObject *p;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (object->parent, NULL);

	p = html_object_prev (object->parent, object);
	while (p && HTML_OBJECT_TYPE (p) == t)
		p = html_object_prev (p->parent, p);

	return p;
}

HTMLObject *
html_object_next_not_slave (HTMLObject *object)
{
	return html_object_next_not_type (object, HTML_TYPE_TEXTSLAVE);
}

HTMLObject *
html_object_prev_not_slave (HTMLObject *object)
{
	return html_object_prev_not_type (object, HTML_TYPE_TEXTSLAVE);
}


gboolean
html_object_save (HTMLObject *self,
                  HTMLEngineSaveState *state)
{
	return (* HO_CLASS (self)->save) (self, state);
}

gboolean
html_object_save_plain (HTMLObject *self,
                        HTMLEngineSaveState *state,
                        gint requested_width)
{
	return (* HO_CLASS (self)->save_plain) (self, state, requested_width);
}

gint
html_object_check_page_split (HTMLObject *self,
                               HTMLPainter *p,
                               gint y)
{
	g_return_val_if_fail (self != NULL, 0);

	return (* HO_CLASS (self)->check_page_split) (self, p, y);
}

void
html_object_change_set (HTMLObject *self,
                        HTMLChangeFlags f)
{
	HTMLObject *obj = self;

	g_assert (self != NULL);

	if (f != HTML_CHANGE_NONE) {
		while (obj) {
			obj->change |= f;
			obj = obj->parent;
		}
	}
}

static void
change (HTMLObject *o,
        HTMLEngine *e,
        gpointer data)
{
	o->change |= GPOINTER_TO_INT (data);
}

void
html_object_change_set_down (HTMLObject *self,
                             HTMLChangeFlags f)
{
	html_object_forall (self, NULL, (HTMLObjectForallFunc) change, GINT_TO_POINTER (f));
}

gboolean
html_object_search (HTMLObject *self,
                    HTMLSearch *info)
{
	return (* HO_CLASS (self)->search) (self, info);
}

gboolean
html_object_search_next (HTMLObject *self,
                         HTMLSearch *info)
{
	return (* HO_CLASS (self)->search_next) (self, info);
}

HTMLObject *
html_object_set_link (HTMLObject *self,
                      HTMLColor *color,
                      const gchar *url,
                      const gchar *target)
{
	return (HO_CLASS (self)->set_link) ? (* HO_CLASS (self)->set_link) (self, color, url, target) : NULL;
}

HTMLObject *
html_object_remove_link (HTMLObject *self,
                         HTMLColor *color)
{
	return (HO_CLASS (self)->set_link) ? (* HO_CLASS (self)->set_link) (self, color, NULL, NULL) : NULL;
}

guint
html_object_get_length (HTMLObject *self)
{
	return (* HO_CLASS (self)->get_length) (self);
}

guint
html_object_get_line_length (HTMLObject *self,
                             HTMLPainter *p,
                             gint line_offset)
{
	return (* HO_CLASS (self)->get_line_length) (self, p, line_offset);
}

guint
html_object_get_recursive_length (HTMLObject *self)
{
	return (* HO_CLASS (self)->get_recursive_length) (self);
}

HTMLObject *
html_object_next_by_type (HTMLObject *self,
                          HTMLType t)
{
	HTMLObject *next;

	g_assert (self);

	next = self->next;
	while (next && HTML_OBJECT_TYPE (next) != t)
		next = next->next;

	return next;
}

HTMLObject *
html_object_prev_by_type (HTMLObject *self,
                          HTMLType t)
{
	HTMLObject *prev;

	g_assert (self);

	prev = self->prev;
	while (prev && HTML_OBJECT_TYPE (prev) != t)
		prev = prev->prev;

	return prev;
}

/* Movement functions */

HTMLObject *
html_object_next (HTMLObject *self,
                  HTMLObject *child)
{
	return (* HO_CLASS (self)->next) (self, child);
}

HTMLObject *
html_object_prev (HTMLObject *self,
                  HTMLObject *child)
{
	return (* HO_CLASS (self)->prev) (self, child);
}

HTMLObject *
html_object_head (HTMLObject *self)
{
	return (* HO_CLASS (self)->head) (self);
}

HTMLObject *
html_object_tail (HTMLObject *self)
{
	return (* HO_CLASS (self)->tail) (self);
}

HTMLObject *
html_object_tail_not_slave (HTMLObject *self)
{
	HTMLObject *o = html_object_tail (self);

	if (o && HTML_OBJECT_TYPE (o) == HTML_TYPE_TEXTSLAVE)
		o = html_object_prev_not_slave (o);
	return o;
}

gboolean
html_object_cursor_forward (HTMLObject *self,
                            HTMLCursor *cursor,
                            HTMLEngine *engine)
{
	return (* HO_CLASS (self)->cursor_forward) (self, cursor, engine);
}

gboolean
html_object_cursor_forward_one (HTMLObject *self,
                                HTMLCursor *cursor,
                                HTMLEngine *engine)
{
	return (* HO_CLASS (self)->cursor_forward_one) (self, cursor, engine);
}

gboolean
html_object_cursor_backward (HTMLObject *self,
                             HTMLCursor *cursor,
                             HTMLEngine *engine)
{
	return (* HO_CLASS (self)->cursor_backward) (self, cursor, engine);
}

gboolean
html_object_cursor_backward_one (HTMLObject *self,
                                 HTMLCursor *cursor,
                                 HTMLEngine *engine)
{
	return (* HO_CLASS (self)->cursor_backward_one) (self, cursor, engine);
}

gboolean
html_object_cursor_right (HTMLObject *self,
                          HTMLPainter *painter,
                          HTMLCursor *cursor)
{
	return (* HO_CLASS (self)->cursor_right) (self, painter, cursor);
}

gboolean
html_object_cursor_left (HTMLObject *self,
                         HTMLPainter *painter,
                         HTMLCursor *cursor)
{
	return (* HO_CLASS (self)->cursor_left) (self, painter, cursor);
}

gboolean
html_object_backspace (HTMLObject *self,
                       HTMLCursor *cursor,
                       HTMLEngine *engine)
{
	return (* HO_CLASS (self)->backspace) (self, cursor, engine);
}

/*********************
 * movement on leafs
 */

/* go up in tree so long as we can get object in neighborhood given by function next_fn */

static HTMLObject *
next_object_uptree (HTMLObject *obj,
                    HTMLObject * (*next_fn) (HTMLObject *,
                    HTMLObject *))
{
	HTMLObject *next = NULL;

	while (obj->parent && !(next = (*next_fn) (obj->parent, obj)))
		obj = obj->parent;

	return next;
}

/* go down in tree to leaf in way given by down_fn children */

static HTMLObject *
move_object_downtree (HTMLObject *obj,
                      HTMLObject * (*down_fn) (HTMLObject *))
{
	HTMLObject *down;

	while ((down = (*down_fn) (obj)))
		obj = down;

	return obj;
}

static HTMLObject *
move_object (HTMLObject *obj,
             HTMLObject * (*next_fn) (HTMLObject *,
             HTMLObject *),
             HTMLObject * (*down_fn) (HTMLObject *))
{
	obj = next_object_uptree (obj, next_fn);
	if (obj)
		obj = move_object_downtree (obj, down_fn);
	return obj;
}

HTMLObject *
html_object_next_leaf (HTMLObject *self)
{
	return move_object (self, html_object_next, html_object_head);
}

HTMLObject *
html_object_next_leaf_not_type (HTMLObject *self,
                                HTMLType t)
{
	HTMLObject *rv = self;
	while ((rv = html_object_next_leaf (rv)) && HTML_OBJECT_TYPE (rv) == t);

	return rv;
}

HTMLObject *
html_object_prev_leaf (HTMLObject *self)
{
	return move_object (self, html_object_prev, html_object_tail);
}

HTMLObject *
html_object_prev_leaf_not_type (HTMLObject *self,
                                HTMLType t)
{
	HTMLObject *rv = self;
	while ((rv = html_object_prev_leaf (rv)) && HTML_OBJECT_TYPE (rv) == t);

	return rv;
}

/* movement on cursor accepting objects */

/* go up in tree so long as we can get object in neighborhood given by function next_fn */

static HTMLObject *
next_object_uptree_cursor (HTMLObject *obj,
                           HTMLObject * (*next_fn) (HTMLObject *))
{
	HTMLObject *next = NULL;

	while (obj->parent && !(next = (*next_fn) (obj))) {
		obj = obj->parent;
		if (html_object_accepts_cursor (obj))
			return obj;
	}

	return next;
}

/* go down in tree to leaf in way given by down_fn children */

static HTMLObject *
move_object_downtree_cursor (HTMLObject *obj,
                             HTMLObject * (*down_fn) (HTMLObject *),
                             HTMLObject * (*next_fn) (HTMLObject *))
{
	HTMLObject *last_obj = obj;

	while ((obj = (*down_fn) (obj))) {
		if (html_object_accepts_cursor (obj))
			break;
		last_obj = obj;
	}

	if (!obj && last_obj) {
		obj = last_obj;

		while ((obj = (*next_fn) (obj))) {
			if (html_object_accepts_cursor (obj))
				break;
			last_obj = obj;
			if ((obj = move_object_downtree_cursor (obj, down_fn, next_fn)))
				break;
			obj = last_obj;
		}
	}

	return obj;
}

static HTMLObject *
move_object_cursor (HTMLObject *obj,
                    gint *offset,
                    gboolean forward,
                    HTMLObject * (*next_fn) (HTMLObject *), HTMLObject * (*down_fn) (HTMLObject *))
{
	HTMLObject *down, *before;

	do {
		gboolean found = FALSE;
		if (((*offset == 0 && forward) || (*offset && !forward)) && html_object_is_container (obj)) {
			if ((down = (*down_fn) (obj))) {
				HTMLObject *down_child;

				down_child = move_object_downtree_cursor (down, down_fn, next_fn);
				if (down_child) {
					if (html_object_is_container (down_child))
						*offset = forward ? 0 : 1;

					return down_child;
				} else {
					obj = down;
				}
			}
		}

		before = obj;
		do {
			obj = next_object_uptree_cursor (obj, next_fn);
			if (obj) {
				if (html_object_accepts_cursor (obj)) {
					if (html_object_is_container (obj))
						*offset = before->parent == obj->parent
							? forward ? 0 : 1
							: forward ? 1 : 0;
					found = TRUE;
				} else {
					HTMLObject *down;
					down = move_object_downtree_cursor (obj, down_fn, next_fn);
					if (down) {
						if (html_object_is_container (down))
							*offset = forward ? 0 : 1;
						obj = down;
						found = TRUE;
					}
				}
			}
		} while (obj && !found);
	} while (obj && !html_object_accepts_cursor (obj));

	return obj;
}

HTMLObject *
html_object_next_cursor (HTMLObject *self,
                         gint *offset)
{
	return move_object_cursor (self, offset, TRUE, html_object_next_not_slave, html_object_head);
}

HTMLObject *
html_object_prev_cursor (HTMLObject *self,
                         gint *offset)
{
	return move_object_cursor (self, offset, FALSE, html_object_prev_not_slave, html_object_tail_not_slave);
}

/***/

guint
html_object_get_bytes (HTMLObject *self)
{
	return html_object_is_text (self) ? html_text_get_bytes (HTML_TEXT (self)) : html_object_get_length (self);
}

guint
html_object_get_index (HTMLObject *self,
                       guint offset)
{
	return html_object_is_text (self) ? html_text_get_index (HTML_TEXT (self), offset) : offset;
}

void
html_object_set_data_nocp (HTMLObject *object,
                           const gchar *key,
                           const gchar *value)
{
	g_datalist_set_data_full (&object->object_data_nocp, key, g_strdup (value), g_free);
}

void
html_object_set_data_full_nocp (HTMLObject *object,
                                const gchar *key,
                                gconstpointer value,
                                GDestroyNotify func)
{
	g_datalist_set_data_full (&object->object_data_nocp, key, (gpointer) value, func);
}

gpointer
html_object_get_data_nocp (HTMLObject *object,
                           const gchar *key)
{
	return g_datalist_get_data (&object->object_data_nocp, key);
}

void
html_object_set_data (HTMLObject *object,
                      const gchar *key,
                      const gchar *value)
{
	g_datalist_set_data_full (&object->object_data, key, g_strdup (value), g_free);
}

void
html_object_set_data_full (HTMLObject *object,
                           const gchar *key,
                           gconstpointer value,
                           GDestroyNotify func)
{
	g_datalist_set_data_full (&object->object_data, key, (gpointer) value, func);
}

gpointer
html_object_get_data (HTMLObject *object,
                      const gchar *key)
{
	return g_datalist_get_data (&object->object_data, key);
}

static void
copy_data (GQuark key_id,
           gpointer data,
           gpointer user_data)
{
	HTMLObject *o = HTML_OBJECT (user_data);

	g_datalist_id_set_data_full (&o->object_data,
				     key_id,
				     g_strdup ((gchar *) data), g_free);
}

void
html_object_copy_data_from_object (HTMLObject *dst,
                                   HTMLObject *src)
{
	g_datalist_foreach (&src->object_data, copy_data, dst);
}

static void
object_save_data (GQuark key_id,
                  gpointer data,
                  gpointer user_data)
{
	HTMLEngineSaveState *state = (HTMLEngineSaveState *) user_data;
	const gchar *str;

	str = html_engine_get_class_data (state->engine, state->save_data_class_name, g_quark_to_string (key_id));
	/* printf ("object %s %s\n", g_quark_to_string (key_id), str); */
	if (!str) {
		/* printf ("save %s %s -> %s\n", state->save_data_class_name, g_quark_to_string (key_id), (gchar *) data); */
		html_engine_save_delims_and_vals (state,
				"<!--+GtkHTML:<DATA class=\"", state->save_data_class_name,
				"\" key=\"", g_quark_to_string (key_id),
				"\" value=\"", (gchar *) data,
				"\">-->", NULL);
		html_engine_set_class_data (state->engine, state->save_data_class_name, g_quark_to_string (key_id), data);
	}
}

static void
handle_object_data (gpointer key,
                    gpointer value,
                    gpointer data)
{
	HTMLEngineSaveState *state = (HTMLEngineSaveState *) data;
	gchar *str;

	str = html_object_get_data (HTML_OBJECT (state->save_data_object), key);
	/* printf ("handle: %s %s %s %s\n", state->save_data_class_name, key, value, str); */
	if (!str) {
		/* printf ("clear\n"); */
		html_engine_save_delims_and_vals (state,
				"<!--+GtkHTML:<DATA class=\"", state->save_data_class_name,
				"\" clear=\"", (gchar *) key,
				"\">-->", NULL);
		state->data_to_remove = g_slist_prepend (state->data_to_remove, key);
	} else if (strcmp (value, str)) {
		/* printf ("change\n"); */
		html_engine_save_delims_and_vals (state,
				"<!--+GtkHTML:<DATA class=\"", state->save_data_class_name,
				"\" key=\"", (gchar *) key,
				"\" value=\"", str,
				"\">-->", NULL);
		html_engine_set_class_data (state->engine, state->save_data_class_name, key, value);
	}
}

static void
clear_data (gchar *key,
            HTMLEngineSaveState *state)
{
	html_engine_clear_class_data (state->engine, state->save_data_class_name, key);
}

gboolean
html_object_save_data (HTMLObject *self,
                       HTMLEngineSaveState *state)
{
	if (state->engine->save_data) {
		GHashTable *t;
		state->save_data_class_name = html_type_name (self->klass->type);
		state->save_data_object = self;
		t = html_engine_get_class_table (state->engine, state->save_data_class_name);
		if (t) {
			state->data_to_remove = NULL;
			g_hash_table_foreach (t, handle_object_data, state);
			g_slist_foreach (state->data_to_remove, (GFunc) clear_data, state);
			g_slist_free (state->data_to_remove);
			state->data_to_remove = NULL;
		}
		g_datalist_foreach (&self->object_data, object_save_data, state);
	}

	return TRUE;
}

GList *
html_object_get_bound_list (HTMLObject *self,
                            GList *list)
{
	return list && list->next
		? (HTML_OBJECT (list->data) == self ? list->next : NULL)
		: NULL;
}

void
html_object_move_cursor_before_remove (HTMLObject *o,
                                       HTMLEngine *e)
{
	if (e->cursor->object == o) {
		if (html_object_next_not_slave (o))
			e->cursor->object = html_object_next_not_slave (o);
		else
			e->cursor->object = html_object_prev_not_slave (o);
	}
}

gboolean
html_object_could_remove_whole (HTMLObject *o,
                                GList *from,
                                GList *to,
                                GList *left,
                                GList *right)
{
	return ((!from && !to)
		|| html_object_next_not_slave (HTML_OBJECT (o))
		|| html_object_prev_not_slave (HTML_OBJECT (o)))
		&& ((!left  || o != left->data) && (!right || o != right->data));

}

void
html_object_check_cut_lists (HTMLObject *self,
                             HTMLObject *replacement,
                             GList *left,
                             GList *right)
{
	if (left && left->data == self)
		left->data = replacement;
	if (right && right->data == self)
		right->data = replacement;
}

typedef struct {
	HTMLInterval *i;
	GString *buffer;
	gboolean in;
} tmpSelData;

static void
select_object (HTMLObject *o,
               HTMLEngine *e,
               gpointer data)
{
	tmpSelData *d = (tmpSelData *) data;

	if (o == d->i->from.object)
		d->in = TRUE;
	if (d->in)
		html_object_select_range (o, e,
					  html_interval_get_start (d->i, o),
					  html_interval_get_length (d->i, o), FALSE);

	if (o == d->i->to.object)
		d->in = FALSE;
}

static void
unselect_object (HTMLObject *o,
                 HTMLEngine *e,
                 gpointer data)
{
	o->selected = FALSE;
}

gchar *
html_object_get_selection_string (HTMLObject *o,
                                  HTMLEngine *e)
{
	HTMLObject *tail;
	tmpSelData data;
	gchar *string;

	g_assert (o);

	tail        = html_object_get_tail_leaf (o);
	data.buffer = g_string_new (NULL);
	data.in     = FALSE;
	data.i      = html_interval_new (html_object_get_head_leaf (o), tail, 0, html_object_get_length (tail));

	html_interval_forall (data.i, e, select_object, &data);
	html_object_append_selection_string (o, data.buffer);
	html_interval_forall (data.i, e, unselect_object, NULL);

	html_interval_destroy (data.i);
	string = data.buffer->str;
	g_string_free (data.buffer, FALSE);

	return string;
}

HTMLObject *
html_object_get_tail_leaf (HTMLObject *o)
{
	HTMLObject *tail, *rv = o;

	do {
		tail = html_object_tail_not_slave (rv);
		if (tail)
			rv = tail;
	} while (tail);

	return rv;
}

HTMLObject *
html_object_get_head_leaf (HTMLObject *o)
{
	HTMLObject *head, *rv = o;

	do {
		head = html_object_head (rv);
		if (head)
			rv = head;
	} while (head);

	return rv;
}

HTMLObject *
html_object_nth_parent (HTMLObject *self,
                        gint n)
{
	while (self && n > 0) {
		self = self->parent;
		n--;
	}

	return self;
}

gint
html_object_get_parent_level (HTMLObject *self)
{
	gint level = 0;

	while (self) {
		level++;
		self = self->parent;
	}

	return level;
}

GList *
html_object_heads_list (HTMLObject *o)
{
	GList *list = NULL;

	g_return_val_if_fail (o, NULL);

	while (o) {
		list = g_list_append (list, o);
		o = html_object_head (o);
	}

	return list;
}

GList *
html_object_tails_list (HTMLObject *o)
{
	GList *list = NULL;

	g_return_val_if_fail (o, NULL);

	while (o) {
		list = g_list_append (list, o);
		o = html_object_tail_not_slave (o);
	}

	return list;
}

static void
merge_down (HTMLEngine *e,
            GList *left,
            GList *right)
{
	HTMLObject *lo;
	HTMLObject *ro;

	while (left && right) {
		lo    = HTML_OBJECT (left->data);
		ro    = HTML_OBJECT (right->data);
		left  = left->next;
		right = right->next;
		if (!html_object_merge (lo, ro, e, &left, &right, NULL))
			break;
	}
}

void
html_object_merge_down (HTMLObject *o,
                        HTMLObject *w,
                        HTMLEngine *e)
{
	merge_down (e, html_object_tails_list (o), html_object_heads_list (w));
}

gboolean
html_object_is_parent (HTMLObject *parent,
                       HTMLObject *child)
{
	g_assert (parent && child);

	while (child) {
		if (child->parent == parent)
			return TRUE;
		child = child->parent;
	}

	return FALSE;
}

gint
html_object_get_insert_level (HTMLObject *o)
{
	switch (HTML_OBJECT_TYPE (o)) {
	case HTML_TYPE_TABLECELL:
	case HTML_TYPE_CLUEV: {
		gint level = 3;

		while (o && (HTML_IS_CLUEV (o) || HTML_IS_TABLE_CELL (o))
		       && HTML_CLUE (o)->head && (HTML_IS_CLUEV (HTML_CLUE (o)->head) || HTML_IS_TABLE_CELL (HTML_CLUE (o)->head))) {
			level++;
			o = HTML_CLUE (o)->head;
		}

		return level;
	}
	case HTML_TYPE_CLUEFLOW:
		return 2;
	default:
		return 1;
	}
}

void
html_object_engine_translation (HTMLObject *o,
                                HTMLEngine *e,
                                gint *tx,
                                gint *ty)
{
	HTMLObject *p;

	*tx = 0;
	*ty = 0;

	for (p = o->parent; p != NULL && HTML_OBJECT_TYPE (p) != HTML_TYPE_IFRAME; p = p->parent) {
		*tx += p->x;
		*ty += p->y - p->ascent;
	}

	/* *tx += e->leftBorder; */
	/* *ty += e->topBorder; */
}

gboolean
html_object_engine_intersection (HTMLObject *o,
                                 HTMLEngine *e,
                                 gint tx,
                                 gint ty,
                                 gint *x1,
                                 gint *y1,
                                 gint *x2,
                                 gint *y2)
{
	*x1 = o->x + tx;
	*y1 = o->y - o->ascent + ty;
	*x2 = o->x + o->width + tx;
	*y2 = o->y + o->descent + ty;

	return html_engine_intersection (e, x1, y1, x2, y2);
}

void
html_object_add_to_changed (GList **changed_objs,
                            HTMLObject *o)
{
	GList *l, *next;

	if (!changed_objs || (*changed_objs && (*changed_objs)->data == o))
		return;

	for (l = *changed_objs; l; l = next) {
		if (l->data == NULL) {
			l = l->next;
			next = l->next;
			continue;
		}
		next = l->next;
		if (html_object_is_parent (o, HTML_OBJECT (l->data))) {
			*changed_objs = g_list_remove_link (*changed_objs, l);
			g_list_free (l);
		} else
			break;
	}

	*changed_objs = g_list_prepend (*changed_objs, o);
}

gint
html_object_get_n_children (HTMLObject *self)
{
	return HO_CLASS (self)->get_n_children ? (* HO_CLASS (self)->get_n_children) (self) : 0;
}

HTMLObject *
html_object_get_child (HTMLObject *self,
                       gint index)
{
	return HO_CLASS (self)->get_child ? (* HO_CLASS (self)->get_child) (self, index) : NULL;
}

gint
html_object_get_child_index (HTMLObject *self,
                             HTMLObject *child)
{
	return HO_CLASS (self)->get_child_index ? (* HO_CLASS (self)->get_child_index) (self, child) : -1;
}

HTMLClearType
html_object_get_clear (HTMLObject *self)
{
	return (* HO_CLASS (self)->get_clear) (self);
}

static HTMLObject *
next_prev_cursor_object (HTMLObject *o,
                         HTMLEngine *e,
                         gint *offset,
                         gboolean forward)
{
	HTMLCursor cursor;
	gboolean result;

	html_cursor_init (&cursor, o, html_object_is_container (o) ? *offset : (forward ? html_object_get_length (o) : 0));

	result = forward ? html_cursor_forward (&cursor, e) : html_cursor_backward (&cursor, e);
	*offset = cursor.offset;

	return result ? cursor.object : NULL;
}

HTMLObject *
html_object_next_cursor_object (HTMLObject *o,
                                HTMLEngine *e,
                                gint *offset)
{
	return next_prev_cursor_object (o, e, offset, TRUE);
}

HTMLObject *
html_object_prev_cursor_object (HTMLObject *o,
                                HTMLEngine *e,
                                gint *offset)
{
	return next_prev_cursor_object (o, e, offset, FALSE);
}

HTMLObject *
html_object_next_cursor_leaf (HTMLObject *o,
                              HTMLEngine *e)
{
	gint offset = html_object_get_length (o);

	o = html_object_next_cursor_object (o, e, &offset);
	while (o && html_object_is_container (o))
		o = html_object_next_cursor_object (o, e, &offset);

	return o;
}

HTMLObject *
html_object_prev_cursor_leaf (HTMLObject *o,
                              HTMLEngine *e)
{
	gint offset = html_object_get_length (o);

	o = html_object_prev_cursor_object (o, e, &offset);
	while (o && html_object_is_container (o))
		o = html_object_prev_cursor_object (o, e, &offset);

	return o;
}

HTMLDirection
html_object_get_direction (HTMLObject *o)
{
	return (* HO_CLASS (o)->get_direction) (o);
}

const gchar *
html_object_get_id (HTMLObject *o)
{
	return o->id;
}

void
html_object_set_id (HTMLObject *o,
                    const gchar *id)
{
	g_free (o->id);
	o->id = g_strdup (id);
}

HTMLClueFlow *
html_object_get_flow (HTMLObject *o)
{
	while (o && !HTML_IS_CLUEFLOW (o))
		o = o->parent;

	return HTML_CLUEFLOW (o);
}

gint
html_object_get_right_edge_offset (HTMLObject *o,
                                   HTMLPainter *painter,
                                   gint offset)
{
	return (* HO_CLASS (o)->get_right_edge_offset) (o, painter, offset);
}

gint
html_object_get_left_edge_offset (HTMLObject *o,
                                  HTMLPainter *painter,
                                  gint offset)
{
	return (* HO_CLASS (o)->get_left_edge_offset) (o, painter, offset);
}
