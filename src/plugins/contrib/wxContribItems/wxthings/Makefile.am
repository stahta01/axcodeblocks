AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL_THINGS \
		-I$(srcdir)/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

wxcontribitemsdir=$(libdir)/axcodeblocks/wxContribItems

wxcontribitems_LTLIBRARIES = libwxcustombutton.la

libwxcustombutton_la_LDFLAGS = -version-info 0:1:0 -shared -no-undefined

libwxcustombutton_la_LIBADD = $(WX_LIBS)

libwxcustombutton_la_SOURCES = \
./src/toggle.cpp

sdkdir=$(includedir)/axcodeblocks/wxContribItems/wxthings/include/wx/things

sdk_HEADERS = \
./include/wx/things/toggle.h \
./include/wx/things/thingdef.h

EXTRA_DIST = ./src/precomp.cpp ./src/precomp.h
