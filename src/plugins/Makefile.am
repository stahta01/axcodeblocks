if BUILD_ASTYLE
MAYBE_ASTYLE=astyle
endif

if BUILD_AUTOSAVE
MAYBE_AUTOSAVE=autosave
endif

if BUILD_CLASSWIZARD
MAYBE_CLASSWIZARD=classwizard
endif

if BUILD_CODECOMPLETION
MAYBE_CODECOMPLETION=codecompletion
endif

if BUILD_COMPILER
MAYBE_COMPILER=compilergcc
endif

if BUILD_DEBUGGER
#MAYBE_DEBUGGER=debuggeraxs debuggergdb
MAYBE_DEBUGGER=debuggeraxs
MAYBE_DEBUGGER2=debuggergdb
endif

if BUILD_MIMEHANDLER
MAYBE_MIMEHANDLER=defaultmimehandler
endif

if BUILD_HEADERGUARD
MAYBE_HEADERGUARD=headerguard
endif

if BUILD_LOGHACKER
MAYBE_LOGHACKER=loghacker
endif

if BUILD_MODPOLLER
MAYBE_MODPOLLER=modpoller
endif

if BUILD_OPENFILESLIST
MAYBE_OPENFILESLIST=openfileslist
endif

if BUILD_OCCURRENCESHIGHLIGHTING
MAYBE_OCCURRENCESHIGHLIGHTING=occurrenceshighlighting
endif

if BUILD_PROJECTSIMPORTER
MAYBE_PROJECTSIMPORTER=projectsimporter
endif

if BUILD_SCRIPTEDWIZARD
MAYBE_SCRIPTEDWIZARD=scriptedwizard
endif

if BUILD_TIDYCMT
MAYBE_TIDYCMT=tidycmt
endif

if BUILD_TODO
MAYBE_TODO=todo
endif

if BUILD_ABBREVIATIONS
MAYBE_ABBREVIATIONS=abbreviations
endif

if BUILD_MANIFEST
MAYBE_MANIFEST=xpmanifest
endif

SUBDIRS=$(MAYBE_ASTYLE) \
		$(MAYBE_AUTOSAVE) \
		$(MAYBE_CLASSWIZARD) \
		$(MAYBE_CODECOMPLETION) \
		$(MAYBE_COMPILER) \
		$(MAYBE_DEBUGGER) \
		$(MAYBE_DEBUGGER2) \
		$(MAYBE_MIMEHANDLER) \
		$(MAYBE_HEADERGUARD) \
		$(MAYBE_LOGHACKER) \
		$(MAYBE_MODPOLLER) \
		$(MAYBE_SCRIPTEDWIZARD) \
		$(MAYBE_PROJECTSIMPORTER) \
		$(MAYBE_OPENFILESLIST) \
		$(MAYBE_OCCURRENCESHIGHLIGHTING) \
		$(MAYBE_TIDYCMT) \
		$(MAYBE_TODO) \
		$(MAYBE_ABBREVIATIONS) \
		$(MAYBE_MANIFEST) \
		contrib
# the 'contrib' subdir contains its own filtering

DIST_SUBDIRS = abbreviations \
		astyle \
		autosave \
		classwizard \
		codecompletion \
		compilergcc \
		debuggeraxs \
		debuggergdb \
		defaultmimehandler \
		headerguard \
		loghacker \
		modpoller \
		openfileslist \
		occurrenceshighlighting \
		projectsimporter \
		scriptedwizard \
		tidycmt \
		todo \
		xpmanifest \
		contrib

EXTRA_DIST = headerguard \
		loghacker \
		modpoller \
		tidycmt
