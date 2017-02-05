/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include <annoyingdialog.h>
#include "axs_driver.h"
#include "axs_commands.h"
#include "debuggeroptionsdlg.h"
#include "debuggerstate.h"
#include <iterator>
#include <fstream>
#include <iostream>
#include <cbdebugger_interfaces.h>
#include <manager.h>
#include <macrosmanager.h>
#include <configmanager.h>
#include <scriptingmanager.h>
#include <globals.h>
#include <infowindow.h>
#include <wx/statline.h>

#include "machine.h"

#ifdef __WXMSW__
// for Registry detection of Cygwin
#include <windows.h>
#endif

// scripting support
DECLARE_INSTANCE_TYPE(AXS_driver);

namespace
{
    const int idCPUTraceList       = wxNewId();
    const int idCPUTraceMode       = wxNewId();
    const int idCPUTraceFetch      = wxNewId();
    const int idCPUTraceClear      = wxNewId();
    const int idCPUTraceSave       = wxNewId();
    const int idCPUTraceHistory    = wxNewId();
    const int idProfilerList       = wxNewId();
    const int idProfilerMode       = wxNewId();
    const int idProfilerSamples    = wxNewId();
    const int idMyListCtrlTimer    = wxNewId();
}

BEGIN_EVENT_TABLE(AXS_driver::CPUTracePanel, wxPanel)
    EVT_LIST_ITEM_ACTIVATED(idCPUTraceList, AXS_driver::CPUTracePanel::OnListDoubleClick)
    EVT_RADIOBOX(idCPUTraceMode, AXS_driver::CPUTracePanel::OnModeChange)
    EVT_SPINCTRL(idCPUTraceHistory, AXS_driver::CPUTracePanel::OnHistoryChange)
    EVT_BUTTON(idCPUTraceFetch, AXS_driver::CPUTracePanel::OnFetchClicked)
    EVT_BUTTON(idCPUTraceClear, AXS_driver::CPUTracePanel::OnClearClicked)
    EVT_BUTTON(idCPUTraceSave, AXS_driver::CPUTracePanel::OnSaveClicked)
END_EVENT_TABLE()

AXS_driver::CPUTracePanel::myListCtrl::Entry::Entry(const wxString& type, time_t sec, unsigned int usec)
    : m_type(type), m_sec(sec), m_usec(usec), m_addr(0), m_line(0), m_level(0), m_block(0), m_mode(mode_misc), m_isasm(false)
{
}

AXS_driver::CPUTracePanel::myListCtrl::Entry::Entry(const wxString& type, time_t sec, unsigned int usec, unsigned int addr)
    : m_type(type), m_sec(sec), m_usec(usec), m_addr(addr), m_line(0), m_level(0), m_block(0), m_mode(mode_addr), m_isasm(false)
{
}

AXS_driver::CPUTracePanel::myListCtrl::Entry::Entry(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
						    const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm)
    : m_type(type), m_name(name), m_sec(sec), m_usec(usec), m_addr(addr), m_line(line), m_level(level), m_block(block),
      m_mode(mode_addrsourceline), m_isasm(isasm)
{
}

AXS_driver::CPUTracePanel::myListCtrl::Entry::Entry(const wxString& type, time_t sec, unsigned int usec, const wxString& chr)
    : m_type(type), m_sec(sec), m_usec(usec), m_addr(0), m_line(0), m_level(0), m_block(0), m_mode(mode_misc), m_isasm(false)
{
    if (chr.Len() != 1)
        return;
    m_addr = chr[0];
    m_mode = mode_char;
}

wxString AXS_driver::CPUTracePanel::myListCtrl::Entry::GetCol(unsigned int col) const
{
    wxString r;
    switch (col)
    {
    case 0:
        return m_type;

    case 1:
    {
        wxDateTime dt(m_sec);
        return dt.FormatISOTime() + wxString::Format(wxT(".%.06d"), m_usec);
    }

    case 2:
        switch (m_mode)
        {
        case mode_addr:
        case mode_addrsourceline:
            return wxString::Format(wxT("0x%04x"), m_addr);

        case mode_char:
            if (m_addr >= 32 && m_addr < 127)
       	        return wxString::Format(wxT("\"%c\""), (char)m_addr);
            return wxString::Format(wxT("\"\\%c%c%c\""), '0' + ((m_addr >> 6) & 3), '0' + ((m_addr >> 3) & 7), '0' + (m_addr & 7));

        default:
            return wxEmptyString;
        }

    case 3:
    {
        if (m_mode != mode_addrsourceline)
            return wxEmptyString;
        wxString r(m_name + wxString::Format(wxT(":%u"), m_line));
        if (m_level || m_block)
       	    r += wxString::Format(wxT(",%u,%u"), m_level, m_block);
        return r;
    }

    case 4:
        if (m_mode != mode_addrsourceline)
            return wxEmptyString;
        return m_isasm ? wxT("Asm") : wxT("C");

    default:
        return wxEmptyString;
    }
}

wxString AXS_driver::CPUTracePanel::myListCtrl::Entry::GetCSV(void) const
{
    wxString r(m_type + wxString::Format(wxT(",%f"), m_sec + m_usec * 1e-6));
    switch (m_mode)
    {
    case mode_addr:
    case mode_addrsourceline:
        r += wxString::Format(wxT(",0x%04x"), m_addr);
        if (m_mode != mode_addrsourceline)
            break;
        r += wxT(",\"") + m_name + wxString::Format(wxT("\",%u,%u,%u,"), m_line, m_level, m_block) + (m_isasm ? wxT("Asm") : wxT("C"));
        break;

    case mode_char:
        r += wxString::Format(wxT(",%u"), m_addr);
        break;

    default:
        break;
    }
    return r;
}

BEGIN_EVENT_TABLE(AXS_driver::CPUTracePanel::myListCtrl, wxListCtrl)
    EVT_TIMER(idMyListCtrlTimer, AXS_driver::CPUTracePanel::myListCtrl::OnTimer)
END_EVENT_TABLE()

AXS_driver::CPUTracePanel::myListCtrl::myListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                                                  long style, const wxValidator& validator, const wxString& name)
    : wxListCtrl(parent, id, pos, size, style | (wxLC_REPORT | wxLC_VIRTUAL), validator, name),
    m_bufwr(0),
    m_bufrd(0),
    m_timer(this, idMyListCtrlTimer)
{
    m_buffer.resize(1024, Entry(wxT(""), 0, 0));
}

wxString AXS_driver::CPUTracePanel::myListCtrl::OnGetItemText(long item, long column) const
{
    if (m_buffer.empty())
        return wxEmptyString;
    unsigned int nr(m_buffer.size() + m_bufwr - m_bufrd);
    if (nr >= m_buffer.size())
        nr -= m_buffer.size();
    if (item >= nr || item < 0)
        return wxEmptyString;
    nr = m_bufrd + item;
    if (nr >= m_buffer.size())
        nr -= m_buffer.size();
    return m_buffer[nr].GetCol(column);
}

std::pair<wxString,unsigned int> AXS_driver::CPUTracePanel::myListCtrl::GetSourceInfo(long item) const
{
    if (m_buffer.empty())
        return std::pair<wxString,unsigned int>(wxEmptyString, 0);
    unsigned int nr(m_buffer.size() + m_bufwr - m_bufrd);
    if (nr >= m_buffer.size())
        nr -= m_buffer.size();
    if (item >= nr || item < 0)
        return std::pair<wxString,unsigned int>(wxEmptyString, 0);
    nr = m_bufrd + item;
    if (nr >= m_buffer.size())
        nr -= m_buffer.size();
    const Entry& e(m_buffer[nr]);
    return std::pair<wxString,unsigned int>(e.get_name(), e.get_line());
}

void AXS_driver::CPUTracePanel::myListCtrl::Add(const wxString& type, time_t sec, unsigned int usec)
{
    Add(Entry(type, sec, usec));
}

void AXS_driver::CPUTracePanel::myListCtrl::Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr)
{
    Add(Entry(type, sec, usec, addr));
}

void AXS_driver::CPUTracePanel::myListCtrl::Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
	 const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm)
{
    Add(Entry(type, sec, usec, addr, name, line, level, block, isasm));
}

void AXS_driver::CPUTracePanel::myListCtrl::Add(const wxString& type, time_t sec, unsigned int usec, const wxString& chr)
{
    Add(Entry(type, sec, usec, chr));
}

void AXS_driver::CPUTracePanel::myListCtrl::Add(const Entry& e)
{
    if (m_buffer.empty())
        return;
    //Freeze();
    m_buffer[m_bufwr++] = e;
    if (m_bufwr >= m_buffer.size())
        m_bufwr -= m_buffer.size();
    if (m_bufrd == m_bufwr)
    {
        ++m_bufrd;
        if (m_bufrd >= m_buffer.size())
            m_bufrd -= m_buffer.size();
        RefreshItems(0, m_buffer.size() - 1);
    }
    else
    {
        unsigned int nr(m_buffer.size() + m_bufwr - m_bufrd);
        if (nr >= m_buffer.size())
            nr -= m_buffer.size();
        RefreshItem(nr - 1);
    }
    unsigned int nr(m_buffer.size() + m_bufwr - m_bufrd);
    if (nr >= m_buffer.size())
        nr -= m_buffer.size();
    if (nr != GetItemCount())
        SetItemCount(nr);
    m_timer.Start(1000, wxTIMER_ONE_SHOT);
}

