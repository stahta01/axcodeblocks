/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk.h"

#include "cpuregistersdlg.h"
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/treectrl.h>
#include <wx/treebase.h>
#include <annoyingdialog.h>
#include <wx/propgrid/propgrid.h>
#include <numeric>

namespace
{
    long myregid(long id)
    {
        wxRegisterId(id);
        return id;
    }

    const long idGrid = wxNewId();
    const long idMenuRefresh = wxNewId();
    const long idMenuAutodetect = wxNewId();
    const long MaxChips = 64;
    const long idMenuSetChip = wxNewId();
    const long idMenuSetChipEnd = myregid(idMenuSetChip + MaxChips - 1);
};

BEGIN_EVENT_TABLE(CPURegistersDlg, wxPanel)
    EVT_PG_ITEM_EXPANDED(idGrid, CPURegistersDlg::OnExpand)
    EVT_PG_ITEM_COLLAPSED(idGrid, CPURegistersDlg::OnCollapse)
    //EVT_PG_SELECTED(idGrid, CPURegistersDlg::OnPropertySelected)
    EVT_PG_CHANGED(idGrid, CPURegistersDlg::OnPropertyChanged)
    EVT_PG_CHANGING(idGrid, CPURegistersDlg::OnPropertyChanging)
    //EVT_PG_LABEL_EDIT_BEGIN(idGrid, CPURegistersDlg::OnPropertyLableEditBegin)
    //EVT_PG_LABEL_EDIT_ENDING(idGrid, CPURegistersDlg::OnPropertyLableEditEnd)
    EVT_PG_RIGHT_CLICK(idGrid, CPURegistersDlg::OnPropertyRightClick)
    //EVT_IDLE(CPURegistersDlg::OnIdle)

    EVT_MENU(idMenuRefresh, CPURegistersDlg::OnMenuRefresh)
    //EVT_MENU(idMenuProperties, CPURegistersDlg::OnMenuProperties)
    EVT_MENU(idMenuAutodetect, CPURegistersDlg::OnMenuAutodetect)
    EVT_MENU_RANGE(idMenuSetChip, idMenuSetChipEnd, CPURegistersDlg::OnMenuSetChip)
END_EVENT_TABLE()

namespace
{
    wxPGEditor *registerPropertyEditor = nullptr;
}

CPURegistersDlg::CPURegistersDlg(wxWindow* parent) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
    m_pList(nullptr),
    m_grid(nullptr),
    m_chipprop(nullptr),
    m_InteractionFlags(RegisterRead | RegisterWrite),
    m_nrcols(2),
    m_ChipAutodetect(false)
{
    memset(m_cols, 0, sizeof(m_cols));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
    SetNormalMode();
}

void CPURegistersDlg::SetNormalMode()
{
    if (m_pList)
        return;
    m_grid = 0;
    m_chipprop = 0;
    Freeze();
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);
    m_pList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    sizer->Add(m_pList, 1, wxGROW);
    Layout();

    wxFont font(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_pList->SetFont(font);
    Thaw();

    m_ChipList.Clear();
    m_Chip.Clear();
    m_rootreg.reset();
    Clear();
}

void CPURegistersDlg::SetPropGridMode()
{
    if (m_grid)
        return;
    m_pList = 0;
    m_chipprop = 0;

    wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);

    Freeze();
    wxSizer* sizer = GetSizer();
    sizer->Clear(true);

    m_grid = new wxPropertyGrid(this, idGrid, wxDefaultPosition, wxDefaultSize,
                                wxPG_SPLITTER_AUTO_CENTER | wxTAB_TRAVERSAL /* | wxPG_TOOLTIPS */ /*| wxWANTS_CHARS*/);

#if wxCHECK_VERSION(2, 9, 0)
    #define wxPG_EX_DISABLE_TLP_TRACKING 0x00000000
