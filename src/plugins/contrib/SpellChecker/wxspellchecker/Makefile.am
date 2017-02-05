AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL_SPELLCHECK -DwxPDF_USE_WXMODULE=0 \
		$(CB_HUNSPELL_CFLAGS) \
		-I$(srcdir)/include \
		-I$(srcdir)/.. \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

noinst_LTLIBRARIES = libwxSpellChecker.la

libwxSpellChecker_la_LDFLAGS = -version-info 0:1:0 -static -no-undefined -avoid-version $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@ @WIN32_GDI@

libwxSpellChecker_la_LIBADD = $(WX_LIBS) \
                            $(CB_HUNSPELL_LIBS)

libwxSpellChecker_la_SOURCES = ../HunspellInterface.cpp \
                                src/PersonalDictionary.cpp \
                                src/SpellCheckCmdLineInterface.cpp \
                                src/SpellCheckDialogInterface.cpp \
                                src/SpellCheckEngineInterface.cpp \
                                src/SpellCheckEngineOption.cpp \
                                src/SpellCheckUserInterface.cpp \
                                src/SpellCheckerOptionsDialog.cpp \
                                src/XmlPersonalDictionaryDialog.cpp \
                                src/XmlSpellCheckDialog.cpp \
                                src/exports.cpp

noinst_HEADERS = ../HunspellInterface.h \
                include/PersonalDictionary.h \
                include/SpellCheckEngineInterface.h \
                include/SpellCheckerOptionsDialog.h \
                include/exports.h \
                include/SpellCheckCmdLineInterface.h \
                include/SpellCheckEngineOption.h \
                include/XmlPersonalDictionaryDialog.h \
                include/SpellCheckDialogInterface.h \
                include/SpellCheckUserInterface.h \
                include/XmlSpellCheckDialog.h

EXTRA_DIST = 	src/open.xpm