void AXS_driver::CPUTracePanel::myListCtrl::OnTimer(wxTimerEvent& evt)
{
    //Thaw();
    EnsureVisible(GetItemCount() -  1);
    for (int ii = 0; ii < GetColumnCount(); ++ii)
    {
        SetColumnWidth(ii, wxLIST_AUTOSIZE);
    }
    std::cerr << "myListCtrl::OnTimer: wr " << m_bufwr << " rd " << m_bufrd << std::endl;
}

int AXS_driver::CPUTracePanel::myListCtrl::SaveCSV(const wxString& filename) const
{
    if (m_buffer.empty())
        return 0;
    wxFileOutputStream of(filename);
    if (!of.IsOk())
        return -1;
    wxTextOutputStream out(of);
    int r(0);
    unsigned int p(m_bufrd);
    while (p != m_bufwr)
    {
        out << m_buffer[p].GetCSV() << endl;
	++p;
        if (p >= m_buffer.size())
            p -= m_buffer.size();
        ++r;
    }
    return r;
}

void AXS_driver::CPUTracePanel::myListCtrl::Clear(void)
{
    m_bufrd = m_bufwr = 0;
    SetItemCount(0);
    m_timer.Stop();
    //Thaw();
}

void AXS_driver::CPUTracePanel::myListCtrl::SetBufferLength(unsigned int len)
{
    if (len < 2)
    {
        m_buffer.clear();
        m_bufrd = m_bufwr = 0;
        SetItemCount(0);
        m_timer.Stop();
        //Thaw();
        return;
    }
    buffer_t b;
    if (!m_buffer.empty())
    {
        if (m_bufrd > m_bufwr)
        {
            b.insert(b.end(), &m_buffer[m_bufrd], &m_buffer[m_buffer.size()]);
            m_bufrd = 0;
        }
        if (m_bufrd < m_bufwr)
            b.insert(b.end(), &m_buffer[m_bufrd], &m_buffer[m_bufwr]);
    }
    m_buffer.clear();
    if (b.size() > len)
        b.erase(b.begin(), b.begin() + (len - b.size()));
    m_bufrd = 0;
    m_bufwr = b.size();
    b.resize(len, Entry(wxT(""), 0, 0));
    m_buffer.swap(b);
    SetItemCount(m_bufwr);
    if (m_bufwr)
        RefreshItems(0, m_bufwr - 1);
    m_timer.Start(1000, wxTIMER_ONE_SHOT);
}

unsigned int AXS_driver::CPUTracePanel::myListCtrl::GetBufferLength(void) const
{
    return m_buffer.size();
}

const wxString AXS_driver::CPUTracePanel::modechoices[] = 
{
    wxT("Off"),
    wxT("On Demand"),
    wxT("Continuous")
};

AXS_driver::CPUTracePanel::CPUTracePanel(wxWindow* parent, AXS_driver *driver)
    : wxPanel(parent),
    m_driver(driver),
    m_list(0),
    m_enable(0),
    m_history(0),
    m_fetch(0),
    m_clear(0),
    m_save(0),
    m_dofetch(false)
{
    m_list = new myListCtrl(this, idCPUTraceList, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    wxStaticLine *line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    m_enable = new wxRadioBox(this, idCPUTraceMode, wxT("Mode"), wxDefaultPosition, wxDefaultSize,
                              sizeof(modechoices)/sizeof(modechoices[0]), modechoices, 1, wxRA_SPECIFY_ROWS);
    m_fetch = new wxButton(this, idCPUTraceFetch, wxT("Fetch"), wxDefaultPosition, wxDefaultSize, 0);
    m_clear = new wxButton(this, idCPUTraceClear, wxT("Clear"), wxDefaultPosition, wxDefaultSize, 0);
    m_save = new wxButton(this, idCPUTraceSave, wxT("Save..."), wxDefaultPosition, wxDefaultSize, 0);
    wxStaticText *txt1 = new wxStaticText(this, wxID_ANY, wxT("History Length:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_history = new wxSpinCtrl(this, idCPUTraceHistory, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 10, 100000, 1000);
    wxBoxSizer *bshi = new wxBoxSizer(wxHORIZONTAL);
    bshi->Add(txt1, 0, wxEXPAND | wxALL);
    bshi->Add(m_history, 0, wxALL);
    wxBoxSizer *bsh = new wxBoxSizer(wxHORIZONTAL);
    bsh->Add(m_enable, 0, wxEXPAND | wxALL);
    bsh->Add(m_fetch, 0, wxEXPAND | wxALL);
    bsh->Add(m_clear, 0, wxEXPAND | wxALL);
    bsh->Add(m_save, 0, wxEXPAND | wxALL);
    bsh->Add(bshi, 0, wxEXPAND | wxALL);
    wxBoxSizer *bs = new wxBoxSizer(wxVERTICAL);
    bs->Add(m_list, 1, wxEXPAND | wxALL);
    bs->Add(line, 0, wxEXPAND | wxALL);
    bs->Add(bsh, 0, wxEXPAND | wxALL);
    SetAutoLayout(true);
    SetSizer(bs);

    wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);
    m_list->SetFont(font);
    m_list->InsertColumn(0, _("State"), wxLIST_FORMAT_LEFT, 10);
    m_list->InsertColumn(1, _("Time"), wxLIST_FORMAT_RIGHT, 15);
    m_list->InsertColumn(2, _("Addr"), wxLIST_FORMAT_RIGHT, 6);
    m_list->InsertColumn(3, _("Source Line"), wxLIST_FORMAT_LEFT);
    m_list->InsertColumn(4, _("Lang"), wxLIST_FORMAT_LEFT, 3);

    m_list->SetBufferLength(m_history->GetValue());
}

void AXS_driver::CPUTracePanel::Add(const wxString& type, time_t sec, unsigned int usec)
{
    if (!m_list)
        return;
    m_list->Add(type, sec, usec);
}

void AXS_driver::CPUTracePanel::Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr)
{
    if (!m_list)
        return;
    m_list->Add(type, sec, usec, addr);
}

void AXS_driver::CPUTracePanel::Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
                                    const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm)
{
    if (!m_list)
        return;
    m_list->Add(type, sec, usec, addr, name, line, level, block, isasm);
}

void AXS_driver::CPUTracePanel::Add(const wxString& type, time_t sec, unsigned int usec, const wxString& chr)
{
    if (!m_list)
        return;
    m_list->Add(type, sec, usec, chr);
}

unsigned int AXS_driver::CPUTracePanel::GetBuffer(void) const
{
    if (!m_enable)
        return 0;
    {
        int m(m_enable->GetSelection());
        if (m < 1 || m > 2)
            return 0;
    }
    if (!m_history)
        return 0;
    return m_history->GetValue();
}

unsigned int AXS_driver::CPUTracePanel::GetMode(void)
{
    if (!m_enable)
        return 0;
    unsigned int r(0);
    switch (m_enable->GetSelection())
    {
    default:
        break;

    case 1:
        r = !!m_dofetch;
        break;

    case 2:
        r = 2;
        break;
    }
    m_dofetch = false;
    return r;
}

void AXS_driver::CPUTracePanel::OnModeChange(wxCommandEvent& event)
{
    cbAssert(m_driver);
    m_driver->OnCPUTraceChange();
}

void AXS_driver::CPUTracePanel::OnHistoryChange(wxSpinEvent& event)
{
    cbAssert(m_driver);
    m_driver->OnCPUTraceChange();
}

void AXS_driver::CPUTracePanel::OnFetchClicked(wxCommandEvent& event)
{
    m_dofetch = true;
    cbAssert(m_driver);
    m_driver->OnCPUTraceChange();
}

void AXS_driver::CPUTracePanel::OnClearClicked(wxCommandEvent& event)
{
    if (!m_list)
        return;
    m_list->Clear();
}

