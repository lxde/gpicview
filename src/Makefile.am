## Process this file with automake to produce Makefile.in

# set the include path found by configure
INCLUDES = $(all_includes) \
	@PACKAGE_CFLAGS@ \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

bin_PROGRAMS = gpicview
gpicview_SOURCES = \
	gpicview.cpp \
	imagelist.cpp \
	mainwin.cpp \
	working-area.c \
	ptk-menu.c

# the library search path.
# use -nodefaultlibs to prevent linking libstdc++
gpicview_LDFLAGS = $(all_libraries) -nodefaultlibs -lc
gpicview_LDADD = @PACKAGE_LIBS@
gpicview_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_HEADERS = \
	imagelist.h \
	mainwin.h \
	working-area.h

