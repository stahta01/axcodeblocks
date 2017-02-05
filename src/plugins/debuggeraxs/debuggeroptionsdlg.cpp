/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include "debuggeroptionsdlg.h"
#ifndef CB_PRECOMP
    #include <wx/checkbox.h>
    #include <wx/choice.h>
    #include <wx/filedlg.h>
    #include <wx/intl.h>
    #include <wx/radiobox.h>
    #include <wx/spinctrl.h>
    #include <wx/textctrl.h>
    #include <wx/xrc/xmlres.h>

    #include <configmanager.h>
    #include <macrosmanager.h>
#endif

#ifdef __WXMSW__
    #include <wx/msw/registry.h>
#endif


#include "debuggeraxs.h"

class DebuggerConfigurationPanel : public wxPanel
{
    public:
        void ValidateExecutablePath()
        {
            wxTextCtrl *pathCtrl = XRCCTRL(*this, "txtExecutablePath", wxTextCtrl);
            wxString path = pathCtrl->GetValue();
            Manager::Get()->GetMacrosManager()->ReplaceEnvVars(path);
            if (!wxFileExists(path))
            {
                pathCtrl->SetForegroundColour(*wxWHITE);
                pathCtrl->SetBackgroundColour(*wxRED);
                pathCtrl->SetToolTip(_("Full path to the debugger's executable. Executable can't be found on the filesystem!"));
            }
            else
            {
                pathCtrl->SetForegroundColour(wxNullColour);
                pathCtrl->SetBackgroundColour(wxNullColour);
                pathCtrl->SetToolTip(_("Full path to the debugger's executable."));
            }
            pathCtrl->Refresh();
        }
    private:
        void OnBrowse(wxCommandEvent &event)
        {
            wxString oldPath = XRCCTRL(*this, "txtExecutablePath", wxTextCtrl)->GetValue();
            Manager::Get()->GetMacrosManager()->ReplaceEnvVars(oldPath);
            wxFileDialog dlg(this, _("Select executable file"), wxEmptyString, oldPath,
                             wxFileSelectorDefaultWildcardStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            PlaceWindow(&dlg);
            if (dlg.ShowModal() == wxID_OK)
            {
                wxString newPath = dlg.GetPath();
                XRCCTRL(*this, "txtExecutablePath", wxTextCtrl)->ChangeValue(newPath);
            }
        }

        void OnTextChange(wxCommandEvent &event)
        {
            ValidateExecutablePath();
        }
    private:
        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DebuggerConfigurationPanel, wxPanel)
    EVT_BUTTON(XRCID("btnBrowse"), DebuggerConfigurationPanel::OnBrowse)
    EVT_TEXT(XRCID("txtExecutablePath"), DebuggerConfigurationPanel::OnTextChange)
END_EVENT_TABLE()

DebuggerConfiguration::DebuggerConfiguration(const ConfigManagerWrapper &config) : cbDebuggerConfiguration(config)
{
}

cbDebuggerConfiguration* DebuggerConfiguration::Clone() const
{
    return new DebuggerConfiguration(*this);
}

wxPanel* DebuggerConfiguration::MakePanel(wxWindow *parent)
{
    DebuggerConfigurationPanel *panel = new DebuggerConfigurationPanel;
    if (!wxXmlResource::Get()->LoadPanel(panel, parent, wxT("dlgAXSDebuggerOptions")))
        return panel;

    XRCCTRL(*panel, "txtExecutablePath", wxTextCtrl)->ChangeValue(GetDebuggerExecutable(false));
    panel->ValidateExecutablePath();

    XRCCTRL(*panel, "txtInit",           wxTextCtrl)->ChangeValue(GetInitCommands());
    XRCCTRL(*panel, "chkCmdsSync",       wxCheckBox)->SetValue(GetFlag(CommandsSynchronous));
    XRCCTRL(*panel, "chkTooltipEval",    wxCheckBox)->SetValue(GetFlag(EvalExpression));
    XRCCTRL(*panel, "chkAddForeignDirs", wxCheckBox)->SetValue(GetFlag(AddOtherProjectDirs));
    XRCCTRL(*panel, "chkDoNotRun",       wxCheckBox)->SetValue(GetFlag(DoNotRun));
    XRCCTRL(*panel, "chkLiveUpdate",     wxCheckBox)->SetValue(GetFlag(LiveUpdate));

    for (axs_cbDbgLink::FuncKey_t f = axs_cbDbgLink::FuncKey_First; f <= axs_cbDbgLink::FuncKey_Last; f = (axs_cbDbgLink::FuncKey_t)(f + 1))
    {
        wxTextCtrl *txt(wxStaticCast(panel->FindWindow(wxXmlResource::GetXRCID(FuncKeyFieldName[f - axs_cbDbgLink::FuncKey_First])), wxTextCtrl));
        txt->ChangeValue(GetDebugLinkFunctionKey(f, false));
    }

    return panel;
}

bool DebuggerConfiguration::SaveChanges(wxPanel *panel)
{
    m_config.Write(wxT("executable_path"),       XRCCTRL(*panel, "txtExecutablePath", wxTextCtrl)->GetValue());
    m_config.Write(wxT("init_commands"),         XRCCTRL(*panel, "txtInit",           wxTextCtrl)->GetValue());
    m_config.Write(wxT("commands_synchronous"),  XRCCTRL(*panel, "chkCmdsSync",       wxCheckBox)->GetValue());
    m_config.Write(wxT("eval_tooltip"),          XRCCTRL(*panel, "chkTooltipEval",    wxCheckBox)->GetValue());
    m_config.Write(wxT("add_other_search_dirs"), XRCCTRL(*panel, "chkAddForeignDirs", wxCheckBox)->GetValue());
    m_config.Write(wxT("do_not_run"),            XRCCTRL(*panel, "chkDoNotRun",       wxCheckBox)->GetValue());
    m_config.Write(wxT("live_update"),           XRCCTRL(*panel, "chkLiveUpdate",     wxCheckBox)->GetValue());

    for (axs_cbDbgLink::FuncKey_t f = axs_cbDbgLink::FuncKey_First; f <= axs_cbDbgLink::FuncKey_Last; f = (axs_cbDbgLink::FuncKey_t)(f + 1))
    {
        wxTextCtrl *txt(wxStaticCast(panel->FindWindow(wxXmlResource::GetXRCID(FuncKeyFieldName[f - axs_cbDbgLink::FuncKey_First])), wxTextCtrl));
        m_config.Write(FuncKeyXMLName[f - axs_cbDbgLink::FuncKey_First], txt->GetValue());
    }

    return true;
}

bool DebuggerConfiguration::GetFlag(Flags flag)
{
    switch (flag)
    {
        case CommandsSynchronous:
            return m_config.ReadBool(wxT("commands_synchronous"), false);
        case EvalExpression:
            return m_config.ReadBool(wxT("eval_tooltip"), false);
        case AddOtherProjectDirs:
            return m_config.ReadBool(wxT("add_other_search_dirs"), false);
        case DoNotRun:
            return m_config.ReadBool(wxT("do_not_run"), false);
        case LiveUpdate:
            return m_config.ReadBool(wxT("live_update"), false);
        default:
            return false;
    }
    return false;
}

// TODO(tpetrov#): move this to the SDK and use it in the compiler plugin
wxString cbFindFileInPATH(const wxString &filename)
{
    wxString pathValues;
    wxGetEnv(_T("PATH"), &pathValues);
    if (pathValues.empty())
        return wxEmptyString;

    const wxString &sep = platform::windows ? _T(";") : _T(":");
    wxChar pathSep = wxFileName::GetPathSeparator();
    const wxArrayString &pathArray = GetArrayFromString(pathValues, sep);
    for (size_t i = 0; i < pathArray.GetCount(); ++i)
    {
        if (wxFileExists(pathArray[i] + pathSep + filename))
        {
            if (pathArray[i].AfterLast(pathSep).IsSameAs(_T("bin")))
                return pathArray[i];
        }
    }
    return wxEmptyString;
}

wxString DetectDebuggerExecutable()
{
    wxString exeName(platform::windows ? wxT("axsdb.exe") : wxT("axsdb"));
    wxString exePath = cbFindFileInPATH(exeName);
    wxChar sep = wxFileName::GetPathSeparator();

    if (exePath.empty())
    {
        #ifdef __WXMSW__
        if (platform::windows)
        {
            wxRegKey axskey(wxRegKey::HKLM, wxT("SOFTWARE\\AXSEM\\AXSDB"));
	    if (axskey.Exists() && axskey.Open(wxRegKey::Read)) {
		    axskey.QueryValue(wxT("InstallDir"), exePath);
	    } else {
		    wxRegKey axskey2(wxRegKey::HKCU, wxT("SOFTWARE\\AXSEM\\AXSDB"));
		    if (axskey2.Exists() && axskey2.Open(wxRegKey::Read))
			    axskey2.QueryValue(wxT("InstallDir"), exePath);
            }
        }
        #else
        if (!platform::windows)
            exePath = wxT("/usr/bin/axsdb");
        #endif
    }

    return exePath + wxFileName::GetPathSeparator() + exeName;
}

wxString DebuggerConfiguration::GetDebuggerExecutable(bool expandMarco)
{
    wxString result = m_config.Read(wxT("executable_path"), wxEmptyString);
    if (expandMarco)
        Manager::Get()->GetMacrosManager()->ReplaceEnvVars(result);
    return !result.empty() ? result : DetectDebuggerExecutable();
}

wxString DebuggerConfiguration::GetInitCommands()
{
    return m_config.Read(wxT("init_commands"), wxEmptyString);
}

wxString DebuggerConfiguration::GetDebugLinkFunctionKey(axs_cbDbgLink::FuncKey_t key, bool expandEscape)
{
    wxString result;
    if (key < axs_cbDbgLink::FuncKey_First || key > axs_cbDbgLink::FuncKey_Last)
        return result;
    result = m_config.Read(FuncKeyXMLName[key - axs_cbDbgLink::FuncKey_First], FuncKeyDefault[key - axs_cbDbgLink::FuncKey_First]);
    if (!expandEscape)
        return result;
    wxString result1;
    for (int i = 0; i < result.Len(); )
    {
        if (result[i] != '\\')
        {
            result1 += result[i++];
            continue;
        }
        ++i;
        if (i >= result.Len())
            break;
        wxChar ch = result[i++];
        switch (ch)
        {
            case 'b':
                result1 += (wxChar)8;
                continue;

            case 't':
                result1 += (wxChar)9;
                continue;

            case 'n':
                result1 += (wxChar)10;
                continue;

            case 'f':
                result1 += (wxChar)12;
                continue;

            case 'r':
                result1 += (wxChar)13;
                continue;

            case 'x':
            {
                wxChar ch1(0);
                for (int j = 0; j < 2; ++j)
                {
                    if (i >= result.Len())
                        break;
                    ch = result[i];
                    if (ch >= (wxChar)'0' && ch <= (wxChar)'9')
                    {
                        ++i;
                        ch1 <<= 4;
                        ch1 |= ch - (wxChar)'0';
                        continue;
                    }
                    if (ch >= (wxChar)'A' && ch <= (wxChar)'F')
                    {
                        ++i;
                        ch1 <<= 4;
                        ch1 |= ch - (wxChar)'A' + 10;
                        continue;
                    }
                    if (ch >= (wxChar)'a' && ch <= (wxChar)'f')
                    {
                        ++i;
                        ch1 <<= 4;
                        ch1 |= ch - (wxChar)'a' + 10;
                        continue;
                    }
                    break;
                }
                result1 += ch1;
                continue;
            }

            default:
                break;
        }
        if (ch < (wxChar)'0' || ch > (wxChar)'7')
            continue;
        {
            wxChar ch1(ch - (wxChar)'0');
            for (int j = 0; j < 2; ++j)
            {
                if (i >= result.Len())
                    break;
                ch = result[i];
                if (ch < (wxChar)'0' || ch > (wxChar)'7')
                    break;
                ++i;
                ch1 <<= 3;
                ch1 |= ch - (wxChar)'0';
            }
            result1 += ch1;
        }
    }
    return result1;
}

const wxString DebuggerConfiguration::FuncKeyXMLName[] =
{
    wxT("funckey_f1"),
    wxT("funckey_f2"),
    wxT("funckey_f3"),
    wxT("funckey_f4"),
    wxT("funckey_f5"),
    wxT("funckey_f6"),
    wxT("funckey_f7"),
    wxT("funckey_f8"),
    wxT("funckey_f9"),
    wxT("funckey_f10"),
    wxT("funckey_f11"),
    wxT("funckey_f12"),
    wxT("funckey_up"),
    wxT("funckey_down"),
    wxT("funckey_left"),
    wxT("funckey_right"),
    wxT("funckey_pgup"),
    wxT("funckey_pgdn"),
    wxT("funckey_home"),
    wxT("funckey_end"),
    wxT("funckey_insert"),
    wxT("funckey_delete")
};

const wxString DebuggerConfiguration::FuncKeyFieldName[] =
{
    wxT("txtDbgLinkF1"),
    wxT("txtDbgLinkF2"),
    wxT("txtDbgLinkF3"),
    wxT("txtDbgLinkF4"),
    wxT("txtDbgLinkF5"),
    wxT("txtDbgLinkF6"),
    wxT("txtDbgLinkF7"),
    wxT("txtDbgLinkF8"),
    wxT("txtDbgLinkF9"),
    wxT("txtDbgLinkF10"),
    wxT("txtDbgLinkF11"),
    wxT("txtDbgLinkF12"),
    wxT("txtDbgLinkUp"),
    wxT("txtDbgLinkDown"),
    wxT("txtDbgLinkLeft"),
    wxT("txtDbgLinkRight"),
    wxT("txtDbgLinkPgUp"),
    wxT("txtDbgLinkPgDn"),
    wxT("txtDbgLinkHome"),
    wxT("txtDbgLinkEnd"),
    wxT("txtDbgLinkIns"),
    wxT("txtDbgLinkDel")
};

const wxString DebuggerConfiguration::FuncKeyDefault[] =
{
    wxT("Help"),
    wxT("Axsem AG"),
    wxT(""),
    wxT(""),
    wxT("AX8052F100"),
    wxT("AX8052F143"),
    wxT("AX8052F151"),
    wxT("AX8052F131"),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT(""),
    wxT("\\002"),
    wxT("\\003"),
    wxT(""),
    wxT("\\177")
};
