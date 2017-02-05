SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
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

pluginlib_LTLIBRARIES = libtodo.la

libtodo_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libtodo_la_LIBADD = ../../sdk/libaxcodeblocks.la $(WX_LIBS)

libtodo_la_SOURCES = addtododlg.cpp \
			asktypedlg.cpp \
			todolist.cpp \
			todolistview.cpp \
			todosettingsdlg.cpp

noinst_HEADERS = addtododlg.h \
			asktypedlg.h \
			todolist.h \
			todolistview.h \
			todosettingsdlg.h
