SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libValgrind.la

libValgrind_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libValgrind_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libValgrind_la_SOURCES = Valgrind.cpp \
			ValgrindListLog.cpp \
			valgrind_config.cpp

noinst_HEADERS = Valgrind.h \
			ValgrindListLog.h \
			valgrind_config.h

EXTRA_DIST = $(srcdir)/*.cbp \
		$(srcdir)/update* \
		wxsmith/valgrind_config_panel.wxs
