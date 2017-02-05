SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		-I$(top_srcdir)/src/include/scripting/include \
		-I$(top_srcdir)/src/include/scripting/sqplus

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libscriptedwizard.la

libscriptedwizard_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libscriptedwizard_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libscriptedwizard_la_SOURCES = buildtargetpanel.cpp \
								compilerpanel.cpp \
								filepathpanel.cpp \
								genericselectpath.cpp \
								infopanel.cpp \
								genericsinglechoicelist.cpp \
								projectpathpanel.cpp \
								wiz.cpp \
								wizpage.cpp

noinst_HEADERS = buildtargetpanel.h \
					compilerpanel.h \
					filepathpanel.h \
					genericselectpath.h \
					infopanel.h \
					genericsinglechoicelist.h \
					projectpathpanel.h \
					wiz.h \
					wizpage.h
