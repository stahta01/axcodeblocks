AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins
pkgdata_DATA = RegExTestbed.zip

pluginlib_LTLIBRARIES = libRegExTestbed.la

libRegExTestbed_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libRegExTestbed_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libRegExTestbed_la_SOURCES = regexdlg.cpp RegExTestbed.cpp

noinst_HEADERS = regexdlg.h RegExTestbed.h

EXTRA_DIST = $(srcdir)/*.cbp \
	$(srcdir)/regexdlg.xrc \
	$(srcdir)/manifest.xml \
	$(srcdir)/wxsmith/RegExDlg.wxs

CLEANFILES = $(pkgdata_DATA)

RegExTestbed.zip: manifest.xml regexdlg.xrc
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/RegExTestbed.zip manifest.xml regexdlg.xrc > /dev/null
