/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library
   Copyright (C) 1999 Helix Code, Inc.
   
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
*/

#ifndef _HTMLDRAWQUEUE_H
#define _HTMLDRAWQUEUE_H

#include <glib.h>

typedef struct _HTMLDrawQueue HTMLDrawQueue;

#include "htmlengine.h"
#include "htmlobject.h"

struct _HTMLDrawQueue {
	/* The associated engine.  */
	HTMLEngine *engine;

	/* Elements to be drawn.  */
	GList *elems;

	/* Pointer to the last element in the list, for faster appending.  */
	GList *last;
};


HTMLDrawQueue *html_draw_queue_new (HTMLEngine *engine);
void html_draw_queue_destroy (HTMLDrawQueue *queue);
void html_draw_queue_add (HTMLDrawQueue *queue, HTMLObject *object);
void html_draw_queue_flush (HTMLDrawQueue *queue);

#endif /* _HTMLDRAWQUEUE_H */