void AXS_driver::CPUTracePanel::OnSaveClicked(wxCommandEvent& event)
{
    if (!m_list)
        return;
    wxFileDialog dlg(this,
                     _("Save as CSV file"),
                     _T("cputrace.csv"),
                     wxEmptyString,
                     FileFilters::GetFilterAll(),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    PlaceWindow(&dlg);
    if (dlg.ShowModal() != wxID_OK)
        return;
    if (m_list->SaveCSV(dlg.GetPath()) == -1)
        cbMessageBox(_("Could not save file..."), _("Error"), wxICON_ERROR);
}

void AXS_driver::CPUTracePanel::OnListDoubleClick(wxListEvent& event)
{
    if (!m_list)
        return;
    if (m_list->GetSelectedItemCount() == 0)
        return;
    // find selected item index
    int index = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    std::pair<wxString,unsigned int> info(m_list->GetSourceInfo(index));
    if (info.first.IsEmpty())
        return;
    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if(plugin)
        plugin->SyncEditor(info.first, info.second, false);
}

BEGIN_EVENT_TABLE(AXS_driver::ProfilerPanel, wxPanel)
    EVT_LIST_ITEM_ACTIVATED(idProfilerList, AXS_driver::ProfilerPanel::OnListDoubleClick)
    EVT_RADIOBOX(idProfilerMode, AXS_driver::ProfilerPanel::OnModeChange)
    EVT_SPINCTRL(idProfilerSamples, AXS_driver::ProfilerPanel::OnSamplesChange)
END_EVENT_TABLE()

int AXS_driver::ProfilerPanel::ProfileEntry::compare(const ProfileEntry& x) const
{
        if (get_addr() < x.get_addr())
                return -1;
        if (get_addr() > x.get_addr())
                return 1;
        if (get_line() < x.get_line())
                return -1;
        if (get_line() > x.get_line())
                return 1;
        if (get_level() < x.get_level())
                return -1;
        if (get_level() > x.get_level())
                return 1;
        if (get_block() < x.get_block())
                return -1;
        if (get_block() > x.get_block())
                return 1;
        if (x.is_asm() && !is_asm())
                return -1;
        if (is_asm() && !x.is_asm())
                return 1;
        return get_name().Cmp(x.get_name());
}

wxString AXS_driver::ProfilerPanel::ProfileEntry::get_locstr() const
{
    if (is_asm())
        return wxString::Format(wxT("0x%04x"), get_addr());
    wxString r(get_name() + wxString::Format(wxT(":%u"), get_line()));
    if (get_level() || get_block())
        r += wxString::Format(wxT(",%u,%u"), get_level(), get_block());
    return r;
}

class AXS_driver::ProfilerPanel::SortDescHitcount {
public:
        bool operator()(const ProfileEntry& a, const ProfileEntry& b) const { return a.get_hitcount() > b.get_hitcount(); }
};

const wxString AXS_driver::ProfilerPanel::modechoices[] = 
{
    wxT("Off"),
    wxT("C"),
    wxT("Asm"),
    wxT("C+Asm")
};

AXS_driver::ProfilerPanel::ProfilerPanel(wxWindow *parent, AXS_driver *driver)
    : wxPanel(parent),
    m_driver(driver),
    m_list(0),
    m_enable(0),
    m_samples(0)
{
    m_list = new wxListCtrl(this, idProfilerList, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    wxStaticLine *line = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    m_enable = new wxRadioBox(this, idProfilerMode, wxT("Mode"), wxDefaultPosition, wxDefaultSize,
                              sizeof(modechoices)/sizeof(modechoices[0]), modechoices, 1, wxRA_SPECIFY_ROWS);
    wxStaticText *txt1 = new wxStaticText(this, wxID_ANY, wxT("Samples"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_samples = new wxSpinCtrl(this, idProfilerSamples, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 100000, 1000);
    wxBoxSizer *bshi = new wxBoxSizer(wxHORIZONTAL);
    bshi->Add(txt1, 0, wxEXPAND | wxALL);
    bshi->Add(m_samples, 0, wxALL);
    wxBoxSizer *bsh = new wxBoxSizer(wxHORIZONTAL);
    bsh->Add(m_enable, 1, wxEXPAND | wxALL);
    bsh->Add(bshi, 0, wxALL);
    wxBoxSizer *bs = new wxBoxSizer(wxVERTICAL);
    bs->Add(m_list, 1, wxEXPAND | wxALL);
    bs->Add(line, 0, wxEXPAND | wxALL);
    bs->Add(bsh, 0, wxEXPAND | wxALL);
    SetAutoLayout(true);
    SetSizer(bs);

    wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);
    m_list->SetFont(font);
    m_list->InsertColumn(0, _("File/Address"), wxLIST_FORMAT_LEFT, 10);
    m_list->InsertColumn(1, _("Lang"), wxLIST_FORMAT_LEFT, 15);
    m_list->InsertColumn(2, _("Percent"), wxLIST_FORMAT_RIGHT);
    m_list->InsertColumn(3, _("Hitcount"), wxLIST_FORMAT_RIGHT);
}

void AXS_driver::ProfilerPanel::Add(const wxString& name, unsigned int line, unsigned int level, unsigned int block,
                                    bool isasm, unsigned int addr, unsigned int hitcount)
{
    m_profile.insert(ProfileEntry(name, line, level, block, isasm, addr, hitcount));
}

void AXS_driver::ProfilerPanel::Add()
{
    {
        profilevector_t prof(m_profile.begin(), m_profile.end());
        m_sortedprofile.swap(prof);
    }
    m_profile.clear();
    std::sort(m_sortedprofile.begin(), m_sortedprofile.end(), SortDescHitcount());
    unsigned int tothitcnt(0);
    for (profilevector_t::const_iterator pi(m_sortedprofile.begin()), pe(m_sortedprofile.end()); pi != pe; ++pi)
        tothitcnt += pi->get_hitcount();
    double scale(0);
    if (tothitcnt)
        scale = 100.0 / tothitcnt;
    m_list->Freeze();
    m_list->DeleteAllItems();
    for (profilevector_t::const_iterator pi(m_sortedprofile.begin()), pe(m_sortedprofile.end()); pi != pe; ++pi)
    {
        long index = m_list->InsertItem(m_list->GetItemCount(), pi->get_locstr());
        m_list->SetItem(index, 1, pi->is_asm() ? wxT("Asm") : wxT("C"));
        m_list->SetItem(index, 2, wxString::Format(wxT("%5.1f%%"), scale * pi->get_hitcount()));
        m_list->SetItem(index, 3, wxString::Format(wxT("%d"), pi->get_hitcount()));
    }
    m_list->EnsureVisible(0);
    m_list->Thaw();
    for (int ii = 0; ii < m_list->GetColumnCount(); ++ii)
    {
        m_list->SetColumnWidth(ii, wxLIST_AUTOSIZE);
    }
}

void AXS_driver::ProfilerPanel::OnModeChange(wxCommandEvent& event)
{
    cbAssert(m_driver);
    m_driver->OnProfilerChange();
}

void AXS_driver::ProfilerPanel::OnSamplesChange(wxSpinEvent& event)
{
    cbAssert(m_driver);
    m_driver->OnProfilerChange();
}

void AXS_driver::ProfilerPanel::OnListDoubleClick(wxListEvent& event)
{
    if (!m_list)
        return;
    if (m_list->GetSelectedItemCount() == 0)
        return;
    // find selected item index
    int index = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (index < 0 || index >= m_sortedprofile.size())
        return;
    const ProfileEntry& e(m_sortedprofile[index]);
    if (e.get_name().IsEmpty())
        return;
    cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
    if(plugin)
        plugin->SyncEditor(e.get_name(), e.get_line(), false);
}

AXS_driver::ProfilerPanel::mode_t AXS_driver::ProfilerPanel::GetMode(void) const
{
    if (!m_enable)
        return mode_off;
    int m(m_enable->GetSelection());
    if (m < 0 || m > 3)
        return mode_off;
    return (mode_t)m;
}

unsigned int AXS_driver::ProfilerPanel::GetSamples(void) const
{
    if (!m_samples)
        return 0;
    return m_samples->GetValue();
}

AXS_driver::AXS_driver(DebuggerAXS* plugin)
    : DebuggerDriver(plugin),
    m_CygwinPresent(false),
    m_IsStarted(false),
    m_pTarget(0),
    m_compilervendor(compilervendor_unknown),
    m_IsInitializing(true),
    m_LiveUpdate(false),
    m_cputracepanel(0),
    m_profilerpanel(0)
{
    //ctor
    m_registers.reset(new AXSRegister(wxT("root")));
    m_registers->Expand(true);
    if (plugin)
        plugin->SetEnabledTools(cbDebuggerPlugin::DebuggerToolbarTools::Debug |
                                cbDebuggerPlugin::DebuggerToolbarTools::RunToCursor |
                                cbDebuggerPlugin::DebuggerToolbarTools::Step);
    // create CPUTrace Panel
    m_cputracepanel = new CPUTracePanel(Manager::Get()->GetAppWindow(), this);
    {
        CodeBlocksDockEvent evt(cbEVT_ADD_DOCK_WINDOW);
        evt.name = _T("CPUTracePane");
        evt.title = _("CPU Trace");
        evt.pWindow = m_cputracepanel;
        evt.dockSide = CodeBlocksDockEvent::dsFloating;
        evt.desiredSize.Set(350, 75);
        evt.floatingSize.Set(450, 75);
        evt.minimumSize.Set(250, 75);
        Manager::Get()->ProcessEvent(evt);
    }
    // create Profiler Panel
    m_profilerpanel = new ProfilerPanel(Manager::Get()->GetAppWindow(), this);
    {
        CodeBlocksDockEvent evt(cbEVT_ADD_DOCK_WINDOW);
        evt.name = _T("ProfilerPane");
        evt.title = _("Profiler");
        evt.pWindow = m_profilerpanel;
        evt.dockSide = CodeBlocksDockEvent::dsFloating;
        evt.desiredSize.Set(350, 75);
        evt.floatingSize.Set(450, 75);
        evt.minimumSize.Set(250, 75);
        Manager::Get()->ProcessEvent(evt);
    }
}

AXS_driver::~AXS_driver()
{
    //dtor
    if (m_cputracepanel)
    {
        wxWindow *window(m_cputracepanel->GetWindow());
        CodeBlocksDockEvent evt(cbEVT_REMOVE_DOCK_WINDOW);
        evt.pWindow = window;
        Manager::Get()->ProcessEvent(evt);
        window->Destroy();
    }
    if (m_profilerpanel)
    {
        wxWindow *window(m_profilerpanel->GetWindow());
        CodeBlocksDockEvent evt(cbEVT_REMOVE_DOCK_WINDOW);
        evt.pWindow = window;
        Manager::Get()->ProcessEvent(evt);
        window->Destroy();
    }
}

const std::string& AXS_driver::to_str(filetype_t ft)
{
    switch (ft) {
    case filetype_omf51:
    {
        static const std::string r("omf51");
        return r;
    }

    case filetype_ihex:
    {
        static const std::string r("ihex");
        return r;
    }

    case filetype_cdb:
    {
        static const std::string r("cdb");
        return r;
    }

    case filetype_ubrof:
    {
        static const std::string r("ubrof");
        return r;
    }

    case filetype_unknown:
    {
        static const std::string r("unknown");
        return r;
    }

    default:
    {
        static const std::string r("?");
        return r;
    }
    }
}

const std::string& AXS_driver::to_str(compilervendor_t cv)
{
    switch (cv) {
    case compilervendor_unknown:
    {
        static const std::string r("unknown");
        return r;
    }

    case compilervendor_sdcc:
    {
        static const std::string r("sdcc");
        return r;
    }

    case compilervendor_keil:
    {
        static const std::string r("keil");
        return r;
    }

    case compilervendor_iar:
    {
        static const std::string r("iar");
        return r;
    }

    case compilervendor_wickenhaeuser:
    {
        static const std::string r("wickenhaeuser");
        return r;
    }

    case compilervendor_noice:
    {
        static const std::string r("noice");
        return r;
    }

    default:
    {
        static const std::string r("??");
        return r;
    }
    }
}

AXS_driver::filetype_t AXS_driver::determine_filetype(std::istream& is)
{
    if (!is.good())
        return filetype_unknown;
    filetype_t ret(filetype_unknown);
    std::string line;
    line.resize(3, 0);
    is.read(&line[0], 3);
    if (!is) {
        // read error
    } else if (line[0] == 'M' && line[1] == ':') {
        ret = filetype_cdb;
    } else if (line[0] == ':') {
        // cannot use std::getline here, as getline discards the delimiter
        while (!is.eof()) {
            char ch;
            is.read(&ch, 1);
            line += ch;
            if (ch == '\r' || ch == '\n')
                break;
        }
        ret = filetype_ihex;
        std::string::const_iterator li(line.begin()), le(line.end());
        if (li == le || *li != ':') {
            ret = filetype_unknown;
        } else {
            ++li;
            typedef std::vector<uint8_t> bvec_t;
            bvec_t b;
            while (li != le) {
                if (*li == '\r' || *li == '\n')
                    break;
                bool ok(true);
                uint8_t val(0);
                for (unsigned int i = 0; i < 2; ++i) {
                    ok = (li != le);
                    if (!ok)
                        break;
                    val <<= 4;
                    if (*li >= '0' && *li <= '9') {
                        val |= *li - '0';
                    } else if (*li >= 'A' && *li <= 'F') {
                        val |= *li - 'A' + 10;
                    } else if (*li >= 'a' && *li <= 'f') {
                        val |= *li - 'a' + 10;
                    } else {
                        ok = false;
                        break;
                    }
                    ++li;
                }
                if (!ok) {
                    b.clear();
                    break;
                }
                b.push_back(val);
            }
            if (b.size() < 5) {
                ret = filetype_unknown;
            } else {
                // check checksum
                uint8_t s(0);
                for (bvec_t::const_iterator bi(b.begin()), be(b.end()); bi != be; ++bi)
                    s += *bi;
                if (s) {
                    ret = filetype_unknown;
                }
            }
        }
    } else if (line[0] == 2 || line[0] == 0x70) {
        // Keil extended OMF starts with 0x70; standard Intel OMF starts with 0x02 (Module HDR record)
        ret = filetype_omf51;
        {
            unsigned int reclen = ((line[2] & 0xff) << 8) | (line[1] & 0xff);
            if (reclen) {
                line.resize(3 + reclen, 0);
                is.read(&line[3], line.size() - 3);
                if (!is)
                    ret = filetype_unknown;
            }
        }
        // check checksum
        if (ret == filetype_omf51) {
            uint8_t s(0);
            for (std::string::const_iterator bi(line.begin()), be(line.end()); bi != be; ++bi)
                s += *bi;
            if (s)
                ret = filetype_unknown;
        }
    } else if (line[0] == (char)165) {
        ret = filetype_ubrof;
    }
    for (std::string::const_reverse_iterator si(line.rbegin()), se(line.rend()); si != se; ++si)
        is.putback(*si);
    return ret;
}

AXS_driver::filetype_t AXS_driver::determine_filetype(const std::string& filename)
{
    std::ifstream is(filename.c_str(), std::ios::in | std::ios::binary);
    if (!is.is_open())
        return filetype_unknown;
    return determine_filetype(is);
}

AXS_driver::filetype_t AXS_driver::determine_filetype(const wxFileName& filename)
{
    std::ifstream is(filename.GetFullPath().mb_str(), std::ios::in | std::ios::binary);
    if (!is.is_open())
        return filetype_unknown;
    return determine_filetype(is);
}

void AXS_driver::InitializeScripting()
{

}

void AXS_driver::RegisterType(const wxString& name, const wxString& regex, const wxString& eval_func, const wxString& parse_func)
{

}

wxString AXS_driver::GetScriptedTypeCommand(const wxString& gdb_type, wxString& parse_func)
{
    return _T("");
}

wxString AXS_driver::GetCommandLine(const wxString& debugger, const wxString& debuggee)
{
    return _("");
}

wxString AXS_driver::GetCommandLine(const wxString& debugger, int pid)
{
    return _("");
}

void AXS_driver::SetTarget(ProjectBuildTarget* target)
{
    m_pTarget = target;
}

void AXS_driver::Prepare(bool isConsole, bool liveUpdate)
{
    // for the possibility that the program to be debugged is compiled under Cygwin
    if(platform::windows)
        DetectCygwinMount();

    SetCpuState(cpustate_targetdisconnected);

    // pass user init-commands
    wxString init = m_pDBG->GetActiveConfigEx().GetInitCommands();
    Manager::Get()->GetMacrosManager()->ReplaceMacros(init);
    wxArrayString init_lines = GetArrayFromString(init, _T('\n'));
    for (unsigned int l = 0; l < init_lines.GetCount(); ++l)
    {
        Opt cmd(init_lines[l]);
        QueueCommand(new DebuggerCmd_Simple(this, cmd, true, false, false, true));
    }

    /// Reset PinEmulation
    axs_cbPinEmDlg *PEMdialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
    PEMdialog->Reset(false);

    {
        wxArrayString as;
        as.Add(wxT("code"));
        as.Add(wxT("direct"));
        as.Add(wxT("indirect"));
        as.Add(wxT("x"));
        as.Add(wxT("sfr"));
        as.Add(wxT("p"));
        as.Add(wxT("flash"));
        cbExamineMemoryDlg *MEMdialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
        MEMdialog->SetAddressSpaces(as);
    }

    /// Activate prompt of DebuggerLink
    axs_cbDbgLink *DLdialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
    DLdialog->TerminalEnable(true);

    m_AxsdbVersion.Clear();

    //QueueCommand(new DebuggerCmd(this, wxT("traceio severity=normal file=tracefile.log")));

    m_IsInitializing = true;
    UpdateEnabledTools();
    m_LiveUpdate = liveUpdate;
}

void AXS_driver::NotifyInitDone()
{
    m_IsInitializing = false;
    UpdateEnabledTools();
}

// Cygwin check code
#ifdef __WXMSW__

enum{ BUFSIZE = 64 };

// routines to handle cygwin compiled programs on a Windows compiled C::B IDE
void AXS_driver::DetectCygwinMount(void)
{
    LONG lRegistryAPIresult;
    HKEY hKey_CU;
    HKEY hKey_LM;
    TCHAR szCygwinRoot[BUFSIZE];
    DWORD dwBufLen=BUFSIZE*sizeof(TCHAR);

    // checking if cygwin mounts are present under HKCU
    lRegistryAPIresult = RegOpenKeyEx( HKEY_CURRENT_USER,
                         TEXT("Software\\Cygnus Solutions\\Cygwin\\mounts v2"),
                         0, KEY_QUERY_VALUE, &hKey_CU );
    if( lRegistryAPIresult == ERROR_SUCCESS )
    {
        // try to readback cygwin root (might not exist!)
        lRegistryAPIresult = RegQueryValueEx( hKey_CU, TEXT("cygdrive prefix"), NULL, NULL,
                             (LPBYTE) szCygwinRoot, &dwBufLen);
    }

    // lRegistryAPIresult can be erroneous for two reasons:
    // 1.) Cygwin entry is not present (could not be opened) in HKCU
    // 2.) "cygdrive prefix" is not present (could not be read) in HKCU
    if( lRegistryAPIresult != ERROR_SUCCESS )
    {
        // Now check if probably present under HKLM
        lRegistryAPIresult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2"),
                             0, KEY_QUERY_VALUE, &hKey_LM );
        if( lRegistryAPIresult != ERROR_SUCCESS )
        {
            // cygwin definitely not installed
            m_CygwinPresent = false;
            return;
        }

        // try to readback cygwin root (now it really should exist here)
        lRegistryAPIresult = RegQueryValueEx( hKey_LM, TEXT("cygdrive prefix"), NULL, NULL,
                             (LPBYTE) szCygwinRoot, &dwBufLen);
    }

    // handle a possible query error
     if( (lRegistryAPIresult != ERROR_SUCCESS) || (dwBufLen > BUFSIZE*sizeof(TCHAR)) )
    {
        // bit of an assumption, but we won't be able to find the root without it
        m_CygwinPresent = false;
        return;
    }

    // close opened keys
    RegCloseKey( hKey_CU ); // ignore key close errors
    RegCloseKey( hKey_LM ); // ignore key close errors

    m_CygwinPresent  = true;           // if we end up here all was OK
    m_CygdrivePrefix = (szCygwinRoot); // convert to wxString type for later use
}

void AXS_driver::CorrectCygwinPath(wxString& path)
{
    unsigned int i=0, EscCount=0;

    // preserve any escape characters at start of path - this is true for
    // breakpoints - value is 2, but made dynamic for safety as we
    // are only checking for the CDprefix not any furthur correctness
    if(path.GetChar(0)== g_EscapeChar)
    {
        while((i<path.Len()) && (path.GetChar(i)==g_EscapeChar))
        {
            // get character
            EscCount++;
            i++;
        }
    }

    // prepare to convert to a valid path if Cygwin is being used

    // step over the escape characters
    wxString PathWithoutEsc(path); PathWithoutEsc.Remove(0, EscCount);

    if(PathWithoutEsc.StartsWith(m_CygdrivePrefix))
    {
        // remove cygwin prefix
        if (m_CygdrivePrefix.EndsWith(_T("/"))) // for the case   "/c/path"
          PathWithoutEsc.Remove(0, m_CygdrivePrefix.Len()  );
        else                                    // for cases e.g. "/cygdrive/c/path"
          PathWithoutEsc.Remove(0, m_CygdrivePrefix.Len()+1);

        // insert ':' after drive label by reading and removing drive the label
        // and adding ':' and the drive label back
        wxString DriveLetter = PathWithoutEsc.GetChar(0);
        PathWithoutEsc.Replace(DriveLetter, DriveLetter + _T(":"), false);
    }

    // Compile corrected path
    path = wxEmptyString;
    for (unsigned int i=0; i<EscCount; i++)
        path += g_EscapeChar;
    path += PathWithoutEsc;
}
#else
    void AXS_driver::DetectCygwinMount(void){/* dummy */}
    void AXS_driver::CorrectCygwinPath(wxString& /*path*/){/* dummy */}
#endif

#ifdef __WXMSW__
bool AXS_driver::UseDebugBreakProcess()
{
    return true;
}
#endif

wxString AXS_driver::GetDisassemblyFlavour(void)
{
    return wxT("intel");
}

// Only called from DebuggerAXS::Debug
void AXS_driver::Start(bool breakOnEntry)
{
    ResetCursor();
    if (Manager::Get()->GetDebuggerManager()->UpdateDisassembly())
    {
        cbDisassemblyDlg *disassembly_dialog = Manager::Get()->GetDebuggerManager()->GetDisassemblyDialog();
        disassembly_dialog->Clear(cbStackFrame());
    }
    QueueCommand(new AxsCmd_ConnectTargetDownload(this, m_registers, breakOnEntry ? AxsCmd_ConnectTargetDownload::mode_break : AxsCmd_ConnectTargetDownload::mode_run));
}

void AXS_driver::Stop()
{
    ResetCursor();
    QueueCommand(new AxsCmd_Quit(this));
    m_IsStarted = false;
    m_LiveUpdate = false;
    m_IsInitializing = true;
}

void AXS_driver::Pause()
{
    ResetCursor();
    QueueCommand(new AxsCmd_Stop(this));
    m_IsStarted = false;
}

void AXS_driver::Continue()
{
    ResetCursor();
    QueueCommand(new AxsCmd_Run(this));
    m_IsStarted = true;
}

void AXS_driver::Step()
{
    ResetCursor();
    QueueCommand(new AxsCmd_StepLine(this));
}

void AXS_driver::StepInstruction()
{
    ResetCursor();
    QueueCommand(new AxsCmd_Step(this));
}

void AXS_driver::StepIntoInstruction()
{
}

void AXS_driver::StepIn()
{
    ResetCursor();
    QueueCommand(new AxsCmd_StepInto(this));
}

void AXS_driver::StepOut()
{
    ResetCursor();
    QueueCommand(new AxsCmd_StepOut(this));
}

void AXS_driver::SetNextStatement(const wxString& filename, int line)
{
}

void AXS_driver::Backtrace()
{
    QueueCommand(new AxsCmd_Backtrace(this));
}

void AXS_driver::Disassemble()
{
    if (!m_ProgramIsStopped)
    {
        if (Manager::Get()->GetDebuggerManager()->UpdateDisassembly())
        {
            cbDisassemblyDlg *disassembly_dialog = Manager::Get()->GetDebuggerManager()->GetDisassemblyDialog();
            disassembly_dialog->Clear(cbStackFrame());
        }
        return;
    }
    QueueCommand(new AxsCmd_Disassemble(this));
}

void AXS_driver::CPURegisters()
{
    if (m_registers && GetCpuState() == cpustate_halt)
        QueueCommand(new AxsCmd_ReadRegisters(this, m_registers, AxsCmd_ReadRegisters::Recurse));
}

void AXS_driver::SwitchToFrame(size_t number)
{
}

void AXS_driver::SetVarValue(const wxString& var, const wxString& value)
{
    Opt cmd("cexpr");
    cmd.set_option("lvalue", 1);
    cmd.set_option("pc", "current");
    cmd.set_option("expr", var + wxT("=") + value);
    QueueCommand(new DebuggerCmd_Simple(this, cmd, false, true, true, false));
}

void AXS_driver::SetRegValue(const wxString& addrspace, unsigned int addr, const wxString& value)
{
    wxString val(value);
    val.Trim(false);
    val.Trim(true);
    unsigned long v;
    if (val.StartsWith(wxT("0b")) || val.StartsWith(wxT("0B")))
    {
        if (!val.Mid(2).ToULong(&v, 2))
            return;
    }
    else
    {
        if (!val.ToULong(&v, 0))
            return;
    }
    if (addrspace.IsEmpty())
    {
        QueueCommand(new AxsCmd_WritePC(this, v));
        return;
    }
    QueueCommand(new AxsCmd_WriteRegister(this, addrspace, addr, v));
}

void AXS_driver::MemoryDump()
{
    if (GetCpuState() == cpustate_halt)
        QueueCommand(new AxsCmd_ExamineMemory(this));
}

void AXS_driver::RunningThreads()
{
}

void AXS_driver::Poll()
{
    if (m_IsInitializing || !m_LiveUpdate)
        return;
    if (GetCpuState() != cpustate_run)
        return;
    QueueCommand(new AxsCmd_ReadPC(this, true, m_registers));
}

// Custom window to display output of DebuggerInfoCmd
class DebuggerInfoWindow : public wxScrollingDialog
{
    public:
        DebuggerInfoWindow(wxWindow *parent, const wxChar *title, const wxString& content)
            : wxScrollingDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
        {
            wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            wxFont font(8, wxMODERN, wxNORMAL, wxNORMAL);
            m_pText = new wxTextCtrl(this, -1, content, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);
            m_pText->SetFont(font);

            sizer->Add(m_pText, 1, wxGROW);

            SetSizer(sizer);
            sizer->Layout();
        }
        wxTextCtrl* m_pText;
};

void AXS_driver::InfoFrame()
{
}

void AXS_driver::InfoDLL()
{
}

void AXS_driver::InfoFiles()
{
    wxString txt;

    if (!m_AxsdbVersion.IsEmpty())
        txt << _T("Axsdb Version: ") << m_AxsdbVersion << _("\n\n");
    txt << _("Binary File: ") << GetBinFile().GetFullPath()
        << _(" type ") << wxString(to_str(determine_filetype(GetBinFile())).c_str(), wxConvUTF8) << _("\n")
        << _("Symbol File: ") << GetSymFile().GetFullPath()
        << _(" type ") << wxString(to_str(determine_filetype(GetSymFile())).c_str(), wxConvUTF8) << _("\n\n");

    txt << _("Debugger Command Queues\n\n  Sequence Number: ") << m_CmdSequence << _("\n\nRun Queue:\n");
    for (int i = 0; i < (int)m_RunDCmds.GetCount(); ++i)
        txt << _("  ") << m_RunDCmds[i].DebugInfo() << _("\n");
    txt << _("\nWait Queue:\n");
    for (int i = 0; i < (int)m_DCmds.GetCount(); ++i)
        txt << _("  ") << m_DCmds[i].DebugInfo() << _("\n");

    DebuggerInfoWindow win(Manager::Get()->GetAppWindow(), _("Files"), txt);
    win.ShowModal();

    // FIXME
    for (;;) {
        bool work = RunQueue();
        PruneRunQueue();
        if (!work)
            break;
    }
}

void AXS_driver::InfoFPU()
{
}

void AXS_driver::InfoSignals()
{
}

void AXS_driver::InfoCPUTrace()
{
    CodeBlocksDockEvent evt(cbEVT_SHOW_DOCK_WINDOW);
    evt.pWindow = m_cputracepanel->GetWindow();
    Manager::Get()->ProcessEvent(evt);
}

void AXS_driver::InfoProfiler()
{
    CodeBlocksDockEvent evt(cbEVT_SHOW_DOCK_WINDOW);
    evt.pWindow = m_profilerpanel->GetWindow();
    Manager::Get()->ProcessEvent(evt);
}

void AXS_driver::OnCPUTraceChange()
{
    QueueCommand(new AxsCmd_CPUTrace(this, m_cputracepanel->GetBuffer(), m_cputracepanel->GetMode(), true));
}

void AXS_driver::OnProfilerChange()
{
    QueueCommand(new AxsCmd_Profile(this, m_profilerpanel->GetSamples(),
                                    !!(m_profilerpanel->GetMode() & ProfilerPanel::mode_c),
                                    !!(m_profilerpanel->GetMode() & ProfilerPanel::mode_asm)));
}

void AXS_driver::SwitchThread(size_t threadIndex)
{
}

void AXS_driver::AddBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp)
{
    QueueCommand(new AxsCmd_AddBreakpoint(this, bp));
}

void AXS_driver::RemoveBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp)
{
    if (bp && bp->index != -1)
        QueueCommand(new AxsCmd_RemoveBreakpoint(this, bp));
}

