PIXMAPS_FILES = \
	pixmaps/object-rotate-right.png \
	pixmaps/object-rotate-left.png

gpicview_DATA = 
gpicviewdir = $(datadir)/gpicview

pixmaps_DATA = $(PIXMAPS_FILES)
pixmapsdir = $(gpicviewdir)/pixmaps

ui_in_files = \
	pref-dlg.glade
ui_DATA = $(ui_in_files:.glade=.ui)
uidir=$(datadir)/gpicview/ui

EXTRA_DIST = \
	$(gpicview_DATA) \
	$(pixmaps_DATA) \
	$(uifile_DATA)

%.ui: %.glade
	cp $< $@
	$(top_builddir)/src/xml-purge $@