#endif
    m_grid->SetExtraStyle(wxPG_EX_DISABLE_TLP_TRACKING | wxPG_EX_HELP_AS_TOOLTIPS);
    //m_grid->SetDropTarget(new WatchesDropTarget);
    m_grid->SetColumnCount(3);
    m_grid->SetFont(font);
    sizer->Add(m_grid, 1, wxEXPAND | wxALL);
    SetAutoLayout(TRUE);

    if (!registerPropertyEditor)
    {
#if wxCHECK_VERSION(2, 9, 0)
        registerPropertyEditor = wxPropertyGrid::RegisterEditorClass(new wxPGTextCtrlEditor, true);
#else
        registerPropertyEditor = wxPropertyGrid::RegisterEditorClass(new wxPGTextCtrlEditor,
                                                                     wxT("wxPGTextCtrlEditor"),
                                                                     true);
#endif
    }

    m_grid->SetColumnProportion(0, 40);
    m_grid->SetColumnProportion(1, 20);
    m_grid->SetColumnProportion(2, 20);

    //wxPGProperty *prop = m_grid->Append(new WatchesProperty(wxEmptyString, wxEmptyString, cbRegister::Pointer(), false));
    //m_grid->SetPropertyAttribute(prop, wxT("Units"), wxEmptyString);
    m_grid->Connect(idGrid, wxEVT_KEY_DOWN, wxKeyEventHandler(CPURegistersDlg::OnKeyDown), NULL, this);


    Layout();
    Thaw();

    m_ChipList.Clear();
    m_Chip.clear();
    m_rootreg.reset();
    Clear();
}

void CPURegistersDlg::Clear()
{
    m_rootreg.reset();
    if (m_grid)
    {
        m_grid->Clear();
        m_chipprop = 0;
    }
    m_propmap.clear();
    if (!m_pList)
        return;
    m_pList->ClearAll();
    m_pList->Freeze();
    m_pList->DeleteAllItems();
    m_pList->InsertColumn(0, _("Register"), wxLIST_FORMAT_LEFT);
    m_pList->InsertColumn(1, _("Hex"), wxLIST_FORMAT_RIGHT);
    m_pList->InsertColumn(2, _("Interpreted"), wxLIST_FORMAT_LEFT);
    m_pList->Thaw();
}

int CPURegistersDlg::RegisterIndex(const wxString& reg_name)
{
    SetNormalMode();
    for (int i = 0; i < m_pList->GetItemCount(); ++i)
    {
        if (m_pList->GetItemText(i).CmpNoCase(reg_name) == 0)
            return i;
    }
    return -1;
}

void CPURegistersDlg::SetRegisterValue(const wxString& reg_name, const wxString& hexValue, const wxString& interpreted)
{
    // find existing register
    int idx = RegisterIndex(reg_name);
    if (idx == -1)
    {
        // if it doesn't exist, add it
        idx = m_pList->GetItemCount();
        m_pList->InsertItem(idx, reg_name);
    }

    m_pList->SetItem(idx, 1, hexValue);
    m_pList->SetItem(idx, 2, interpreted);

#if defined(__WXMSW__) || wxCHECK_VERSION(3, 1, 0)
    const int autoSizeMode = wxLIST_AUTOSIZE_USEHEADER;
#else
    const int autoSizeMode = wxLIST_AUTOSIZE;
#endif

    for (int i = 0; i < 3; ++i)
    {
        m_pList->SetColumnWidth(i, autoSizeMode);
    }
}

void CPURegistersDlg::EnableWindow(bool enable)
{
    if (m_pList)
        m_pList->Enable(enable);
    if (m_grid)
        m_grid->Enable(enable);
}

