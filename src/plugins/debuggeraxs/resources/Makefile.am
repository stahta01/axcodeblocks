pkgdata_DATA = debuggeraxs.zip
CLEANFILES = $(pkgdata_DATA)

DEBUG_RESOURCES = breakpoints.xrc \
	debugger_options_dlg.xrc \
	debugger_project_options_dlg.xrc \
	edit_watches_dlg.xrc \
	edit_axskey_dlg.xrc \
	manifest.xml

EXTRA_DIST = $(DEBUG_RESOURCES)

debuggeraxs.zip: $(DEBUG_RESOURCES)
	PWD=`pwd` cd $(srcdir) && zip $(PWD)/debuggeraxs.zip manifest.xml *.xrc > /dev/null
