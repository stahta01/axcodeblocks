noinst_PROGRAMS = auto_revision
auto_revision_SOURCES = autorevision.cpp

AM_CPPFLAGS = -I$(top_srcdir)/src/include

auto_revision_LDADD = ../../base/tinyxml/libtinyxml.la

auto_revision_CXXFLAGS=-DTIXML_USE_STL

if CROSSCOMP

all-local:

else

all-local: auto_revision$(EXEEXT)
	./auto_revision$(EXEEXT) +int +wx +t +git $(top_srcdir) $(top_builddir)/src/include/autorevision.h

endif
