/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk.h"
#ifndef CB_PRECOMP
#include "scrollingdialog.h"
#include <wx/font.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/frame.h>
#include "manager.h"
#endif
#include <cbdebugger_interfaces.h>
#include "debugger_defs.h"
#include "debuggerdriver.h"
#include "machine.h"

#include <wx/arrimpl.cpp>

#if !defined(CB_TEST_PROJECT)

const int DEBUGGER_CURSOR_CHANGED = wxNewId();
const int DEBUGGER_SHOW_FILE_LINE = wxNewId();

const std::string& to_str(cpustate_t cs)
{
    switch (cs) {
    case cpustate_targetdisconnected:
    {
        static const std::string r("targetdisconnected");
        return r;
    }

    case cpustate_disconnected:
    {
        static const std::string r("disconnected");
        return r;
    }

    case cpustate_locked:
    {
        static const std::string r("locked");
        return r;
    }

    case cpustate_halt:
    {
        static const std::string r("halt");
        return r;
    }

    case cpustate_run:
    {
        static const std::string r("run");
        return r;
    }

    case cpustate_busy:
    {
        static const std::string r("busy");
        return r;
    }

    default:
    {
        static const std::string r("??");
        return r;
    }
    }
}

DebuggerCmd::DebuggerCmd(DebuggerDriver* driver, bool logToNormalLog)
    : m_pDriver(driver),
    m_CurSeq(0),
    m_State(cpustate_targetdisconnected),
    m_LogToNormalLog(logToNormalLog),
    m_Running(false)
{
}

DebuggerCmd::~DebuggerCmd()
{
}

void DebuggerCmd::Action()
{
    Done();
}

void DebuggerCmd::ParseOutput(const Opt& output)
{
    Done();
}

bool DebuggerCmd::ParseAllOutput(const Opt& output, unsigned int seq)
{
    // check if the command is for us
    if (!seq)
        return false;
    if (seq != m_CurSeq)
    {
        m_CurSeq = 0;
        cmdseqset_t::iterator si(m_CmdSeq.find(seq));
        if (si == m_CmdSeq.end())
            return false;
        m_CurSeq = seq;
        m_CmdSeq.erase(si);
    }
    // actually run the command
    if (m_LogToNormalLog)
        m_pDriver->Log(_T("< ") + output.get_cmdwxstring());
    else
        m_pDriver->DebugLog(_T("< ") + output.get_cmdwxstring());
    ParseOutput(output);
    return true;
}

unsigned int DebuggerCmd::SendCommand(const Opt& opt, bool debugLog)
{
    Opt opt1(opt);
    unsigned int seq(m_pDriver->CommandAddSeq(opt1));
    m_pDriver->DoSendCommand(opt1, debugLog);
    m_CmdSeq.insert(seq);
    return seq;
}

void DebuggerCmd::Done()
{
    m_CmdSeq.clear();
    m_CurSeq = 0;
    m_Running = false;
}

void DebuggerCmd::CurrentDone()
{
    m_CurSeq = 0;
}

bool DebuggerCmd::IsDone() const
{
    return !m_Running && !m_CurSeq && m_CmdSeq.empty();
}

bool DebuggerCmd::IsLast() const
{
    return m_CurSeq && m_CmdSeq.empty();
}

bool DebuggerCmd::IsPending() const
{
    return !m_CmdSeq.empty();
}

void DebuggerCmd::RunAction(cpustate_t state)
{
    m_State = state;
    m_Running = true;
    Action();
}

void DebuggerCmd::RunStateChange(cpustate_t state)
{
    m_State = state;
    StateChange();
}

wxString DebuggerCmd::DebugInfo()
{
    wxString txt;
    txt << wxString(typeid(*this).name(), wxConvUTF8) << _(" [");
    if (CanOverlap())
        txt << _("O");
    if (m_Running)
        txt << _("R");
    if (m_LogToNormalLog)
        txt << _("L");
    txt << _("C ") << m_CurSeq << _(" W");
    for (cmdseqset_t::const_iterator i(m_CmdSeq.begin()), e(m_CmdSeq.end()); i != e; ++i)
        txt << _(" ") << *i;
    txt << _(" S ") << wxString(to_str(m_State).c_str(), wxConvUTF8) << _("]");
    return txt;
}

DbgCmd_UpdateWatchesTree::DbgCmd_UpdateWatchesTree(DebuggerDriver* driver)
    : DebuggerCmd(driver)
{
}

void DbgCmd_UpdateWatchesTree::Action()
{
    Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
    Done();
}

#endif // !defined(CB_TEST_PROJECT)

void DebuggerBreakpoint::SetEnabled(bool flag)
{
    enabled = flag;
}

wxString DebuggerBreakpoint::GetLocation() const
{
    return filenameAsPassed;
}

int DebuggerBreakpoint::GetLine() const
{
    return line;
}

wxString DebuggerBreakpoint::GetLineString() const
{
    return wxString::Format(wxT("%d"), line);
}

