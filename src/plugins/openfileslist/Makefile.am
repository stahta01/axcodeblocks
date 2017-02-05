AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libopenfileslist.la

libopenfileslist_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libopenfileslist_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libopenfileslist_la_SOURCES = openfileslistplugin.cpp
noinst_HEADERS = openfileslistplugin.h

pkgdata_DATA = openfileslist.zip
CLEANFILES = $(pkgdata_DATA)

EXTRA_DIST = manifest.xml

openfileslist.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/openfileslist.zip manifest.xml > /dev/null

