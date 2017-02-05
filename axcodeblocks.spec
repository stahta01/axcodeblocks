%global gitver 10778

Name:		axcodeblocks
Version:	16.01svn
Release:	1%{?dist}
Summary:	An open source, cross platform, free C++ IDE for Axsem Microcontrollers
Group:		Development/Tools
License:	GPLv3+
URL:		http://www.codeblocks.org/
Source0:	%{name}/%{name}-%{version}%{gitver}.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:	libtool
BuildRequires:	wxGTK-devel >= 2.8.0
BuildRequires:	zlib-devel
BuildRequires:	bzip2-devel
BuildRequires:	tinyxml-devel
BuildRequires:	zip
BuildRequires:	dos2unix
BuildRequires:	desktop-file-utils
BuildRequires:	hunspell-devel
BuildRequires:  gamin-devel
Requires:	%{name}-libs = %{version}-%{release}
Requires:	shared-mime-info
Requires:	xterm
# use system tinyxml lib
#Patch1:		%{name}-tinyxml.patch
# https://bugzilla.redhat.com/show_bug.cgi?id=565198 (fully fixed in svn rev 6330)
#Patch2:		%{name}-dso.patch
# update for tinyxml 2.6
#Patch3:		%{name}-tinyxml-26.patch
# D support - svn revisions 6553-6556
#Patch4:         %{name}-10.05-D.patch

%define		pkgdatadir	%{_datadir}/%{name}
%define		pkglibdir	%{_libdir}/%{name}
%define		plugindir	%{pkglibdir}/plugins

%filter_provides_in %{plugindir}
%filter_setup


%description
Code::Blocks is a free C++ IDE built specifically to meet the most demanding
needs of its users. It was designed, right from the start, to be extensible
and configurable. Built around a plug-in framework, Code::Blocks can be
extended with plug-in DLLs. It includes a plug-in wizard, so you can compile
your own plug-ins.

%package libs
Summary:	Libraries needed to run Code::Blocks and its plug-ins
Group:		System Environment/Libraries

%description libs
Libraries needed to run Code::Blocks and its plug-ins.

%package devel
Summary:	Files needed to build Code::Blocks plug-ins
Group:		Development/Libraries
Requires:	%{name}-libs = %{version}-%{release}

%description devel
Development files needed to build Code::Blocks plug-ins.

%package contrib-libs
Summary:	Libraries needed to run Code::Blocks contrib plug-ins
Group:		System Environment/Libraries

%description contrib-libs
Libraries needed to run Code::Blocks contrib plug-ins.

%package contrib-devel
Summary:	Files needed to build Code::Blocks contrib plug-ins
Group:		Development/Libraries
Requires:	%{name}-contrib-libs = %{version}-%{release}

%description contrib-devel
Development files needed to build Code::Blocks contrib plug-ins.

%package contrib
Summary:	Additional Code::Blocks plug-ins
Group:		Development/Tools
Requires:	%{name} = %{version}-%{release}
Requires:	%{name}-contrib-libs = %{version}-%{release}
Requires:	cppcheck
Requires:	valgrind

%description contrib
Additional Code::Blocks plug-ins.


%prep
%setup -q -n %{name}-%{version}%{gitver}
#patch1 -p1 -b .tinyxml
#patch2 -p1 -b .dso
#patch3 -p1 -b .tinyxml-26
#patch4 -p1 -b .D

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

autoreconf -f -i


%build
# temporary workaround/hack
#export CPPFLAGS="-fpermissive"
%configure --with-contrib-plugins=all --enable-header-guard --enable-log-hacker --enable-modpoller --enable-tidycmt