// PropGrid Interface
void CPURegistersDlg::PropertyMap::Update(const unsigned int col[5])
{
    switch (m_proptype)
    {
    case proptype_register:
        {
            wxString val;
            m_reg->GetValue(val);
            m_property->SetValue(val);
        }
        {
            wxArrayString cv;
            cv.SetCount(5);
            m_reg->GetValueAlt(cv[0]);
            m_reg->GetWriteMask(cv[1]);
            m_reg->GetAddrSpace(cv[2]);
            m_reg->GetAddr(cv[3]);
            m_reg->GetDescription(cv[4]);
            wxArrayString txt;
            txt.SetCount(5);
            for (int i = 0; i < 5; ++i)
            {
                if (col[i] < 2 || col[i] >= 7)
                    continue;
                txt[col[i]-2].swap(cv[i]);
            }
            m_property->SetAttribute(wxT("Units"), txt[0]);
            for (int i = 1; i < 5; ++i)
            {
                if (txt[i].IsEmpty())
                {
                    m_property->SetCell(i + 2, wxPGCell());
                    continue;
                }
                m_property->SetCell(i + 2, wxPGCell(txt[i]));
            }
            m_property->SetHelpString(cv[4]);
        }
        m_property->SetExpanded(m_reg->IsExpanded());
        m_property->ChangeFlag(wxPG_PROP_DISABLED, false && m_reg->IsOutdated());
        m_property->ChangeFlag(wxPG_PROP_CATEGORY, false && m_reg->IsCategory());
        m_property->ChangeFlag(wxPG_PROP_READONLY, m_reg->IsCategory() || m_reg->IsReadonly());
        {
            wxColor col(0, 0, 0);
            if (!m_reg->IsCategory())
            {
                if (m_reg->IsChanged())
                    col = wxColor(255, 0, 0);
                if (m_reg->IsOutdated())
                    col = wxColour(192, 192, 192);
            }
            m_property->GetGrid()->SetPropertyTextColour(m_property, col);
        }
        {
            wxColor col(255, 255, 255);
            if (!m_reg->IsReadSafe())
                col = wxColour(255, 255, 128);
            m_property->GetGrid()->SetPropertyBackgroundColour(m_property, col);
        }
        break;

    case proptype_category:
        {
            wxArrayString cv;
            cv.SetCount(5);
            m_reg->GetDescription(cv[4]);
            wxArrayString txt;
            txt.SetCount(5);
            for (int i = 0; i < 5; ++i)
            {
                if (col[i] < 2 || col[i] >= 7)
                    continue;
                txt[col[i]-2].swap(cv[i]);
            }
            m_property->SetAttribute(wxT("Units"), txt[0]);
            for (int i = 1; i < 5; ++i)
            {
                if (txt[i].IsEmpty())
                {
                    m_property->SetCell(i + 2, wxPGCell());
                    continue;
                }
                m_property->SetCell(i + 2, wxPGCell(txt[i]));
            }
            m_property->SetHelpString(cv[4]);
        }
        m_property->SetExpanded(m_reg->IsExpanded());
        m_property->ChangeFlag(wxPG_PROP_DISABLED, false && m_reg->IsOutdated());
        m_property->ChangeFlag(wxPG_PROP_CATEGORY, true);
        m_property->ChangeFlag(wxPG_PROP_READONLY, false);
        break;

    case proptype_chips:
        m_property->ChangeFlag(wxPG_PROP_READONLY, true);
        break;
    }
}

void CPURegistersDlg::PropertyMap::SetChip(const wxString& chip)
{
    if (m_proptype != proptype_chips)
        return;
    m_property->SetValue(chip);
}

void CPURegistersDlg::SetChips(const wxArrayString& chips, bool can_autodetect)
{
    SetPropGridMode();
    m_ChipList = chips;
    m_ChipAutodetect = can_autodetect;
    UpdateRegisters();
}

void CPURegistersDlg::SetCurrentChip(const wxString& chip)
{
    m_Chip = chip;
    if (m_grid)
    {
        if (m_chipprop)
            m_chipprop->SetChip(m_Chip);
        UpdateRegisters();
    }
}

void CPURegistersDlg::FindColWidth(wxPGProperty *root, wxClientDC &dc, int width[7])
{
    if (!root)
        return;
#if wxCHECK_VERSION(2, 9, 0)
    wxPropertyGridPageState *state = m_grid->GetState();
#else
    wxPropertyGridState *state = m_grid->GetState();
#endif
    for (int i = 0; i < root->GetChildCount(); ++i)
    {
        wxPGProperty *p(root->Item(i));
        if (!p)
            continue;
        for (int i = 0; i < m_nrcols; ++i)
            width[i] = std::max(width[i], state->GetColumnFullWidth(dc, p, i));
        FindColWidth(p, dc, width);
    }
}

#include <iostream>

void CPURegistersDlg::SetColWidths()
{
#if wxCHECK_VERSION(2, 9, 0)
    wxPropertyGridPageState *state = m_grid->GetState();
#else
    wxPropertyGridState *state = m_grid->GetState();
#endif
    wxClientDC dc(m_grid);
    dc.SetFont(m_grid->GetFont());
    int width[7];
    {
        int i = 0;
        for (; i < m_nrcols; ++i)
            width[i] = state->GetColumnMinWidth(i);
        for (; i < 7; ++i)
            width[i] = 0;
    }
    FindColWidth(m_grid->GetRoot(), dc, width);
    wxSize rect = GetSize();
    rect.SetWidth(std::max(rect.GetWidth(), std::accumulate(width, width + 7, m_cols[4] ? -width[m_cols[4]] : 0)));

    int proportions[7];
    for (int i = 0; i < 7; ++i)
        proportions[i] = (width[i] * 100) / rect.GetWidth();

    if (m_cols[4])
    {
        proportions[m_cols[4]] = 0;
        proportions[m_cols[4]]= std::max(100 - std::accumulate(proportions, proportions + 7, 0), 0);
    }

    for (int i = 0; i < m_nrcols; ++i)
        m_grid->SetColumnProportion(i, proportions[i]);
    m_grid->ResetColumnSizes(true);

    std::cerr << "SetColWidths";
    for (int i = 0; i < m_nrcols; ++i)
        std::cerr << ' ' << proportions[i];
    std::cerr << std::endl;
}

