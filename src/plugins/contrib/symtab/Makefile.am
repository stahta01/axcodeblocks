SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libSymTab.la

libSymTab_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libSymTab_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libSymTab_la_SOURCES = symtabconfig.cpp \
			symtab.cpp \
			symtabexec.cpp

noinst_HEADERS = symtabconfig.h \
			symtabexec.h \
			symtab.h

EXTRA_DIST = $(srcdir)/*.cbp
