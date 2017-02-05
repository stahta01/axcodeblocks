SUBDIRS = 	bzip2

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libdevpakupdater.la

libdevpakupdater_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libdevpakupdater_la_LIBADD =  $(PLUGIN_WX_LIBS) ../../../sdk/libaxcodeblocks.la -lbz2

libdevpakupdater_la_SOURCES = cbiniparser.cpp \
		cbnetwork.cpp \
		conf.cpp \
		crc32.cpp \
		devpakinstaller.cpp \
		devpakupdater.cpp \
		mytar.cpp \
		updatedlg.cpp \
		DevPakPlugin.cbp
noinst_HEADERS = cbiniparser.h \
		cbnetwork.h \
		conf.h \
		crc32.h \
		devpakinstaller.h \
		devpakupdater.h \
		mytar.h \
		updatedlg.h \
		$(srcdir)/*.cbp \
		manifest.xml \
		update.xrc
EXTRA_DIST = update.xrc manifest.xml
pkgdata_DATA = devpakupdater.zip
CLEANFILES = $(pkgdata_DATA)

devpakupdater.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/devpakupdater.zip manifest.xml *.xrc > /dev/null
