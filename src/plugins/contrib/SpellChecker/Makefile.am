SUBDIRS = wxspellchecker

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN -DWXMAKINGDLL_SPELLCHECKER -DWXUSINGDLL \
		$(CB_HUNSPELL_CFLAGS) \
		-I$(srcdir)/wxspellchecker/include \
		-I$(top_srcdir)/src/include \
		-I$(top_srcdir)/src/sdk/wxscintilla/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libSpellChecker.la

libSpellChecker_la_LDFLAGS = -module -version-info 0:1:0 -shared -no-undefined -avoid-version @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

libSpellChecker_la_LIBADD = ../../../sdk/libaxcodeblocks.la \
							wxspellchecker/libwxSpellChecker.la \
							$(WX_LIBS) \
							$(CB_HUNSPELL_LIBS)

libSpellChecker_la_SOURCES = DictionariesNeededDialog.cpp \
                            MySpellingDialog.cpp \
                            OnlineSpellChecker.cpp \
                            SpellCheckHelper.cpp \
                            SpellCheckSettingsPanel.cpp \
                            SpellCheckerConfig.cpp \
                            SpellCheckerPlugin.cpp \
                            StatusField.cpp \
                            Thesaurus.cpp \
                            ThesaurusDialog.cpp \
                            mythes.cxx \
                            wxThes.cpp

noinst_HEADERS = DictionariesNeededDialog.h \
                MySpellingDialog.h \
                SpellCheckHelper.h \
                SpellCheckerConfig.h \
                StatusField.h \
                ThesaurusDialog.h \
                OnlineSpellChecker.h \
                SpellCheckSettingsPanel.h \
                SpellCheckerPlugin.h \
                Thesaurus.h \
                mythes.hxx \
                wxThes.h

EXTRA_DIST = 	$(srcdir)/*.cbp \
		$(srcdir)/*.xrc \
		$(srcdir)/*.png \
		wxsmith \
		hunspell \
		dictionaries \
		manifest.xml \
		$(srcdir)/update*

pkgdata_DATA = SpellChecker.zip

CLEANFILES = $(pkgdata_DATA)

SpellChecker.zip: manifest.xml SpellCheckSettingsPanel.xrc
	PWD=`pwd` cd $(srcdir) && zip -R $(PWD)/SpellChecker.zip manifest.xml SpellCheckSettingsPanel.xrc > /dev/null

confimagesdir = $(datadir)/@PACKAGE@/images/settings

dist_confimages_DATA = $(srcdir)/*.png

onlinecheckerconfigdir = $(datadir)/@PACKAGE@/SpellChecker

dist_onlinecheckerconfig_DATA = $(srcdir)/rc/*.png $(srcdir)/OnlineSpellChecking.xml
