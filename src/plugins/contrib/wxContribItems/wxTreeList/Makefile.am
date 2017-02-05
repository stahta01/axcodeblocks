AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL \
		-I$(srcdir)/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

wxcontribitemsdir=$(libdir)/axcodeblocks/wxContribItems

wxcontribitems_LTLIBRARIES = libwxtreelist.la

libwxtreelist_la_LDFLAGS = -version-info 0:1:0 -shared -no-undefined

libwxtreelist_la_LIBADD = $(WX_LIBS)

libwxtreelist_la_SOURCES = \
./src/treelistctrl.cpp

sdkdir=$(includedir)/axcodeblocks/wxContribItems/wxTreeList/include/wx

sdk_HEADERS = \
./include/wx/treelistctrl.h
