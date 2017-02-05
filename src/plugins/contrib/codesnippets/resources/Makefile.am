AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		$(WX_GTK2_CFLAGS) \
		$(WX_X11_CFLAGS)

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

##pluginlib_LTLIBRARIES = codesnippets
##bin_PROGRAMS = codesnippets

##codesnippets_LIBADD = $(WX_LIBS) ../../../../sdk/libaxcodeblocks.la
##codesnippets_LDADD = $(WX_LIBS) ../../../../sdk/libaxcodeblocks.la
##codesnippets_LDADD = -L../../../../base/tinyxml -ltinyxml \
##			-L../../../../sdk ../../../../sdk/libaxcodeblocks.la \
##			$(WX_LIBS) $(WX_GTK2_LIBS) $(WX_X11_LIBS)

pkgdata_DATA = codesnippets.zip
CLEANFILES = $(pkgdata_DATA)

imagedir = $(pkgdatadir)/images/codesnippets

dist_image_DATA =  allsnippets.png \
		category.png \
		snippet.png \
		findf.png \
		findfdisabled.png \
		options.png \
		optionsdisabled.png \
		stop.png \
		stopdisabled.png

EXTRA_DIST = manifest.xml

codesnippets.zip: $(EXTRA_DIST)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/codesnippets.zip manifest.xml > /dev/null

