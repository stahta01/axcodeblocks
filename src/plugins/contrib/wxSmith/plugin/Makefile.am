AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL_PROPGRID -DwxPG_USE_WXMODULE=0 \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		$(WXPROPGRID_INCLUDE)

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlib_LTLIBRARIES = libwxsmith.la

pluginlibdir = $(pkglibdir)/plugins

libwxsmith_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

libwxsmith_la_LIBADD = ../../../../sdk/libaxcodeblocks.la ../libwxsmithlib.la $(WX_LIBS)

libwxsmith_la_SOURCES = wxsmithpluginregistrants.cpp

