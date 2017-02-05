AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/include/scripting/include \
		-I$(top_srcdir)/src/include/scripting/sqplus \
		-I$(top_srcdir)/src/include/scripting/bindings \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins
pkgdata_DATA = envvars.zip

pluginlib_LTLIBRARIES = libenvvars.la

libenvvars_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libenvvars_la_LIBADD = ../../../sdk/libaxcodeblocks.la $(WX_LIBS)

libenvvars_la_SOURCES = envvars.cpp \
                        envvars_cfgdlg.cpp \
                        envvars_common.cpp \
                        envvars_prjoptdlg.cpp

noinst_HEADERS = envvars.h \
                 envvars_cfgdlg.h \
                 envvars_common.h \
                 envvars_prjoptdlg.h

EXTRA_DIST = $(srcdir)/*.cbp \
             envvars.xrc \
             project_settings.xrc \
             manifest.xml

CLEANFILES = $(pkgdata_DATA)

envvars.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/envvars.zip manifest.xml *.xrc > /dev/null
