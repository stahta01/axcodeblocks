AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libcb_koders.la

libcb_koders_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libcb_koders_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libcb_koders_la_SOURCES = kodersdialog.cpp \
			cb_koders.cpp

noinst_HEADERS = kodersdialog.h \
			cb_koders.h

EXTRA_DIST = $(srcdir)/*.cbp \
		wxsmith/KodersDialog.wxs \
			manifest.xml

pkgdata_DATA = cb_koders.zip
CLEANFILES = $(pkgdata_DATA)

cb_koders.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/cb_koders.zip manifest.xml > /dev/null
