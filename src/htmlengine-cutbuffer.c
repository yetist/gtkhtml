/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This file is part of the GtkHTML library

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
*/

/* The cut buffer is used to temporarily store parts of the documents.  The
   implementation is a quick & dirty hack, but should work well for our
   purposes.

   So basically, we just store all the leaf elements of the document in a list.
   We simply use HTMLClueFlows as end-of-paragraphs marker, and consequently
   they also carry information about the style and indentation of the paragraph
   they belong to.  This is used to emulate the common word processor behavior
   of associating the paragraph properties to a new line.  */


#include <config.h>
#include "htmlclueflow.h"
#include "htmlobject.h"
#include "htmltext.h"

#include "htmlengine-cutbuffer.h"


void