void AXS_driver::EvaluateSymbol(const wxString& symbol, const wxRect& tipRect)
{
    QueueCommand(new AxsCmd_TooltipEvaluation(this, symbol, tipRect));
}

void AXS_driver::UpdateWatches(WatchesContainer &watches)
{
    for(WatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
    {
        WatchesContainer::reference watch = *it;
        if (watch->IsAutoUpdateEnabled())
        {
            QueueCommand(new AxsCmd_Watch(this, watch, false));
        }
    }
    // run this action-only command to update the tree
    QueueCommand(new DbgCmd_UpdateWatchesTree(this));
}

void AXS_driver::UpdateWatch(cb::shared_ptr<GDBWatch> const &watch)
{
    QueueCommand(new AxsCmd_Watch(this, watch, true));
    // run this action-only command to update the tree
    QueueCommand(new DbgCmd_UpdateWatchesTree(this));
}

void AXS_driver::ExpandWatch(cb::shared_ptr<GDBWatch> watch)
{
}

void AXS_driver::CollapseWatch(cb::shared_ptr<GDBWatch> watch)
{
}

void AXS_driver::ExpandRegister(cb::shared_ptr<AXSRegister> reg)
{
    if (GetCpuState() == cpustate_halt)
        QueueCommand(new AxsCmd_ReadRegisters(this, reg, AxsCmd_ReadRegisters::Recurse));
}

void AXS_driver::CollapseRegister(cb::shared_ptr<AXSRegister> reg)
{
}

void AXS_driver::UpdateRegister(cb::shared_ptr<AXSRegister> reg)
{
    int flags(AxsCmd_ReadRegisters::Recurse | AxsCmd_ReadRegisters::Force);
    if (reg && !reg->IsReadSafe())
        flags |= AxsCmd_ReadRegisters::Unsafe;
    if (!reg)
        reg = m_registers;
    if (GetCpuState() == cpustate_halt)
        QueueCommand(new AxsCmd_ReadRegisters(this, reg, flags));
}

void AXS_driver::SetChip(const wxString& chip)
{
    if (chip.IsEmpty())
        QueueCommand(new AxsCmd_AutodetectChip(this, true));
    else
        QueueCommand(new AxsCmd_SetChip(this, chip, true));
    QueueCommand(new AxsCmd_RegisterList(this, m_registers, true));
}

void AXS_driver::MarkAllRegistersAsUnchanged()
{
    m_registers->MarkAsChangedRecursive(false);
}

void AXS_driver::Attach()
{
    ResetCursor();
    QueueCommand(new AxsCmd_ConnectTargetDownload(this, m_registers, AxsCmd_ConnectTargetDownload::mode_attach));
}

void AXS_driver::Detach()
{
    ResetCursor();
    QueueCommand(new AxsCmd_Quit(this));
    m_IsStarted = false;
    m_LiveUpdate = false;
    m_IsInitializing = true;
}

void AXS_driver::HWR()
{
    QueueCommand(new AxsCmd_HardwareReset(this));
}

void AXS_driver::SWR()
{
    QueueCommand(new AxsCmd_SoftwareReset(this));
}

void AXS_driver::PinEmulation()
{
    QueueCommand(new AxsCmd_PinEmulation(this));
}

void AXS_driver::DebugLink()
{
    QueueCommand(new AxsCmd_DebugLink(this));
}

bool AXS_driver::ParseStatus(const Opt& optline)
{
    bool chg(false);

    if (optline.is_cmd_status()) {
        if (GetCpuState() == cpustate_targetdisconnected) {
            SetCpuState(cpustate_disconnected);
            chg = true;
        }
        std::pair<std::string,bool> status(optline.get_option("status"));
        if (status.second) {
            static const cpustate_t stats[] = {
                cpustate_disconnected,
                cpustate_locked,
                cpustate_halt,
                cpustate_run,
                cpustate_busy
            };
            int i = 0;
            for (; i < (int)(sizeof(stats) / sizeof(stats[0])); ++i) {
                cpustate_t cs(stats[i]);
                if (::to_str(cs) != status.first)
                    continue;
                chg = chg || GetCpuState() != cs;
                SetCpuState(cs);
            }
        }

        // Breakpoints
        if (0) {
            std::pair<Opt::intarray_t,bool> bpfired(optline.get_option_intarray("breakpoints"));
            if (bpfired.second && !bpfired.first.empty())
            {
                for (Opt::intarray_t::const_iterator bi(bpfired.first.begin()), be(bpfired.first.end()); bi != be; ++bi)
                {
                    cb::shared_ptr<DebuggerBreakpoint> bp(m_pDBG->GetState().GetBreakpointByNumber(*bi));
                }
            }
        }

        // Debug Link
        {
            std::pair<wxString,bool> dbglnkrx(optline.get_option_wxstring("dbglnkrx"));
            if (dbglnkrx.second)
            {
                axs_cbDbgLink *dialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
                dialog->AddReceive(dbglnkrx.first);
            }
            std::pair<unsigned long,bool> dbglnktxfree(optline.get_option_uint("dbglnktxfree"));
            std::pair<unsigned long,bool> dbglnktxcount(optline.get_option_uint("dbglnktxcount"));
            if (dbglnktxfree.second)
            {
                axs_cbDbgLink *dialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
                dialog->SetTransmitBuffer(dbglnktxfree.first, dbglnktxcount.second ? dbglnktxcount.first : 0);
            }
        }

        // Pin Emulation
        {
            std::pair<long,bool> peenable(optline.get_option_int("peenable"));
            if (peenable.second)
            {
                axs_cbPinEmDlg *dialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
                dialog->SetEnable(!!peenable.first);
            }
        }
        {
            std::pair<long,bool> pedir(optline.get_option_int("pedirb6"));
            std::pair<long,bool> pedrv(optline.get_option_int("pedrvb6"));
            std::pair<long,bool> peport(optline.get_option_int("peportb6"));
            if (pedir.second && pedrv.second && peport.second)
            {
                axs_cbPinEmDlg *dialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
                dialog->SetPortB6(!!pedir.first, !!peport.first, !!pedrv.first);
            }
        }
        {
            std::pair<long,bool> pedir(optline.get_option_int("pedirb7"));
            std::pair<long,bool> pedrv(optline.get_option_int("pedrvb7"));
            std::pair<long,bool> peport(optline.get_option_int("peportb7"));
            if (pedir.second && pedrv.second && peport.second)
            {
                axs_cbPinEmDlg *dialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
                dialog->SetPortB7(!!pedir.first, !!peport.first, !!pedrv.first);
            }
        }
    }

    // Handle a few state changing commands
    if (optline.get_cmdname() == "disconnect_target") {
        chg = chg || GetCpuState() != cpustate_targetdisconnected;
        SetCpuState(cpustate_targetdisconnected);
        SetCurrentKey(ProjectTargetOptions::defaultKey);
    }

    if (optline.get_cmdname() == "connect") {
        std::pair<unsigned long long,bool> key(optline.get_option_ulong("key"));
        if (key.second)
            SetCurrentKey(key.first);
    }

    if (optline.get_cmdname() == "bulkerase")
        SetCurrentKey(ProjectTargetOptions::defaultKey);

    if (optline.get_cmdname() == "writekey") {
        std::pair<unsigned long long,bool> key(optline.get_option_ulong("key"));
        if (key.second)
            SetCurrentKey(GetCurrentKey() & key.first);
    }

    // Handle Busy Waiting Dialog
    if (GetCpuState() == cpustate_busy) {
        std::pair<long,bool> percent(optline.get_option_int("percent"));
        //std::cout << "Busy dialog: first " << percent.first << " second " << (percent.second ? "true" : "false") << std::endl;
        if (percent.second && percent.first >= 0) {
            if (!m_ProgDlg) {
                if (percent.first < 100) {
                    m_ProgDlg.reset(new wxProgressDialog(_T("Debugger busy"),
                                                         _T("Please wait while the FLASH memory is being programmed..."),
                                                         100, NULL, /* wxPD_APP_MODAL |*/ wxPD_AUTO_HIDE |
                                                         wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME));
                    if (percent.first > 0)
                        m_ProgDlg->Update(percent.first);
                }
            } else {
                m_ProgDlg->Update(percent.first);
            }
        } else {
            m_ProgDlg.reset();
        }
    } else {
        m_ProgDlg.reset();
    }

    // Handle State Changes
    if (chg) {
        m_ProgramIsStopped = (!m_IsInitializing && GetCpuState() == cpustate_halt);
        // update toolbar
        UpdateEnabledTools();
        // update register dialog flags
        {
            cbCPURegistersDlg *dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
            if (dialog)
            {
                int flags(0);
                if (!m_IsInitializing)
                    flags |= cbCPURegistersDlg::ChipSet;
                if (GetCpuState() == cpustate_halt)
                    flags |= cbCPURegistersDlg::ChipAutodetect;
                flags |= cbCPURegistersDlg::RegisterRead | cbCPURegistersDlg::RegisterWrite;
                dialog->EnableInteraction(flags);
                dialog->UpdateRegisters();
            }
        }

        if (true) {
            wxString output(_T(">> Status change: "));
            output << wxString(::to_str(GetCpuState()).c_str(), wxConvUTF8);
            m_pDBG->DebugLog(output);
        }
    }
    return chg;
}

void AXS_driver::UpdateEnabledTools()
{
    DebuggerAXS *plugin = dynamic_cast<DebuggerAXS *>(Manager::Get()->GetDebuggerManager()->GetActiveDebugger());
    if (!plugin)
        return;
    int tool_flags(cbDebuggerPlugin::DebuggerToolbarTools::None);
    if (m_IsInitializing) {
        tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Stop;
    } else {
        switch (GetCpuState()) {
        case cpustate_targetdisconnected:
        default:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Debug;
            break;

        case cpustate_disconnected:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Debug | cbDebuggerPlugin::DebuggerToolbarTools::HWR  |
                cbDebuggerPlugin::DebuggerToolbarTools::Stop;
            break;

        case cpustate_locked:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::HWR | cbDebuggerPlugin::DebuggerToolbarTools::Stop;
            break;

        case cpustate_halt:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Debug | cbDebuggerPlugin::DebuggerToolbarTools::Next |
                cbDebuggerPlugin::DebuggerToolbarTools::NextInstr | cbDebuggerPlugin::DebuggerToolbarTools::StepIntoInstr |
                cbDebuggerPlugin::DebuggerToolbarTools::Step | cbDebuggerPlugin::DebuggerToolbarTools::StepOut |
                cbDebuggerPlugin::DebuggerToolbarTools::Stop | cbDebuggerPlugin::DebuggerToolbarTools::HWR |
                cbDebuggerPlugin::DebuggerToolbarTools::SWR | cbDebuggerPlugin::DebuggerToolbarTools::Continue |
                cbDebuggerPlugin::DebuggerToolbarTools::RunToCursor | cbDebuggerPlugin::DebuggerToolbarTools::DebuggerIdle;
            break;

        case cpustate_run:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Stop | cbDebuggerPlugin::DebuggerToolbarTools::Break |
                cbDebuggerPlugin::DebuggerToolbarTools::HWR | cbDebuggerPlugin::DebuggerToolbarTools::Continue;
            break;

        case cpustate_busy:
            tool_flags = cbDebuggerPlugin::DebuggerToolbarTools::Stop;
            break;
        }
    }
    plugin->SetEnabledTools(tool_flags);
}

wxString AXS_driver::FilePathSearch(const wxString& filename)
{
    cbProject* project = Manager::Get()->GetProjectManager()->GetActiveProject();

    // sdcc debug info replaces spaces in filenames with underscores;
    // try to reverse that
    bool filenameonly(false);
    {
        wxFileName fname(filename);
	filenameonly = fname.GetPath().IsEmpty();
    }

    wxString unixfilename = UnixFilename(filename);
    {
        wxFileName fname(unixfilename);

        if (fname.IsAbsolute())
        {
            if (false)
                std::cerr << "FilePathSearch (abs): " << (const char *)filename.mb_str() << " -> " << (const char *)filename.mb_str() << std::endl;
            return filename;
        }

        if (project)
            fname.MakeAbsolute(project->GetBasePath());

        if (fname.FileExists())
        {
            if (false)
                std::cerr << "FilePathSearch (rel): " << (const char *)filename.mb_str() << " -> " << (const char *)filename.mb_str()
                          << " (full " << (const char *)fname.GetFullPath().mb_str() << ')' << std::endl;
            return filename;
        }
        if (filenameonly)
        {
            wxDir dir(fname.GetPath());
            if (dir.IsOpened())
            {
                wxString dfilename;
                bool cont(dir.GetFirst(&dfilename, wxEmptyString, wxDIR_FILES));
                while (cont)
                {
                    wxString dfilename1(dfilename);
                    dfilename1.Replace(wxT(" "), wxT("_"));
	            if (dfilename1 == filename)
                    {
                        if (false)
                            std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)dfilename.mb_str() << std::endl;
                        return dfilename;
                    }
                    cont = dir.GetNext(&dfilename);
                }
            }
        }
    }

    // Search Directories
    for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i) {
        wxFileName fname(unixfilename);
        fname.MakeAbsolute(m_Dirs[i]);
        if (false)
            std::cerr << "FilePathSearch: " << (const char *)unixfilename.mb_str() << " trying path " << (const char *)fname.GetFullPath().mb_str() << std::endl;
        if (fname.FileExists())
        {
            if (false)
                std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)fname.GetFullPath().mb_str() << std::endl;
            return fname.GetFullPath();
        }
        if (filenameonly)
        {
            wxDir dir(fname.GetPath());
            if (dir.IsOpened())
            {
                wxString dfilename;
                bool cont(dir.GetFirst(&dfilename, wxEmptyString, wxDIR_FILES));
                while (cont)
                {
                    wxString dfilename1(dfilename);
                    dfilename1.Replace(wxT(" "), wxT("_"));
	            if (dfilename1 == filename)
                    {
                        fname.SetFullName(dfilename);
                        if (false)
                            std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)fname.GetFullPath().mb_str() << std::endl;
                        return fname.GetFullPath();
                    }
                    cont = dir.GetNext(&dfilename);
                }
            }
        }
    }

    if (!filenameonly)
    {
        if (false)
            std::cerr << "FilePathSearch (?): " << (const char *)filename.mb_str() << " -> " << (const char *)filename.mb_str() << std::endl;
        return filename;
    }

    if (project)
    {
        wxDir dir(project->GetBasePath());
        if (dir.IsOpened())
        {
            std::vector<wxString> hdrfiles;
            wxString dfilename;
            bool cont(dir.GetFirst(&dfilename, wxEmptyString, wxDIR_FILES));
            while (cont)
            {
                wxString dfilename1(dfilename);
                dfilename1.Replace(wxT(" "), wxT("_"));
                wxFileName fname(dfilename1);
                if (!fname.GetName().CmpNoCase(filename))
                {
			if (!fname.GetExt().CmpNoCase(wxT("c")) || !fname.GetExt().CmpNoCase(wxT("cc")) || !fname.GetExt().CmpNoCase(wxT("cxx")))
                    {
                        if (false)
                            std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)dfilename.mb_str() << std::endl;
                        return dfilename;
                    }
                    hdrfiles.push_back(dfilename);
                }
                cont = dir.GetNext(&dfilename);
            }
            if (!hdrfiles.empty())
            {
                if (false)
                    std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)hdrfiles[0].mb_str() << std::endl;
                return hdrfiles[0];
            }
        }
    }

    // Search Directories
    for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i) {
        wxDir dir(m_Dirs[i]);
        if (dir.IsOpened())
        {
            std::vector<wxString> hdrfiles;
            wxString dfilename;
            bool cont(dir.GetFirst(&dfilename, wxEmptyString, wxDIR_FILES));
            while (cont)
            {
                wxString dfilename1(dfilename);
                dfilename1.Replace(wxT(" "), wxT("_"));
                wxFileName fname(dfilename1);
                if (!fname.GetName().CmpNoCase(filename))
                {
                    fname.SetFullName(dfilename);
                    fname.MakeAbsolute(m_Dirs[i]);
                    if (!fname.GetExt().CmpNoCase(wxT("c")) || !fname.GetExt().CmpNoCase(wxT("cc")) || !fname.GetExt().CmpNoCase(wxT("cxx")))
                    {
                        if (false)
                            std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)fname.GetFullPath().mb_str() << std::endl;
                        return fname.GetFullPath();
                    }
                    hdrfiles.push_back(fname.GetFullPath());
                }
                cont = dir.GetNext(&dfilename);
            }
            if (!hdrfiles.empty())
            {
                if (false)
                    std::cerr << "FilePathSearch: " << (const char *)filename.mb_str() << " -> " << (const char *)hdrfiles[0].mb_str() << std::endl;
                return hdrfiles[0];
            }
        }
    }

    if (false)
        std::cerr << "FilePathSearch (?): " << (const char *)filename.mb_str() << " -> " << (const char *)filename.mb_str() << std::endl;
    return filename;
}

