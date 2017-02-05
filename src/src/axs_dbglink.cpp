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
    #include <wx/event.h>


    #include "cbexception.h"
    #include "cbplugin.h"
    #include "logmanager.h"
    #include "scrollingdialog.h"
#endif

#include "debuggermanager.h"

#include <map>
#include <wx/propgrid/propgrid.h>

#include "axs_dbglink.h"
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/control.h>


namespace
{
    const int id_dl_term = wxNewId();
    const int id_dl_clr = wxNewId();
    const int id_dl_echo = wxNewId();
    const int id_dl_bl = wxNewId();
}

BEGIN_EVENT_TABLE(AXS_DbgLnkDlg, wxPanel)
    EVT_BUTTON(id_dl_clr, AXS_DbgLnkDlg::OnClear)
END_EVENT_TABLE()



AXS_DbgLnkDlg::AXS_DbgLnkDlg() :
    wxPanel(Manager::Get()->GetAppWindow(), -1,-1,-1, wxWANTS_CHARS),
    m_txfree(0),
    m_txcount(0)
{
    m_FuncKeys.Clear();
    m_FuncKeys.Add(wxEmptyString, axs_cbDbgLink::FuncKey_Last - axs_cbDbgLink::FuncKey_First + 1);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* s_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* g_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lg_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_terminal = new wxTextCtrl(this, id_dl_term, _(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxHSCROLL|wxTE_READONLY);

    m_clrBttn = new wxButton(this, id_dl_clr, _("Clear"));
    m_echoBox = new wxCheckBox(this, id_dl_echo, _("Local echo"));
    m_BufferLevel = new wxGauge(this, id_dl_bl, m_txfree + m_txcount);
    m_BufferLevel->SetValue(m_txcount);

    wxStaticText* GaugeText = new wxStaticText(this, wxID_ANY, _("Available debugger buffer space:     0\%"));
    wxStaticText* AfterGaugeText = new wxStaticText(this, wxID_ANY, _("100\%"));
    m_LocalGauge = new wxStaticText(this, -1, _("Local buffer size: 0 symbols"));

    sizer->Add(m_terminal,1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 2);
    sizer->AddSpacer(5);
    g_sizer->Add(GaugeText,0,wxALL|wxALIGN_CENTER_VERTICAL,2);
    g_sizer->Add(m_BufferLevel,0,wxALL|wxALIGN_CENTER_VERTICAL,2);
    g_sizer->Add(AfterGaugeText,0,wxALL|wxALIGN_CENTER_VERTICAL,2);
    sizer->Add(g_sizer,0,wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 2);
    lg_sizer->Add(m_LocalGauge,0,wxALL|wxALIGN_CENTER_VERTICAL,2);
    sizer->Add(lg_sizer,0,wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 2);
    sizer->AddSpacer(5);
    s_sizer->Add(m_echoBox,0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    s_sizer->Add(0,1,wxEXPAND);
    s_sizer->Add(m_clrBttn,0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 2);
    sizer->Add(s_sizer,0,wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 2);

    SetSizer(sizer);
    Layout();

    wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);

    m_terminal->SetFont(font);

    m_echoBox->SetValue(false);
    m_LastLineWasEcho = false;
    m_level = 256;
    m_BufferLevel->SetValue(m_level);

    m_TxLabel = new wxStaticText(this, -1, _(""));
    m_TxLabel->Show(false);

    this->Refresh();

    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (plugin)
        TerminalEnable(plugin->IsRunning());

    m_terminal->Connect(id_dl_term, wxEVT_CHAR, wxKeyEventHandler(AXS_DbgLnkDlg::OnChar), m_TxLabel, this);
}

AXS_DbgLnkDlg::~AXS_DbgLnkDlg()
{
    // no need for deletion of childrens of wxwindow
}

void AXS_DbgLnkDlg::OnClear(wxCommandEvent& event)
{
    m_terminal->Clear();
}

void AXS_DbgLnkDlg::AddReceive(const wxString& word)
{
    if (word.IsEmpty())
        return;
    m_terminal->SetDefaultStyle(wxTextAttr(*wxBLUE));
    (*m_terminal) << word;
}

void AXS_DbgLnkDlg::AddTransmit(const wxString& word)
{
    if (word.IsEmpty() || !m_echoBox->IsChecked())
        return;
    m_terminal->SetDefaultStyle(wxTextAttr(*wxBLACK));
    (*m_terminal) << word;
}

void AXS_DbgLnkDlg::GetTransmit(wxString& word)
{
    unsigned int bufsz(m_txbuffer.Len());
    if (bufsz <= m_txfree)
    {
        word.swap(m_txbuffer);
        m_txbuffer.Clear();
        m_txfree -= bufsz;
        m_txcount += bufsz;
    }
    else
    {
        word = m_txbuffer.Left(m_txfree);
        m_txbuffer = m_txbuffer.Mid(m_txfree);
        m_txcount += m_txfree;
        m_txfree = 0;
    }
    UpdateTxBuffer();
}

void AXS_DbgLnkDlg::SetTransmitBuffer(unsigned int free, unsigned int count)
{
    m_txfree = free;
    m_txcount = count;
    m_BufferLevel->SetRange(m_txfree + m_txcount);
    UpdateGauge();
}

void AXS_DbgLnkDlg::TerminalEnable(bool enabled)
{
    m_terminal->Enable(enabled);
    if (!enabled)
    {
        m_txbuffer.Clear();
        UpdateTxBuffer();
    }
}

void AXS_DbgLnkDlg::SetFunctionKey(axs_cbDbgLink::FuncKey_t key, const wxString& text)
{
    if (key < axs_cbDbgLink::FuncKey_First || key > axs_cbDbgLink::FuncKey_Last)
        return;
    m_FuncKeys[key - axs_cbDbgLink::FuncKey_First] = text;
}

const wxString& AXS_DbgLnkDlg::GetFunctionKey(axs_cbDbgLink::FuncKey_t key)
{
    static const wxString empty(wxEmptyString);
    if (key < axs_cbDbgLink::FuncKey_First || key > axs_cbDbgLink::FuncKey_Last)
        return empty;
    return m_FuncKeys[key - axs_cbDbgLink::FuncKey_First];
}

void AXS_DbgLnkDlg::UpdateGauge()
{
    unsigned int bufsz(m_txbuffer.Len());
    bufsz = std::min(bufsz, m_txfree);
    m_BufferLevel->SetValue(m_txcount + bufsz);
}

void AXS_DbgLnkDlg::UpdateTxBuffer()
{
    m_TxLabel->SetLabel(m_txbuffer);
    if (m_txbuffer.Len() == 1)
        m_LocalGauge->SetLabel(wxT("Local buffer size: 1 symbol"));
    else
        m_LocalGauge->SetLabel(wxString::Format(wxT("Local buffer size: %d symbols"), m_txbuffer.Len()));
    UpdateGauge();
}

void AXS_DbgLnkDlg::OnChar(wxKeyEvent& event)
{
    unsigned int bufsz(m_txbuffer.Len());
    if (bufsz >= m_txfree)
        return;
    int keycode(event.GetKeyCode());
    if (keycode >= 128 || keycode == WXK_DELETE)
    {
        int fidx = -1;
        switch (keycode)
        {
            case WXK_F1:
                fidx = axs_cbDbgLink::FuncKey_F1 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F2:
                fidx = axs_cbDbgLink::FuncKey_F2 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F3:
                fidx = axs_cbDbgLink::FuncKey_F3 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F4:
                fidx = axs_cbDbgLink::FuncKey_F4 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F5:
                fidx = axs_cbDbgLink::FuncKey_F5 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F6:
                fidx = axs_cbDbgLink::FuncKey_F6 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F7:
                fidx = axs_cbDbgLink::FuncKey_F7 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F8:
                fidx = axs_cbDbgLink::FuncKey_F8 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F9:
                fidx = axs_cbDbgLink::FuncKey_F9 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F10:
                fidx = axs_cbDbgLink::FuncKey_F10 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F11:
                fidx = axs_cbDbgLink::FuncKey_F11 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_F12:
                fidx = axs_cbDbgLink::FuncKey_F12 - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_UP:
                fidx = axs_cbDbgLink::FuncKey_Up - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_DOWN:
                fidx = axs_cbDbgLink::FuncKey_Down - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_LEFT:
                fidx = axs_cbDbgLink::FuncKey_Left - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_RIGHT:
                fidx = axs_cbDbgLink::FuncKey_Right - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_PAGEUP:
                fidx = axs_cbDbgLink::FuncKey_PgUp - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_PAGEDOWN:
                fidx = axs_cbDbgLink::FuncKey_PgDn - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_HOME:
                fidx = axs_cbDbgLink::FuncKey_Home - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_END:
                fidx = axs_cbDbgLink::FuncKey_End - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_INSERT:
                fidx = axs_cbDbgLink::FuncKey_Ins - axs_cbDbgLink::FuncKey_First;
                break;

            case WXK_DELETE:
                fidx = axs_cbDbgLink::FuncKey_Del - axs_cbDbgLink::FuncKey_First;
                break;

            default:
                return;
        }
        if (fidx == -1)
            return;
        wxString t(m_FuncKeys[fidx]), t2;
        for (int i = 0; i < t.Len(); )
        {
            if (t[i] >= 0 && t[i] <= 127)
       	    {
                t2 += t[i];
       	        ++i;
                continue;
            }
        }
        if (bufsz + t2.Len() > m_txfree)
            return;
        m_txbuffer += t2;
    }
    else
    {
        m_txbuffer += (wxChar)keycode;
    }
    UpdateTxBuffer();
    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    cbAssert(plugin);
    plugin->RequestUpdate(cbDebuggerPlugin::DebugLink);
}
