AM_CPPFLAGS = $(WX_CXXFLAGS) \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libProjectOptionsManipulator.la

libProjectOptionsManipulator_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libProjectOptionsManipulator_la_LIBADD = ../../../sdk/libaxcodeblocks.la

libProjectOptionsManipulator_la_SOURCES = ProjectOptionsManipulator.cpp \
		ProjectOptionsManipulatorDlg.cpp \
		ProjectOptionsManipulatorResultDlg.cpp

noinst_HEADERS = ProjectOptionsManipulator.h \
		ProjectOptionsManipulatorDlg.h \
		ProjectOptionsManipulatorResultDlg.h

EXTRA_DIST =	$(srcdir)/*.cbp \
		manifest.xml \
		$(srcdir)/wxsmith/*.wxs

pkgdata_DATA = ProjectOptionsManipulator.zip

CLEANFILES = $(pkgdata_DATA)

ProjectOptionsManipulator.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/ProjectOptionsManipulator.zip manifest.xml > /dev/null
