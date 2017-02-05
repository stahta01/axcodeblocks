/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk_precomp.h"

#ifndef CB_PRECOMP
    #include <wx/xrc/xmlres.h>
    #include <wx/button.h>
    #include <wx/textctrl.h>
    #include "globals.h"
#endif

#include "editaxsemkeydlg.h"

BEGIN_EVENT_TABLE(EditAxsemKeyDlg, wxScrollingDialog)
    EVT_UPDATE_UI(-1, EditAxsemKeyDlg::OnUpdateUI)
    EVT_TEXT(XRCID("txtKey"), EditAxsemKeyDlg::OnTextChange)
END_EVENT_TABLE()

EditAxsemKeyDlg::EditAxsemKeyDlg(wxWindow* parent, uint64_t& key, const wxString& title)
    : m_Key(key)
{
    //ctor
	wxXmlResource::Get()->LoadObject(this, parent, _T("dlgEditAxsemKey"),_T("wxScrollingDialog"));
	SetTitle(title);
	wxTextCtrl *txtKey = XRCCTRL(*this, "txtKey", wxTextCtrl);
	txtKey->ChangeValue(key_to_str(key));
	txtKey->SetSelection(-1, -1);
}

EditAxsemKeyDlg::~EditAxsemKeyDlg()
{
    //dtor
}

std::pair<uint64_t,bool> EditAxsemKeyDlg::str_to_key(const wxString& str)
{
    std::pair<uint64_t,bool> r(~0ULL, true);
    if (str.IsEmpty())
        return r;
    wxULongLong_t v;
    r.second = str.ToULongLong(&v, 16);
    if (r.second)
        r.first = v;
    return r;
}

wxString EditAxsemKeyDlg::key_to_str(uint64_t key)
{
    unsigned long kh((key >> 32) & 0xFFFFFFFF), kl(key  & 0xFFFFFFFF);
    return wxString::Format(_T("%08lX%08lX"), kh, kl);
}

void EditAxsemKeyDlg::OnUpdateUI(wxUpdateUIEvent& /*event*/)
{
    wxString value(XRCCTRL(*this, "txtKey", wxTextCtrl)->GetValue());
    bool ok(value.Len() == 16);
    if (ok) {
        for (std::size_t i(0); i < 16; ++i) {
            wxChar ch(value[i]);
            if ((ch >= '0' && ch <= '9') ||
                (ch >= 'A' && ch <= 'F') ||
                (ch >= 'a' && ch <= 'f'))
                continue;
            ok = false;
            break;
        }
    }
    if (false && value.IsEmpty())
        ok = true;
    XRCCTRL(*this, "wxID_OK", wxButton)->Enable(ok);
}

void EditAxsemKeyDlg::OnTextChange(wxCommandEvent& event)
{
    wxString value(XRCCTRL(*this, "txtKey", wxTextCtrl)->GetValue());
    bool chg(false);
    for (std::size_t i(0); i < value.Len();) {
        wxChar ch(value[i]);
        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'F') ||
            (ch >= 'a' && ch <= 'f')) {
            ++i;
            continue;
        }
        value.Remove(i, 1);
        chg = true;
    }
    if (chg)
        XRCCTRL(*this, "txtKey", wxTextCtrl)->ChangeValue(value);
}

void EditAxsemKeyDlg::EndModal(int retCode)
{
    if (retCode == wxID_OK)
    {
        std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(XRCCTRL(*this, "txtKey", wxTextCtrl)->GetValue()));
        if (k.second)
            m_Key = k.first;
    }
    wxScrollingDialog::EndModal(retCode);
}
