DIST_SUBDIRS = bzip2 zlib
SUBDIRS =
if ! HAVE_BZIP2
SUBDIRS += bzip2
endif
if ! HAVE_ZLIB
SUBDIRS += zlib
endif

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		-I$(top_srcdir)/src/include/scripting/include \
		-I$(top_srcdir)/src/include/scripting/bindings \
		-I$(top_srcdir)/src/include/scripting/sqplus

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libhelp_plugin.la

libhelp_plugin_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libhelp_plugin_la_LIBADD = ../../../sdk/libaxcodeblocks.la \
					$(WX_LIBS)


if ! HAVE_BZIP2
libhelp_plugin_la_LIBADD += bzip2/libbzip2.la
AM_CPPFLAGS += -I$(srcdir)/bzip2
else
libhelp_plugin_la_LIBADD += -lbz2
endif
if ! HAVE_ZLIB
libhelp_plugin_la_LIBADD += zlib/libz.la
AM_CPPFLAGS += -I$(srcdir)/zlib
else
libhelp_plugin_la_LIBADD += -lz
endif

libhelp_plugin_la_SOURCES = help_common.cpp \
			HelpConfigDialog.cpp \
			help_plugin.cpp \
			man2html.cpp \
			MANFrame.cpp

noinst_HEADERS = help_common.h \
			HelpConfigDialog.h \
			help_plugin.h \
			man2html.h \
			defs.h \
			MANFrame.h

EXTRA_DIST = $(srcdir)/*.cbp \
			help_plugin.xrc \
			zoomin.png \
			zoomout.png \
			manifest.xml \
			Readme.txt

pkgdata_DATA = help_plugin.zip
CLEANFILES = $(pkgdata_DATA)

help_plugin.zip: manifest.xml help_plugin.xrc
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/help_plugin.zip manifest.xml *.xrc *.png > /dev/null
