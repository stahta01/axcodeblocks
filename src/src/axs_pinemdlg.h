/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef AXS_PINEMDLG_H
#define AXS_PINEMDLG_H

#include <vector>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

//#include <wx/propgrid/propgrid.h>

#include "debuggermanager.h"
#include <cbdebugger_interfaces.h>

class wxPropertyGrid;
class wxPropertyGridEvent;
class wxPGProperty;

class AXS_PinEmDlg : public wxPanel, public axs_cbPinEmDlg
{
    public:
        AXS_PinEmDlg();
        ~AXS_PinEmDlg();

        void SetEnable(bool enabled);
        void SetPortB6(bool pdir, bool pout, bool pin);
        void SetPortB7(bool pdir, bool pout, bool pin);

        bool IsEnabled();
        bool GetPortB6();
        bool GetPortB7();

        void Reset(bool hard);

        wxWindow* GetWindow() { return this; };

    protected:
        void OnRefreshUI(wxCommandEvent& event);
        void OnVetoClick6(wxCommandEvent& event);
        void OnVetoClick7(wxCommandEvent& event);

        DECLARE_EVENT_TABLE()

        bool m_GlobEn;

        wxCheckBox* m_pe_enabled;
        wxCheckBox* m_pe_in_6;
        wxCheckBox* m_pe_in_7;
        wxStaticText* m_pe_dir_6;
        wxStaticText* m_pe_dir_7;
        wxButton* m_pe_out_6;
        wxButton* m_pe_out_7;
};

#endif // AXS_PINEMDLG_H