wxString DebuggerBreakpoint::GetType() const
{
    return _("Code");
}

wxString DebuggerBreakpoint::GetInfo() const
{
    wxString s;
    if (useCondition)
        s += _("condition: ") + condition;
    if (useIgnoreCount)
    {
        if (!s.empty())
            s += wxT(" ");
        s += wxString::Format(_("ignore count: %d"), ignoreCount);
    }
    if (temporary)
    {
        if (!s.empty())
            s += wxT(" ");
        s += _("temporary");
    }
    if (!s.empty())
        s += wxT(" ");
    s += wxString::Format(wxT("(index: %d)"), index);
    return s;
}

bool DebuggerBreakpoint::IsEnabled() const
{
    return enabled;
}

bool DebuggerBreakpoint::IsVisibleInEditor() const
{
    return true;
}

bool DebuggerBreakpoint::IsTemporary() const
{
    return temporary;
}

GDBWatch::GDBWatch(wxString const &symbol) :
    m_symbol(symbol),
    m_format(Undefined),
    m_watchtype(TypeVoid),
    m_array_start(0),
    m_array_count(0),
    m_bitsize(0),
    m_forTooltip(false),
    m_disabled(false)
{
}

GDBWatch::~GDBWatch()
{
}

void GDBWatch::GetSymbol(wxString &symbol) const
{
    symbol = m_symbol;
}

void GDBWatch::GetValue(wxString &value) const
{
    value = m_raw_value;
}

bool GDBWatch::SetValue(const wxString &value)
{
    if(m_raw_value != value)
    {
        MarkAsChanged(true);
        m_raw_value = value;
    }
    return true;
}

void GDBWatch::GetFullWatchString(wxString &full_watch) const
{
    cb::shared_ptr<const cbWatch> parent = GetParent();
    if (parent)
    {
        parent->GetFullWatchString(full_watch);
        if (cb::static_pointer_cast<const GDBWatch>(parent)->GetWatchType() == TypeStruct)
            full_watch += wxT(".");
        full_watch += m_symbol;
    }
    else
    {
        full_watch = m_symbol;
    }
}

void GDBWatch::GetType(wxString &type) const
{
    type = m_type;
}
void GDBWatch::SetType(const wxString &type)
{
    m_type = type;
}

void GDBWatch::GetAddrSpace(wxString &as) const
{
    as = m_addrspace;
}

void GDBWatch::SetAddrSpace(const wxString &as)
{
    m_addrspace = as;
}

void GDBWatch::GetAddr(wxString &addr) const
{
    addr = m_addr;
}

void GDBWatch::SetAddr(const wxString &addr)
{
    m_addr = addr;
}

wxString const & GDBWatch::GetDebugString() const
{
    return m_debug_value;
}
void GDBWatch::SetDebugValue(wxString const &value)
{
    m_debug_value = value;
}

void GDBWatch::SetSymbol(const wxString& symbol)
{
    m_symbol = symbol;
}

void GDBWatch::DoDestroy()
{
    delete this;
}

void GDBWatch::SetFormat(WatchFormat format)
{
    m_format = format;
}

WatchFormat GDBWatch::GetFormat() const
{
    return m_format;
}

void GDBWatch::SetWatchType(WatchType type)
{
    m_watchtype = type;
}

WatchType GDBWatch::GetWatchType() const
{
    return m_watchtype;
}

void GDBWatch::SetArrayParams(int start, int count)
{
    m_array_start = start;
    m_array_count = count;
}

int GDBWatch::GetArrayStart() const
{
    return m_array_start;
}

int GDBWatch::GetArrayCount() const
{
    return m_array_count;
}

void GDBWatch::SetBitSize(unsigned int bitsize)
{
    m_bitsize = bitsize;
}

unsigned int GDBWatch::GetBitSize() const
{
    return m_bitsize;
}

void GDBWatch::SetForTooltip(bool flag)
{
    m_forTooltip = flag;
}

bool GDBWatch::GetForTooltip() const
{
    return m_forTooltip;
}

void GDBWatch::SetDisabled(bool flag)
{
    m_disabled = flag;
}

void GDBWatch::SetDisabledRecursive(bool flag)
{
    SetDisabled(flag);
    for (int i = 0; i < GetChildCount(); ++i)
        cb::static_pointer_cast<GDBWatch>(GetChild(i))->SetDisabledRecursive(flag);
}

bool GDBWatch::IsDisabled() const
{
    return m_disabled;
}

bool GDBWatch::IsReadonly() const
{
    return (m_addr.IsEmpty() || m_addrspace.IsEmpty());
}

void GDBWatch::ClearEnums(void)
{
    m_enumvalues.clear();
}

void GDBWatch::AddEnum(unsigned long val, const wxString& name)
{
    m_enumvalues[val] = name;
}

bool GDBWatch::FindEnum(wxString& name, unsigned long val) const
{
    name.Clear();
    enumvalues_t::const_iterator it(m_enumvalues.find(val));
    if (it == m_enumvalues.end())
        return false;
    name = it->second;
    return true;
}

