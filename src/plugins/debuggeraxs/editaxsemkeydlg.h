/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 */

#ifndef EDITAXSEMKEYDLG_H
#define EDITAXSEMKEYDLG_H

#include <stdint.h>
#include <wx/intl.h>
#include "scrollingdialog.h"

class EditAxsemKeyDlg : public wxScrollingDialog
{
    public:
        EditAxsemKeyDlg(wxWindow* parent, uint64_t& key, const wxString& title = _("Edit Key"));
        virtual ~EditAxsemKeyDlg();
		virtual void EndModal(int retCode);
		static wxString key_to_str(uint64_t key);
		static std::pair<uint64_t,bool> str_to_key(const wxString& str);
    protected:
        void OnUpdateUI(wxUpdateUIEvent& event);
        void OnTextChange(wxCommandEvent& event);

        uint64_t& m_Key;
    private:
        DECLARE_EVENT_TABLE()
};

#endif // EDITAXSEMKEYDLG_H
