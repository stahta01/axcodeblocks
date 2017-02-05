AM_CPPFLAGS = $(WX_CPPFLAGS) -DEXPORT_LIB -DEXPORT_EVENTS -DWXMAKINGDLL_SCI \
				-I$(top_srcdir)/src/sdk/wxscintilla/include \
				-I$(top_srcdir)/src/include \
				-I$(top_srcdir)/src/include/tinyxml

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS)

noinst_LTLIBRARIES = libtinyxml.la

libtinyxml_la_SOURCES = tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp tinywxuni.cpp

EXTRA_DIST = tinystr.cpp
