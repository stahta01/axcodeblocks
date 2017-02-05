AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libBrowseTracker.la

libBrowseTracker_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libBrowseTracker_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libBrowseTracker_la_SOURCES = BrowseTracker.cpp \
			BrowseSelector.cpp \
			Version.cpp \
			BrowseMarks.cpp \
			BrowseTrackerConfPanel.cpp \
			BrowseTrackerLayout.cpp \
			ConfigPanel.cpp \
			ProjectData.cpp \
			JumpTracker.cpp \
			JumpData.cpp

noinst_HEADERS =  	BrowseTracker.h \
			BrowseSelector.h \
			Version.h \
			wx_pch.h \
			BrowseMarks.h \
			BrowseXpms.h \
			BrowseTrackerDefs.h \
			BrowseTrackerConfPanel.h \
			BrowseTrackerLayout.h \
			ConfigPanel.h \
			ProjectData.h \
			JumpTracker.h \
			JumpData.h

EXTRA_DIST = 	$(srcdir)/*.cbp \
			BrowseTrackerToolbar.xrc \
			manifest.xml \
			$(srcdir)/images/*.png \
			$(srcdir)/images/16x16/*.png

pkgdata_DATA = BrowseTracker.zip
CLEANFILES = $(pkgdata_DATA)

BrowseTracker.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip -R $(PWD)/BrowseTracker.zip manifest.xml *.xrc images/*.png images/16x16/*.png > /dev/null
