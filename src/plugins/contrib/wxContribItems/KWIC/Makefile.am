SUBDIRS = include/wx/KWIC include/wx/xrc

AM_CPPFLAGS = $(WX_CPPFLAGS) -DBUILDING_PLUGIN \
		-I$(srcdir)/include

AM_CFLAGS = $(WX_CFLAGS)
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS = $(WX_LIBS) @WIN32_DLLLDFLAGS@ @WIN32_DLLEXPSTDCALL@

wxcontribitemsdir=$(libdir)/axcodeblocks/wxContribItems

wxcontribitems_LTLIBRARIES = libwxkwic.la

libwxkwic_la_LDFLAGS = -version-info 0:1:0 -shared -no-undefined

libwxkwic_la_LIBADD = $(WX_LIBS)

libwxkwic_la_SOURCES = \
./src/wx/KWIC/AngularMeter.cpp \
./src/wx/KWIC/AngularRegulator.cpp \
./src/wx/KWIC/BmpCheckBox.cpp \
./src/wx/KWIC/BmpSwitcher.cpp \
./src/wx/KWIC/LCDClock.cpp \
./src/wx/KWIC/LCDWindow.cpp \
./src/wx/KWIC/LinearMeter.cpp \
./src/wx/KWIC/LinearRegulator.cpp \
./src/wx/KWIC/TimeAlarm.cpp \
./src/wx/xrc/xh_kwxangularmeterhandler.cpp \
./src/wx/xrc/xh_kwxangularregulatorhandler.cpp \
./src/wx/xrc/xh_kwxlcdclockhandler.cpp \
./src/wx/xrc/xh_kwxlcddisplayhandler.cpp \
./src/wx/xrc/xh_kwxlinearmeterhandler.cpp \
./src/wx/xrc/xh_kwxlinearregulatorhandler.cpp

EXTRA_DIST = \
./CREDITS \
./KWIC-LICENSE
