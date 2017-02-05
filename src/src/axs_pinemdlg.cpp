/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk_precomp.h"
#ifndef CB_PRECOMP
    #include <wx/dnd.h>
    #include <wx/menu.h>
    #include <wx/sizer.h>

    #include "cbexception.h"
    #include "cbplugin.h"
    #include "logmanager.h"
    #include "scrollingdialog.h"
#endif

#include "debuggermanager.h"

#include <map>
#include <wx/propgrid/propgrid.h>

#include "axs_pinemdlg.h"
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/intl.h>
#include <wx/sizer.h>


namespace
{

    const int id_pe_en = wxNewId();
    const int id_pe_in6 = wxNewId();
    const int id_pe_in7 = wxNewId();
    const int id_pe_dir6 = wxNewId();
    const int id_pe_dir7 = wxNewId();
    const int id_pe_out6 = wxNewId();
    const int id_pe_out7 = wxNewId();

}

BEGIN_EVENT_TABLE(AXS_PinEmDlg, wxPanel)
    EVT_CHECKBOX(id_pe_en, AXS_PinEmDlg::OnRefreshUI)
    EVT_CHECKBOX(id_pe_in6, AXS_PinEmDlg::OnVetoClick6)
    EVT_CHECKBOX(id_pe_in7, AXS_PinEmDlg::OnVetoClick7)
    EVT_BUTTON(id_pe_out6, AXS_PinEmDlg::OnRefreshUI)
    EVT_BUTTON(id_pe_out7, AXS_PinEmDlg::OnRefreshUI)
END_EVENT_TABLE()



AXS_PinEmDlg::AXS_PinEmDlg() :
    wxPanel(Manager::Get()->GetAppWindow(), -1)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    m_pe_enabled = new wxCheckBox(this, id_pe_en, _("Enabled"));
    sizer->Add(m_pe_enabled, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    sizer->AddSpacer(15);

    wxBoxSizer* p67 = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* pe_sizer_6 = new wxBoxSizer(wxHORIZONTAL);
    m_pe_in_6 = new wxCheckBox(this, id_pe_in6, _("B6"));
    pe_sizer_6->Add(m_pe_in_6, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    pe_sizer_6->AddSpacer(20);

    m_pe_dir_6 = new wxStaticText(this, id_pe_dir6, _("           "));
    pe_sizer_6->Add(m_pe_dir_6, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    pe_sizer_6->AddSpacer(20);

    m_pe_out_6 = new wxButton(this, id_pe_out6, _("Toggle B6"));
    pe_sizer_6->Add(m_pe_out_6, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    p67->Add(pe_sizer_6);

    wxBoxSizer* pe_sizer_7 = new wxBoxSizer(wxHORIZONTAL);
    m_pe_in_7 = new wxCheckBox(this, id_pe_in7, _("B7"));
    pe_sizer_7->Add(m_pe_in_7, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    pe_sizer_7->AddSpacer(20);

    m_pe_dir_7 = new wxStaticText(this, id_pe_dir7, _("           "));
    pe_sizer_7->Add(m_pe_dir_7, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    pe_sizer_7->AddSpacer(20);

    m_pe_out_7 = new wxButton(this, id_pe_out7, _("Toggle B7"));
    pe_sizer_7->Add(m_pe_out_7, 0, wxALL | wxALIGN_CENTER_VERTICAL, 1);
    p67->Add(pe_sizer_7);

    sizer->Add(p67,0, wxALL | wxALIGN_CENTER_VERTICAL, 2);

    SetSizer(sizer);
    Layout();

    wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);
    m_pe_enabled->SetFont(font);
    m_pe_in_6->SetFont(font);
    m_pe_in_7->SetFont(font);
    m_pe_dir_6->SetFont(font);
    m_pe_dir_7->SetFont(font);
    m_pe_out_6->SetFont(font);
    m_pe_out_7->SetFont(font);

    SetEnable(false);

    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    m_pe_enabled->Enable(plugin && plugin->IsRunning());
}


AXS_PinEmDlg::~AXS_PinEmDlg()
{
    // no need for deletion of childrens of wxwindow
}


void AXS_PinEmDlg::Reset(bool hard)
{
    m_pe_in_6->SetValue(false);
    m_pe_in_7->SetValue(false);

    m_pe_in_6->SetBackgroundColour(wxColour(204,204,204));
    m_pe_in_7->SetBackgroundColour(wxColour(204,204,204));

    m_pe_dir_6->SetLabel(_("           "));
    m_pe_dir_7->SetLabel(_("           "));

    SetEnable(false);

    m_pe_enabled->Enable(!hard);
}

void AXS_PinEmDlg::SetEnable(bool enabled)
{
    m_pe_in_6->Enable(enabled);
    m_pe_in_7->Enable(enabled);
    m_pe_dir_6->Enable(enabled);
    m_pe_dir_7->Enable(enabled);
    m_pe_out_6->Enable(enabled);
    m_pe_out_7->Enable(enabled);
    m_pe_enabled->SetValue(enabled);
    m_GlobEn = enabled;
}

void AXS_PinEmDlg::SetPortB6(bool pdir, bool pout, bool pin)
{
    bool pval;
    if (!pdir)  // input
    {
        m_pe_dir_6->SetLabel(_("B6 is input"));
        if (m_GlobEn)
            m_pe_out_6->Enable(true);
        pval = pin;
    }
    else        // output
    {
        m_pe_dir_6->SetLabel(_("B6 is output"));
        m_pe_out_6->Enable(false);
        pval = pout;
    }
    m_pe_in_6->SetBackgroundColour(pval ? wxColour(255, 255, 0) : wxColour(204, 204, 204));
    m_pe_in_6->SetValue(pval);
}

void AXS_PinEmDlg::SetPortB7(bool pdir, bool pout, bool pin)
{
    bool pval;
    if (!pdir)  // input
    {
        m_pe_dir_7->SetLabel(_("B7 is input"));
        if (m_GlobEn)
            m_pe_out_7->Enable(true);
        pval = pin;
    }
    else        // output
    {
        m_pe_dir_7->SetLabel(_("B7 is output"));
        m_pe_out_7->Enable(false);
        pval = pout;
    }
    m_pe_in_7->SetBackgroundColour(pval ? wxColour(255, 255, 0) : wxColour(204, 204, 204));
    m_pe_in_7->SetValue(pval);
}

bool AXS_PinEmDlg::IsEnabled()
{
    return m_pe_enabled->IsChecked();
}

bool AXS_PinEmDlg::GetPortB6()
{
    return m_pe_in_6->IsChecked();
}

bool AXS_PinEmDlg::GetPortB7()
{
    return m_pe_in_7->IsChecked();
}

void AXS_PinEmDlg::OnRefreshUI(wxCommandEvent& event)
{
    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    cbAssert(plugin);
    plugin->RequestUpdate(cbDebuggerPlugin::PinEmulation);
}

void AXS_PinEmDlg::OnVetoClick6(wxCommandEvent& event)
{
    m_pe_in_6->SetValue(!event.IsChecked());
}

void AXS_PinEmDlg::OnVetoClick7(wxCommandEvent& event)
{
    m_pe_in_7->SetValue(!event.IsChecked());
}


