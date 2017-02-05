/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include "debuggeroptionsprjdlg.h"
#include "editaxsemkeydlg.h"
#include <wx/intl.h>
#include <wx/xrc/xmlres.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <cbproject.h>
#include <editpathdlg.h>
#include <manager.h>
#include <globals.h>

#include "debuggeraxs.h"

BEGIN_EVENT_TABLE(DebuggerOptionsProjectDlg, wxPanel)
    EVT_UPDATE_UI(-1, DebuggerOptionsProjectDlg::OnUpdateUI)
    EVT_BUTTON(XRCID("btnAdd"), DebuggerOptionsProjectDlg::OnAdd)
    EVT_BUTTON(XRCID("btnEdit"), DebuggerOptionsProjectDlg::OnEdit)
    EVT_BUTTON(XRCID("btnDelete"), DebuggerOptionsProjectDlg::OnDelete)
    EVT_BUTTON(XRCID("btnKeyAdd"), DebuggerOptionsProjectDlg::OnKeyAdd)
    EVT_BUTTON(XRCID("btnKeyEdit"), DebuggerOptionsProjectDlg::OnKeyEdit)
    EVT_BUTTON(XRCID("btnKeyDelete"), DebuggerOptionsProjectDlg::OnKeyDelete)
    EVT_LISTBOX(XRCID("lstTargets"), DebuggerOptionsProjectDlg::OnTargetSel)
END_EVENT_TABLE()

DebuggerOptionsProjectDlg::DebuggerOptionsProjectDlg(wxWindow* parent, DebuggerAXS* debugger, cbProject* project)
    : m_pDBG(debugger),
    m_pProject(project),
    m_LastTargetSel(-1)
{
    wxXmlResource::Get()->LoadPanel(this, parent, _T("pnlDebuggerAXSProjectOptions"));

    m_OldPaths = m_pDBG->GetSearchDirs(project);
    m_CurrentProjectTargetOptions = m_pDBG->GetProjectTargetOptionsMap(project);

    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);
    control->Clear();
    for (size_t i = 0; i < m_OldPaths.GetCount(); ++i)
    {
        control->Append(m_OldPaths[i]);
    }

    control = XRCCTRL(*this, "lstTargets", wxListBox);
    control->Clear();
    control->Append(_("<Project>"));
    for (int i = 0; i < project->GetBuildTargetsCount(); ++i)
    {
        control->Append(project->GetBuildTarget(i)->GetTitle());
    }
    control->SetSelection(-1);

    LoadCurrentProjectTargetOptions();
    Manager::Get()->RegisterEventSink(cbEVT_BUILDTARGET_REMOVED, new cbEventFunctor<DebuggerOptionsProjectDlg, CodeBlocksEvent>(this, &DebuggerOptionsProjectDlg::OnBuildTargetRemoved));
    Manager::Get()->RegisterEventSink(cbEVT_BUILDTARGET_ADDED, new cbEventFunctor<DebuggerOptionsProjectDlg, CodeBlocksEvent>(this, &DebuggerOptionsProjectDlg::OnBuildTargetAdded));
    Manager::Get()->RegisterEventSink(cbEVT_BUILDTARGET_RENAMED, new cbEventFunctor<DebuggerOptionsProjectDlg, CodeBlocksEvent>(this, &DebuggerOptionsProjectDlg::OnBuildTargetRenamed));
}

DebuggerOptionsProjectDlg::~DebuggerOptionsProjectDlg()
{
    Manager::Get()->RemoveAllEventSinksFor(this);
}

void DebuggerOptionsProjectDlg::OnBuildTargetRemoved(CodeBlocksEvent& event)
{
    cbProject* project = event.GetProject();
    if(project != m_pProject)
    {
        return;
    }
    wxString theTarget = event.GetBuildTargetName();
    for (ProjectTargetOptionsMap::iterator it = m_CurrentProjectTargetOptions.begin(); it != m_CurrentProjectTargetOptions.end(); ++it)
    {
        // find our target
        if ( !it->first || it->first->GetTitle() != theTarget)
            continue;

        m_CurrentProjectTargetOptions.erase(it);
        // if we erased it, just break, there can only be one map per target
        break;
    }
    wxListBox* lstBox = XRCCTRL(*this, "lstTargets", wxListBox);
    int idx = lstBox->FindString(theTarget);
    if (idx > 0)
    {
        lstBox->Delete(idx);
    }
    if((size_t)idx >= lstBox->GetCount())
    {
        idx--;
    }
    lstBox->SetSelection(idx);
    LoadCurrentProjectTargetOptions();
}

