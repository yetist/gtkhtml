ACLOCAL_AMFLAGS=-I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

SUBDIRS =					\
	a11y					\
	gtkhtml					\
	components				\
	art					\
	icons					\
	po					\
	docs

CLEANFILES =					\
	po/.intltool-merge-cache

MAINTAINERCLEANFILES =				\
	$(srcdir)/INSTALL			\
	$(srcdir)/aclocal.m4			\
	$(srcdir)/autoscan.log			\
	$(srcdir)/compile			\
	$(srcdir)/config.guess			\
	$(srcdir)/config.h.in			\
	$(srcdir)/config.sub			\
	$(srcdir)/configure.scan		\
	$(srcdir)/depcomp			\
	$(srcdir)/install-sh			\
	$(srcdir)/ltmain.sh			\
	$(srcdir)/missing			\
	$(srcdir)/mkinstalldirs			\
	$(srcdir)/m4/intltool.m4		\
	$(srcdir)/m4/libtool.m4			\
	$(srcdir)/m4/ltoptions.m4		\
	$(srcdir)/m4/ltsugar.m4			\
	$(srcdir)/m4/ltversion.m4		\
	$(srcdir)/m4/lt~obsolete.m4

EXTRA_DIST =					\
	COPYING.LIB				\
	BUGS					\
	intltool-extract.in			\
	intltool-update.in			\
	intltool-merge.in

.PHONY: ChangeLog

CHANGELOG_START = tags/GTKHTML_4_10_0

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
          (./missing --run git log $(CHANGELOG_START)^^.. --stat $(srcdir) ) | fmt --split-only > $@.tmp \
          && mv -f $@.tmp $@ \
          || ($(RM) $@.tmp; \
              echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
              (test -f $@ || echo git-log is required to generate this file >> $@)); \
        else \
          test -f $@ || \
          (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
          echo A git checkout and git-log is required to generate this file >> $@); \
        fi

dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
	if test -f $(srcdir)/INSTALL.in && test -f $(srcdir)/README.md ; then \
		CONFIG_FILES="INSTALL:$(srcdir)/INSTALL.in README:$(srcdir)/README.md" \
		CONFIG_HEADERS= \
		$(SHELL) config.status \
		&& cp INSTALL README $(distdir) ; \
	fi

distclean-local:
	if test "$(srcdir)" = "."; then :; else \
                rm -f ChangeLog; \
        fi


-include $(top_srcdir)/git.mk