make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT INSTALL="/usr/bin/install -p" install
  
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/wxSmithContribItems/*.la
rm -f $RPM_BUILD_ROOT%{plugindir}/*.la

desktop-file-install --vendor fedora \
	--dir $RPM_BUILD_ROOT%{_datadir}/applications \
	--delete-original \
	$RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop

# set a fixed timestamp (source archive creation) to generated resource archives
touch -r %{SOURCE0} $RPM_BUILD_ROOT%{pkgdatadir}/*.zip

# generate linker config file for wxSmithContribItems libraries
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/ld.so.conf.d
echo "%{_libdir}/%{name}/wxContribItems" > $RPM_BUILD_ROOT%{_sysconfdir}/ld.so.conf.d/%{name}-contrib-%{_arch}.conf

rm -f $RPM_BUILD_ROOT/%{_libdir}/%{name}/wxContribItems/*.la

rm -f $RPM_BUILD_ROOT/%{pkgdatadir}/compilers/compiler_sdcc.xml
rm -f $RPM_BUILD_ROOT/%{pkgdatadir}/compilers/compiler_keilc51.xml

%clean
rm -rf $RPM_BUILD_ROOT


%post
update-mime-database /usr/share/mime &> /dev/null || :

%postun
update-mime-database /usr/share/mime &> /dev/null || :

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post contrib-libs -p /sbin/ldconfig

%postun contrib-libs -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc README AUTHORS BUGS COMPILERS NEWS ChangeLog

%{_bindir}/*
%{_mandir}/man1/*.gz

%dir %{pkglibdir}
%dir %{plugindir}
%{plugindir}/libastyle.so
%{plugindir}/libautosave.so
%{plugindir}/libclasswizard.so
%{plugindir}/libcodecompletion.so
%{plugindir}/libcompiler.so
%{plugindir}/libdebugger.so
%{plugindir}/libdebuggeraxs.so
%{plugindir}/libdefaultmimehandler.so
%{plugindir}/liboccurrenceshighlighting.so
%{plugindir}/libopenfileslist.so
%{plugindir}/libprojectsimporter.so
%{plugindir}/libscriptedwizard.so
%{plugindir}/libtodo.so

%{_datadir}/applications/fedora-%{name}.desktop
%{_datadir}/icons/hicolor/48x48/mimetypes/*.png
%{_datadir}/mime/packages/%{name}.xml
%{_datadir}/pixmaps/%{name}.png

%dir %{pkgdatadir}
%{pkgdatadir}/compilers
%{pkgdatadir}/icons
%dir %{pkgdatadir}/images
%{pkgdatadir}/images/*.png
%{pkgdatadir}/images/16x16
%{pkgdatadir}/images/codecompletion
%{pkgdatadir}/images/settings
%{pkgdatadir}/lexers
%{pkgdatadir}/scripts
%{pkgdatadir}/templates
%{pkgdatadir}/astyle.zip
%{pkgdatadir}/autosave.zip
%{pkgdatadir}/classwizard.zip
%{pkgdatadir}/codecompletion.zip
%{pkgdatadir}/compiler.zip
%{pkgdatadir}/debugger.zip
%{pkgdatadir}/debuggeraxs.zip
%{pkgdatadir}/defaultmimehandler.zip
%{pkgdatadir}/manager_resources.zip
%{pkgdatadir}/occurrenceshighlighting.zip
%{pkgdatadir}/openfileslist.zip
%{pkgdatadir}/projectsimporter.zip
%{pkgdatadir}/resources.zip
%{pkgdatadir}/scriptedwizard.zip
%{pkgdatadir}/start_here.zip
%{pkgdatadir}/todo.zip
%{pkgdatadir}/tips.txt

%files libs
%defattr(-,root,root,-)
%doc COPYING
%{_libdir}/lib%{name}.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}
%{_libdir}/lib%{name}.so
%{_libdir}/pkgconfig/%{name}.pc

%files contrib-libs
%defattr(-,root,root,-)
%{_sysconfdir}/ld.so.conf.d/%{name}-contrib-%{_arch}.conf
%{_libdir}/libwxsmithlib.so.*
%{_libdir}/%{name}/wxContribItems/*.so.*
%exclude %{_libdir}/libwxsmithlib.so

%files contrib-devel
%defattr(-,root,root,-)
%{_includedir}/wxsmith
%{_libdir}/%{name}/wxContribItems/*.so
%{_libdir}/pkgconfig/wxsmith.pc
%{_libdir}/pkgconfig/wxsmithaui.pc
%{_libdir}/pkgconfig/wxsmith-contrib.pc
%{_libdir}/pkgconfig/cb_wxKWIC.pc
%{_libdir}/pkgconfig/cb_wxchartctrl.pc
%{_libdir}/pkgconfig/cb_wxcontrib.pc
%{_libdir}/pkgconfig/cb_wxcustombutton.pc
%{_libdir}/pkgconfig/cb_wxflatnotebook.pc
%{_libdir}/pkgconfig/cb_wximagepanel.pc
%{_libdir}/pkgconfig/cb_wxled.pc
%{_libdir}/pkgconfig/cb_wxmathplot.pc
%{_libdir}/pkgconfig/cb_wxspeedbutton.pc
%{_libdir}/pkgconfig/cb_wxtreelist.pc
%{_includedir}/%{name}/wxContribItems/wxFlatNotebook/include/wx/wxFlatNotebook/*.h
%{_includedir}/%{name}/wxContribItems/wxImagePanel/include/wx/*.h

%files contrib
%defattr(-,root,root,-)
%{pkgdatadir}/abbreviations.zip
%{pkgdatadir}/AutoVersioning.zip
%{pkgdatadir}/BrowseTracker.zip
%{pkgdatadir}/byogames.zip
%{pkgdatadir}/cb_koders.zip
%{pkgdatadir}/Cccc.zip
%{pkgdatadir}/codesnippets.zip
%{pkgdatadir}/codestat.zip
%{pkgdatadir}/copystrings.zip
%{pkgdatadir}/CppCheck.zip
%{pkgdatadir}/Cscope.zip
%{pkgdatadir}/devpakupdater.zip
%{pkgdatadir}/DoxyBlocks.zip
%{pkgdatadir}/dragscroll.zip
%{pkgdatadir}/EditorConfig.zip
%{pkgdatadir}/EditorTweaks.zip
%{pkgdatadir}/envvars.zip
%{pkgdatadir}/exporter.zip
%{pkgdatadir}/FileManager.zip
%{pkgdatadir}/headerfixup.zip
%{pkgdatadir}/headerguard.zip
%{pkgdatadir}/help_plugin.zip
%{pkgdatadir}/HexEditor.zip
%{pkgdatadir}/IncrementalSearch.zip
%{pkgdatadir}/keybinder.zip
%{pkgdatadir}/lib_finder.zip
%{pkgdatadir}/loghacker.zip
%{pkgdatadir}/ModPoller.zip
%{pkgdatadir}/MouseSap.zip
%{pkgdatadir}/NassiShneiderman.zip
%{pkgdatadir}/Profiler.zip
%{pkgdatadir}/ProjectOptionsManipulator.zip
%{pkgdatadir}/RegExTestbed.zip
%{pkgdatadir}/ReopenEditor.zip
%{pkgdatadir}/SmartIndentCpp.zip
%{pkgdatadir}/SmartIndentFortran.zip
%{pkgdatadir}/SmartIndentHDL.zip
%{pkgdatadir}/SmartIndentLua.zip
%{pkgdatadir}/SmartIndentPascal.zip
%{pkgdatadir}/SmartIndentPython.zip
%{pkgdatadir}/SmartIndentXML.zip
%{pkgdatadir}/SpellChecker.zip
%{pkgdatadir}/SymTab.zip
%{pkgdatadir}/ThreadSearch.zip
%{pkgdatadir}/tidycmt.zip
%{pkgdatadir}/ToolsPlus.zip
%{pkgdatadir}/Valgrind.zip
%{pkgdatadir}/wxsmith.zip
%{pkgdatadir}/wxSmithAui.zip
%{pkgdatadir}/wxsmithcontribitems.zip
%{pkgdatadir}/images/codesnippets
%{pkgdatadir}/images/DoxyBlocks
%{pkgdatadir}/images/ThreadSearch
%{pkgdatadir}/images/wxsmith
%{pkgdatadir}/lib_finder
%{pkgdatadir}/SpellChecker

%{plugindir}/libabbreviations.so
%{plugindir}/libAutoVersioning.so
%{plugindir}/libBrowseTracker.so
%{plugindir}/libbyogames.so
%{plugindir}/libcb_koders.so
%{plugindir}/libCccc.so
%{plugindir}/libcodesnippets.so
%{plugindir}/libcodestat.so
%{plugindir}/libcopystrings.so
%{plugindir}/libCppCheck.so
%{plugindir}/libCscope.so
%{plugindir}/libdevpakupdater.so
%{plugindir}/libDoxyBlocks.so
%{plugindir}/libdragscroll.so
%{plugindir}/libEditorConfig.so
%{plugindir}/libEditorTweaks.so
%{plugindir}/libenvvars.so
%{plugindir}/libexporter.so
%{plugindir}/libwxPdfDocument.so
%{plugindir}/libFileManager.so
%{plugindir}/libheaderfixup.so
%{plugindir}/libheaderguard.so
%{plugindir}/libhelp_plugin.so
%{plugindir}/libHexEditor.so
%{plugindir}/libIncrementalSearch.so
%{plugindir}/libkeybinder.so
%{plugindir}/liblib_finder.so
%{plugindir}/libloghacker.so
%{plugindir}/libModPoller.so
%{plugindir}/libMouseSap.so
%{plugindir}/libNassiShneiderman.so
%{plugindir}/libProfiler.so
%{plugindir}/libProjectOptionsManipulator.so
%{plugindir}/libRegExTestbed.so
%{plugindir}/libReopenEditor.so
%{plugindir}/libSmartIndentCpp.so
%{plugindir}/libSmartIndentFortran.so
%{plugindir}/libSmartIndentHDL.so
%{plugindir}/libSmartIndentLua.so
%{plugindir}/libSmartIndentPascal.so
%{plugindir}/libSmartIndentPython.so
%{plugindir}/libSmartIndentXML.so
%{plugindir}/libSpellChecker.so
%{plugindir}/libSymTab.so
%{plugindir}/libThreadSearch.so
%{plugindir}/libtidycmt.so
%{plugindir}/libToolsPlus.so
%{plugindir}/libValgrind.so
%{plugindir}/libwxsmith.so
%{plugindir}/libwxSmithAui.so
%{plugindir}/libwxsmithcontribitems.so

%changelog
* Thu Dec  6 2012 Thomas Sailer <t.sailer@axsem.com> - 10.05svn-6
- update to 8644

