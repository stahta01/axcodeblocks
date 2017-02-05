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

pluginlib_LTLIBRARIES = libdebuggeraxs.la

libdebuggeraxs_la_LDFLAGS = $(WX_GTK2_LIBS) -module -version-info 0:1:0 -shared -no-undefined -avoid-version

libdebuggeraxs_la_LIBADD = ../../sdk/libaxcodeblocks.la $(PLUGIN_WX_LIBS)

libdebuggeraxs_la_SOURCES = databreakpointdlg.cpp \
			debugger_defs.cpp \
			debuggerdriver.cpp \
			debuggeraxs.cpp \
			debuggeroptionsdlg.cpp \
			debuggeroptionsprjdlg.cpp \
			debuggerstate.cpp \
			editwatchdlg.cpp \
			editaxsemkeydlg.cpp \
			editbreakpointdlg.cpp \
			axs_driver.cpp \
			gdb_tipwindow.cpp \
			projtargetoptions.cpp \
			axspipedprocess.cpp \
			machine.cpp

noinst_HEADERS = 	databreakpointdlg.h \
			debugger_defs.h \
			debuggerdriver.h \
			debuggeraxs.h \
			debuggeroptionsdlg.h \
			debuggeroptionsprjdlg.h \
			debuggerstate.h \
			editwatchdlg.h \
			editaxsemkeydlg.h \
			editbreakpointdlg.h \
			axs_commands.h \
			axs_driver.h \
			gdb_tipwindow.h \
			projtargetoptions.h \
			axspipedprocess.h \
			machine.h
