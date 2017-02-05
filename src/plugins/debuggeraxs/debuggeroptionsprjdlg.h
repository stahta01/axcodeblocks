/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DEBUGGEROPTIONSPRJDLG_H
#define DEBUGGEROPTIONSPRJDLG_H

#include <wx/intl.h>
#include "configurationpanel.h"
#include <settings.h>

#include "projtargetoptions.h"

class cbProject;
class wxListBox;
class DebuggerAXS;
class CodeBlocksEvent;

class DebuggerOptionsProjectDlg : public cbConfigurationPanel
{
    public:
        DebuggerOptionsProjectDlg(wxWindow* parent, DebuggerAXS* debugger, cbProject* project);
        virtual ~DebuggerOptionsProjectDlg();

        virtual wxString GetTitle() const { return _("Axsem Debugger"); }
        virtual wxString GetBitmapBaseName() const { return _T("debugger"); }
        virtual void OnApply();
        virtual void OnCancel(){}
    protected:
        void OnTargetSel(wxCommandEvent& event);
        void OnAdd(wxCommandEvent& event);
        void OnEdit(wxCommandEvent& event);
        void OnDelete(wxCommandEvent& event);
        void OnUpdateUI(wxUpdateUIEvent& event);
        void OnKeyAdd(wxCommandEvent& event);
        void OnKeyEdit(wxCommandEvent& event);
        void OnKeyDelete(wxCommandEvent& event);
    private:
        void OnBuildTargetRemoved(CodeBlocksEvent& event);
        void OnBuildTargetAdded(CodeBlocksEvent& event);
        void OnBuildTargetRenamed(CodeBlocksEvent& event);
        void LoadCurrentProjectTargetOptions();
        void SaveCurrentProjectTargetOptions();
        void NormalizeKeyList();

        DebuggerAXS* m_pDBG;
        cbProject* m_pProject;
        wxArrayString m_OldPaths;
        ProjectTargetOptionsMap m_CurrentProjectTargetOptions;
        int m_LastTargetSel;
        DECLARE_EVENT_TABLE()
};

#endif // DEBUGGEROPTIONSPRJDLG_H
