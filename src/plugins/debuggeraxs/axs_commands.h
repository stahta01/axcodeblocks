/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef AXS_DEBUGGER_COMMANDS_H
#define AXS_DEBUGGER_COMMANDS_H

// get rid of wxWidgets debug ugliness
#ifdef new
    #undef new
#endif

#include <map>

#include <wx/string.h>
#include <wx/regex.h>
#include <wx/tipwin.h>
#include <wx/tokenzr.h>
#include <limits>
#include "configmanager.h"
#include <globals.h>
#include <manager.h>
#include <editormanager.h>
#include <cbeditor.h>
#include <cbstyledtextctrl.h>
#include <scriptingmanager.h>
#include <sqplus.h>
#include <infowindow.h>
#include "logmanager.h"

#include <cbdebugger_interfaces.h>
#include "debugger_defs.h"
#include "debuggeraxs.h"
#include "axs_driver.h"
#include "gdb_tipwindow.h"
#include "machine.h"

namespace
{
  template <typename T>
  T wxStrHexTo(const wxString &str)
  {
    T ret = 0; // return
    std::size_t count = 0; // how many characters we've converted
    std::size_t pos = 0; // string position

    // if it begins with 0x or 0X, just ignore it
    if (str[pos] == _T('0'))
    {
      ++pos;

      if (str[pos] == _T('x') || str[pos] == _T('X'))
      {
        ++pos; // start after the x or X
      }

      while (str[pos] == _T('0')) // skip all zeros
      {
        ++pos;
      }
    }

    while (count < sizeof(T) * 2) // be sure we don't keep adding more to ret
    {
      #if wxCHECK_VERSION(2, 9, 0)
      switch (str[pos].GetValue())
      #else
      switch (str[pos])
      #endif
      {
        case _T('0'):
        case _T('1'):
        case _T('2'):
        case _T('3'):
        case _T('4'):
        case _T('5'):
        case _T('6'):
        case _T('7'):
        case _T('8'):
        case _T('9'):
          ret <<= 4;
          ret |= str[pos] - _T('0');
          ++count;
          break;

        case _T('a'):
        case _T('b'):
        case _T('c'):
        case _T('d'):
        case _T('e'):
        case _T('f'):
          ret <<= 4;
          ret |= str[pos] - _T('a') + 10;
          ++count;
          break;

        case _T('A'):
        case _T('B'):
        case _T('C'):
        case _T('D'):
        case _T('E'):
        case _T('F'):
          ret <<= 4;
          ret |= str[pos] - _T('A') + 10;
          ++count;
          break;

        default: // whatever we find that doesn't match ends the conversion
          return ret;
      }

      ++pos;
    }

    return ret;
  }
}

static wxRegEx reGenericHexAddress(_T("(0x[A-Fa-f0-9]+)"));


DECLARE_INSTANCE_TYPE(wxString);


/**
  * Command to add a breakpoint.
  */
