SUBDIRS = wxPdfDocument

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(top_srcdir)/src/include \
		$(CB_FONTCONFIG_CFLAGS) \
		-I$(top_srcdir)/src/sdk/wxscintilla/include \
		-I$(top_srcdir)/src/plugins/contrib/source_exporter/wxPdfDocument/include \
		-include "sdk.h" \
		-DwxPDF_USE_WXMODULE=0

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) $(CB_FONTCONFIG_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libexporter.la

libexporter_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

libexporter_la_LIBADD = ../../../sdk/libaxcodeblocks.la \
				wxPdfDocument/libwxPdfDocument.la \
				$(WX_LIBS) $(CB_FONTCONFIG_LIBS)

libexporter_la_SOURCES = BaseExporter.cpp \
				exporter.cpp \
				HTMLExporter.cpp \
				ODTExporter.cpp \
				PDFExporter.cpp \
				RTFExporter.cpp

noinst_HEADERS = BaseExporter.h \
				exporter.h \
				HTMLExporter.h \
				ODTExporter.h \
				PDFExporter.h \
				RTFExporter.h

EXTRA_DIST = $(srcdir)/*.cbp manifest.xml

pkgdata_DATA = exporter.zip
CLEANFILES = $(pkgdata_DATA)

exporter.zip: manifest.xml
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/exporter.zip manifest.xml > /dev/null
