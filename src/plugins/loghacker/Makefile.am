AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libloghacker.la

libloghacker_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libloghacker_la_LIBADD =  $(PLUGIN_WX_LIBS) ../../sdk/libaxcodeblocks.la

libloghacker_la_SOURCES = loghacker.cpp
noinst_HEADERS = loghacker.h
EXTRA_DIST = manifest.xml
pkgdata_DATA = loghacker.zip
CLEANFILES = $(pkgdata_DATA)

loghacker.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/loghacker.zip manifest.xml > /dev/null
