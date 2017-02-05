%{?mingw_package_header}

%global mingw_build_win32 1
%global mingw_build_win64 0

%global gitver 10778
%global name1 axcodeblocks

Name:           mingw-%{name1}
Version:	16.01svn
Release:	1%{?dist}
Summary:	An open source, cross platform, free C++ IDE for Axsem Microcontrollers
Group:		Development/Tools
License:	GPLv3+
URL:		http://www.axsem.com/
Source0:	%{name}/%{name1}-%{version}%{gitver}.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch:      noarch
BuildRequires:  dos2unix
BuildRequires:  zip
BuildRequires:  rsync
BuildRequires:  mingw32-binutils
BuildRequires:  mingw64-binutils
BuildRequires:  mingw32-boost
BuildRequires:  mingw64-boost
BuildRequires:  mingw32-bzip2
BuildRequires:  mingw64-bzip2
BuildRequires:  mingw32-cpp
BuildRequires:  mingw64-cpp
BuildRequires:  mingw32-expat
BuildRequires:  mingw64-expat
BuildRequires:  mingw32-filesystem
BuildRequires:  mingw64-filesystem
BuildRequires:  mingw32-gcc
BuildRequires:  mingw64-gcc
BuildRequires:  mingw32-gcc-c++
BuildRequires:  mingw64-gcc-c++
BuildRequires:  mingw32-libjpeg
BuildRequires:  mingw64-libjpeg
BuildRequires:  mingw32-libtiff
BuildRequires:  mingw64-libtiff
BuildRequires:  mingw32-pthreads
BuildRequires:  mingw64-pthreads
BuildRequires:  mingw32-runtime
BuildRequires:  mingw64-runtime
BuildRequires:  mingw32-w32api
BuildRequires:  mingw32-wxWidgets
BuildRequires:  mingw64-wxWidgets
BuildRequires:  mingw32-zlib
BuildRequires:  mingw64-zlib
BuildRequires:  mingw32-hunspell
BuildRequires:  mingw64-hunspell
%{?_with_buildcb:BuildRequires:  codeblocks}

%global		pkgdatadir32	%{mingw32_datadir}/%{name1}
%global		plugindir32	%{pkgdatadir32}/plugins
%global		pkgdatadir64	%{mingw64_datadir}/%{name1}
%global		plugindir64	%{pkgdatadir64}/plugins

%filter_provides_in %{plugindir32} %{plugindir64}
%filter_setup

%description
AxCode::Blocks is a free C++ IDE built specifically to meet the most demanding
needs of its users. It was designed, right from the start, to be extensible
and configurable. Built around a plug-in framework, Code::Blocks can be
extended with plug-in DLLs. It includes a plug-in wizard, so you can compile
your own plug-ins.

# Mingw32
%package -n mingw32-%{name1}
Summary:                %{summary}

%description -n mingw32-%{name1}
AxCode::Blocks is a free C++ IDE built specifically to meet the most demanding
needs of its users. It was designed, right from the start, to be extensible
and configurable. Built around a plug-in framework, Code::Blocks can be
extended with plug-in DLLs. It includes a plug-in wizard, so you can compile
your own plug-ins.

# Mingw64
%package -n mingw64-%{name1}
Summary:                %{summary}

%description -n mingw64-%{name1}
AxCode::Blocks is a free C++ IDE built specifically to meet the most demanding
needs of its users. It was designed, right from the start, to be extensible
and configurable. Built around a plug-in framework, Code::Blocks can be
extended with plug-in DLLs. It includes a plug-in wizard, so you can compile
your own plug-ins.

%{?mingw_debug_package}

%prep
%setup -q -n %{name1}-%{version}%{gitver}

# convert EOLs
find . -type f -and -not -name "*.cpp" -and -not -name "*.h" -and -not -name "*.png" -and -not -name "*.bmp" -and -not -name "*.c" -and -not -name "*.cxx" -and -not -name "*.ico" | sed "s/.*/\"\\0\"/" | xargs dos2unix --keepdate &> /dev/null

# remove execute bits from source files
find src/plugins/contrib/regex_testbed -type f -exec chmod a-x {} ';'
find src/plugins/contrib/IncrementalSearch -type f -exec chmod a-x {} ';'
find src/plugins/compilergcc -type f -exec chmod a-x {} ';'

# fix version string
sed -i 's/-release//g' revision.m4

