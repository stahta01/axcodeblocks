AM_CPPFLAGS = $(WX_CPPFLAGS) -DWXMAKINGLIB_PROPGRID -DwxPG_USE_WXMODULE=0 -DwxPG_SUPPORT_TOOLTIPS -DEXPORT_LIB \
		-I$(top_srcdir)/src/sdk/wxpropgrid/include
		-DEXPORT_LIB \
		-DwxPG_SUPPORT_TOOLTIPS

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS)

if WIN32
lib_LTLIBRARIES = libwxpropgrid.la
win32_dlls_ldflags = -Wl,--export-all-symbols
else
noinst_LTLIBRARIES = libwxpropgrid.la
win32_dlls_ldflags =
endif

libwxpropgrid_la_LDFLAGS = $(win32_dlls_ldflags) -no-undefined

libwxpropgrid_la_LIBADD = $(WX_LIBS)
libwxpropgrid_la_SOURCES = ./src/advprops.cpp \
			./src/editors.cpp \
			./src/extras.cpp \
			./src/manager.cpp \
			./src/odcombo.cpp \
			./src/propgrid.cpp \
 			./src/props.cpp \
 			./src/xh_propgrid.cpp

EXTRA_DIST = ./readme.txt \
			./changes.txt

sdkdir=$(includedir)/axcodeblocks/wxpropgrid/include/wx/propgrid/

sdk_HEADERS = ./include/wx/propgrid/advprops.h \
			./include/wx/propgrid/editors.h \
			./include/wx/propgrid/extras.h \
			./include/wx/propgrid/manager.h \
			./include/wx/propgrid/odcombo.h \
			./include/wx/propgrid/pg_dox_mainpage.h \
			./include/wx/propgrid/propdev.h \
			./include/wx/propgrid/propgrid.h \
			./include/wx/propgrid/props.h \
			./include/wx/propgrid/xh_propgrid.h
