/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include <prep.h>
#include "compilerIAR.h"
#include <wx/dir.h>
#include <wx/intl.h>
#include <wx/regex.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/msgdlg.h>

#ifdef __WXMSW__
    #include <wx/msw/registry.h>
#endif

#define FEDORA

CompilerIAR::CompilerIAR(wxString arch)
    : Compiler(_("IAR ") + arch + _(" Compiler"), _T("iar") + arch)
{
    m_Weight = 75;
    m_Arch = arch;
    Reset();
}

CompilerIAR::~CompilerIAR()
{
    //dtor
}

Compiler * CompilerIAR::CreateCopy()
{
    return (new CompilerIAR(*this));
}

AutoDetectResult CompilerIAR::AutoDetectInstallationDir()
{
    wxString axsdb;
    if (platform::windows)
    {
        m_MasterPath.Clear();
#ifdef __WXMSW__ // for wxRegKey
        wxString iarversion;
        wxRegKey key;   // defaults to HKCR
        static const wxString regroot[2] = {
            wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\IAR Systems\\Embedded Workbench"),
            wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\IAR Systems\\Embedded Workbench")
        };
        for (unsigned int rootidx = 0; rootidx < sizeof(regroot)/sizeof(regroot[0]); ++rootidx)
        {
            key.SetName(regroot[rootidx]);
            if (key.Exists() && key.Open(wxRegKey::Read))
            {
                wxString subkeyname;
                long idx;
                if (key.GetFirstKey(subkeyname, idx))
                {
                    do
                    {
                         wxRegKey keys;
                        keys.SetName(key.GetName() + wxFILE_SEP_PATH + subkeyname + wxFILE_SEP_PATH + wxT("EW") + m_Arch);
                        if (!keys.Exists() || !keys.Open(wxRegKey::Read))
                            continue;
                        wxString subkeyversion;
                        long idxv;
                        if (keys.GetFirstKey(subkeyversion, idxv))
                        {
                            do
                            {
                                wxRegKey keyv;
                                keyv.SetName(keys.GetName() + wxFILE_SEP_PATH + subkeyversion);
                                if (!keyv.Exists() || !keyv.Open(wxRegKey::Read))
                                    continue;
                                wxString masterpath;
                                keyv.QueryValue(wxT("InstallPath"), masterpath);
                                if (masterpath.IsEmpty())
                                    continue;
                                masterpath += wxFILE_SEP_PATH + m_Arch;
                                if (!wxFileExists(masterpath + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + m_Programs.C))
                                    continue;
                                if (!m_MasterPath.IsEmpty() && subkeyversion < iarversion)
                                    continue;
                                m_MasterPath = masterpath;
                                iarversion = subkeyversion;
                            } while (keys.GetNextKey(subkeyversion, idxv));
                        }
                    } while (key.GetNextKey(subkeyname, idx));
                }
            }
        }
        wxRegKey keyaxsdb;
        keyaxsdb.SetName(wxT("HKEY_LOCAL_MACHINE\\Software\\AXSEM\\AXSDB"));
        if (keyaxsdb.Exists() && key.Open(wxRegKey::Read)) {
            keyaxsdb.QueryValue(wxT("InstallDir"), axsdb);
        } else {
            wxRegKey keyaxsdb;
            keyaxsdb.SetName(wxT("HKEY_CURRENT_USER\\Software\\AXSEM\\AXSDB"));
            if (keyaxsdb.Exists() && key.Open(wxRegKey::Read))
                keyaxsdb.QueryValue(wxT("InstallDir"), axsdb);
        }
#endif // __WXMSW__
        wxString env_path = wxGetenv(_T("ProgramFiles(x86)"));
        if (m_MasterPath.IsEmpty())
        {
            wxDir dir(env_path + wxT("\\IAR Systems"));
            if (wxDirExists(dir.GetName()) && dir.IsOpened())
            {
                wxString filename;
                bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
                while (cont)
                {
                    if ( filename.StartsWith(wxT("Embedded Workbench")) )
                    {
                        wxFileName fn(dir.GetName() + wxFILE_SEP_PATH + filename + wxFILE_SEP_PATH +
                                      m_Arch + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + m_Programs.C);
                        if (   wxFileName::IsFileExecutable(fn.GetFullPath())
                            && (m_MasterPath.IsEmpty() || fn.GetPath() > m_MasterPath) )
                        {
                            m_MasterPath = dir.GetName() + wxFILE_SEP_PATH + filename + wxFILE_SEP_PATH + m_Arch;
                        }
                    }
                    cont = dir.GetNext(&filename);
                }
            }
        }
        if (m_MasterPath.IsEmpty())
        {
            // just a guess; the default installation dir
            m_MasterPath = env_path + wxT("\\IAR Systems\\Embedded Workbench\\" + m_Arch);
        }

        if ( wxDirExists(m_MasterPath) )
        {
            AddIncludeDir(m_MasterPath + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(m_MasterPath + wxFILE_SEP_PATH + wxT("lib") + wxFILE_SEP_PATH + wxT("clib"));
            m_ExtraPaths.Add(m_MasterPath + wxFILE_SEP_PATH + wxT("bin"));
        }
    }
    else
    {
        m_MasterPath=_T("/usr/local"); // default
#ifdef FEDORA
        m_MasterPath=_T("/usr");
#endif
#ifdef __WXGTK__
        axsdb = _T("/usr/share/microfoot");
#endif
    }
    if (m_Arch == wxT("8051"))
    {
        if ( wxDirExists(axsdb) )
        {
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libmf") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libmf") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libmfcrypto") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libmfcrypto") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdvk2") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdvk2") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5031") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5031") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5042") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5042") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5043") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5043") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5051") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5051") + wxFILE_SEP_PATH + wxT("iar"));
            AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdsp") + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdsp") + wxFILE_SEP_PATH + wxT("iar"));
        }
    }
    else // IAR
    {
        AddCompilerOption(wxT("--no_wrap_diagnostics"));
    }
    return wxFileExists(m_MasterPath + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + m_Programs.C) ? adrDetected : adrGuessed;
}
