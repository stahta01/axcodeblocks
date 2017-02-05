bin_PROGRAMS = axcb_share_config

AM_CPPFLAGS = $(WX_CXXFLAGS) \
			$(CB_GLIB2_CFLAGS) \
			-I$(top_srcdir)/src/include \
			-I$(top_srcdir)/src/include/tinyxml

axcb_share_config_LDFLAGS =

axcb_share_config_LDADD =  ../../base/tinyxml/libtinyxml.la \
			$(WX_LIBS) \
			$(CB_GLIB2_LIBS)

axcb_share_config_SOURCES = app.cpp \
			mainframe.cpp

noinst_HEADERS = app.h \
			mainframe.h

man_MANS = axcb_share_config.1

EXTRA_DIST = $(srcdir)/*.cbp \
		$(srcdir)/wxsmith/*.wxs \
		$(man_MANS)
