/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef AXS_DBGLNKDLG_H
#define AXS_DBGLNKDLG_H

#include <vector>
#include <wx/panel.h>
#include <wx/event.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/control.h>

//#include <wx/propgrid/propgrid.h>

#include "debuggermanager.h"
#include <cbdebugger_interfaces.h>

class wxPropertyGrid;
class wxPropertyGridEvent;
class wxPGProperty;
//class WatchesProperty;
//class wxListCtrl;

class AXS_DbgLnkDlg : public wxPanel, public axs_cbDbgLink
{
    public:
        AXS_DbgLnkDlg();
        ~AXS_DbgLnkDlg();

        void AddReceive(const wxString& word);
        void AddTransmit(const wxString& word);
        void GetTransmit(wxString& word);
        void SetTransmitBuffer(unsigned int free, unsigned int count);

        void TerminalEnable(bool enabled);

        void SetFunctionKey(axs_cbDbgLink::FuncKey_t key, const wxString& text);
        const wxString& GetFunctionKey(axs_cbDbgLink::FuncKey_t key);

        wxWindow* GetWindow() { return this; };


    protected:
        void AddEcho(wxString word);
        void OnClear(wxCommandEvent& event);
        void OnChar(wxKeyEvent& event);
        void UpdateGauge();
        void UpdateTxBuffer();

        DECLARE_EVENT_TABLE()

        wxArrayString m_FuncKeys;

       	unsigned int m_txfree, m_txcount;
        wxString m_txbuffer;

        bool m_LastLineWasEcho;
        bool m_RTS;
        int m_level;

        wxStaticText* m_TxLabel;
        wxStaticText* m_LocalGauge;

        wxTextCtrl* m_terminal;
        wxButton* m_clrBttn;
        wxCheckBox* m_echoBox;
        wxGauge* m_BufferLevel;

};

#endif // AXS_DBGLNKDLG_H
