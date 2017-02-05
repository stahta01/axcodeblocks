AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libcopystrings.la

libcopystrings_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libcopystrings_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libcopystrings_la_SOURCES = copystrings.cpp

noinst_HEADERS = copystrings.h

EXTRA_DIST = 	$(srcdir)/*.cbp \
		manifest.xml

pkgdata_DATA = copystrings.zip

CLEANFILES = $(pkgdata_DATA)

copystrings.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/copystrings.zip manifest.xml > /dev/null
