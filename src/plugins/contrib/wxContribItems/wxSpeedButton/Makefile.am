AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(srcdir)/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

wxcontribitemsdir=$(libdir)/axcodeblocks/wxContribItems

wxcontribitems_LTLIBRARIES = libwxspeedbutton.la

libwxspeedbutton_la_LDFLAGS = -version-info 0:1:0 -shared -no-undefined

libwxspeedbutton_la_LIBADD = $(WX_LIBS)

libwxspeedbutton_la_SOURCES = \
./wxSpeedButton.cpp

sdkdir=$(includedir)/axcodeblocks/wxContribItems/wxSpeedButton

sdk_HEADERS = \
./wxSpeedButton.h

