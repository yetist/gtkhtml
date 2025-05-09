## Licensing

The library is released under the LGPL

## What is GtkHTML?

GtkHTML is a lightweight HTML rendering/printing/editing engine.  It
was originally based on KHTMLW, part of the KDE project, but is now
being developed independently.

NOTICE: While GtkHTML is still being maintained, it is no longer being
        actively developed and is not recommended for new applications.
        Consider WebKit/GTK+ or Gecko instead.

## Requirements

In order to compile GtkHTML, you need:

```
gail >= 3.2
GTK+ >= 3.2
enchant >= 2.2.0
iso-codes >= 0.49
```

If you want to try the test program `testgtkhtml`, you also need
libsoup-2.4 or later.

To try it out, run testgtkhtml in the source directory, i.e:

```
cd gtkhtml
./testgtkhtml
```

WARNING: testgtkhtml's URL fetching code is very buggy.

You can also test GtkHTML's simple HTML editor widget:

```
cd components/editor
./gtkhtml-editor-test
```

The editor widget is used in Evolution's mail composer.

## Package status

Binary packages for the following systems are currently available.

[![Packaging status](https://repology.org/badge/vertical-allrepos/gtkhtml.svg)](https://repology.org/project/gtkhtml/versions)