class AxsCmd_AddBreakpoint : public DebuggerCmd
{
    private:
        cb::shared_ptr<DebuggerBreakpoint> m_BP;
        Opt m_cmd;
        bool m_slcmd;
    public:
        /** @param bp The breakpoint to set. */
        AxsCmd_AddBreakpoint(DebuggerDriver* driver, cb::shared_ptr<DebuggerBreakpoint> bp)
            : DebuggerCmd(driver),
              m_BP(bp),
              m_cmd("breakpoint"),
              m_slcmd(false)
        {
            // axsdb doesn't allow setting the bp number.
            // instead, we must read it back in ParseOutput()...
            m_BP->index = -1;
            m_cmd.set_option("new", "");
            if (m_BP->filename.IsEmpty() && !m_BP->line)
            {
                m_cmd.set_option("addr", m_BP->address);
            }
            else
            {
                // SDCC replaces spaces in filenames by underscores when writing the debug info
                wxString fn(m_BP->filename.AfterLast('/'));
                fn.Replace(wxT(" "), wxT("_"));
                m_cmd.set_option("sourceline", fn + wxString::Format(_T(":%d,C*"), m_BP->line));
            }
            m_cmd.set_option("enable", m_BP->enabled ? 1 : 0);
            m_cmd.set_option("count", m_BP->useIgnoreCount ? m_BP->ignoreCount : 0);
            m_BP->alreadySet = true;
        }
        void Action()
        {
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            if (!m_slcmd)
            {
                if (output.is_option("error"))
                {
                    // Silently ignore breakpoint set errors, as the gdb driver does
                    Done();
                    return;
                }
                std::pair<long,bool> idx(output.get_option_int("index"));
                if (idx.second)
                    m_BP->index = idx.first;
                std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                if (!addr.second)
                {
                    Done();
                    Manager::Get()->GetDebuggerManager()->GetBreakpointDialog()->Reload();
                    return;
                }
                m_BP->address = addr.first;
                m_slcmd = true;
                m_cmd = Opt("sourcelines");
                m_cmd.set_option("addr", addr.first);
                m_cmd.set_option("asm", 0);
                m_cmd.set_option("C", 1);
                SendCommand(m_cmd);
                return;
            }
            std::pair<long,bool> line(output.get_option_int("line"));
            if (line.second)
            {
                m_BP->line = line.first;
                Manager::Get()->GetDebuggerManager()->GetBreakpointDialog()->Reload();
            }
            Done();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to remove a breakpoint.
  */
class AxsCmd_RemoveBreakpoint : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        /** @param bp The breakpoint to remove. If NULL, all breakpoints are removed. */
        AxsCmd_RemoveBreakpoint(DebuggerDriver* driver, cb::shared_ptr<DebuggerBreakpoint> bp)
            : DebuggerCmd(driver),
              m_cmd("breakpoint")
        {
            if (!bp || bp->index < 0)
                return;
            m_cmd.set_option("delete", bp->index);
            bp->index = -1;
        }
        void Action()
        {
            if (!m_cmd.is_option("delete"))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            m_pDriver->KillOnError(output);
            Done();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to examine a memory region.
  */
class AxsCmd_ExamineMemory : public DebuggerCmd
{
    private:
        wxString m_addrspace;
        wxString m_addr;
        int m_bytes;
        typedef enum {
            state_expr,
            state_memread,
            state_done
        } state_t;
        state_t m_state;

        void Error(const wxString& err)
        {
            m_state = state_done;
            Done();
            cbExamineMemoryDlg *dialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
            if (!dialog)
                return;
            dialog->Begin();
            dialog->Clear();
            dialog->AddError(err);
            dialog->End();
        }

    public:
        AxsCmd_ExamineMemory(DebuggerDriver* driver)
            : DebuggerCmd(driver),
              m_state(state_done)
        {
            cbExamineMemoryDlg *dialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
            if (!dialog)
                return;
            m_addrspace = dialog->GetAddressSpace();
            m_addr = dialog->GetBaseAddress();
            m_bytes = dialog->GetBytes();
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Examine Memory")) || m_addr.IsEmpty())
            {
                Done();
                return;
            }
            {
                unsigned long val;
                if (m_addr.ToULong(&val, 0))
                {
                    Opt cmd("read_mem");
                    cmd.set_option("as", m_addrspace);
                    cmd.set_option("addr", m_addr);
                    cmd.set_option("length", m_bytes);
                    SendCommand(cmd);
                    m_state = state_memread;
                    return;
                }
            }
            Opt cmd("cexpr");
            cmd.set_option("expr", m_addr);
            cmd.set_option("lvalue", 0);
            cmd.set_option("typeinfo", 1);
            SendCommand(cmd);
            m_state = state_expr;
        }
        void ParseOutput(const Opt& output)
        {
            {
                std::pair<wxString,bool> err(output.get_option_wxstring("error"));
                if (err.second)
                {
                    Error(err.first);
                    return;
                }
            }
            switch (m_state)
            {
                case state_memread:
                {
                    m_state = state_done;
                    Done();
                    cbExamineMemoryDlg *dialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
                    if (!dialog)
                        break;
                    std::pair<Opt::intarray_t,bool> memw(output.get_option_intarray("data"));
                    std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                    if (!memw.second || !addr.second || !dialog)
                        break;
                    dialog->Begin();
                    dialog->Clear();
                    for(Opt::intarray_t::const_iterator wi(memw.first.begin()), we(memw.first.end()); wi != we; ++wi, ++addr.first)
                    {
                        dialog->AddHexByte(wxString::Format(wxT("%04lx"), addr.first), wxString::Format(wxT("%02x"), (unsigned int)*wi));
                    }
                    dialog->End();
                    break;
                }

                case state_expr:
                {
                    {
                        std::pair<std::string,bool> rt(output.get_option("resulttype"));
                        if (!rt.second || rt.first != "int")
                        {
                            Error(_T("Address Expression: Invalid Result Type\n") + output.get_cmdwxstring());
                            break;
                        }
                    }
                    std::pair<long,bool> res(output.get_option_int("result"));
                    if (!res.second)
                    {
                        Error(_T("Address Expression: No Result\n") + output.get_cmdwxstring());
                        break;
                    }
                    m_addr.Printf(wxT("%ld"), res.first);
                    {
                        std::pair<std::string,bool> cls(output.get_option("class"));
                        std::pair<std::string,bool> ptype(output.get_option("ptrtype"));
                        cbExamineMemoryDlg *dialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
                        if (cls.second && cls.first == "pointer" && ptype.second && dialog)
                        {
                            if (ptype.first == "code")
                            {
                                m_addrspace = wxT("code");
                                dialog->SetAddressSpace(m_addrspace);
                            }
                            else if (ptype.first == "xram")
                            {
                                m_addrspace = wxT("x");
                                dialog->SetAddressSpace(m_addrspace);
                            }
                            else if (ptype.first == "iram" || ptype.first == "upper128")
                            {
                                m_addrspace = wxT("indirect");
                                dialog->SetAddressSpace(m_addrspace);
                            }
                            else if (ptype.first == "paged")
                            {
                                m_addrspace = wxT("p");
                                dialog->SetAddressSpace(m_addrspace);
                            }
                        }
                    }
                    {
                        Opt cmd("read_mem");
                        cmd.set_option("as", m_addrspace);
                        cmd.set_option("addr", m_addr);
                        cmd.set_option("length", m_bytes);
                        SendCommand(cmd);
                        m_state = state_memread;
                        return;
                    }
                    break;
                }

                default:
                {
                    Done();
                    break;
                }
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to read the register list.
  */
class AxsCmd_ReadRegisters : public DebuggerCmd
{
    public:
        enum {
            Recurse    = 0x0001,
            Force      = 0x0002,
            Collapsed  = 0x0004,
            Unsafe     = 0x0008
        };

    private:
        typedef std::map<unsigned int,cb::shared_ptr<AXSRegister> > cmdmap_t;
        cmdmap_t m_cmdmap;
        cb::shared_ptr<AXSRegister> m_root;
        int m_flags;

        void IssueCommands(cb::shared_ptr<AXSRegister> root)
        {
            if (!root)
                return;
            if (root->IsCategory())
            {
                root->SetOutdated(false);
                root->MarkAsChanged(false);
            }
            else if (((m_flags & Force) || root->IsOutdated()) &&
                     ((m_flags & Unsafe) || root->IsReadSafe() || !root->IsOutdated()))
            {
                if (root->IsPC())
                {
                    Opt cmd("read_pc");
                    m_cmdmap[SendCommand(cmd)] = root;
                }
                else
                {
                    wxString as;
                    root->GetAddrSpace(as);
                    Opt cmd("read_mem");
                    cmd.set_option("as", as);
                    cmd.set_option("addr", root->GetAddr());
                    cmd.set_option("length", 1);
                    m_cmdmap[SendCommand(cmd)] = root;
                }
            }
            if (!(m_flags & Recurse))
                return;
            if (!((m_flags & Collapsed) || root->IsExpanded() || !root->GetParent()))
                return;
            for (int i = 0; i < root->GetChildCount(); ++i)
            {
                cb::shared_ptr<AXSRegister> child(cb::static_pointer_cast<AXSRegister>(root->GetChild(i)));
                IssueCommands(child);
            }
        }

    public:
        AxsCmd_ReadRegisters(DebuggerDriver* driver, cb::shared_ptr<AXSRegister> root, int flags)
            : DebuggerCmd(driver),
              m_root(root),
              m_flags(flags)
        {
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Read Registers")))
            {
                Done();
                return;
            }
            IssueCommands(m_root);
            if (m_cmdmap.empty())
            {
                Done();
                return;
            }
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            cb::shared_ptr<AXSRegister> child;
            {
                cmdmap_t::iterator ci(m_cmdmap.find(m_CurSeq));
                if (ci != m_cmdmap.end())
                {
                    child = ci->second;
                    m_cmdmap.erase(ci);
                }
            }
            if (m_cmdmap.empty())
                Done();
            if (child)
            {
                std::pair<unsigned long,bool> data(output.get_option_uint(child->IsPC() ? "pc" : "data"));
                if (data.second)
                {
                    child->SetValue(data.first);
                    child->SetOutdated(false);
                }
            }
            if (m_cmdmap.empty()) {
                cbCPURegistersDlg *dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
                if (dialog)
                    dialog->UpdateRegisters();
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to get the register list.
  */
class AxsCmd_RegisterList : public DebuggerCmd
{
    private:
        cb::shared_ptr<AXSRegister> m_root;
        bool m_update;

        static void UnmarkRemoved(cb::shared_ptr<cbRegister> reg)
        {
            while (reg)
            {
                reg->MarkAsRemoved(false);
                reg = reg->GetParent();
            }
        }

        static void SortedInsert(cb::shared_ptr<AXSRegister> parent, cb::shared_ptr<AXSRegister> child, int from_idx = 0, int to_idx = std::numeric_limits<int>::max())
        {
            if (from_idx < 0)
                from_idx = 0;
            if (to_idx > parent->GetChildCount())
                to_idx = parent->GetChildCount();
            if (from_idx >= to_idx)
            {
                parent->InsertChild(parent, child, std::max(to_idx, 0));
                return;
            }
            int mid((from_idx + to_idx) >> 1);
            cb::shared_ptr<AXSRegister> mid_child(cb::static_pointer_cast<AXSRegister>(parent->GetChild(mid)));
            wxString name, mid_name;
            child->GetName(name);
            mid_child->GetName(mid_name);
            if (name < mid_name)
            {
                SortedInsert(parent, child, from_idx, mid);
                return;
            }
            if (from_idx + 1 < to_idx)
            {
                SortedInsert(parent, child, mid, to_idx);
                return;
            }
            parent->InsertChild(parent, child, std::max(to_idx, 0));
        }

    public:
        AxsCmd_RegisterList(AXS_driver* driver, cb::shared_ptr<AXSRegister> root, bool update)
            : DebuggerCmd(driver), m_root(root), m_update(update)
        {
        }

        void Action()
        {
            if (!m_root)
            {
                Done();
                return;
            }
            m_root->MarkChildrenAsRemoved(true);
            {
                cb::shared_ptr<AXSRegister> reg(cb::static_pointer_cast<AXSRegister>(m_root->FindChild(wxT("PC"))));
                if (reg)
                {
                    reg->SetReadSafe(true);
                    reg->SetBitLength(16);
                    reg->SetWriteMask(0xffff);
                    reg->SetAddrSpace(wxEmptyString);
                    reg->SetAddr(0);
                }
                else
                {
                    reg.reset(new AXSRegister(wxT("PC"), 16, wxEmptyString, 0, 0xffff, true, wxEmptyString));
                    m_root->InsertChild(m_root, reg, 0);
                }
                reg->SetDescription(_T("The Program Counter"));
                reg->SetOutdated(true);
                UnmarkRemoved(reg);
            }
            {
                Opt cmd("registers");
                SendCommand(cmd);
            }
            {
                Opt cmd("nop");
                SendCommand(cmd);
            }

        }

        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output) || IsLast())
            {
                Done();
                m_root->RemoveMarkedChildren(true);
                cbCPURegistersDlg *dialog(Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog());
                if (!dialog)
                    return;
                dialog->SetRootRegister(m_root);
                if (m_update && GetState() == cpustate_halt)
                    m_pDriver->QueueCommand(new AxsCmd_ReadRegisters(m_pDriver, m_root, AxsCmd_ReadRegisters::Recurse));
                else
                    dialog->UpdateRegisters();
                return;
            }
            if (output.is_option("error"))
                return;
            std::pair<wxString,bool> name(output.get_option_wxstring("name"));
            if (!name.second)
                return;
            cb::shared_ptr<AXSRegister> parent(m_root);
            {
                std::pair<wxString,bool> group(output.get_option_wxstring("group"));
                if (group.second && !group.first.IsEmpty())
                {
                    cb::shared_ptr<AXSRegister> p(cb::static_pointer_cast<AXSRegister>(m_root->FindChild(group.first)));
                    if (!p)
                    {
                        p.reset(new AXSRegister(group.first));
                        SortedInsert(m_root, p, 1);
                    }
                    parent = p;
                    p->SetOutdated(true);
                }
            }
            std::pair<wxString,bool> addrspace(output.get_option_wxstring("addrspace"));
            if (!addrspace.second)
                addrspace.first.Clear();
            std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
            if (!addr.second)
                addr.first = 0;
            std::pair<unsigned long,bool> writemask(output.get_option_uint("writemask"));
            if (!writemask.second)
                writemask.first = 0;
            std::pair<unsigned long,bool> dflt(output.get_option_uint("default"));
            if (!dflt.second)
                dflt.first = 0;
            std::pair<unsigned long,bool> readsafe(output.get_option_uint("readsafe"));
            std::pair<wxString,bool> desc(output.get_option_wxstring("description"));
            if (!desc.second)
                desc.first.Clear();
            cb::shared_ptr<AXSRegister> reg(cb::static_pointer_cast<AXSRegister>(parent->FindChild(name.first)));
            if (reg)
            {
                reg->SetReadSafe(readsafe.second && readsafe.first);
                reg->SetBitLength(8);
                reg->SetWriteMask(writemask.first);
                reg->SetAddrSpace(addrspace.first);
                reg->SetAddr(addr.first);
                reg->SetDescription(desc.first);
            }
            else
            {
                reg.reset(new AXSRegister(name.first, 8, addrspace.first, addr.first, writemask.first, readsafe.second && readsafe.first, desc.first));
                SortedInsert(parent, reg, 0);
            }
            reg->SetOutdated(true);
            UnmarkRemoved(reg);
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to autodetect the chip in use.
  */
class AxsCmd_ChipBase : public DebuggerCmd
{
    private:
        bool m_Refresh;
    public:
        AxsCmd_ChipBase(DebuggerDriver* driver, bool isidle)
            : DebuggerCmd(driver),
              m_Refresh(isidle)
        {
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            Done();
            cbCPURegistersDlg *dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
            std::pair<wxArrayString,bool> chiplist(output.get_option_wxarraystring("available"));
            std::pair<wxString,bool> curchip(output.get_option_wxstring("current"));
            if (chiplist.second)
                dialog->SetChips(chiplist.first, true);
            if (curchip.second) {
                dialog->SetCurrentChip(curchip.first);
                if (m_Refresh)
                    dialog->UpdateRegisters();
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to autodetect the chip in use.
  */
class AxsCmd_AutodetectChip : public AxsCmd_ChipBase
{
    public:
        AxsCmd_AutodetectChip(DebuggerDriver* driver, bool isidle = true)
            : AxsCmd_ChipBase(driver, isidle)
        {
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Autodetect Chip")))
            {
                Done();
                return;
            }
            Opt cmd("chips");
            cmd.set_option("autodetect", 1);
            SendCommand(cmd);
        }
};

/**
  * Command to set the chip in use.
  */
class AxsCmd_SetChip : public AxsCmd_ChipBase
{
    private:
        wxString m_chip;
    public:
        AxsCmd_SetChip(DebuggerDriver* driver, const wxString& chip, bool isidle = true)
            : AxsCmd_ChipBase(driver, isidle),
              m_chip(chip)
        {
        }
        void Action()
        {
            Opt cmd("chips");
            cmd.set_option("set", m_chip);
            SendCommand(cmd);
        }
};

/**
  * Command to get the chip in use.
  */
class AxsCmd_GetChip : public AxsCmd_ChipBase
{
    public:
        AxsCmd_GetChip(DebuggerDriver* driver, bool isidle = true)
            : AxsCmd_ChipBase(driver, isidle)
        {
        }
        void Action()
        {
            Opt cmd("chips");
            SendCommand(cmd);
        }
};

/**
  * Command to read the program counter.
  */
class AxsCmd_ReadPC : public DebuggerCmd
{
    private:
        class Symbol
        {
            public:
                Symbol(const wxString& name, const wxString& filename, const wxString& funcname, unsigned int block, unsigned int level, unsigned int addr, unsigned int endaddr)
                    : m_name(name), m_filename(filename), m_funcname(funcname), m_block(block), m_level(level), m_addr(addr), m_endaddr(endaddr) {}
                const wxString& get_name() const { return m_name; }
                const wxString& get_filename() const { return m_filename; }
                const wxString& get_funcname() const { return m_funcname; }
                unsigned int get_block() const { return m_block; }
                unsigned int get_level() const { return m_level; }
                unsigned int get_addr() const { return m_addr; }
                unsigned int get_endaddr() const { return m_endaddr; }

                bool operator<(const Symbol& x) const { return compare(x) < 0; }
                int compare(const Symbol& x) const
                {
                    if (get_addr() < x.get_addr())
                        return -1;
                    if (get_addr() > x.get_addr())
                        return 1;
                    if (get_endaddr() < x.get_endaddr())
                        return -1;
                    if (get_endaddr() > x.get_endaddr())
                        return 1;
                    if (get_block() < x.get_block())
                        return -1;
                    if (get_block() > x.get_block())
                        return 1;
                    if (get_level() < x.get_level())
                        return -1;
                    if (get_level() > x.get_level())
                        return 1;
                    int r = get_name().Cmp(x.get_name());
                    if (r)
                        return r;
                    r = get_filename().Cmp(x.get_filename());
                    if (r)
                        return r;
                    return get_funcname().Cmp(x.get_funcname());
                }

            protected:
                wxString m_name;
                wxString m_filename;
                wxString m_funcname;
                unsigned int m_block;
                unsigned int m_level;
                unsigned int m_addr;
                unsigned int m_endaddr;
        };

        class SourceLine
        {
            public:
                SourceLine(const wxString& name, unsigned int line, unsigned int endline,
                           unsigned int block, unsigned int level, unsigned int addr)
                    : m_name(name), m_line(line), m_endline(endline), m_block(block), m_level(level), m_addr(addr) {}
                const wxString& get_name() const { return m_name; }
                unsigned int get_line() const { return m_line; }
                unsigned int get_endline() const { return m_endline; }
                unsigned int get_block() const { return m_block; }
                unsigned int get_level() const { return m_level; }
                unsigned int get_addr() const { return m_addr; }

                bool operator<(const SourceLine& x) const { return compare(x) < 0; }
                int compare(const SourceLine& x) const
                {
                    if (get_addr() < x.get_addr())
                        return -1;
                    if (get_addr() > x.get_addr())
                        return 1;
                    if (get_block() < x.get_block())
                        return -1;
                    if (get_block() > x.get_block())
                        return 1;
                    if (get_level() < x.get_level())
                        return -1;
                    if (get_level() > x.get_level())
                        return 1;
                    if (get_line() < x.get_line())
                        return -1;
                    if (get_line() > x.get_line())
                        return 1;
                    if (get_endline() < x.get_endline())
                        return -1;
                    if (get_endline() > x.get_endline())
                        return 1;
                    return get_name().Cmp(x.get_name());
                }

            protected:
                wxString m_name;
                unsigned int m_line;
                unsigned int m_endline;
                unsigned int m_block;
                unsigned int m_level;
                unsigned int m_addr;
        };

        Cursor m_cursor;
        typedef std::set<Symbol> symbols_t;
        symbols_t m_symbols;
        typedef std::set<SourceLine> sourcelines_t;
        sourcelines_t m_sourcelines;
        cb::shared_ptr<AXSRegister> m_pcreg;
        bool m_updatecursor;
    public:
        AxsCmd_ReadPC(AXS_driver* driver, bool updatecursor, cb::shared_ptr<AXSRegister> root)
            : DebuggerCmd(driver), m_pcreg(root), m_updatecursor(updatecursor)
        {
            if (m_pcreg)
            {
                if (!m_pcreg->IsPC())
                {
                    m_pcreg = cb::static_pointer_cast<AXSRegister>(m_pcreg->FindChild(wxT("PC")));
                    if (m_pcreg && !m_pcreg->IsPC())
                        m_pcreg.reset();
                }
            }
        }

        void Action()
        {
            if (m_pDriver->KillOnNotHaltRun(GetState(), _T("Read PC")))
            {
                Done();
                return;
            }
            if (m_updatecursor)
            {
                {
                    Opt cmd("symbols");
                    cmd.set_option("containsaddr", "current");
                    cmd.set_option("pc", "current");
                    SendCommand(cmd);
                }
                {
                    Opt cmd("sourcelines");
                    cmd.set_option("addr", "current");
                    cmd.set_option("asm", 0);
                    cmd.set_option("C", 1);
                    SendCommand(cmd);
                }
            }
            {
                Opt cmd("read_pc");
                SendCommand(cmd);
            }
        }

        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            if (output.get_cmdname() == "read_pc")
            {
                Done();
                AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
                if (!m_sourcelines.empty())
                {
                    m_cursor.file = driver->FilePathSearch(m_sourcelines.rbegin()->get_name());
                    m_cursor.line = m_sourcelines.rbegin()->get_line();
                    m_cursor.line_end = m_sourcelines.rbegin()->get_endline();
                    m_cursor.changed = true;
                }
                if (!m_symbols.empty())
                {
                    m_cursor.function = m_symbols.begin()->get_name();
                    m_cursor.changed = true;
                }
                std::pair<long,bool> pc(output.get_option_int("pc"));
                if (pc.second)
                {
                    m_cursor.address.Printf(wxT("0x%04x"), pc.first);
                    m_cursor.changed = true;
                    if (m_pcreg)
                    {
                        m_pcreg->SetValue(pc.first);
                        m_pcreg->SetOutdated(false);
                        cbCPURegistersDlg *dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
                        if (dialog)
                            dialog->UpdateRegisters();
                    }
                }
                if (m_updatecursor && m_cursor.changed)
                {
                    m_pDriver->SetCursor(m_cursor);
                    m_pDriver->NotifyCursorChanged(GetState() != cpustate_halt);
                }
                return;
            }
            if (output.get_cmdname() == "symbols")
            {
                {
                    std::pair<unsigned long,bool> func(output.get_option_uint("function"));
                    if (!func.second || !func.first)
                        return;
                }
                {
                    Opt::stringopt_t as(output.get_option("as"));
                    if (!as.second || (as.first != "code" && as.first != "codestatic"))
                        return;
                }
                std::pair<wxString,bool> name(output.get_option_wxstring("symname"));
                std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                std::pair<unsigned long,bool> endaddr(output.get_option_uint("endaddr"));
                if (!name.second || !addr.second)
                    return;
                if (!endaddr.second)
                    endaddr.first = addr.first + 1;
                std::pair<wxString,bool> filename(output.get_option_wxstring("filename"));
                if (!filename.second)
                    filename.first.Clear();
                std::pair<wxString,bool> funcname(output.get_option_wxstring("funcname"));
                if (!funcname.second)
                    funcname.first.Clear();
                std::pair<unsigned long,bool> level(output.get_option_uint("level"));
                if (!level.second)
                    level.first = 0;
                std::pair<unsigned long,bool> block(output.get_option_uint("block"));
                if (!block.second)
                    block.first = 0;
                m_symbols.insert(Symbol(name.first, filename.first, funcname.first, block.first, level.first, addr.first, endaddr.first));
                return;
            }
            if (output.get_cmdname() == "sourcelines")
            {
                {
                    Opt::stringopt_t lang(output.get_option("lang"));
                    if (!lang.second || lang.first != "C")
                        return;
                }
                std::pair<wxString,bool> name(output.get_option_wxstring("name"));
                std::pair<unsigned long,bool> line(output.get_option_uint("line"));
                std::pair<wxString,bool> ename(output.get_option_wxstring("ename"));
                std::pair<unsigned long,bool> eline(output.get_option_uint("eline"));
                std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                if (!name.second || !line.second || !addr.second)
                    return;
                std::pair<unsigned long,bool> level(output.get_option_uint("level"));
                if (!level.second)
                    level.first = 0;
                std::pair<unsigned long,bool> block(output.get_option_uint("block"));
                if (!block.second)
                    block.first = 0;
                if (!ename.second || ename.first != name.first || !eline.second)
                    eline.first = line.first;
                m_sourcelines.insert(SourceLine(name.first, line.first, eline.first, block.first, level.first, addr.first));
                return;
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to read a register.
  */
class AxsCmd_ReadRegister : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        AxsCmd_ReadRegister(DebuggerDriver* driver, const wxString& addrspace, unsigned long addr)
            : DebuggerCmd(driver),
              m_cmd("read_mem")
        {
            m_cmd.set_option("as", addrspace);
            m_cmd.set_option("addr", addr);
            m_cmd.set_option("length", 1);
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Read Register")))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            Done();
            std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
            std::pair<wxString,bool> addrspace(output.get_option_wxstring("as"));
            std::pair<unsigned long,bool> data(output.get_option_uint("data"));
            if (!addr.second || !addrspace.second || !data.second)
                return;
            //cbCPURegistersDlg *dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
            //dialog->SetRegisterValue(addr.first, addrspace.first, data.first);
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to write the program counter.
  */
class AxsCmd_WritePC : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        AxsCmd_WritePC(DebuggerDriver* driver, unsigned long value)
            : DebuggerCmd(driver),
              m_cmd("write_pc")
        {
            m_cmd.set_option("pc", value);
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Write PC")))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            m_pDriver->KillOnError(output);
            Done();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to write a register.
  */
class AxsCmd_WriteRegister : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        AxsCmd_WriteRegister(DebuggerDriver* driver, const wxString& addrspace, unsigned long addr, unsigned long value)
            : DebuggerCmd(driver),
              m_cmd("write_mem")
        {
            m_cmd.set_option("as", addrspace);
            m_cmd.set_option("addr", addr);
            m_cmd.set_option("data", value);
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Write Register")))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            m_pDriver->KillOnError(output);
            Done();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to issue a hardware reset.
  */
class AxsCmd_HardwareReset : public DebuggerCmd
{
    public:
        AxsCmd_HardwareReset(DebuggerDriver* driver)
            : DebuggerCmd(driver)
        {
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHaltRun(GetState(), _T("Hardware Reset")))
            {
                Done();
                return;
            }
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            {
                Opt cmd("hwreset");
                cmd.set_option("mode", "on");
                SendCommand(cmd);
            }
            {
                Opt cmd("connect");
                driver->CommandAddKeys(cmd);
                SendCommand(cmd);
            }
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output) || IsLast())
            {
                Done();
                m_pDriver->NotifyDebuggeeContinued();
                return;
            }
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to issue a software reset.
  */
class AxsCmd_SoftwareReset : public DebuggerCmd
{
    private:
        bool m_statechg;
    public:
        AxsCmd_SoftwareReset(DebuggerDriver* driver)
            : DebuggerCmd(driver),
              m_statechg(true)
        {
        }
        void Action()
        {
            m_statechg = (GetState() == cpustate_run);
            if (m_statechg)
            {
                Opt cmd("stop");
                SendCommand(cmd);
                return;
            }
            if (m_pDriver->KillOnNotHaltRun(GetState(), _T("Software Reset")))
            {
                Done();
                return;
            }
            {
                Opt cmd("reset");
                SendCommand(cmd);
            }
        }
        void StateChange()
        {
            if (!m_statechg || GetState() != cpustate_halt)
                return;
            {
                Opt cmd("reset");
                SendCommand(cmd);
            }
            m_statechg = false;
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output) || (!m_statechg && IsLast()))
            {
                Done();
                return;
            }
        }
        bool CanOverlap() const { return !m_statechg; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to write back program memory changes.
  */
class AxsCmd_Writeback : public DebuggerCmd
{
    public:
        AxsCmd_Writeback(DebuggerDriver* driver)
            : DebuggerCmd(driver)
        {
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Writeback")))
            {
                Done();
                return;
            }
            {
                Opt cmd("writeback");
                SendCommand(cmd);
            }
        }
        void StateChange()
        {
            if (!IsLast() || GetState() != cpustate_halt)
                return;
            Done();
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            StateChange();
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to terminate the inferior.
  */
class AxsCmd_Quit : public DebuggerCmd
{
    protected:
        typedef enum {
            state_done = 0,
            state_enumbkp,
            state_waitnonbusybkp,
            state_deletebkp,
            state_waitnonbusy,
            state_waitrun
        } state_t;

        state_t m_State;

        void StateMachine()
        {
            switch (m_State) {
            case state_waitnonbusybkp:
            {
                if (GetState() == cpustate_busy)
                    break;
                if (GetState() != cpustate_halt)
                {
                    Opt cmd("stop");
                    SendCommand(cmd);
                    break;
                }
                m_State = state_deletebkp;
            }

            // fall through
            case state_deletebkp:
            {
                if (GetState() != cpustate_halt)
                    break;
                {
                    Opt cmd("breakpoint");
                    cmd.set_option("delete", "all");
                    SendCommand(cmd);
                }
                {
                    Opt cmd("run");
                    SendCommand(cmd);
                }
                m_State = state_waitrun;
                break;
            }

            case state_waitnonbusy:
            {
                if (GetState() == cpustate_busy)
                    break;
                if (GetState() != cpustate_run)
                {
                    Opt cmd("run");
                    SendCommand(cmd);
                    break;
                }
                m_State = state_waitrun;
            }

            // fall through
            case state_waitrun:
            {
                if (GetState() != cpustate_run)
                    break;
                {
                    Opt cmd("quit");
                    SendCommand(cmd);
                }
                m_State = state_done;
                break;
            }

            case state_done:
            default:
            {
                Done();
                m_State = state_done;
                break;
            }
            }
        }

    public:
        AxsCmd_Quit(DebuggerDriver* driver)
            : DebuggerCmd(driver),
              m_State(state_enumbkp)
        {
        }

        void Action()
        {
            if (GetState() == cpustate_targetdisconnected ||
                GetState() == cpustate_disconnected ||
                GetState() == cpustate_locked)
            {
                Opt cmd("quit");
                SendCommand(cmd);
                m_State = state_done;
                return;
            }

            {
                Opt cmd("breakpoint");
                SendCommand(cmd);
            }
        }

        void StateChange()
        {
            if (!IsLast())
                return;
            StateMachine();
        }

        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                m_State = state_done;
                Done();
                return;
            }
            if (!IsLast())
                return;
            switch (m_State) {
            case state_enumbkp:
            {
                std::pair<unsigned long,bool> cnt(output.get_option_uint("breakpoint"));
                m_State = (cnt.second || cnt.first) ? state_waitnonbusybkp : state_waitnonbusy;
                StateMachine();
                break;
            }

            default:
                StateMachine();
                break;
            }
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to terminate the inferior after an error.
  */
class AxsCmd_Terminate : public DebuggerCmd
{
    public:
        AxsCmd_Terminate(DebuggerDriver* driver)
            : DebuggerCmd(driver)
        {
        }

        void Action()
        {
            Opt cmd("quit");
            SendCommand(cmd);
        }

        void ParseOutput(const Opt& output)
        {
            if (IsLast())
            {
                Done();
                return;
            }
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command that notifies the plugin that the debuggee has continued.
  */
class AxsCmd_ContinueBase : public DebuggerCmd
{
    protected:
        Opt m_opt;
        wxString m_cmdname;

    public:
        AxsCmd_ContinueBase(DebuggerDriver* driver, const std::string& cmd, const wxString& cmdname)
            : DebuggerCmd(driver), m_opt(cmd), m_cmdname(cmdname)
        {
        }

        virtual void Action()
        {
            if ((GetState() == cpustate_targetdisconnected ||
                 GetState() == cpustate_disconnected ||
                 GetState() == cpustate_locked) &&
                m_pDriver->KillOnNotHaltRun(GetState(), m_cmdname))
            {
                Done();
                return;
            }
            SendCommand(m_opt);
        }

        virtual void StateChange()
        {
            if (GetState() != cpustate_busy)
            {
                m_pDriver->NotifyDebuggeeContinued();
                Done();
            }
        }

        virtual void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            StateChange();
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to stop the debuggee.
  */
class AxsCmd_Stop : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_Stop(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "stop", _T("Stop"))
        {
        }

        void Action()
        {
            if (GetState() == cpustate_halt)
            {
                Done();
                return;
            }
            AxsCmd_ContinueBase::Action();
        }

        void StateChange()
        {
            if (GetState() != cpustate_busy)
                Done();
        }
};

/**
  * Command to run the debuggee.
  */
class AxsCmd_Run : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_Run(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "run", _T("Run"))
        {
        }
};

/**
  * Command to step the debuggee.
  */
class AxsCmd_Step : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_Step(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "step", _T("Step"))
        {
        }
};

/**
  * Command to step the debuggee a source line.
  */
class AxsCmd_StepLine : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_StepLine(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "stepline", _T("StepLine"))
        {
        }
};

/**
  * Command to step the debuggee into a function.
  */
class AxsCmd_StepInto : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_StepInto(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "stepinto", _T("StepInto"))
        {
        }
};

/**
  * Command to step the debuggee out of a function.
  */
class AxsCmd_StepOut : public AxsCmd_ContinueBase
{
    public:
        AxsCmd_StepOut(DebuggerDriver* driver)
            : AxsCmd_ContinueBase(driver, "stepout", _T("StepOut"))
        {
        }
};

/**
  * Command to get the disassembly.
  */
class AxsCmd_Disassemble : public DebuggerCmd
{
    private:
        class Symbol
        {
            public:
                Symbol(const wxString& name, const wxString& filename, const wxString& funcname, unsigned int block, unsigned int level, unsigned int addr, unsigned int endaddr)
                    : m_name(name), m_filename(filename), m_funcname(funcname), m_block(block), m_level(level), m_addr(addr), m_endaddr(endaddr) {}
                const wxString& get_name() const { return m_name; }
                const wxString& get_filename() const { return m_filename; }
                const wxString& get_funcname() const { return m_funcname; }
                unsigned int get_block() const { return m_block; }
                unsigned int get_level() const { return m_level; }
                unsigned int get_addr() const { return m_addr; }
                unsigned int get_endaddr() const { return m_endaddr; }

                bool operator<(const Symbol& x) const { return compare(x) < 0; }
                int compare(const Symbol& x) const
                {
                    if (get_addr() < x.get_addr())
                        return -1;
                    if (get_addr() > x.get_addr())
                        return 1;
                    if (get_endaddr() > x.get_endaddr())
                        return -1;
                    if (get_endaddr() < x.get_endaddr())
                        return 1;
                    if (get_block() < x.get_block())
                        return -1;
                    if (get_block() > x.get_block())
                        return 1;
                    if (get_level() < x.get_level())
                        return -1;
                    if (get_level() > x.get_level())
                        return 1;
                    int r = get_name().Cmp(x.get_name());
                    if (r)
                        return r;
                    r = get_filename().Cmp(x.get_filename());
                    if (r)
                        return r;
                    return get_funcname().Cmp(x.get_funcname());
                }

            protected:
                wxString m_name;
                wxString m_filename;
                wxString m_funcname;
                unsigned int m_block;
                unsigned int m_level;
                unsigned int m_addr;
                unsigned int m_endaddr;
        };

        class SourceLine
        {
            public:
                SourceLine(const wxString& name, unsigned int line, unsigned int block, unsigned int level, unsigned int addr)
                    : m_name(name), m_line(line), m_block(block), m_level(level), m_addr(addr) {}
                const wxString& get_name() const { return m_name; }
                unsigned int get_line() const { return m_line; }
                unsigned int get_block() const { return m_block; }
                unsigned int get_level() const { return m_level; }
                unsigned int get_addr() const { return m_addr; }

                bool operator<(const SourceLine& x) const { return compare(x) < 0; }
                int compare(const SourceLine& x) const
                {
                    if (get_addr() < x.get_addr())
                        return -1;
                    if (get_addr() > x.get_addr())
                        return 1;
                    if (get_block() < x.get_block())
                        return -1;
                    if (get_block() > x.get_block())
                        return 1;
                    if (get_level() < x.get_level())
                        return -1;
                    if (get_level() > x.get_level())
                        return 1;
                    if (get_line() < x.get_line())
                        return -1;
                    if (get_line() > x.get_line())
                        return 1;
                    return get_name().Cmp(x.get_name());
                }

            protected:
                wxString m_name;
                unsigned int m_line;
                unsigned int m_block;
                unsigned int m_level;
                unsigned int m_addr;
        };

        class Disass
        {
            public:
                Disass(unsigned long addr, const wxString& opcode, const wxString& mnemonic, const wxString& sourceline, const wxString& sym)
                    : m_opcode(opcode), m_mnemonic(mnemonic), m_sourceline(sourceline), m_sym(sym), m_addr(addr)
                {
                    int idx(m_mnemonic.Find(';', false));
                    if (idx != wxNOT_FOUND)
                    {
                        m_comment = m_mnemonic.Mid(idx);
                        m_mnemonic.Truncate(idx);
                        m_mnemonic.Trim(true);
                    }
                    idx = m_mnemonic.Find('\t', false);
                    if (idx != wxNOT_FOUND)
                    {
                        m_mnemonic2 = m_mnemonic.Mid(idx);
                        m_mnemonic.Truncate(idx);
                        m_mnemonic.Trim(true);
                        m_mnemonic2.Trim(false);
                    }
                }
                const wxString& get_opcode() const { return m_opcode; }
                const wxString& get_mnemonic() const { return m_mnemonic; }
                const wxString& get_mnemonic2() const { return m_mnemonic2; }
                const wxString& get_comment() const { return m_comment; }
                const wxString& get_sourceline() const { return m_sourceline; }
                const wxString& get_sym() const { return m_sym; }
                unsigned long get_addr() const { return m_addr; }

                bool operator<(const Disass& x) const { return compare(x) < 0; }
                int compare(const Disass& x) const
                {
                    if (get_addr() < x.get_addr())
                        return -1;
                    if (get_addr() > x.get_addr())
                        return 1;
                    return 0;
                }

            protected:
                wxString m_opcode;
                wxString m_mnemonic;
                wxString m_mnemonic2;
                wxString m_comment;
                wxString m_sourceline;
                wxString m_sym;
                unsigned long m_addr;
        };

        typedef enum {
            state_done = 0,
            state_symbols,
            state_disassemble,
            state_sourcelines
        } state_t;

        typedef std::set<Symbol> symbols_t;
        symbols_t m_symbols;
        typedef std::set<SourceLine> sourcelines_t;
        sourcelines_t m_sourcelines;
        typedef std::set<Disass> disass_t;
        disass_t m_disass;
        unsigned long m_addr;
        unsigned long m_startaddr;
        unsigned long m_endaddr;
        state_t m_State;
        bool m_addrok;
        bool m_mixedasmmode;

        cbEditor* FindEditor(const wxString& filename)
        {
            DebuggerAXS *plugin = m_pDriver->GetDebugger();
            // This follows cbplugin.cpp: SyncEditor
            FileType ft = FileTypeOf(filename);
            if (ft != ftSource && ft != ftHeader && ft != ftResource)
            {
                // if ft == ftOther assume that we are in header without extension
                if (ft != ftOther)
                {
                    plugin->ShowLog(false);
                    plugin->Log(_("Unknown file: ") + filename, Logger::error);
                    InfoWindow::Display(_("Unknown file"), _("File: ") + filename, 5000);
                    return nullptr;
                }
            }

            cbProject* project = Manager::Get()->GetProjectManager()->GetActiveProject();
            ProjectFile* f = project ? project->GetFileByFilename(filename, false, true) : 0;

            wxString unixfilename = UnixFilename(filename);
            wxFileName fname(unixfilename);

            if (project && fname.IsRelative())
                fname.MakeAbsolute(project->GetBasePath());

            cbEditor* ed = Manager::Get()->GetEditorManager()->Open(fname.GetLongPath());
            if (!ed)
            {
                plugin->ShowLog(false);
                plugin->Log(_("Cannot open file: ") + filename, Logger::error);
                InfoWindow::Display(_("Cannot open file"), _("File: ") + filename, 5000);
            }
            return ed;
        }

        void UpdateDialog()
        {
            cbDisassemblyDlg *dialog = Manager::Get()->GetDebuggerManager()->GetDisassemblyDialog();
            if (!dialog)
                return;
            {
                cbStackFrame frame;
                frame.SetNumber(0);
                frame.SetAddress(m_startaddr);
                if (!m_symbols.empty())
                    frame.SetSymbol(m_symbols.begin()->get_name());
                frame.MakeValid(true);
                dialog->Clear(frame);
            }
            unsigned int colw_opcode(6), colw_mnemo(8), colw_mnemo2(30);
            for (disass_t::const_iterator di(m_disass.begin()), de(m_disass.end()); di != de; ++di)
            {
                colw_opcode = std::max(colw_opcode, (unsigned int)di->get_opcode().Len());
                colw_mnemo = std::max(colw_mnemo, (unsigned int)di->get_mnemonic().Len());
                colw_mnemo2 = std::max(colw_mnemo2, (unsigned int)di->get_mnemonic2().Len());
            }
            colw_opcode += 2;
            colw_mnemo += 2;
            colw_mnemo2 += 2;
            typedef std::map<wxString, cbEditor *> editormap_t;
            editormap_t editormap;
            sourcelines_t::const_iterator sli(m_sourcelines.end()), sle(sli);
            if (m_mixedasmmode)
                sli = m_sourcelines.begin();
            for (disass_t::const_iterator di(m_disass.begin()), de(m_disass.end()); di != de; ++di)
            {
                while (sli != sle && sli->get_addr() < di->get_addr())
                    ++sli;
                while (sli != sle && sli->get_addr() == di->get_addr())
                {
                    const wxString& fname(sli->get_name());
                    editormap_t::iterator edi(editormap.find(fname));
                    if (edi == editormap.end())
                    {
                        AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
                        wxString fname2(driver->FilePathSearch(fname));
                        edi = editormap.find(fname2);
                        if (edi == editormap.end())
                        {
                            std::pair<editormap_t::iterator,bool> ins(editormap.insert(std::make_pair(fname2, FindEditor(fname2))));
                            edi = ins.first;
                            editormap.insert(std::make_pair(fname, edi->second));
                        }
                    }
                    wxString ln;
                    cbEditor *ed(edi->second);
                    if (ed && sli->get_line() >= 1)
                    {
                        ln = ed->GetControl()->GetLine(sli->get_line() - 1);
                    }
                    else
                    {
                        ln = fname;
                        if (sli->get_level() || sli->get_block())
                        {
                            ln << wxT(",") << sli->get_level();
                            if (sli->get_block())
                                ln << wxT(",") << sli->get_block();
                        }
                    }
                    ln.Trim(true);
                    dialog->AddSourceLine(sli->get_line(), ln);
                    ++sli;
                }
                {
                    wxString sym(di->get_sym());
                    if (!sym.IsEmpty() && sym.Find((wxChar)'+') == wxNOT_FOUND && sym.Find((wxChar)'-') == wxNOT_FOUND)
                        dialog->AddAssemblerLine(di->get_addr(), sym + wxT(":"));
                }
                wxString opcode(di->get_opcode());
                wxString mnemo(di->get_mnemonic());
                wxString mnemo2(di->get_mnemonic2());
                opcode.Pad(colw_opcode - opcode.Len());
                mnemo.Pad(colw_mnemo - mnemo.Len());
                mnemo2.Pad(colw_mnemo2 - mnemo2.Len());
                dialog->AddAssemblerLine(di->get_addr(), opcode + mnemo + mnemo2 + di->get_comment());
            }
            dialog->SetActiveAddress(m_addr);
        }


    public:
        AxsCmd_Disassemble(AXS_driver* driver, const wxString& hexAddrStr = wxT(""))
            : DebuggerCmd(driver), m_addr(0), m_startaddr(~0UL), m_endaddr(0), m_State(state_symbols), m_addrok(false)
        {
            m_mixedasmmode = Manager::Get()->GetDebuggerManager()->IsDisassemblyMixedMode();
            if (!hexAddrStr.empty())
            {
                m_addrok = hexAddrStr.ToULong(&m_addr, (hexAddrStr.Left(2) == wxT("0x") || hexAddrStr.Left(2) == wxT("0X")) ? 0 : 16);
            }
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Disassemble")))
            {
                Done();
                return;
            }
            if (!m_addrok)
            {
                const Cursor &cursor = m_pDriver->GetCursor();
                m_addrok = !cursor.address.empty() && cursor.address.ToULong(&m_addr, 16);
            }
            {
                Opt cmd("symbols");
                cmd.set_option("containsaddr", "current");
                cmd.set_option("pc", "current");
                SendCommand(cmd);
            }
            if (m_addrok)
            {
                Opt cmd("nop");
                SendCommand(cmd);
                return;
            }
            {
                Opt cmd("read_pc");
                SendCommand(cmd);
            }
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            switch (m_State)
            {
                case state_symbols:
                {
                    if (output.get_cmdname() != "symbols")
                    {
                        if (output.get_cmdname() == "read_pc")
                        {
                            std::pair<unsigned long,bool> pc(output.get_option_uint("pc"));
                            if (!pc.second)
                            {
                                Done();
                                return;
                            }
                            m_addr = pc.first;
                            m_addrok = true;
                        }
                        {
                            Opt cmd("disass");
                            if (!m_symbols.empty())
                            {
                                cmd.set_option("addr", m_symbols.begin()->get_addr());
                                cmd.set_option("endaddr", m_symbols.begin()->get_endaddr());
                            }
                            else
                            {
                                cmd.set_option("addr", m_addr);
                                cmd.set_option("count", 30);
                            }
                            SendCommand(cmd);
                        }
                        {
                            Opt cmd("nop");
                            SendCommand(cmd);
                        }
                        m_State = state_disassemble;
                        return;
                    }
                    {
                        std::pair<unsigned long,bool> func(output.get_option_uint("function"));
                        if (!func.second || !func.first)
                            break;
                    }
                    {
                        Opt::stringopt_t as(output.get_option("as"));
                        if (!as.second || (as.first != "code" && as.first != "codestatic"))
                            break;
                    }
                    std::pair<wxString,bool> name(output.get_option_wxstring("symname"));
                    std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                    std::pair<unsigned long,bool> endaddr(output.get_option_uint("endaddr"));
                    if (!name.second || !addr.second)
                        break;
                    if (!endaddr.second)
                        endaddr.first = addr.first + 1;
                    std::pair<wxString,bool> filename(output.get_option_wxstring("filename"));
                    if (!filename.second)
                        filename.first.Clear();
                    std::pair<wxString,bool> funcname(output.get_option_wxstring("funcname"));
                    if (!funcname.second)
                        funcname.first.Clear();
                    std::pair<unsigned long,bool> level(output.get_option_uint("level"));
                    if (!level.second)
                        level.first = 0;
                    std::pair<unsigned long,bool> block(output.get_option_uint("block"));
                    if (!block.second)
                        block.first = 0;
                    m_symbols.insert(Symbol(name.first, filename.first, funcname.first, block.first, level.first, addr.first, endaddr.first));
                    break;
                }

                case state_disassemble:
                {
                    if (output.get_cmdname() != "disass")
                    {
                        if (!m_mixedasmmode)
                        {
                            UpdateDialog();
                            m_State = state_done;
                            Done();
                            return;
                        }
                        if (m_endaddr >= m_startaddr)
                        {
                            Opt cmd("sourcelines");
                            cmd.set_option("addr", m_startaddr);
                            cmd.set_option("endaddr", m_endaddr);
                            cmd.set_option("asm", 0);
                            cmd.set_option("C", 1);
                            SendCommand(cmd);
                        }
                        {
                            Opt cmd("nop");
                            SendCommand(cmd);
                        }
                        m_State = state_sourcelines;
                        return;
                    }
                    std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                    std::pair<wxString,bool> mnemonic(output.get_option_wxstring("mnemonic"));
                    std::pair<wxString,bool> opcode(output.get_option_wxstring("opcode"));
                    if (!addr.second || !mnemonic.second || !opcode.second)
                        break;
                    m_startaddr = std::min(m_startaddr, addr.first);
                    m_endaddr = std::max(m_endaddr, addr.first);
                    std::pair<wxString,bool> sourceline(output.get_option_wxstring("sourceline"));
                    if (!sourceline.second)
                        sourceline.first.Clear();
                    std::pair<wxString,bool> sym(output.get_option_wxstring("sym"));
                    if (!sym.second)
                        sym.first.Clear();
                    m_disass.insert(Disass(addr.first, opcode.first, mnemonic.first, sourceline.first, sym.first));
                    break;
                }

                case state_sourcelines:
                {
                    if (output.get_cmdname() != "sourcelines")
                    {
                        UpdateDialog();
                        m_State = state_done;
                        Done();
                        return;
                    }
                    {
                        Opt::stringopt_t lang(output.get_option("lang"));
                        if (!lang.second || lang.first != "C")
                            return;
                    }
                    std::pair<wxString,bool> name(output.get_option_wxstring("name"));
                    std::pair<unsigned long,bool> line(output.get_option_uint("line"));
                    std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                    if (!name.second || !line.second || !addr.second)
                        return;
                    std::pair<unsigned long,bool> level(output.get_option_uint("level"));
                    if (!level.second)
                        level.first = 0;
                    std::pair<unsigned long,bool> block(output.get_option_uint("block"));
                    if (!block.second)
                        block.first = 0;
                    m_sourcelines.insert(SourceLine(name.first, line.first, block.first, level.first, addr.first));
                    break;
                }

                default:
                    m_State = state_done;
                    Done();
                    return;
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to get info about a watched variable.
  */
class AxsCmd_Watch : public DebuggerCmd
{
    private:
        cb::shared_ptr<GDBWatch> m_watch;
        typedef std::map<unsigned int, cb::shared_ptr<GDBWatch> > watchmap_t;
        watchmap_t m_typecmd;
        watchmap_t m_valcmd;

        cb::shared_ptr<GDBWatch> AddChild(cb::shared_ptr<GDBWatch> parent, wxString const &symbol)
        {
            cb::shared_ptr<cbWatch> old_child = parent->FindChild(symbol);
            cb::shared_ptr<GDBWatch> child;
            if (old_child)
            {
                child = cb::static_pointer_cast<GDBWatch>(old_child);
                child->MarkAsRemoved(true);
            }
            else
            {
                child = cb::shared_ptr<GDBWatch>(new GDBWatch(symbol));
                cbWatch::AddChild(parent, child);
                child->MarkAsRemoved(false);
            }
            child->MarkChildsAsRemoved();
            {
                Opt cmd("cexpr");
                cmd.set_option("lvalue", 1);
                cmd.set_option("typeinfo", 1);
                if (GetState() == cpustate_halt)
                    cmd.set_option("pc", "current");
                wxString ex;
                child->GetFullWatchString(ex);
                cmd.set_option("expr", ex);
                m_typecmd[SendCommand(cmd)] = child;
            }
            return child;
        }

        void CheckEnd()
        {
            if (IsPending())
                return;
            Done();
            m_watch->RemoveMarkedChildren();
        }

    public:
        AxsCmd_Watch(DebuggerDriver* driver, cb::shared_ptr<GDBWatch> watch, bool newwatch)
            : DebuggerCmd(driver),
              m_watch(watch)
        {
            if (m_watch)
                m_watch->MarkAsRemoved(!newwatch);
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHaltRun(GetState(), _T("Watch")) || !m_watch)
            {
                Done();
                return;
            }
            m_watch->MarkChildsAsRemoved();
            {
                Opt cmd("cexpr");
                cmd.set_option("lvalue", 1);
                cmd.set_option("typeinfo", 1);
                if (GetState() == cpustate_halt)
                    cmd.set_option("pc", "current");
                wxString ex;
                m_watch->GetFullWatchString(ex);
                cmd.set_option("expr", ex);
                m_typecmd[SendCommand(cmd)] = m_watch;
            }
        }
        void ParseOutput(const Opt& output)
        {
            // find appropriate watch in command maps
            cb::shared_ptr<GDBWatch> watch;
            bool is_type;
            {
                watchmap_t::iterator it(m_typecmd.find(m_CurSeq));
                if (it != m_typecmd.end())
                {
                    watch = it->second;
                    is_type = true;
                    m_typecmd.erase(it);
                }
                else
                {
                    watchmap_t::iterator it(m_valcmd.find(m_CurSeq));
                    if (it != m_valcmd.end())
                    {
                        watch = it->second;
                        is_type = false;
                        m_valcmd.erase(it);
                    }
                }
            }
            // check error, done handling
            CurrentDone();
            if (output.is_option("error") || !watch)
            {
                CheckEnd();
                return;
            }
            // handle cexpr issued to get type information
            if (is_type)
            {
                watch->SetDebugValue(output.get_cmdwxstring());
                watch->SetDisabled(false);
                bool newwatch(!watch->IsRemoved());
                watch->MarkAsRemoved(false);
                WatchType wtype(TypeVoid);
                {
                    std::pair<std::string,bool> cls(output.get_option("class"));
                    if (cls.second)
                    {
                        if (cls.first == "simple")
                            wtype = TypeSimple;
                        else if (cls.first == "bitfield")
                            wtype = TypeBitfield;
                        else if (cls.first == "struct")
                            wtype = TypeStruct;
                        else if (cls.first == "pointer")
                            wtype = TypePointer;
                        else if (cls.first == "array")
                            wtype = TypeArray;
                        else
                            wtype = TypeVoid;
                    }
                    watch->SetWatchType(wtype);
                }
                {
                    watch->ClearEnums();
                    std::pair<Opt::ulongarray_t,bool> evalues(output.get_option_ulongarray("enumvalues"));
                    std::pair<wxArrayString,bool> enames(output.get_option_wxarraystring("enumnames"));
                    if (evalues.second && enames.second)
                    {
                        for (Opt::ulongarray_t::size_type i(0), n(evalues.first.size()); i < n; ++i)
                        {
                            if (i >= enames.first.GetCount())
                                break;
                            watch->AddEnum(evalues.first[i], enames.first[i]);
                        }
                    }
                }
                {
                    std::pair<wxString,bool> type(output.get_option_wxstring("type"));
                    if (type.second)
                    {
                        wxString old_type;
                        watch->GetType(old_type);
                        if (old_type != type.first)
                        {
                            watch->SetType(type.first);
                            watch->SetValue(wxEmptyString);
                            std::pair<unsigned long,bool> tsize(output.get_option_uint("size"));
                            if (tsize.second)
                                watch->SetBitSize(tsize.first * 8);
                            cb::shared_ptr<GDBWatch> watchf(cb::static_pointer_cast<GDBWatch>(watch->GetParent()));
                            if (!watchf || watchf->GetWatchType() != TypeArray)
                                watchf = watch;
                            if (newwatch)
                                watch->SetFormat(Undefined);
                            switch (wtype)
                            {
                                case TypeArray:
                                {
                                    std::pair<unsigned long,bool> dim(output.get_option_uint("dimension"));
                                    if (newwatch)
                                        watch->SetArrayParams(0, dim.second ? dim.first : 0);
                                    break;
                                }

                                case TypeSimple:
                                {
                                    std::pair<unsigned long,bool> isbit(output.get_option_uint("bit"));
                                    if (newwatch)
                                    {
                                        std::pair<unsigned long,bool> issigned(output.get_option_uint("signed"));
                                        std::pair<unsigned long,bool> isfloat(output.get_option_uint("float"));
                                        if (isfloat.second && isfloat.first)
                                            watchf->SetFormat(Float);
                                        else if (isbit.second && isbit.first)
                                            watchf->SetFormat(Binary);
                                        else if (issigned.second && issigned.first)
                                            watchf->SetFormat(Decimal);
                                        else
                                            watchf->SetFormat(Unsigned);
                                    }
                                    if (isbit.second && isbit.first)
                                        watch->SetBitSize(1);
                                    break;
                                }

                                case TypeBitfield:
                                {
                                    if (newwatch)
                                    {
                                        std::pair<unsigned long,bool> issigned(output.get_option_uint("signed"));
                                        if (issigned.second && issigned.first)
                                            watchf->SetFormat(Decimal);
                                        else
                                            watchf->SetFormat(Unsigned);
                                    }
                                    std::pair<unsigned long,bool> len(output.get_option_uint("length"));
                                    if (len.second)
                                        watch->SetBitSize(len.first);
                                    break;
                                }

                                default:
                                    break;
                            }
                        }
                    }
                }
                {
                    std::pair<Opt::ulongarray_t,bool> addr(output.get_option_ulongarray("addr"));
                    std::pair<wxArrayString,bool> as(output.get_option_wxarraystring("as"));
                    int len = 4;
                    if (as.second && !as.first.IsEmpty() && (as.first[0] == wxT("direct") || as.first[0] == wxT("indirect") ||
                                                             as.first[0] == wxT("sfr") || as.first[0] == wxT("p")))
                        len = 2;
                    if (addr.second && !addr.first.empty())
                        watch->SetAddr(wxString::Format(wxT("0x%0*x"), len, addr.first.front()));
                    if (as.second && !as.first.IsEmpty())
                        watch->SetAddrSpace(as.first[0]);
                }
                switch (wtype)
                {
                    case TypeArray:
                    {
                        for (int i = 0; i < watch->GetArrayCount(); ++i)
                        {
                            AddChild(watch, wxString::Format(wxT("[%d]"), watch->GetArrayStart() + i));
                        }
                        {
                            Opt cmd("cexpr");
                            cmd.set_option("lvalue", 0);
                            if (GetState() == cpustate_halt)
                                cmd.set_option("pc", "current");
                            wxString ex;
                            watch->GetFullWatchString(ex);
                            cmd.set_option("expr", ex);
                            m_valcmd[SendCommand(cmd)] = watch;
                        }
                        break;
                    }

                    case TypeStruct:
                    {
                        std::pair<wxArrayString,bool> fldnames(output.get_option_wxarraystring("fieldnames"));
                        if (!fldnames.second)
                            break;
                        for (int i = 0; i < (int)fldnames.first.GetCount(); ++i)
                        {
                            AddChild(watch, fldnames.first[i]);
                        }
                        break;
                    }

                    case TypeSimple:
                    case TypeBitfield:
                    case TypePointer:
                    {
                        Opt cmd("cexpr");
                        cmd.set_option("lvalue", 0);
                        if (GetState() == cpustate_halt)
                            cmd.set_option("pc", "current");
                        wxString ex;
                        watch->GetFullWatchString(ex);
                        cmd.set_option("expr", ex);
                        m_valcmd[SendCommand(cmd)] = watch;
                        break;
                    }

                    default:
                        break;
                }
                CheckEnd();
                return;
            }
            // handle cexpr issued to get value
            WatchFormat watchfmt(watch->GetFormat());
            bool watchfmtparent(false);
            for (cb::shared_ptr<GDBWatch> w(cb::static_pointer_cast<GDBWatch>(watch->GetParent()));
                 watchfmt == Undefined && w; w = cb::static_pointer_cast<GDBWatch>(w->GetParent()))
            {
                watchfmt = w->GetFormat();
                watchfmtparent = true;
            }
            {
                Opt::stringopt_t restype(output.get_option("resulttype"));
                if (restype.second)
                {
                    if (restype.first == "int")
                    {
                        std::pair<long,bool> res(output.get_option_int("result"));
                        if (res.second)
                        {
                            wxString suffix;
			    if (watch->FindEnum(suffix, res.first))
                            {
                                suffix = wxT(" (") + suffix + wxT(")");
                            }
                            else
                            {
                                suffix.Clear();
                            }
                            unsigned int bsize(watch->GetBitSize());
                            switch (watchfmt)
                            {
                                case Float:
                                case String:
                                    if (!watchfmtparent)
                                        watch->SetFormat(Decimal);
                                    // fall through

                                default:
                                case Decimal:
                                {
                                    long val(res.first);
                                    val &= (1UL << bsize) - 1UL;
                                    val |= -(val & (1UL << (bsize - 1)));
                                    watch->SetValue(wxString::Format(wxT("%ld"), val) + suffix);
                                    break;
                                }

                                case Unsigned:
                                {
                                    res.first &= (1ULL << bsize) - 1;
                                    watch->SetValue(wxString::Format(wxT("%lu"), res.first) + suffix);
                                    break;
                                }

                                case Hex:
                                {
                                    res.first &= (1ULL << bsize) - 1;
                                    watch->SetValue(wxString::Format(wxT("0x%0*lx"), (bsize + 3) >> 2, res.first) + suffix);
                                    break;
                                }

                                case Binary:
                                {
                                    wxString st;
                                    for (unsigned int i = 0; i < bsize; ++i)
                                    {
                                        st.Prepend((wxChar)('0' + (res.first & 1)));
                                        res.first >>= 1;
                                    }
                                    watch->SetValue(st + suffix);
                                    break;
                                }

                                case Char:
                                {
                                    if (res.first >= 32 && res.first < 127)
                                        watch->SetValue(wxString::Format(wxT("'%c'"), (char)res.first) + suffix);
                                    else
                                        watch->SetValue(wxString::Format(wxT("%ld"), res.first) + suffix);
                                    break;
                                }
                            }
                        }
                    }
                    else if (restype.first == "float")
                    {
                        std::pair<double,bool> res(output.get_option_float("result"));
                        if (res.second)
                        {
                            switch (watchfmt)
                            {
                                case Decimal:
                                case Unsigned:
                                case Hex:
                                case Binary:
                                case Char:
                                case String:
                                    if (!watchfmtparent)
                                        watch->SetFormat(Float);
                                    // fall through

                                default:
                                case Float:
                                {
                                    watch->SetValue(wxString::Format(wxT("%g"), res.first));
                                    break;
                                }
                            }
                        }
                    }
                    else if (restype.first == "string")
                    {
                        std::pair<wxString,bool> res(output.get_option_wxstring("result"));
                        if (res.second)
                        {
                            switch (watchfmt)
                            {
                                case Decimal:
                                case Unsigned:
                                case Hex:
                                case Binary:
                                case Char:
                                case Float:
                                    if (!watch->GetChildCount() && !watchfmtparent)
                                        watch->SetFormat(String);
                                    // fall through

                                default:
                                case String:
                                {
                                    watch->SetValue(res.first);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            CheckEnd();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to display a tooltip about a variables value.
  */
class AxsCmd_TooltipEvaluation : public DebuggerCmd
{
    private:
        wxRect m_WinRect;
        cb::shared_ptr<GDBWatch> m_watch;
        typedef enum {
            state_done,
            state_lval,
            state_ptrlval,
            state_expr
        } state_t;
        state_t m_state;

    public:
        AxsCmd_TooltipEvaluation(DebuggerDriver* driver, const wxString& what, const wxRect& tiprect)
            : DebuggerCmd(driver),
            m_WinRect(tiprect),
            m_state(state_done)
        {
            m_watch.reset(new GDBWatch(what));
        }
        void Action()
        {
            if (m_pDriver->KillOnNotHalt(GetState(), _T("Tooltip Evaluation")))
            {
                Done();
                return;
            }
            {
                Opt cmd("cexpr");
                cmd.set_option("lvalue", 1);
                cmd.set_option("typeinfo", 1);
                cmd.set_option("pc", "current");
                wxString sym;
                m_watch->GetSymbol(sym);
                cmd.set_option("expr", sym);
                SendCommand(cmd);
            }
            m_state = state_lval;
        }
        void ParseOutput(const Opt& output)
        {
            CurrentDone();
            if (output.is_option("error"))
            {
                Done();
                m_state = state_done;
                return;
            }
            switch (m_state)
            {
                default:
                {
                    Done();
                    m_state = state_done;
                    break;
                }

                case state_lval:
                case state_ptrlval:
                {
                    {
                        std::pair<Opt::ulongarray_t,bool> addr(output.get_option_ulongarray("addr"));
                        std::pair<wxArrayString,bool> as(output.get_option_wxarraystring("as"));
                        if (!addr.second || addr.first.empty() || !as.second || as.first.IsEmpty())
                        {
                            Done();
                            m_state = state_done;
                            break;
                        }
                        m_watch->SetAddrSpace(as.first[0]);
                        m_watch->SetAddr(wxString::Format(wxT("0x%04lx"), addr.first.front()));
                    }
                    {
                        std::pair<std::string,bool> cls(output.get_option("class"));
                        if (!cls.second)
                        {
                            Done();
                            m_state = state_done;
                            break;
                        }
                        if (cls.first == "pointer" && m_state == state_lval)
                        {
                            wxString sym;
                            m_watch->GetSymbol(sym);
                            sym.Prepend(wxT("*("));
                            sym.Append(wxT(")"));
                            m_watch->SetSymbol(sym);
                            {
                                Opt cmd("cexpr");
                                cmd.set_option("lvalue", 1);
                                cmd.set_option("typeinfo", 1);
                                cmd.set_option("pc", "current");
                                cmd.set_option("expr", sym);
                                SendCommand(cmd);
                            }
                            m_state = state_lval;
                            break;
                        }
                        if (cls.first == "pointer" || cls.first == "simple" || cls.first == "bitfield" || cls.first == "array")
                        {
                            {
                                Opt cmd("cexpr");
                                cmd.set_option("lvalue", 0);
                                cmd.set_option("typeinfo", 0);
                                cmd.set_option("pc", "current");
                                wxString sym;
                                m_watch->GetSymbol(sym);
                                cmd.set_option("expr", sym);
                                SendCommand(cmd);
                            }
                            m_state = state_expr;
                            break;
                        }
                    }
                    Done();
                    m_state = state_done;
                    break;
                }

                case state_expr:
                {
                    Opt::stringopt_t restype(output.get_option("resulttype"));
                    if (!restype.second)
                    {
                        Done();
                        m_state = state_done;
                        break;
                    }
                    {
                        std::pair<wxString,bool> typ(output.get_option_wxstring("type"));
                        if (typ.second)
                            m_watch->SetType(typ.first);
                    }
                    bool ok(false);
                    if (restype.first == "int")
                    {
                        std::pair<long,bool> res(output.get_option_int("result"));
                        if (res.second)
                            m_watch->SetType(wxString::Format(wxT("%ld"), res.first));
                    }
                    else if (restype.first == "float")
                    {
                        std::pair<double,bool> res(output.get_option_float("result"));
                        if (res.second)
                            m_watch->SetType(wxString::Format(wxT("%g"), res.first));
                    }
                    else if (restype.first == "string")
                    {
                        std::pair<wxString,bool> res(output.get_option_wxstring("result"));
                        if (res.second)
                            m_watch->SetType(res.first);
                    }
                    if (!ok)
                    {
                        Done();
                        m_state = state_done;
                        break;
                    }
                    m_watch->SetForTooltip(true);
                    if (Manager::Get()->GetDebuggerManager()->ShowValueTooltip(m_watch, m_WinRect))
                        m_pDriver->GetDebugger()->AddWatchNoUpdate(m_watch);
                    Done();
                    m_state = state_done;
                    break;
                }
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to update pin emulation state.
  */
class AxsCmd_PinEmulation : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        AxsCmd_PinEmulation(DebuggerDriver* driver)
            : DebuggerCmd(driver),
              m_cmd("pinemul")
        {
            axs_cbPinEmDlg *dialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
            if (dialog)
            {
                m_cmd.set_option("peenable", (unsigned int)!!dialog->IsEnabled());
                m_cmd.set_option("pedrvb6", (unsigned int)!!dialog->GetPortB6());
                m_cmd.set_option("pedrvb7", (unsigned int)!!dialog->GetPortB7());
            }
        }
        void Action()
        {
            if (!m_cmd.is_option("peenable"))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            Done();
            axs_cbPinEmDlg *dialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
            if (!dialog)
                return;
            {
                std::pair<long,bool> peenable(output.get_option_int("peenable"));
                if (peenable.second)
                    dialog->SetEnable(!!peenable.first);
            }
            {
                std::pair<long,bool> pedir(output.get_option_int("pedirb6"));
                std::pair<long,bool> pedrv(output.get_option_int("pedrvb6"));
                std::pair<long,bool> peport(output.get_option_int("peportb6"));
                if (pedir.second && pedrv.second && peport.second)
                    dialog->SetPortB6(!!pedir.first, !!peport.first, !!pedrv.first);
            }
            {
                std::pair<long,bool> pedir(output.get_option_int("pedirb7"));
                std::pair<long,bool> pedrv(output.get_option_int("pedrvb7"));
                std::pair<long,bool> peport(output.get_option_int("peportb7"));
                if (pedir.second && pedrv.second && peport.second)
                    dialog->SetPortB7(!!pedir.first, !!peport.first, !!pedrv.first);
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to update debuglink state.
  */
class AxsCmd_DebugLink : public DebuggerCmd
{
    private:
        Opt m_cmd;
    public:
        AxsCmd_DebugLink(DebuggerDriver* driver)
            : DebuggerCmd(driver),
              m_cmd("debuglink")
        {
            axs_cbDbgLink *dialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
            if (dialog)
            {
                wxString tx;
                dialog->GetTransmit(tx);
                if (!tx.IsEmpty())
                    m_cmd.set_option("dbglnktx", tx);
            }
        }
        void Action()
        {
            if (!m_cmd.is_option("dbglnktx"))
            {
                Done();
                return;
            }
            SendCommand(m_cmd);
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                Done();
                return;
            }
            Done();
            axs_cbDbgLink *dialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
            if (!dialog)
                return;
            {
                std::pair<unsigned long,bool> dbglnktxfree(output.get_option_uint("dbglnktxfree"));
                std::pair<unsigned long,bool> dbglnktxcount(output.get_option_uint("dbglnktxcount"));
                if (dbglnktxfree.second)
                    dialog->SetTransmitBuffer(dbglnktxfree.first, dbglnktxcount.second ? dbglnktxcount.first : 0);
            }
            {
                std::pair<wxString,bool> dbglnktx(output.get_option_wxstring("dbglnktx"));
                if (dbglnktx.second)
                    dialog->AddTransmit(dbglnktx.first);
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to update a stack frame with symbol and source line information.
  */
class AxsCmd_UpdateStackFrame : public DebuggerCmd
{
    private:
        cb::shared_ptr<cbStackFrame> m_sf;
    public:
        AxsCmd_UpdateStackFrame(AXS_driver* driver, cb::shared_ptr<cbStackFrame> sf)
            : DebuggerCmd(driver), m_sf(sf)
        {
        }
        void Action()
        {
            if (!m_sf || !m_sf->IsValid())
            {
                Done();
                return;
            }
            {
                Opt cmd("sourcelines");
                cmd.set_option("asm", 0);
                cmd.set_option("C", 1);
                cmd.set_option("addr", m_sf->GetAddress());
                SendCommand(cmd);
            }
            {
                Opt cmd("symbols");
                cmd.set_option("containsaddr", m_sf->GetAddress());
                cmd.set_option("pc", m_sf->GetAddress());
                SendCommand(cmd);
            }
            {
                Opt cmd("nop");
                SendCommand(cmd);
            }
        }
        void ParseOutput(const Opt& output)
        {
            bool cmdsl(output.get_cmdname() == "sourcelines");
            bool cmdsym(output.get_cmdname() == "symbols");
            if (m_pDriver->KillOnError(output) || IsLast() || !(cmdsl || cmdsym))
            {
                Done();
                return;
            }
            if (cmdsl)
            {
                std::pair<wxString,bool> name(output.get_option_wxstring("name"));
                std::pair<wxString,bool> line(output.get_option_wxstring("line"));
                if (name.second && line.second)
                {
                    AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
                    m_sf->SetFile(driver->FilePathSearch(name.first), line.first);
                }
                return;
            }
            if (cmdsym)
            {
                {
                    std::pair<unsigned long,bool> func(output.get_option_uint("function"));
                    if (!func.second || !func.first)
                        return;
                }
                {
                    Opt::stringopt_t as(output.get_option("as"));
                    if (!as.second || !(as.first == "code" || as.first == "codestatic"))
                        return;
                }
                Opt::stringopt_t name(output.get_option("name"));
                std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
                if (name.second && addr.second)
                {
                    std::ostringstream oss;
                    oss << name.first;
                    long diff(m_sf->GetAddress() - addr.first);
                    if (diff)
                        oss << std::showpos << diff;
                    m_sf->SetSymbol(wxString(oss.str().c_str(), wxConvUTF8));
                }
                return;
            }
            Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to update the backtrace window.
  */
class AxsCmd_UpdateBacktrace : public DebuggerCmd
{
    public:
        AxsCmd_UpdateBacktrace(DebuggerDriver* driver)
            : DebuggerCmd(driver)
        {
        }
        void Action()
        {
            Done();
            Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
            long line;
            cbStackFrame validSF;

            // replace the valid stack frame with the first frame or the user selected frame

            if (!m_pDriver->GetStackFrames().empty())
            {
                if (m_pDriver->GetUserSelectedFrame() != -1)
                {
                    int validFrameNumber = m_pDriver->GetUserSelectedFrame();
                    DebuggerDriver::StackFrameContainer const &frames = m_pDriver->GetStackFrames();

                    if (validFrameNumber >= 0 && validFrameNumber <= static_cast<int>(frames.size()))
                        validSF = *frames[validFrameNumber];
                    else if (!frames.empty())
                        validSF = *frames.front();
                    if (validSF.GetLine().ToLong(&line))
                    {
                        m_pDriver->Log(wxString::Format(_T("Displaying first frame with valid source info (#%d)"), validFrameNumber));
                        m_pDriver->ShowFile(validSF.GetFilename(), line);
                    }
                }
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to run a backtrace.
  */
class AxsCmd_Backtrace : public DebuggerCmd
{
    private:
        int m_sfnumber;
    public:
        AxsCmd_Backtrace(AXS_driver* driver)
            : DebuggerCmd(driver), m_sfnumber(0)
        {
        }
        void Action()
        {
            {
                Opt cmd("backtrace");
                SendCommand(cmd);
            }
            {
                Opt cmd("nop");
                SendCommand(cmd);
            }
            m_pDriver->GetStackFrames().clear();
        }
        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output) || IsLast() || output.get_cmdname() != "backtrace")
            {
                m_pDriver->QueueCommand(new AxsCmd_UpdateBacktrace(m_pDriver));
                Done();
                return;
            }
            std::pair<unsigned long,bool> addr(output.get_option_uint("addr"));
            if (!addr.second)
                return;
            cbStackFrame sf;
            sf.SetNumber(m_sfnumber++);
            sf.SetAddress(addr.first);
            sf.MakeValid(true);
            cb::shared_ptr<cbStackFrame> sfp(new cbStackFrame(sf));
            m_pDriver->GetStackFrames().push_back(sfp);
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            driver->QueueCommand(new AxsCmd_UpdateStackFrame(driver, sfp));
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return false; }
};

/**
  * Command to notify the end of the init phase.
  */
class AxsCmd_NotifyInitDone : public DebuggerCmd
{
    public:
        AxsCmd_NotifyInitDone(AXS_driver* driver)
            : DebuggerCmd(driver)
        {
        }
        void Action()
        {
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            driver->NotifyInitDone();
            Done();
            driver->DebugLog(_T("Init Done."));
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to set CPU trace.
  */
class AxsCmd_CPUTrace : public DebuggerCmd
{
    private:
	unsigned int m_buffer;
	unsigned int m_tx;
	bool m_sourcelines;
    public:
        AxsCmd_CPUTrace(AXS_driver* driver, unsigned int buffer = 0, unsigned int tx = 0, bool sourcelines = false)
            : DebuggerCmd(driver), m_buffer(buffer), m_tx(tx), m_sourcelines(sourcelines)
        {
        }
        void Action()
        {
            {
                Opt cmd("cputrace");
                cmd.set_option("buffer", m_buffer);
                cmd.set_option("sl", (unsigned int)!!m_sourcelines);
                cmd.set_option("tx", m_tx);
                SendCommand(cmd);
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to set Profile.
  */
class AxsCmd_Profile : public DebuggerCmd
{
    private:
	unsigned int m_samples;
	bool m_enable_c;
	bool m_enable_asm;
    public:
        AxsCmd_Profile(AXS_driver* driver, unsigned int samples, bool enable_c = false, bool enable_asm = false)
            : DebuggerCmd(driver), m_samples(samples), m_enable_c(enable_c), m_enable_asm(enable_asm)
        {
            if (!m_samples)
                m_enable_c = m_enable_asm = false;
        }
        void Action()
        {
            {
                Opt cmd("profile");
                cmd.set_option("samples", m_samples);
                cmd.set_option("asm", (unsigned int)!!m_enable_asm);
                cmd.set_option("C", (unsigned int)!!m_enable_c);
                SendCommand(cmd);
            }
        }
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return true; }
};

/**
  * Command to connect to a target and download code.
  */
class AxsCmd_ConnectTargetDownload : public DebuggerCmd
{
    public:
        typedef enum {
            mode_run,
            mode_break,
            mode_attach,
            mode_undetermined
        } mode_t;

    private:
        cb::shared_ptr<AXSRegister> m_registerlist;

        typedef enum {
            state_idle = 0,
            state_dead,
            state_disconnecttarget,
            state_enumtargets,
            state_conntarget,
            state_waitbulkerase,
            state_bulkerase,
            state_allerase,
            state_erase,
            state_prepare,
            state_waitdone,
        } state_t;

        state_t m_State;
        mode_t m_Mode;

        void EraseProgMemory(ProjectTargetOptions::FlashEraseType fe = ProjectTargetOptions::BulkErase)
        {
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            switch (fe) {
            default:
            case ProjectTargetOptions::BulkErase:
            {
                // if the CPU is running, stop it first; it may be processing a breakpoint
                // at which point bulkerase will complain about the wrong state
                if (GetState() == cpustate_run)
                {
                    Opt cmd("stop");
                    SendCommand(cmd);
                    m_State = state_waitbulkerase;
                    break;
                }
                Opt cmd("bulkerase");
                driver->CommandAddKeys(cmd);
                SendCommand(cmd);
                m_State = state_bulkerase;
                break;
            }

            case ProjectTargetOptions::AllSectorErase:
            {
                Opt cmd("stop");
                SendCommand(cmd);
                m_State = state_allerase;
                break;
            }

            case ProjectTargetOptions::NeededSectorErase:
            {
                Opt cmd("stop");
                SendCommand(cmd);
                m_State = state_erase;
                break;
            }
            }
        }

        void KillDebugger()
        {
            m_State = state_dead;
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            driver->KillDebugger();
        }

    public:
        AxsCmd_ConnectTargetDownload(AXS_driver* driver, cb::shared_ptr<AXSRegister> registerlist, mode_t mode = mode_undetermined)
            : DebuggerCmd(driver),
              m_registerlist(registerlist),
              m_State(state_dead),
              m_Mode(mode)
        {
        }

        void Action()
        {
            if (GetState() != cpustate_targetdisconnected) {
                Opt cmd("disconnect_target");
                SendCommand(cmd);
                m_State = state_disconnecttarget;
                return;
            }
            {
                Opt cmd("list_targets");
                SendCommand(cmd);
                m_State = state_enumtargets;
            }
        }

        void StateChange()
        {
            if (!IsLast())
                return;
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            switch (m_State) {
            case state_conntarget:
            {
                if (GetState() == cpustate_busy)
                    break;
                if (GetState() == cpustate_locked) {
                    if (m_Mode == mode_attach)
                    {
                        KillDebugger();
                        cbMessageBox(_T("The Microcontroller is inaccessible or locked and none of the keys from the project keyring matches.\n\n"
                                        "Check Debugger connection, recycle the Device or add the correct key to Project->Properties->Axsem Debugger: Additional Keys.\n"),
                                     _T("Inaccessible or Locked Microfoot Device"), wxICON_EXCLAMATION);
                        Done();
                        break;
                    }
                    AnnoyingDialog dlg(_("Inaccessible or Locked Microfoot Device"),
                                       _("The Microcontroller is inaccessible or locked and none of the keys from the project keyring matches.\n\n"
					 "Check Debugger connection and recycle the Device.\n\n"
                                         "If you know the correct key, cancel and add the key to Project->Properties->Axsem Debugger: Additional Keys.\n\n"
                                         "Do you want to fully erase the Microcontroller, but loosing the factory calibration data?"),
                                       wxART_QUESTION, AnnoyingDialog::YES_NO, AnnoyingDialog::rtNO);
                    switch (dlg.ShowModal()) {
                    case wxID_YES:
                        if (m_Mode == mode_undetermined)
                            m_Mode = mode_run;
                        EraseProgMemory();
                        break;

                    default:
                        KillDebugger();
                        Done();
                        break;
                    }
                    break;
                }
                if (m_Mode == mode_undetermined)
                {
                    const wxFileName& BinFile(driver->GetBinFile());
                    AnnoyingDialog dlg(_("Program Microfoot Device"),
                                       wxString::Format(_("Load the freshly built firmware '%s' to the connected device?"), BinFile.GetFullName().c_str()),
                                       wxART_QUESTION, AnnoyingDialog::YES_NO_CANCEL, AnnoyingDialog::rtYES);
                    switch (dlg.ShowModal()) {
                    case wxID_YES:
                        m_Mode = mode_run;
                        break;

                    case wxID_NO:
                        m_Mode = mode_attach;
                        break;

                    default:
                        m_Mode = mode_undetermined;
                        break;
                    }
                }
                if (m_Mode == mode_undetermined)
                {
                    KillDebugger();
                    Done();
                    break;
                }
                if (m_Mode == mode_attach)
                {
                    m_State = state_prepare;
                    Opt cmd("nop");
                    driver->CommandAddKeys(cmd);
                    SendCommand(cmd);
                }
                else
                {
                    ProjectTargetOptions::FlashEraseType fe(driver->GetProjTargetOpt().flashErase);
                    if (m_pDriver->GetCurrentKey() != driver->GetProjTargetOpt().key)
                        fe = ProjectTargetOptions::BulkErase;
                    EraseProgMemory(fe);
                }
                break;
            }

            case state_waitbulkerase:
            {
                if (GetState() != cpustate_halt)
                    break;
                Opt cmd("bulkerase");
                driver->CommandAddKeys(cmd);
                SendCommand(cmd);
                m_State = state_bulkerase;
                break;
            }

            case state_bulkerase:
            {
                if (GetState() == cpustate_busy)
                    break;
                {
                    Opt cmd("stop");
                    SendCommand(cmd);
                }
                ProjectTargetOptions::key_t key(driver->GetProjTargetOpt().key);
                if (key != ProjectTargetOptions::defaultKey) {
                    Opt cmd("writekey");
                    cmd.set_option("key", key);
                    SendCommand(cmd);
                }
                m_State = state_erase;
                break;
            }

            case state_allerase:
            {
                if (GetState() != cpustate_halt)
                    break;
                Opt cmd("fill_mem");
                cmd.set_option("as", "flash");
                cmd.set_option("addr", 0x0000);
                cmd.set_option("length", 0xFC00);
                cmd.set_option("data", 0xFF);
                SendCommand(cmd);
                m_State = state_erase;
            }

            // fall through!
            case state_erase:
            {
                if (GetState() != cpustate_halt)
                    break;
                const wxFileName& BinFile(driver->GetBinFile());
                if (BinFile.IsOk()) {
                    Opt cmd("load_mem");
                    cmd.set_option("file", BinFile.GetFullPath());
                    if (driver->GetProjTargetOpt().fillBreakpoints)
                        cmd.set_option("fillbkpt", (unsigned long)0xfc00);
                    SendCommand(cmd);
                }
                m_State = state_prepare;
            }

            // fall through!
            case state_prepare:
            {
                const wxFileName& SymFile(driver->GetSymFile());
                if (SymFile.IsOk()) {
                    Opt cmd("load_mem");
                    cmd.set_option("file", SymFile.GetFullPath());
                    cmd.set_option("load", "debug");
                    SendCommand(cmd);
                }
                AXS_driver::compilervendor_t cv(driver->GetCompilerVendor());
                if (cv != AXS_driver::compilervendor_unknown) {
                    Opt cmd("compilervendor");
                    cmd.set_option("compilervendor", AXS_driver::to_str(cv));
                    SendCommand(cmd);
                }
                m_State = state_waitdone;
            }

            // fall through!
            case state_waitdone:
            {
                if (IsLast())
                {
                    Done();
                    m_State = state_idle;
                    if (GetState() == cpustate_halt)
                        driver->QueueCommand(new AxsCmd_AutodetectChip(driver, true));
                    else
                        driver->QueueCommand(new AxsCmd_GetChip(driver, true));
                    driver->QueueCommand(new AxsCmd_RegisterList(driver, m_registerlist, false));
                    if (GetState() == cpustate_halt)
                        driver->QueueCommand(new AxsCmd_ReadRegisters(driver, m_registerlist, AxsCmd_ReadRegisters::Recurse));
                    {
                        DebuggerState& state = driver->GetDebugger()->GetState();
                        state.DebuggerState::ApplyBreakpoints();
                    }
                    if (m_Mode == mode_break)
                        m_pDriver->GetDebugger()->GetState().AddBreakpoint(0x0000, true);
                    if (GetState() == cpustate_halt)
                        driver->QueueCommand(new AxsCmd_Writeback(driver));
                    if (m_Mode != mode_attach)
                        driver->QueueCommand(new AxsCmd_HardwareReset(driver));
                    driver->QueueCommand(new AxsCmd_NotifyInitDone(driver));
                }
                break;
            }

            default:
            case state_idle:
                break;
            }
        }

        void ParseOutput(const Opt& output)
        {
            if (m_pDriver->KillOnError(output))
            {
                m_State = state_dead;
                Done();
                return;
            }
            if (!IsLast())
                return;
            AXS_driver* driver = static_cast<AXS_driver*>(m_pDriver);
            switch (m_State) {
            case state_disconnecttarget:
            {
                m_State = state_enumtargets;
                Opt cmd("list_targets");
                SendCommand(cmd);
                break;
            }

            case state_enumtargets:
            {
                std::pair<wxArrayString,bool> serials(output.get_option_wxarraystring("serials"));
                if (!serials.second || serials.first.GetCount() < 1) { // no target
                    // wxMessageBox must run before KillDebugger; if KillDebugger was befo
                    // it would issue a quit command, which axsdb honours and exits, causi
                    // Messages are processed during the message box, which would stop the                    KillDebugger();
                    KillDebugger();
                    cbMessageBox(_T("No devices found"), _T("Warning"), wxICON_EXCLAMATION);
                    Done();
                    break;
                }
                wxString serial_device;
                if (serials.first.GetCount() == 1) { // there is only one target
                    serial_device = serials.first[0];
                } else {
                    serial_device = wxGetSingleChoice(_("Choose serial to attach to:\n"), _("Choose serial"), serials.first);
                }
                if (serial_device.IsNull()) {
                    KillDebugger();
                    Done();
                    break;
                }
                {
                    Opt cmd("connect_target");
                    cmd.set_option("serial", serial_device);
                    SendCommand(cmd);
                }
                driver->UpdateProjectTargetOptions();
                driver->FindProgramFiles();
                if (m_Mode == mode_run)
                {
                    Opt cmd("hwreset");
                    cmd.set_option("mode", "on");
                    SendCommand(cmd);
                }
                {
                    Opt cmd("connect");
                    driver->CommandAddKeys(cmd);
                    SendCommand(cmd);
                }
                m_State = state_conntarget;
                break;
            }

            case state_conntarget:
            case state_waitbulkerase:
            case state_bulkerase:
            case state_allerase:
            case state_erase:
            case state_prepare:
            case state_waitdone:
                StateChange();
                break;

            default:
            case state_idle:
                break;
            }
        }
        bool CanOverlap() const { return false; }
        bool IsBarrier() const { return true; }
};

#endif // AXS_DEBUGGER_COMMANDS_H
