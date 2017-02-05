AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libautosave.la

libautosave_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libautosave_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libautosave_la_SOURCES = autosave.cpp
noinst_HEADERS = autosave.h
EXTRA_DIST = autosave.xrc manifest.xml
pkgdata_DATA = autosave.zip
CLEANFILES = $(pkgdata_DATA)

autosave.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/autosave.zip manifest.xml *.xrc > /dev/null
