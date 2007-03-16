/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
/* dllmain.c: DLL entry point for libgtkhtml on Win32
 * Copyright (C) 2005 Novell, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <glib.h>

#include <libgnome/gnome-init.h>

/* localedir uses system codepage as it is passed to the non-UTF8ified
 * gettext library
 */
static const char *localedir = NULL;

/* The others are in UTF-8 */
static char *prefix;
static const char *libdir;
static const char *datadir;
static const char *sysconfdir;
static const char *icondir;
static const char *gtkhtml_datadir;
static const char *glade_datadir;

static gpointer hmodule;
G_LOCK_DEFINE_STATIC (mutex);

static char *
replace_prefix (const char *runtime_prefix,
                const char *configure_time_path)
{
        if (runtime_prefix &&
            strncmp (configure_time_path, PREFIX "/",
                     strlen (PREFIX) + 1) == 0)
                return g_strconcat (runtime_prefix,
                                    configure_time_path + strlen (PREFIX),
                                    NULL);
        else
                return g_strdup (configure_time_path);
}

static void
setup (void)
{
	char *cp_prefix; 

        G_LOCK (mutex);
        if (localedir != NULL) {
                G_UNLOCK (mutex);
                return;
        }

        gnome_win32_get_prefixes (hmodule, &prefix, &cp_prefix);

        localedir = replace_prefix (cp_prefix, GNOMELOCALEDIR);
        g_free (cp_prefix);

        libdir = replace_prefix (prefix, LIBDIR);
        datadir = replace_prefix (prefix, DATADIR);
        sysconfdir = replace_prefix (prefix, SYSCONFDIR);
	icondir = replace_prefix (prefix, ICONDIR);
	gtkhtml_datadir = replace_prefix (prefix, GTKHTML_DATADIR);
	glade_datadir = replace_prefix (prefix, GLADE_DATADIR);

        G_UNLOCK (mutex);
}

/* <windows.h> drags in a definition of DATADIR, argh. So #undef
 * DATADIR and include <windows.h> only here.
 */
#undef DATADIR
#include <windows.h>

/* Silence gcc with a prototype */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

/* DllMain used to tuck away the the DLL's HMODULE */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
        switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
                hmodule = hinstDLL;
                break;
        }
        return TRUE;
}

/* Include gtkhtml-private.h now to get prototypes for the getter
 * functions, to silence gcc. Can't include it earlier as we need the
 * definitions of the *DIR macros from the Makefile above, and
 * gtkhtml-private overrides them.
 */
#include "gtkhtml-private.h"

#define GETTER(varbl)				\
const char *					\
_get_##varbl (void)				\
{						\
        setup ();				\
        return varbl;				\
}

GETTER (prefix)
GETTER (localedir)
GETTER (libdir)
GETTER (datadir)
GETTER (sysconfdir)
GETTER (icondir)
GETTER (gtkhtml_datadir)
GETTER (glade_datadir)