void CPURegistersDlg::FindColContent(cbRegister::Pointer reg, bool nonempty[5])
{
    if (!reg)
        return;
    for (int i = 0; i < reg->GetChildCount(); ++i)
    {
        cbRegister::Pointer p(reg->GetChild(i));
        if (!p)
            continue;
        bool allfull(true);
        if (!nonempty[0])
        {
            wxString t;
            p->GetValueAlt(t);
            nonempty[0] = !t.IsEmpty();
            allfull = allfull && nonempty[0];
        }
        if (!nonempty[1])
        {
            wxString t;
            p->GetWriteMask(t);
            nonempty[1] = !t.IsEmpty();
            allfull = allfull && nonempty[1];
        }
        if (!nonempty[2])
        {
            wxString t;
            p->GetAddrSpace(t);
            nonempty[2] = !t.IsEmpty();
            allfull = allfull && nonempty[2];
        }
        if (!nonempty[3])
        {
            wxString t;
            p->GetAddr(t);
            nonempty[3] = !t.IsEmpty();
            allfull = allfull && nonempty[3];
        }
        if (!nonempty[4])
        {
            wxString t;
            p->GetDescription(t);
            nonempty[4] = !t.IsEmpty();
            allfull = allfull && nonempty[4];
        }
        if (allfull)
            return;
        FindColContent(p, nonempty);
    }
}

void CPURegistersDlg::UpdateRegisters(cbRegister::Pointer reg, wxPGProperty *prop)
{
    if (!reg || !prop)
        return;

    int offs = 0;
    if (prop->GetChildCount())
        offs = m_chipprop && FindPropMap(prop->Item(0)) == m_chipprop;

    int i = 0;
    for (; i < reg->GetChildCount(); ++i)
    {
        cbRegister::Pointer reg1(reg->GetChild(i));
        wxPGProperty *child_prop(0);
        PropertyMap *child_pmap(0);
        if (i + offs < prop->GetChildCount())
        {
            child_prop = prop->Item(i + offs);
            child_pmap = FindPropMap(child_prop);
            if (!child_pmap || child_pmap->GetReg() != reg1)
            {
                m_grid->DeleteProperty(child_prop);
                child_prop = 0;
            }
        }
        if (!child_prop)
        {
            wxString name;
            reg1->GetName(name);
            if (reg1->IsCategory())
                child_prop = prop->InsertChild(i + offs, new wxPropertyCategory(name));
            else
                child_prop = prop->InsertChild(i + offs, new wxStringProperty(name));
            if (child_prop)
            {
                PropertyMap pmap(child_prop, reg1->IsCategory() ? PropertyMap::proptype_category : PropertyMap::proptype_register, reg1, true);
                std::pair<propmap_t::const_iterator,bool> ins(m_propmap.insert(pmap));
                child_pmap = const_cast<PropertyMap *>(&*ins.first);
                child_pmap->SetType(pmap.GetType());
                child_pmap->SetReg(reg1);
            }
            if (!child_prop)
                continue;
        }
        child_pmap->SetUsed(true);
        m_grid->EnableProperty(child_prop, m_grid->IsPropertyEnabled(prop));
        child_pmap->Update(m_cols);
        if (!(m_InteractionFlags & RegisterWrite) && child_pmap->IsRegister())
            child_prop->ChangeFlag(wxPG_PROP_READONLY, true);
        UpdateRegisters(reg1, child_prop);
    }
    while (i + offs < prop->GetChildCount())
        m_grid->DeleteProperty(prop->Item(i + offs));
}

