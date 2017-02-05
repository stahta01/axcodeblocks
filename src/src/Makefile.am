SUBDIRS = resources

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
			-I$(top_srcdir)/src/sdk/wxscintilla/include \
			$(WXPROPGRID_INCLUDE) \
			-I$(top_srcdir)/src/include \
			-I$(top_srcdir)/src/include/scripting/include \
			-I$(top_srcdir)/src/include/scripting/sqplus \
			-I$(top_srcdir)/src/include/mozilla_chardet \
			-I$(top_srcdir)/src/include/mozilla_chardet/mfbt \
			-I$(top_srcdir)/src/include/mozilla_chardet/nsprpub/pr/include \
			-I$(top_srcdir)/src/include/mozilla_chardet/xpcom \
			-I$(top_srcdir)/src/include/mozilla_chardet/xpcom/base \
			-I$(top_srcdir)/src/include/mozilla_chardet/xpcom/glue

if GTK_NOTEBOOK
GTK_NOTEBOOK_FLAGS = -DUSE_GTK_NOTEBOOK
endif

if CODEBLOCKS_LINUX
AM_CPPFLAGS += -DENABLE_BINRELOC -DAPP_PREFIX="\"@prefix@\"" $(WX_GTK2_CFLAGS) $(GTK_NOTEBOOK_FLAGS)
endif

if WIN32
win32_sources = associations.cpp ipc.cpp resources/resources.rc
win32_ldflags = -Wl,--enable-auto-import -lshfolder -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm -lcomctl32 -lodbc32 -lole32 -loleaut32 -luuid -lrpcrt4 -ladvapi32 -lwsock32

.rc.o:
	$(RC) -O coff -I$(top_srcdir)/src $(AM_CPPFLAGS) $< $@
else
win32_sources = 
win32_ldflags =
endif

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) $(win32_ldflags)

bin_PROGRAMS = axcodeblocks

# wxMac resource fork/unbundled app
install-exec-hook:
	$(POSTLINK_COMMAND) $(DESTDIR)$(bindir)/axcodeblocks$(EXEEXT) \
	                    $(srcdir)/resources/icons/app.r
	$(MACSETFILE) -a C $(DESTDIR)$(bindir)/axcodeblocks$(EXEEXT)

# wxAUI uses GTK+ libraries on wxGTK
# axcodeblocks_LDFLAGS = $(WX_GTK2_LIBS)

axcodeblocks_LDADD =  ../sdk/libaxcodeblocks.la \
			$(WX_LIBS) \
			$(WX_GTK2_LIBS)

axcodeblocks_SOURCES = app.cpp \
			appglobals.cpp \
			backtracedlg.cpp \
			breakpointsdlg.cpp \
			compilersettingsdlg.cpp \
			cpuregistersdlg.cpp \
			axs_pinemdlg.cpp \
			axs_dbglink.cpp \
			crashhandler.cpp \
			debugger_interface_creator.cpp \
			debuggermenu.cpp \
			debuggersettingscommonpanel.cpp \
			debuggersettingsdlg.cpp \
			debuggersettingspanel.cpp \
			disassemblydlg.cpp \
			dlgabout.cpp \
			dlgaboutplugin.cpp \
			editkeywordsdlg.cpp \
			editorconfigurationdlg.cpp \
			environmentsettingsdlg.cpp \
			examinememorydlg.cpp \
			find_replace.cpp \
			infopane.cpp \
			main.cpp \
			notebookstyles.cpp \
			prefix.cpp \
			printdlg.cpp \
			projectmanagerui.cpp \
			projectdepsdlg.cpp \
			projectoptionsdlg.cpp \
			recentitemslist.cpp \
			scriptconsole.cpp \
			scriptingsettingsdlg.cpp \
			splashscreen.cpp \
			startherepage.cpp \
			switcherdlg.cpp \
			threadsdlg.cpp \
			virtualbuildtargetsdlg.cpp \
			watchesdlg.cpp \
			$(win32_sources)

noinst_HEADERS = app.h \
			appglobals.h \
			backtracedlg.h \
			batchbuild.h \
			breakpointsdlg.h \
			compilersettingsdlg.h \
			cpuregistersdlg.h \
			axs_pinemdlg.h \
			axs_dbglink.h \
			crashhandler.h \
			debugger_interface_creator.h \
			debuggermenu.h \
			debuggersettingscommonpanel.h \
			debuggersettingsdlg.h \
			debuggersettingspanel.h \
			disassemblydlg.h \
			dlgabout.h \
			dlgaboutplugin.h \
			editkeywordsdlg.h \
			editorconfigurationdlg.h \
			environmentsettingsdlg.h \
			examinememorydlg.h \
			find_replace.h \
			infopane.h \
			main.h \
			notebookstyles.h \
			prefix.h \
			printdlg.h \
			projectmanagerui.h \
			projectdepsdlg.h \
			projectoptionsdlg.h \
			recentitemslist.h \
			scriptconsole.h \
			scriptingsettingsdlg.h \
			splashscreen.h \
			startherepage.h \
			switcherdlg.h \
			threadsdlg.h \
			virtualbuildtargetsdlg.h \
			watchesdlg.h

man_MANS = axcodeblocks.1

EXTRA_DIST = associations.cpp \
			associations.h \
			ipc.h \
			ipc.cpp \
			$(man_MANS)
