NULL =

# These icons were in gnome-icon-theme prior to GNOME 2.30.
# XXX Need to reevaluate which icons we really need and drop the rest.
#     Many are not Tango-compliant and look dated and out of place.
stock_private_icons = \
	hicolor_actions_16x16_stock_insert-rule.png \
	hicolor_actions_16x16_stock_insert-table.png \
	hicolor_actions_16x16_stock_text-monospaced.png \
	hicolor_actions_24x24_stock_insert-rule.png \
	hicolor_actions_24x24_stock_insert-table.png \
	hicolor_actions_24x24_stock_text-monospaced.png \
	$(NULL)

# These are source files for future editing
# and are not meant to be installed as icons.
noinst_DATA = \
	hicolor_actions_scalable_stock_insert-rule-symbolic.svg \
	hicolor_actions_scalable_stock_insert-table-symbolic.svg \
	hicolor_actions_scalable_stock_text-monospaced.svg \
	$(NULL)

EXTRA_DIST = \
	$(stock_private_icons)	\
	$(noinst_DATA)		\
	$(NULL)

-include $(top_srcdir)/git.mk
