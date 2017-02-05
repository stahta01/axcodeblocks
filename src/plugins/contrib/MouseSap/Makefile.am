AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		$(WX_GTK2_CFLAGS) \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libMouseSap.la

libMouseSap_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libMouseSap_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS) $(WX_GTK2_LIBS)

libMouseSap_la_SOURCES = MouseSap.cpp

noinst_HEADERS = MouseSap.h

EXTRA_DIST = $(srcdir)/*.cbp \
		manifest.xml

pkgdata_DATA = MouseSap.zip

CLEANFILES = $(pkgdata_DATA)

MouseSap.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/MouseSap.zip manifest.xml > /dev/null