void DebuggerOptionsProjectDlg::OnBuildTargetAdded(CodeBlocksEvent& event)
{
    cbProject* project = event.GetProject();
    if(project != m_pProject)
    {
        return;
    }
    wxString newTarget = event.GetBuildTargetName();
    wxString oldTarget = event.GetOldBuildTargetName();
    if(!oldTarget.IsEmpty())
    {
        for (ProjectTargetOptionsMap::iterator it = m_CurrentProjectTargetOptions.begin(); it != m_CurrentProjectTargetOptions.end(); ++it)
        {
            // find our target
            if ( !it->first || it->first->GetTitle() != oldTarget)
                continue;
            ProjectBuildTarget* bt = m_pProject->GetBuildTarget(newTarget);
            if(bt)
                m_CurrentProjectTargetOptions.insert(m_CurrentProjectTargetOptions.end(), std::make_pair(bt, it->second));
            // if we inserted it, just break, there can only be one map per target
            break;
        }
    }
    wxListBox* lstBox = XRCCTRL(*this, "lstTargets", wxListBox);
    int idx = lstBox->FindString(newTarget);
    if (idx == wxNOT_FOUND)
    {
        idx = lstBox->Append(newTarget);
    }
    lstBox->SetSelection(idx);
    LoadCurrentProjectTargetOptions();
}

void DebuggerOptionsProjectDlg::OnBuildTargetRenamed(CodeBlocksEvent& event)
{
    cbProject* project = event.GetProject();
    if(project != m_pProject)
    {
        return;
    }
    wxString newTarget = event.GetBuildTargetName();
    wxString oldTarget = event.GetOldBuildTargetName();
    for (ProjectTargetOptionsMap::iterator it = m_CurrentProjectTargetOptions.begin(); it != m_CurrentProjectTargetOptions.end(); ++it)
    {
        // find our target
        if ( !it->first || it->first->GetTitle() != oldTarget)
            continue;
        it->first->SetTitle(newTarget);
        // if we renamed it, just break, there can only be one map per target
        break;
    }

    wxListBox* lstBox = XRCCTRL(*this, "lstTargets", wxListBox);
    int idx = lstBox->FindString(oldTarget);
    if (idx == wxNOT_FOUND)
    {
        return;
    }
    lstBox->SetString(idx, newTarget);
    lstBox->SetSelection(idx);
    LoadCurrentProjectTargetOptions();
}

void DebuggerOptionsProjectDlg::LoadCurrentProjectTargetOptions()
{
    wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
    additionalKeys->Clear();

    // -1 because entry 0 is "<Project>"
    m_LastTargetSel = XRCCTRL(*this, "lstTargets", wxListBox)->GetSelection() - 1;
    ProjectBuildTarget* bt = m_pProject->GetBuildTarget(m_LastTargetSel);
    if (m_CurrentProjectTargetOptions.find(bt) != m_CurrentProjectTargetOptions.end())
    {
        ProjectTargetOptions& pto = m_CurrentProjectTargetOptions[bt];
        XRCCTRL(*this, "cmbFlashErase", wxChoice)->SetSelection((int)pto.flashErase);
        XRCCTRL(*this, "chkFillBreakpoints", wxCheckBox)->SetValue(pto.fillBreakpoints);
        XRCCTRL(*this, "txtKey", wxTextCtrl)->SetValue(EditAxsemKeyDlg::key_to_str(pto.key));
        for (ProjectTargetOptions::additionalKeys_t::const_iterator ki(pto.additionalKeys.begin()), ke(pto.additionalKeys.end()); ki != ke; ++ki) {
            if (*ki == ProjectTargetOptions::defaultKey || *ki == pto.key)
                continue;
            additionalKeys->Append(EditAxsemKeyDlg::key_to_str(*ki));
        }
    }
    else
    {
        XRCCTRL(*this, "cmbFlashErase", wxChoice)->SetSelection(0);
        XRCCTRL(*this, "chkFillBreakpoints", wxCheckBox)->SetValue(false);
        XRCCTRL(*this, "txtKey", wxTextCtrl)->SetValue(wxEmptyString);
    }
}

void DebuggerOptionsProjectDlg::SaveCurrentProjectTargetOptions()
{
//  if (m_LastTargetSel == -1)
//      return;

    ProjectBuildTarget* bt = m_pProject->GetBuildTarget(m_LastTargetSel);
//  if (!bt)
//      return;

    ProjectTargetOptionsMap::iterator it = m_CurrentProjectTargetOptions.find(bt);
    if (it == m_CurrentProjectTargetOptions.end())
        it = m_CurrentProjectTargetOptions.insert(m_CurrentProjectTargetOptions.end(), std::make_pair(bt, ProjectTargetOptions()));

    ProjectTargetOptions& pto = it->second;

    pto.flashErase = (ProjectTargetOptions::FlashEraseType)XRCCTRL(*this, "cmbFlashErase", wxChoice)->GetSelection();
    pto.fillBreakpoints = XRCCTRL(*this, "chkFillBreakpoints", wxCheckBox)->GetValue();
    {
        std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(XRCCTRL(*this, "txtKey", wxTextCtrl)->GetValue()));
        if (!k.second)
            k.first = ProjectTargetOptions::defaultKey;
        pto.key = k.first;
    }
    wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
    pto.additionalKeys.clear();
    for (unsigned int i = 0; i < additionalKeys->GetCount(); ++i) {
        std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(additionalKeys->GetString(i)));
        if (!k.second || k.first == ProjectTargetOptions::defaultKey || k.first == pto.key)
            continue;
        pto.additionalKeys.insert(k.first);
    }
}

