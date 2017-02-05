SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL_PROPGRID -DwxPG_USE_WXMODULE=0 \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		-I$(top_srcdir)/src/include/mozilla_chardet \
		-I$(top_srcdir)/src/include/mozilla_chardet/mfbt \
		-I$(top_srcdir)/src/include/mozilla_chardet/nsprpub/pr/include \
		-I$(top_srcdir)/src/include/mozilla_chardet/xpcom \
		-I$(top_srcdir)/src/include/mozilla_chardet/xpcom/base \
		-I$(top_srcdir)/src/include/mozilla_chardet/xpcom/glue

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libprojectsimporter.la

libprojectsimporter_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libprojectsimporter_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libprojectsimporter_la_SOURCES = projectsimporter.cpp \
				devcpploader.cpp \
				msvc10loader.cpp \
				msvc7loader.cpp \
				msvc7workspaceloader.cpp \
				msvcloader.cpp \
				msvcworkspacebase.cpp \
				msvcworkspaceloader.cpp

noinst_HEADERS = projectsimporter.h \
				devcpploader.h \
				msvc10loader.h \
				msvc7loader.h \
				msvc7workspaceloader.h \
				msvcloader.h \
				msvcworkspacebase.h \
				msvcworkspaceloader.h
