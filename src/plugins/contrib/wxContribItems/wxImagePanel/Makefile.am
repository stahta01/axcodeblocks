AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(srcdir)/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

wxcontribitemsdir=$(libdir)/axcodeblocks/wxContribItems

wxcontribitems_LTLIBRARIES = libwximagepanel.la

libwximagepanel_la_LDFLAGS = -version-info 0:1:0 -shared -no-undefined

libwximagepanel_la_LIBADD = $(WX_LIBS)

libwximagepanel_la_SOURCES = \
./src/wxImagePanel.cpp

sdkdir=$(includedir)/axcodeblocks/wxContribItems/wxImagePanel/include/wx

sdk_HEADERS = \
./include/wx/wxImagePanel.h

