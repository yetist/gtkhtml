/*  This file is part of the GtkHTML library.
 *
 *  Copyright 2002 Ximian, Inc.
 *  Copyright 2025 Xiaotian Wu <yetist@gmail.com>
 *
 *  Author: Radek Doulik
 *
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

#ifndef __HTML_A11Y_HYPER_LINK_H__
#define __HTML_A11Y_HYPER_LINK_H__

#include "text.h"

G_BEGIN_DECLS

#define G_TYPE_HTML_A11Y_HYPER_LINK            (html_a11y_hyper_link_get_type ())
G_DECLARE_FINAL_TYPE (HTMLA11YHyperLink, html_a11y_hyper_link, HTML, A11Y_HYPER_LINK, AtkHyperlink)

AtkHyperlink * html_a11y_hyper_link_new (HTMLA11Y *a11y, gint link_index);
G_END_DECLS

#endif
