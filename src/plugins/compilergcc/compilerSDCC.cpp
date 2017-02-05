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
#include "compilerSDCC.h"
#include <wx/intl.h>
#include <wx/regex.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>

#ifdef __WXMSW__
    #include <wx/msw/registry.h>
#endif

#define FEDORA

CompilerSDCC::CompilerSDCC()
    : Compiler(_("SDCC Compiler"), _T("sdcc"))
{
    Reset();
}

CompilerSDCC::~CompilerSDCC()
{
    //dtor
}

Compiler * CompilerSDCC::CreateCopy()
{
    return (new CompilerSDCC(*this));
}

AutoDetectResult CompilerSDCC::AutoDetectInstallationDir()
{
    wxString axsdb;
    if (platform::windows)
    {
#ifdef __WXMSW__ // for wxRegKey
        wxRegKey key;   // defaults to HKCR
        key.SetName(wxT("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SDCC"));
        bool keyfound(false);
        if (key.Exists() && key.Open(wxRegKey::Read)) // found; read it
            keyfound = key.QueryValue(wxT("UninstallString"), m_MasterPath);
        if (!keyfound) {
            key.SetName(wxT("HKEY_LOCAL_MACHINE\\Software\\SDCC"));
            if (key.Exists() && key.Open(wxRegKey::Read)) // found; read it
                keyfound = key.QueryValue(wxT(""), m_MasterPath);
        }
        wxRegKey keyaxsdb;
        keyfound = false;
        keyaxsdb.SetName(wxT("HKEY_LOCAL_MACHINE\\Software\\AXSEM\\AXSDB"));
        if (keyaxsdb.Exists() && key.Open(wxRegKey::Read))
            keyfound = keyaxsdb.QueryValue(wxT("InstallDir"), axsdb);
        if (!keyfound) {
            wxRegKey keyaxsdb;
            keyaxsdb.SetName(wxT("HKEY_CURRENT_USER\\Software\\AXSEM\\AXSDB"));
            if (keyaxsdb.Exists() && key.Open(wxRegKey::Read))
                keyfound = keyaxsdb.QueryValue(wxT("InstallDir"), axsdb);
        }
#endif

        if (m_MasterPath.IsEmpty()) {
            wxString path;
            if (wxGetEnv(wxT("PATH"), &path)) {
                wxStringTokenizer pathtok(path, wxT(";"), wxTOKEN_STRTOK);
                while (pathtok.HasMoreTokens()) {
                    wxFileName fn(pathtok.GetNextToken(), wxT(""));
		    {
                        const wxArrayString& dirs(fn.GetDirs());
                        if (!dirs.Count() || dirs.Last().CmpNoCase(wxT("bin")))
                            continue;
                    }
                    fn.RemoveLastDir();
                    if (!wxFileExists(fn.GetFullPath() + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + m_Programs.C))
                        continue;
                    m_MasterPath = fn.GetFullPath();
                    break;
                }
            }
            // just a guess; the default installation dir
            if (m_MasterPath.IsEmpty())
                m_MasterPath = wxT("C:\\sdcc");
        } else {
            wxFileName fn(m_MasterPath);
            m_MasterPath = fn.GetPath();
        }

        if (!m_MasterPath.IsEmpty())
        {
            AddIncludeDir(m_MasterPath + wxFILE_SEP_PATH + wxT("include"));
            AddLibDir(m_MasterPath + wxFILE_SEP_PATH + wxT("lib"));
            m_ExtraPaths.Add(m_MasterPath + wxFILE_SEP_PATH + wxT("bin"));
        }

        if (axsdb.IsEmpty()) {
            wxString path;
            if (wxGetEnv(wxT("PATH"), &path)) {
                wxStringTokenizer pathtok(path, wxT(";"), wxTOKEN_STRTOK);
                while (pathtok.HasMoreTokens()) {
                    wxFileName fn(pathtok.GetNextToken(), wxT(""));
		    {
                        const wxArrayString& dirs(fn.GetDirs());
                        if (!dirs.Count() || dirs.Last().CmpNoCase(wxT("bin")))
                            continue;
                    }
                    fn.RemoveLastDir();
                    if (!wxFileExists(fn.GetFullPath() + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + wxT("axsdb.exe")))
                        continue;
                    axsdb = fn.GetFullPath();
                    break;
                }
            }
        }
    }
    else
    {
        m_MasterPath = _T("/usr/local/bin"); // default
#ifdef FEDORA
        m_MasterPath = _T("/usr");
#endif
#ifdef __WXGTK__
        axsdb = _T("/usr/share/microfoot");
#endif
    }

    if (!axsdb.IsEmpty())
    {
        // libreent does not have includes (it contains only compiler helper functions)
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libreent") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libmf") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libmf") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libmfcrypto") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libmfcrypto") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdvk2") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdvk2") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5031") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5031") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5042") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5042") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5043") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5043") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libax5051") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libax5051") + wxFILE_SEP_PATH + wxT("sdcc"));
        AddIncludeDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdsp") + wxFILE_SEP_PATH + wxT("include"));
        AddLibDir(axsdb + wxFILE_SEP_PATH + wxT("libaxdsp") + wxFILE_SEP_PATH + wxT("sdcc"));
    }

    return wxFileExists(m_MasterPath + wxFILE_SEP_PATH + wxT("bin") + wxFILE_SEP_PATH + m_Programs.C) ? adrDetected : adrGuessed;
}
