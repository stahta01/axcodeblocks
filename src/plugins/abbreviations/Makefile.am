SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		-I$(top_srcdir)/src/include/scripting/include \
		-I$(top_srcdir)/src/include/scripting/sqplus \
		-I$(top_srcdir)/src/include/scripting/bindings


AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libabbreviations.la

libabbreviations_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libabbreviations_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libabbreviations_la_SOURCES = abbreviations.cpp \
			abbreviationsconfigpanel.cpp

noinst_HEADERS = abbreviations.h \
			abbreviationsconfigpanel.h
