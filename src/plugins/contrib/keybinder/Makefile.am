AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libkeybinder.la

libkeybinder_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libkeybinder_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libkeybinder_la_SOURCES = cbkeybinder.cpp \
			keybinder.cpp \
			menuutils.cpp

noinst_HEADERS = cbkeybinder.h \
			debugging.h \
			keybinder.h \
			menuutils.h

EXTRA_DIST = $(srcdir)/*.cbp manifest.xml


pkgdata_DATA = keybinder.zip
CLEANFILES = $(pkgdata_DATA)

keybinder.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/keybinder.zip manifest.xml > /dev/null