# remove resource archives, they are corrupted
rm -f src/src/resources/*.zip

%if %{?_with_buildcb:0}%{!?_with_buildcb:1}
aclocal
autoconf
automake --foreign

# temporary workaround/hack
#export MINGW_CFLAGS="-fpermissive"
#export MINGW_CXXFLAGS="-fpermissive"
export MINGW32_CONFIGURE_ARGS=--with-wx-config=/usr/i686-w64-mingw32/sys-root/mingw/bin/wx-config
export MINGW64_CONFIGURE_ARGS=--with-wx-config=/usr/x86_64-w64-mingw32/sys-root/mingw/bin/wx-config
%{mingw_configure} --with-contrib-plugins=all --enable-header-guard --enable-log-hacker --enable-modpoller --enable-tidycmt
%endif


%build
%if %{?_with_buildcb:1}%{!?_with_buildcb:0}
pushd src
DISPLAY=:0 codeblocks --no-splash-screen --no-log --build --target=tinyXMLnative CodeBlocks-cross.cbp
DISPLAY=:0 codeblocks --no-splash-screen --no-log --build CodeBlocks-cross.cbp
popd
%endif

%if %{?_with_buildcb:0}%{!?_with_buildcb:1}
%{mingw_make} %{?_smp_mflags}
%endif

%install
rm -rf $RPM_BUILD_ROOT

%if %{?_with_buildcb:1}%{!?_with_buildcb:0}
install -d $RPM_BUILD_ROOT%{_mingw32_bindir}
install src/devel/axcodeblocks.exe $RPM_BUILD_ROOT%{_mingw32_bindir}
install src/tools/ConsoleRunner/cb_console_runner.exe $RPM_BUILD_ROOT%{_mingw32_bindir}
install src/devel/axcodeblocks.dll $RPM_BUILD_ROOT%{_mingw32_bindir}
install src/devel/wxpropgrid.dll $RPM_BUILD_ROOT%{_mingw32_bindir}
install src/exchndl.dll $RPM_BUILD_ROOT%{_mingw32_bindir}
install -d $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/astyle.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/debugger.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/autosave.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/todo.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/codecompletion.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/abbreviations.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/projectsimporter.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/classwizard.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/compiler.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/openfileslist.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/scriptedwizard.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/defaultmimehandler.dll $RPM_BUILD_ROOT%{plugindir}
install src/devel/share/CodeBlocks/plugins/xpmanifest.dll $RPM_BUILD_ROOT%{plugindir}

install -d $RPM_BUILD_ROOT%{pkgdatadir}/lexers
install -d $RPM_BUILD_ROOT%{pkgdatadir}/images/settings
install -d $RPM_BUILD_ROOT%{pkgdatadir}/images/16x16
install -d $RPM_BUILD_ROOT%{pkgdatadir}/images/codecompletion
install -d $RPM_BUILD_ROOT%{pkgdatadir}/plugins
install -d $RPM_BUILD_ROOT%{pkgdatadir}/templates/wizard
install -d $RPM_BUILD_ROOT%{pkgdatadir}/scripts
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/resources.zip src/src/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/manager_resources.zip src/sdk/resources/*.xrc src/sdk/resources/images/*.png
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/start_here.zip src/src/resources/start_here/*.html src/src/resources/start_here/*.png
# Compressing plugins UI resources
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/astyle.zip src/plugins/astyle/resources/manifest.xml src/plugins/astyle/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/autosave.zip src/plugins/autosave/manifest.xml src/plugins/autosave/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/classwizard.zip src/plugins/classwizard/resources/manifest.xml src/plugins/classwizard/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/codecompletion.zip src/plugins/codecompletion/resources/manifest.xml src/plugins/codecompletion/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/compiler.zip src/plugins/compilergcc/resources/manifest.xml src/plugins/compilergcc/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/debugger.zip src/plugins/debuggergdb/resources/manifest.xml src/plugins/debuggergdb/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/defaultmimehandler.zip src/plugins/defaultmimehandler/resources/manifest.xml src/plugins/defaultmimehandler/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/openfileslist.zip src/plugins/openfileslist/manifest.xml
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/projectsimporter.zip src/plugins/projectsimporter/resources/manifest.xml src/plugins/projectsimporter/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/scriptedwizard.zip src/plugins/scriptedwizard/resources/manifest.xml
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/todo.zip src/plugins/todo/resources/manifest.xml src/plugins/todo/resources/*.xrc
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/xpmanifest.zip src/plugins/xpmanifest/manifest.xml
zip -jqu9 $RPM_BUILD_ROOT%{pkgdatadir}/abbreviations.zip src/plugins/abbreviations/resources/manifest.xml src/plugins/abbreviations/resources/*.xrc
# Packing core UI bitmaps
pushd src/src/resources
zip -0 -qu $RPM_BUILD_ROOT%{pkgdatadir}/resources.zip images/*.png images/16x16/*.png
popd
pushd src/sdk/resources
zip -0 -qu $RPM_BUILD_ROOT%{pkgdatadir}/manager_resources.zip images/*.png images/16x16/*.png
popd
pushd src/plugins/compilergcc/resources
zip -0 -qu $RPM_BUILD_ROOT%{pkgdatadir}/compiler.zip images/*.png images/16x16/*.png
popd
#pushd src/plugins/debuggergdb/resources
#zip -0 -qu $RPM_BUILD_ROOT%{pkgdatadir}/debugger.zip images/*.png images/16x16/*.png
#popd
install src/sdk/resources/lexers/lexer_* $RPM_BUILD_ROOT%{pkgdatadir}/lexers
install src/src/resources/images/*.png $RPM_BUILD_ROOT%{pkgdatadir}/images
install src/src/resources/images/settings/*.png $RPM_BUILD_ROOT%{pkgdatadir}/images/settings
install src/src/resources/images/16x16/*.png $RPM_BUILD_ROOT%{pkgdatadir}/images/16x16
install src/plugins/codecompletion/resources/images/*.png $RPM_BUILD_ROOT%{pkgdatadir}/images/codecompletion
echo Makefile.am > excludes.txt
echo Makefile.in >> excludes.txt
echo /.svn/ >> excludes.txt
echo *.gdb >> excludes.txt
rsync -r --exclude-from=excludes.txt src/plugins/scriptedwizard/resources/* $RPM_BUILD_ROOT%{pkgdatadir}/templates/wizard
rsync --exclude-from=excludes.txt src/templates/common/* src/templates/win32/* $RPM_BUILD_ROOT%{pkgdatadir}/templates
rsync --exclude-from=excludes.txt src/scripts/* $RPM_BUILD_ROOT%{pkgdatadir}/scripts
rm excludes.txt
install src/tips.txt $RPM_BUILD_ROOT%{pkgdatadir}
%endif

%if %{?_with_buildcb:0}%{!?_with_buildcb:1}
%{mingw_make} DESTDIR=$RPM_BUILD_ROOT install
# fixup paths: win32
%if 0%{?mingw_build_win32} == 1
mv $RPM_BUILD_ROOT%{mingw32_bindir}/i686-w64-mingw32-axcodeblocks.exe $RPM_BUILD_ROOT%{mingw32_bindir}/axcodeblocks.exe
mv $RPM_BUILD_ROOT%{mingw32_bindir}/i686-w64-mingw32-axcb_console_runner.exe $RPM_BUILD_ROOT%{mingw32_bindir}/axcb_console_runner.exe
mv $RPM_BUILD_ROOT%{mingw32_bindir}/i686-w64-mingw32-axcb_share_config.exe $RPM_BUILD_ROOT%{mingw32_bindir}/axcb_share_config.exe
#mv $RPM_BUILD_ROOT%{mingw32_bindir}/i686-w64-mingw32-codesnippets.exe $RPM_BUILD_ROOT%{mingw32_bindir}/codesnippets.exe
rm -f $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/plugins/*.la
rm -f $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/plugins/*.a
rm -f $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/wxContribItems/*.la
rm -f $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/wxContribItems/*.a
rmdir $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/wxContribItems
mv $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/bin/* $RPM_BUILD_ROOT%{mingw32_bindir}/
rmdir $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/bin
mv $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}/plugins $RPM_BUILD_ROOT%{pkgdatadir32}
rmdir $RPM_BUILD_ROOT%{mingw32_libdir}/%{name1}
rm -rf $RPM_BUILD_ROOT%{mingw32_datadir}/applications
rm -rf $RPM_BUILD_ROOT%{mingw32_datadir}/icons
rm -rf $RPM_BUILD_ROOT%{mingw32_datadir}/man
rm -rf $RPM_BUILD_ROOT%{mingw32_datadir}/mime
rm -rf $RPM_BUILD_ROOT%{mingw32_datadir}/pixmaps
rm -f $RPM_BUILD_ROOT%{pkgdatadir32}/compilers/compiler_sdcc.xml
rm -f $RPM_BUILD_ROOT%{pkgdatadir32}/compilers/compiler_keilc51.xml
mv $RPM_BUILD_ROOT%{pkgdatadir32}/plugins/libwxPdfDocument.dll $RPM_BUILD_ROOT%{mingw32_bindir}/
%endif
# fixup paths: win64
%if 0%{?mingw_build_win64} == 1
mv $RPM_BUILD_ROOT%{mingw64_bindir}/x86_64-w64-mingw64-axcodeblocks.exe $RPM_BUILD_ROOT%{mingw64_bindir}/axcodeblocks.exe
mv $RPM_BUILD_ROOT%{mingw64_bindir}/x86_64-w64-mingw64-axcb_console_runner.exe $RPM_BUILD_ROOT%{mingw64_bindir}/axcb_console_runner.exe
mv $RPM_BUILD_ROOT%{mingw64_bindir}/x86_64-w64-mingw64-axcb_share_config.exe $RPM_BUILD_ROOT%{mingw64_bindir}/axcb_share_config.exe
#mv $RPM_BUILD_ROOT%{mingw64_bindir}/x86_64-w64-mingw64-codesnippets.exe $RPM_BUILD_ROOT%{mingw64_bindir}/codesnippets.exe
rm -f $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/plugins/*.la
rm -f $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/plugins/*.a
rm -f $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/wxContribItems/*.la
rm -f $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/wxContribItems/*.a
rmdir $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/wxContribItems
mv $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/bin/* $RPM_BUILD_ROOT%{mingw64_bindir}/
rmdir $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/bin
mv $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}/plugins $RPM_BUILD_ROOT%{pkgdatadir64}
rmdir $RPM_BUILD_ROOT%{mingw64_libdir}/%{name1}
rm -rf $RPM_BUILD_ROOT%{mingw64_datadir}/applications
rm -rf $RPM_BUILD_ROOT%{mingw64_datadir}/icons
rm -rf $RPM_BUILD_ROOT%{mingw64_datadir}/man
rm -rf $RPM_BUILD_ROOT%{mingw64_datadir}/mime
rm -rf $RPM_BUILD_ROOT%{mingw64_datadir}/pixmaps
rm -f $RPM_BUILD_ROOT%{pkgdatadir64}/compilers/compiler_sdcc.xml
rm -f $RPM_BUILD_ROOT%{pkgdatadir64}/compilers/compiler_keilc51.xml
mv $RPM_BUILD_ROOT%{pkgdatadir64}/plugins/libwxPdfDocument.dll $RPM_BUILD_ROOT%{mingw64_bindir}/
%endif
%endif


%clean
rm -rf $RPM_BUILD_ROOT


%files -n mingw32-%{name1}
%defattr(-,root,root,-)
%doc README AUTHORS BUGS COMPILERS NEWS ChangeLog

%if %{?_with_buildcb:1}%{!?_with_buildcb:0}

%{mingw32_bindir}/*

%dir %{plugindir32}
%{plugindir32}/abbreviations.dll
%{plugindir32}/astyle.dll
%{plugindir32}/autosave.dll
%{plugindir32}/classwizard.dll
%{plugindir32}/codecompletion.dll
%{plugindir32}/compiler.dll
%{plugindir32}/debugger.dll
%{plugindir32}/debuggeraxs.dll
%{plugindir32}/defaultmimehandler.dll
%{plugindir32}/liboccurrenceshighlighting.dll
%{plugindir32}/openfileslist.dll
%{plugindir32}/projectsimporter.dll
%{plugindir32}/scriptedwizard.dll
%{plugindir32}/todo.dll
%{plugindir32}/xpmanifest.dll

%dir %{pkgdatadir32}
%{pkgdatadir32}/compilers
%dir %{pkgdatadir32}/images
%{pkgdatadir32}/images/*.png
%{pkgdatadir32}/images/16x16
%{pkgdatadir32}/images/codecompletion
%{pkgdatadir32}/images/settings
%{pkgdatadir32}/lexers
%{pkgdatadir32}/scripts
%{pkgdatadir32}/templates
%{pkgdatadir32}/abbreviations.zip
%{pkgdatadir32}/astyle.zip
%{pkgdatadir32}/autosave.zip
%{pkgdatadir32}/classwizard.zip
%{pkgdatadir32}/codecompletion.zip
%{pkgdatadir32}/compiler.zip
%{pkgdatadir32}/debugger.zip
%{pkgdatadir32}/debuggeraxs.zip
%{pkgdatadir32}/defaultmimehandler.zip
%{pkgdatadir32}/manager_resources.zip
%{pkgdatadir32}/occurrenceshighlighting.zip
%{pkgdatadir32}/openfileslist.zip
%{pkgdatadir32}/projectsimporter.zip
%{pkgdatadir32}/resources.zip
%{pkgdatadir32}/scriptedwizard.zip
%{pkgdatadir32}/start_here.zip
%{pkgdatadir32}/todo.zip
%{pkgdatadir32}/tips.txt
%{pkgdatadir32}/xpmanifest.zip
%endif

%if %{?_with_buildcb:0}%{!?_with_buildcb:1}
%if 0%{?mingw_build_win32} == 1
%{mingw32_bindir}/axcodeblocks.exe
%{mingw32_bindir}/axcb_console_runner.exe
%{mingw32_bindir}/axcb_share_config.exe
#%{mingw32_bindir}/codesnippets.exe
%{mingw32_bindir}/libaxcodeblocks-0.dll
%{mingw32_bindir}/libwxchartctrl-0.dll
%{mingw32_bindir}/libwxcustombutton-0.dll
%{mingw32_bindir}/libwxflatnotebook-0.dll
%{mingw32_bindir}/libwxpropgrid-0.dll
%{mingw32_bindir}/libwxscintilla-0.dll
%{mingw32_bindir}/libwxsmithlib-0.dll
%{mingw32_bindir}/libwximagepanel-0.dll
%{mingw32_bindir}/libwxspeedbutton-0.dll
%{mingw32_bindir}/libwxPdfDocument.dll
%{mingw32_bindir}/libwxkwic-0.dll
%{mingw32_bindir}/libwxled-0.dll
%{mingw32_bindir}/libwxtreelist-0.dll
%{mingw32_includedir}/axcodeblocks
%{mingw32_includedir}/wxsmith
%{mingw32_libdir}/libaxcodeblocks.dll.a
%{mingw32_libdir}/libaxcodeblocks.la
%{mingw32_libdir}/libwxpropgrid.dll.a
%{mingw32_libdir}/libwxpropgrid.la
%{mingw32_libdir}/libwxscintilla.dll.a
%{mingw32_libdir}/libwxscintilla.la
%{mingw32_libdir}/libwxsmithlib.dll.a
%{mingw32_libdir}/libwxsmithlib.la
%{mingw32_libdir}/pkgconfig
%{pkgdatadir32}
%endif
%endif

%if %{?_with_buildcb:0}%{!?_with_buildcb:1}
%if 0%{?mingw_build_win64} == 1
%files -n mingw64-%{name1}
%defattr(-,root,root,-)
%doc README AUTHORS BUGS COMPILERS NEWS ChangeLog

%{mingw64_bindir}/axcodeblocks.exe
%{mingw64_bindir}/axcb_console_runner.exe
%{mingw64_bindir}/axcb_share_config.exe
#%{mingw64_bindir}/codesnippets.exe
%{mingw64_bindir}/libaxcodeblocks-0.dll
%{mingw64_bindir}/libwxchartctrl-0.dll
%{mingw64_bindir}/libwxcustombutton-0.dll
%{mingw64_bindir}/libwxflatnotebook-0.dll
%{mingw64_bindir}/libwxpropgrid-0.dll
%{mingw64_bindir}/libwxscintilla-0.dll
%{mingw64_bindir}/libwxsmithlib-0.dll
%{mingw64_bindir}/libwximagepanel-0.dll
%{mingw64_bindir}/libwxspeedbutton-0.dll
%{mingw64_bindir}/libwxPdfDocument.dll
%{mingw64_bindir}/libwxkwic-0.dll
%{mingw64_bindir}/libwxled-0.dll
%{mingw64_bindir}/libwxtreelist-0.dll
%{mingw64_includedir}/axcodeblocks
%{mingw64_includedir}/wxsmith
%{mingw64_libdir}/libaxcodeblocks.dll.a
%{mingw64_libdir}/libaxcodeblocks.la
%{mingw64_libdir}/libwxpropgrid.dll.a
%{mingw64_libdir}/libwxpropgrid.la
%{mingw64_libdir}/libwxscintilla.dll.a
%{mingw64_libdir}/libwxscintilla.la
%{mingw64_libdir}/libwxsmithlib.dll.a
%{mingw64_libdir}/libwxsmithlib.la
%{mingw64_libdir}/pkgconfig
%{pkgdatadir64}
%endif
%endif

%changelog
* Thu Dec  6 2012 Thomas Sailer <t.sailer@axsem.com> - 10.05svn-2
- update to 8644

* Thu Apr 28 2011 Thomas Sailer <t.sailer@axsem.com> - 10.05svn-1
- initial version based on native spec file