void AXS_driver::UpdateProjectTargetOptions(void)
{
    if (!m_pTarget) {
        m_ProjTargetOpt = ProjectTargetOptions();
        return;
    }

    ProjectTargetOptionsMap& tomap(m_pDBG->GetProjectTargetOptionsMap());

    // first, project-level (straight copy)
    m_ProjTargetOpt = tomap[0];

    // then merge with target settings
    ProjectTargetOptionsMap::iterator it = tomap.find(m_pTarget);
    if (it != tomap.end()) {
        m_ProjTargetOpt.MergeWith(it->second);
    }
}

void AXS_driver::FindProgramFiles(void)
{
    m_BinFile.Clear();
    m_SymFile.Clear();
    static const struct {
        wxString id;
        compilervendor_t vnd;
    } compilertable[] = {
        { _T("sdcc"), compilervendor_sdcc },
        { _T("keil"), compilervendor_keil },
        { _T("iar"), compilervendor_iar },
        { _T("wickenhaeuser"), compilervendor_wickenhaeuser }
    };
    m_compilervendor = compilervendor_unknown;
    cbProject* prj = Manager::Get()->GetProjectManager()->GetActiveProject();
    if (!prj)
        return;
    // determine compiler
    wxString compid(prj->GetCompilerID());
    for (int i = 0; i < (int)(sizeof(compilertable)/sizeof(compilertable[0])); ++i)
    {
        if (!compid.StartsWith(compilertable[i].id))
            continue;
        m_compilervendor = compilertable[i].vnd;
        break;
    }
    // determine files
    ProjectBuildTarget* target = prj->GetBuildTarget(prj->GetActiveBuildTarget());
    wxFileName tfile = target->GetOutputFilename();
    if (!tfile.IsOk())
        return;
    filetype_t binft(filetype_unknown);
    filetype_t symft(filetype_unknown);
    if (wxFileName::IsFileReadable(tfile.GetFullPath())) {
        filetype_t ft(determine_filetype(tfile));
        switch (ft) {
        case filetype_omf51:
        case filetype_ihex:
        case filetype_ubrof:
            m_BinFile = tfile;
            binft = ft;
            break;

        case filetype_cdb:
            m_SymFile = tfile;
            symft = ft;
            break;

        default:
            break;
        }
    }
    static const char * const fext[] = {
        "cdb",
        "",
        "omf",
        "ubr",
        "hex",
        0
    };
    for (const char * const *fextp = fext; *fextp; ++fextp) {
        tfile.SetExt(wxString(*fextp, wxConvUTF8));
        if (!tfile.IsOk() || !wxFileName::IsFileReadable(tfile.GetFullPath()))
            continue;
        filetype_t ft(determine_filetype(tfile));
        if (false)
            m_pDBG->DebugLog(_T("Testing File: ") + tfile.GetFullPath() + _T(" (filetype ") + wxString(to_str(ft).c_str(), wxConvUTF8) + _T(")"));
        switch (ft) {
        case filetype_ihex:
            if (m_BinFile.IsOk())
                break;
            m_BinFile = tfile;
            binft = ft;
            break;

        case filetype_cdb:
            if (m_SymFile.IsOk())
                break;
            m_SymFile = tfile;
            symft = ft;
            break;

        case filetype_omf51:
        case filetype_ubrof:
            if (!m_BinFile.IsOk()) {
                m_BinFile = tfile;
                binft = ft;
            }
            if (!m_SymFile.IsOk()) {
                m_SymFile = tfile;
                symft = ft;
            }
            break;

        default:
            break;
        }
        if (m_BinFile.IsOk() && m_SymFile.IsOk())
            break;
    }
    if ((binft == filetype_omf51 || binft == filetype_ubrof) && !m_SymFile.IsOk()) {
        m_SymFile = m_BinFile;
        symft = binft;
    }
    if (true) {
        m_pDBG->DebugLog(_T("Binary File: ") + m_BinFile.GetFullPath() + _T(" (filetype ") + wxString(to_str(binft).c_str(), wxConvUTF8) + _T(")"));
        m_pDBG->DebugLog(_T("Symbol File: ") + m_SymFile.GetFullPath() + _T(" (filetype ") + wxString(to_str(symft).c_str(), wxConvUTF8) + _T(")"));
        m_pDBG->DebugLog(_T("Compiler ID: ") + compid + _T(" (compilervendor ") + wxString(to_str(m_compilervendor).c_str(), wxConvUTF8) + _T(")"));
    }
}