bool GDBWatch::FindEnum(unsigned long& val, const wxString& name) const
{
    val = 0;
    wxString name1(name);
    name1.Trim(false);
    name1.Trim(true);
    for (enumvalues_t::const_iterator i(m_enumvalues.begin()), e(m_enumvalues.end()); i != e; ++i)
    {
        if (i->second != name1)
            continue;
        val = i->first;
        return true;
    }
    return false;
}

AXSRegister::AXSRegister(wxString const& name, wxString const &desc) :
    cbRegister(),
    m_name(name),
    m_desc(desc),
    m_addrspace(),
    m_addr(0),
    m_writemask(0),
    m_value(0),
    m_bitlength(0),
    m_readsafe(true),
    m_outdated(false)
{
}

AXSRegister::AXSRegister(wxString const& name, unsigned int bitlength, const wxString& addrspace, uint16_t addr, uint16_t writemask, bool readsafe, wxString const &desc) :
    cbRegister(),
    m_name(name),
    m_desc(desc),
    m_addrspace(addrspace),
    m_addr(addr),
    m_writemask(writemask),
    m_value(0),
    m_bitlength(std::min(bitlength, (unsigned int)(8U * sizeof(m_value)))),
    m_readsafe(readsafe),
    m_outdated(false)
{
    m_writemask &= (1UL << m_bitlength) - 1;
}

AXSRegister::~AXSRegister()
{
}

void AXSRegister::GetName(wxString& name) const
{
    name = m_name;
}

void AXSRegister::GetDescription(wxString& desc) const
{
    desc = m_desc;
}

void AXSRegister::SetReadSafe(bool rs)
{
    m_readsafe = rs;
}

void AXSRegister::SetBitLength(unsigned int bl)
{
    m_bitlength = std::min(bl, (unsigned int)(8U * sizeof(m_value)));
    m_writemask &= (1UL << m_bitlength) - 1;
    m_value &= (1UL << m_bitlength) - 1;
}

void AXSRegister::SetWriteMask(uint16_t writemask)
{
    m_writemask = writemask & ((1UL << m_bitlength) - 1);
}

void AXSRegister::SetAddrSpace(const wxString &as)
{
    m_addrspace = as;
}

void AXSRegister::SetAddr(uint16_t addr)
{
    m_addr = addr;
}

void AXSRegister::SetDescription(const wxString &desc)
{
    m_desc = desc;
}

void AXSRegister::GetValue(wxString& value) const
{
    if (IsCategory())
    {
        value.Clear();
        return;
    }
    int len((m_bitlength + 3) >> 2);
    value.Printf(wxT("0x%0*x"), len, m_value);
}

void AXSRegister::GetValueAlt(wxString& value) const
{
    if (IsCategory())
    {
        value.Clear();
        return;
    }
    // len = ceil(m_bitlength * log10(2))
    int len((m_bitlength * 3 + 9) / 10);
    if (m_bitlength <= 8 && m_value >= 32 && m_value < 127)
    {
        value.Printf(wxT("%*d '%c'"), len, m_value, (char)m_value);
        return;
    }
    value.Printf(wxT("%*d"), len, m_value);
}

void AXSRegister::GetWriteMask(wxString& mask) const
{
    // for now, do not display the write mask
    if (true || IsCategory())
    {
        mask.Clear();
        return;
    }
    int len((m_bitlength + 3) >> 2);
    mask.Printf(wxT("0x%0*x"), len, m_writemask);
}

void AXSRegister::GetAddrSpace(wxString& as) const
{
    if (IsCategory())
    {
        as.Clear();
        return;
    }
    as = m_addrspace;
}

void AXSRegister::GetAddr(wxString& addr) const
{
    if (IsCategory() || m_addrspace.IsEmpty())
    {
        addr.Clear();
        return;
    }

    if ((m_addrspace == wxT("direct") || m_addrspace == wxT("sfr")) && m_addr < 0x100)
    {
        addr.Printf(wxT("0x%02x"), m_addr);
        return;
    }
    addr.Printf(wxT("0x%04x"), m_addr);
}

wxString const& AXSRegister::GetDebugString() const
{
    static const wxString e(wxEmptyString);
    return e;
}

void AXSRegister::SetValue(uint16_t val)
{
    uint16_t mask(((1UL << m_bitlength) - 1));
    uint16_t chg((m_value ^ val) & mask);
    m_value = val & mask;
    if (chg)
        MarkAsChanged(true);
}

void AXSRegister::SetOutdated(bool outdated)
{
    m_outdated = outdated;
}

void AXSRegister::SetChildrenOutdated(bool outdated)
{
    for (int i = 0; i < GetChildCount(); ++i)
    {
        cb::shared_ptr<AXSRegister> child(cb::static_pointer_cast<AXSRegister>(GetChild(i)));
        child->SetOutdated(outdated);
        child->SetChildrenOutdated(outdated);
    }
}