void CPURegistersDlg::UpdateRegisters()
{
    if (!m_rootreg)
    {
        if (m_grid)
            m_grid->Clear();
        return;
    }
    if (!m_grid)
        return;
    m_grid->Freeze();
    // workaround for an evil PropGrid crash
    if (true)
    {
        m_grid->Clear();
        m_propmap.clear();
	m_chipprop = 0;
    }
    for (propmap_t::const_iterator i(m_propmap.begin()), e(m_propmap.end()); i != e; ++i)
    {
        PropertyMap *pmap(const_cast<PropertyMap *>(&*i));
        pmap->SetUsed(false);
    }
    bool colchg = UpdateColumns();
    wxPGProperty *root_prop(m_grid->GetRoot());
    if (m_ChipList.IsEmpty() && m_chipprop)
    {
        m_grid->DeleteProperty(m_chipprop->GetProp());
        m_chipprop = 0;
    }
    else if (!m_ChipList.IsEmpty() && !m_chipprop)
    {
        wxPGProperty *p = root_prop->InsertChild(0, new wxStringProperty(_("Chip")));
        PropertyMap pmap(p, PropertyMap::proptype_chips);
        std::pair<propmap_t::const_iterator,bool> ins(m_propmap.insert(pmap));
        m_chipprop = const_cast<PropertyMap *>(&*ins.first);
        m_chipprop->SetType(PropertyMap::proptype_chips);
    }
    if (m_chipprop)
    {
        m_chipprop->SetChip(m_Chip);
        m_chipprop->Update(m_cols);
        m_chipprop->SetUsed(true);
    }
    UpdateRegisters(m_rootreg, root_prop);
    if (colchg)
        SetColWidths();
    for (propmap_t::const_iterator i(m_propmap.begin()), e(m_propmap.end()); i != e;)
    {
        if (i->IsUsed())
        {
            ++i;
            continue;
        }
        propmap_t::const_iterator i2(i);
        ++i;
        m_propmap.erase(i2);
    }
    m_grid->Thaw();
    m_grid->RefreshGrid();
}

bool CPURegistersDlg::UpdateColumns()
{
    bool nonempty[5] = { false, };
    FindColContent(m_rootreg, nonempty);
    unsigned int colnr(2);
    unsigned int colw[5];
    static const unsigned int default_column_width[5] =
    {
        20, 20, 20, 20, 40
    };

    for (int i = 0; i < 5; ++i)
    {
        if (!nonempty[i])
        {
            m_cols[i] = 0;
            continue;
        }
        colw[colnr - 2] = default_column_width[i];
        m_cols[i] = colnr++;
    }
    bool chg(m_nrcols != colnr);
    m_nrcols = colnr;
    SetPropGridMode();
    m_grid->SetColumnCount(m_nrcols);
    //for (unsigned int i = 2; i < m_nrcols; ++i)
    //    m_grid->SetColumnProportion(i, colw[i - 2]);
    std::cerr << "nrcol: " << m_nrcols << " content:";
    for (unsigned int i = 0; i < 5; ++i)
        std::cerr << ' ' << (char)('0' + !!nonempty[i]);
    std::cerr << std::endl;
    return chg;
}

void CPURegistersDlg::SetRootRegister(cbRegister::Pointer reg)
{
    if (!reg)
    {
        m_rootreg.reset();
        return;
    }
    SetPropGridMode();
    m_rootreg = reg;
    UpdateRegisters();
    //m_grid->FitColumns();
    SetColWidths();
}

void CPURegistersDlg::RefreshUI()
{
    SetPropGridMode();
}

void CPURegistersDlg::EnableInteraction(int flags)
{
    int chg(m_InteractionFlags ^ flags);
    m_InteractionFlags = flags;
    if (chg & RegisterWrite)
        UpdateRegisters();
}

void CPURegistersDlg::SetValue(wxPGProperty *prop)
{
    PropertyMap *pmap(FindPropMap(prop));
    if (!pmap)
        return;
    cbRegister::Pointer reg = pmap->GetReg();
    if (reg)
    {
        cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
        if (plugin)
            plugin->SetRegisterValue(reg, prop->GetValue());
        if (plugin)
        {
            wxString name;
            reg->GetName(name);
            wxString val((const wxString&)prop->GetValue());
            std::cerr << "Setting Register: " << (const char *)name.mb_str() << " Value: " << (const char *)val.mb_str() << std::endl;
        }
    }
}

CPURegistersDlg::PropertyMap *CPURegistersDlg::FindPropMap(wxPGProperty *prop)
{
    if (!prop)
        return nullptr;
    PropertyMap pmap(prop);
    propmap_t::const_iterator i(m_propmap.find(pmap));
    if (i == m_propmap.end())
        return nullptr;
    return const_cast<PropertyMap *>(&*i);
}