void DebuggerOptionsProjectDlg::OnTargetSel(wxCommandEvent& WXUNUSED(event))
{
    SaveCurrentProjectTargetOptions();
    LoadCurrentProjectTargetOptions();
}

void DebuggerOptionsProjectDlg::OnAdd(wxCommandEvent& WXUNUSED(event))
{
    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);

    EditPathDlg dlg(this,
            m_pProject ? m_pProject->GetBasePath() : _T(""),
            m_pProject ? m_pProject->GetBasePath() : _T(""),
            _("Add directory"));

    PlaceWindow(&dlg);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString path = dlg.GetPath();
        control->Append(path);
    }
}

void DebuggerOptionsProjectDlg::OnEdit(wxCommandEvent& WXUNUSED(event))
{
    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);
    int sel = control->GetSelection();
    if (sel < 0)
        return;

    EditPathDlg dlg(this,
            control->GetString(sel),
            m_pProject ? m_pProject->GetBasePath() : _T(""),
            _("Edit directory"));

    PlaceWindow(&dlg);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString path = dlg.GetPath();
        control->SetString(sel, path);
    }
}

void DebuggerOptionsProjectDlg::OnDelete(wxCommandEvent& WXUNUSED(event))
{
    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);
    int sel = control->GetSelection();
    if (sel < 0)
        return;

    control->Delete(sel);
}

void DebuggerOptionsProjectDlg::OnUpdateUI(wxUpdateUIEvent& WXUNUSED(event))
{
    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);
    bool en = control->GetSelection() >= 0;

    XRCCTRL(*this, "btnEdit", wxButton)->Enable(en);
    XRCCTRL(*this, "btnDelete", wxButton)->Enable(en);

    en = XRCCTRL(*this, "lstTargets", wxListBox)->GetSelection() != wxNOT_FOUND;

    XRCCTRL(*this, "cmbFlashErase", wxChoice)->Enable(en);
    XRCCTRL(*this, "chkFillBreakpoints", wxCheckBox)->Enable(en);
    XRCCTRL(*this, "txtKey", wxTextCtrl)->Enable(en);
    XRCCTRL(*this, "lstAdditionalKeys", wxListBox)->Enable(en);
}

void DebuggerOptionsProjectDlg::OnApply()
{
    wxListBox* control = XRCCTRL(*this, "lstSearchDirs", wxListBox);

    m_OldPaths.Clear();
    for (int i = 0; i < (int)control->GetCount(); ++i)
    {
        m_OldPaths.Add(control->GetString(i));
    }

    SaveCurrentProjectTargetOptions();

    m_pDBG->GetSearchDirs(m_pProject) = m_OldPaths;
    m_pDBG->GetProjectTargetOptionsMap(m_pProject) = m_CurrentProjectTargetOptions;
}

void DebuggerOptionsProjectDlg::OnKeyDelete(wxCommandEvent& event)
{
    wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
    int sel = additionalKeys->GetSelection();
    if (sel < 0)
        return;
    additionalKeys->Delete(sel);
}

void DebuggerOptionsProjectDlg::OnKeyEdit(wxCommandEvent& event)
{
    wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
    int sel = additionalKeys->GetSelection();
    if (sel < 0)
        return;
    std::pair<uint64_t,bool> key(EditAxsemKeyDlg::str_to_key(additionalKeys->GetString(sel)));
    if (!key.second)
        key.first = ProjectTargetOptions::defaultKey;
    EditAxsemKeyDlg dlg(this, key.first, _T("Add new key..."));
    PlaceWindow(&dlg);
    if (dlg.ShowModal() == wxID_OK)
    {
        if (key.first != ProjectTargetOptions::defaultKey) {
            additionalKeys->SetString(sel, EditAxsemKeyDlg::key_to_str(key.first));
            NormalizeKeyList();
        } else {
            additionalKeys->Delete(sel);
        }
    }
}

void DebuggerOptionsProjectDlg::OnKeyAdd(wxCommandEvent& event)
{
    uint64_t key(ProjectTargetOptions::defaultKey);
    EditAxsemKeyDlg dlg(this, key, _T("Add new key..."));
    PlaceWindow(&dlg);
    if (dlg.ShowModal() == wxID_OK && key != ProjectTargetOptions::defaultKey)
    {
        wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
        additionalKeys->Append(EditAxsemKeyDlg::key_to_str(key));
        NormalizeKeyList();
    }
}

void DebuggerOptionsProjectDlg::NormalizeKeyList()
{
    wxListBox* additionalKeys = XRCCTRL(*this, "lstAdditionalKeys", wxListBox);
    std::set<uint64_t> ks;
    {
        unsigned int i(additionalKeys->GetCount());
        while (i > 0) {
            --i;
            std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(additionalKeys->GetString(i)));
            if (!k.second || k.first == ProjectTargetOptions::defaultKey)
                continue;
            ks.insert(k.first);
        }
    }
    additionalKeys->Clear();
    for (std::set<uint64_t>::const_iterator ki(ks.begin()), ke(ks.end()); ki != ke; ++ki)
        additionalKeys->Append(EditAxsemKeyDlg::key_to_str(*ki));
}