void AXS_driver::CommandAddKeys(Opt& cmd)
{
    ProjectTargetOptions::additionalKeys_t keys(m_ProjTargetOpt.additionalKeys);
    keys.insert(m_ProjTargetOpt.key);
    keys.erase(ProjectTargetOptions::defaultKey);
    if (keys.empty())
        return;
    std::ostringstream oss;
    std::copy(keys.begin(), keys.end(), std::ostream_iterator<ProjectTargetOptions::key_t>(oss, ","));
    const std::string& osss(oss.str());
    cmd.set_option("keys", osss.substr(0, osss.length() - 1));
}

void AXS_driver::KillDebugger(bool terminate)
{
    ClearQueue();
    // do not clear run queue, as KillDebugger might be called from a running command
    MarkRunQueueDone();
    if (terminate)
        QueueCommand(new AxsCmd_Terminate(this));
    else
        QueueCommand(new AxsCmd_Quit(this));
    m_IsStarted = false;
    m_LiveUpdate = false;
    m_IsInitializing = true;
}

void AXS_driver::ProcessCommand(const Opt& optline)
{
    bool status_changed(ParseStatus(optline));

    /// Is mcu halted?
    if (status_changed)
    {
        if (GetCpuState() == cpustate_halt)
        {
            m_ProgDlg.reset();
            if (!m_IsInitializing)
            {
                //cbWatchesDlg *wdialog = Manager::Get()->GetDebuggerManager()->GetWatchesDialog();
                //if (IsWindowReallyShown(wdialog->GetWindow()))
                //    wdialog->OnRefresh();

                QueueCommand(new AxsCmd_ReadPC(this, true, m_registers));

                if (false)
                {
                    cbDisassemblyDlg *ddialog = Manager::Get()->GetDebuggerManager()->GetDisassemblyDialog();
                    if (ddialog && IsWindowReallyShown(ddialog->GetWindow()))
                    {
                        //ddialog->Clear();
                        QueueCommand(new AxsCmd_Disassemble(this));
                    }
                }

            }
        }
        else if (GetCpuState() != cpustate_busy)
        {
            m_registers->SetChildrenOutdated(true);
            Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog()->UpdateRegisters();
            Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog()->SetInvalid();
            GetDebugger()->SetWatchesDisabled(true);
            Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
        }
    }

    // Store axsdb version
    if (optline.get_cmdname() == "axsdb")
    {
        std::pair<wxString,bool> ver(optline.get_option_wxstring("version"));
        if (ver.second)
            m_AxsdbVersion.swap(ver.first);
        OnCPUTraceChange();
        OnProfilerChange();
    }

    // CPU Trace
    if (optline.get_cmdname() == "cputracelog")
    {
        std::pair<wxString,bool> type(optline.get_option_wxstring("type"));
        std::pair<unsigned long,bool> sec(optline.get_option_uint("sec"));
        std::pair<unsigned long,bool> usec(optline.get_option_uint("usec"));
        std::pair<unsigned long,bool> addr(optline.get_option_uint("addr"));
        std::pair<wxString,bool> dchar(optline.get_option_wxstring("char"));
        std::pair<wxString,bool> name(optline.get_option_wxstring("name"));
        std::pair<unsigned long,bool> line(optline.get_option_uint("line"));
        std::pair<wxString,bool> lang(optline.get_option_wxstring("lang"));
        std::pair<unsigned long,bool> level(optline.get_option_uint("level"));
        std::pair<unsigned long,bool> block(optline.get_option_uint("block"));
        if (type.second && sec.second && usec.second)
        {
            if (addr.second)
            {
                if (name.second && line.second && lang.second)
                {
                    name.first = FilePathSearch(name.first);
                    m_cputracepanel->Add(type.first, sec.first, usec.first, (unsigned int)addr.first, name.first,
                                         line.first, level.second ? level.first : 0U, block.second ? block.first : 0U,
                                         lang.first == wxT("asm"));
                }
                else
                {
                    m_cputracepanel->Add(type.first, sec.first, usec.first, (unsigned int)addr.first);
                }
            }
            else if (dchar.second)
            {
                m_cputracepanel->Add(type.first, sec.first, usec.first, dchar.first);
            }
            else
            {
                m_cputracepanel->Add(type.first, sec.first, usec.first);
            }
        }
    }

    // Profile
    if (optline.get_cmdname() == "profilestat")
    {
        std::pair<wxString,bool> name(optline.get_option_wxstring("name"));
        std::pair<unsigned long,bool> line(optline.get_option_uint("line"));
        std::pair<wxString,bool> lang(optline.get_option_wxstring("lang"));
        std::pair<unsigned long,bool> level(optline.get_option_uint("level"));
        std::pair<unsigned long,bool> block(optline.get_option_uint("block"));
        std::pair<unsigned long,bool> addr(optline.get_option_uint("addr"));
        std::pair<unsigned long,bool> hitcount(optline.get_option_uint("hitcount"));
        if (name.second && line.second && lang.second && addr.second && hitcount.second)
        {
            name.first = FilePathSearch(name.first);
            m_profilerpanel->Add(name.first, line.first, level.second ? level.first : 0U, block.second ? block.first : 0U,
                                 lang.first == wxT("asm"), addr.first, hitcount.first);
        }
        else
        {
            m_profilerpanel->Add();
        }
    }

    /// Breakpoint setting error
    if (false && optline.get_cmdname() == "breakpoint")
    {
        std::pair<wxString,bool> error(optline.get_option_wxstring("error"));
        if (error.second)
        {
            cbProject* prj = Manager::Get()->GetProjectManager()->GetActiveProject();
            ProjectBuildTarget* target = prj->GetBuildTarget(prj->GetActiveBuildTarget());
            wxString bpath = target->GetBasePath();
            long line;
            error.first.AfterFirst(':').BeforeFirst(' ').ToLong(&line, 10);
            cbEditor* ed = Manager::Get()->GetEditorManager()->IsBuiltinOpen(bpath + error.first.Mid(error.first.Find(_("line ")) + 5).BeforeFirst(':'));
            if(ed)
                ed->ToggleBreakpoint(line - 1, true);
        }
    }
}

bool AXS_driver::ParseOutput(const Opt& output)
{
    m_Cursor.changed = false;

    ProcessCommand(output);
    bool consumed = DebuggerDriver::ParseOutput(output);
    if (!consumed)
        m_pDBG->DebugLog(_T("<< ") + output.get_cmdwxstring());

    for (;;)
    {
        bool work = RunQueue();
        PruneRunQueue();
        if (!work)
            break;
    }
    return consumed;
}