void CPURegistersDlg::OnExpand(wxPropertyGridEvent &event)
{
    PropertyMap *pmap(FindPropMap(event.GetProperty()));
    if (!pmap)
        return;
    cbRegister::Pointer reg = pmap->GetReg();
    if (!reg)
        return;
    reg->Expand(true);

    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (plugin)
        plugin->ExpandRegister(reg);
}

void CPURegistersDlg::OnCollapse(wxPropertyGridEvent &event)
{
    PropertyMap *pmap(FindPropMap(event.GetProperty()));
    if (!pmap)
        return;
    cbRegister::Pointer reg = pmap->GetReg();
    if (!reg)
        return;
    reg->Expand(false);

    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (plugin)
        plugin->CollapseRegister(reg);
}

void CPURegistersDlg::OnPropertyChanged(wxPropertyGridEvent &event)
{
    wxPGProperty *prop = event.GetProperty();
    SetValue(prop);
}


void CPURegistersDlg::OnPropertyChanging(wxPropertyGridEvent &event)
{
    if (event.GetProperty()->GetChildCount() > 0)
        event.Veto(true);
}

void CPURegistersDlg::OnKeyDown(wxKeyEvent &event)
{
    PropertyMap *pmap(FindPropMap(m_grid->GetSelection()));
    if (!pmap)
        return;

    switch (event.GetKeyCode())
    {
        case WXK_SPACE:
            {
                cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
                cbRegister::Pointer reg = pmap->GetReg();
                if (plugin && reg && (m_InteractionFlags & RegisterRead))
                    plugin->UpdateRegister(reg);
            }
            break;
    }
}

void CPURegistersDlg::OnPropertyRightClick(wxPropertyGridEvent &event)
{
    PropertyMap *pmap(FindPropMap(event.GetProperty()));
    if (pmap)
    {
        wxMenu m;
        if (pmap->IsRegister() && (m_InteractionFlags & RegisterRead))
        {
            wxString txt(_("Read the Register"));
            wxString nm(_T("Read"));
            cbRegister::Pointer reg(pmap->GetReg());
            if (reg && !reg->IsOutdated())
            {
                txt = _("Reread the Register");
                nm = _T("Reread");
            }
            m.Append(idMenuRefresh, nm, txt);
        }
        if (pmap->IsChip())
        {
            wxMenu *mchip(0);
            if (m_ChipAutodetect && (m_InteractionFlags & ChipAutodetect))
            {
                mchip = new wxMenu;
                mchip->Append(idMenuAutodetect, _T("Autodetect"), _("Perform Autodetection of the Chip"));
            }
            if (!m_ChipList.IsEmpty() && (m_InteractionFlags & ChipSet))
            {
                for (int i = 0; i < m_ChipList.GetCount() && i < MaxChips; ++i)
                {
                    if (!mchip)
                        mchip = new wxMenu;
                    else if (!i)
                        mchip->AppendSeparator();
                    mchip->Append(idMenuSetChip + i, m_ChipList[i], _T("Set Chip to ") + m_ChipList[i]);
                }
            }
            if (mchip)
                m.AppendSubMenu(mchip, _T("Set Chip"), _T("Set Chip Type"));
        }
        PopupMenu(&m);
    }
}

void CPURegistersDlg::OnMenuRefresh(wxCommandEvent &event)
{
    PropertyMap *pmap(FindPropMap(m_grid->GetSelection()));
    if (pmap)
    {
        cbRegister::Pointer reg = pmap->GetReg();
        if (reg)
        {
            cbDebuggerPlugin *debugger = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
            if (debugger && (m_InteractionFlags & RegisterRead))
                debugger->UpdateRegister(reg);
        }
    }
}

void CPURegistersDlg::OnMenuAutodetect(wxCommandEvent& event)
{
    if (!(m_InteractionFlags & ChipAutodetect))
        return;
    cbDebuggerPlugin *debugger = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (!debugger)
        return;
    debugger->SetChip(wxEmptyString);
}

void CPURegistersDlg::OnMenuSetChip(wxCommandEvent& event)
{
    if (!(m_InteractionFlags & ChipSet))
        return;
    cbDebuggerPlugin *debugger = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if (!debugger)
        return;
    int idx = event.GetId() - idMenuSetChip;
    if (idx < 0 || idx >= m_ChipList.GetCount())
        return;
    debugger->SetChip(m_ChipList[idx]);
}
