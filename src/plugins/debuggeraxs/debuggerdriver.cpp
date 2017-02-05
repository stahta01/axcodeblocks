/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include "debuggerdriver.h"
#include "debuggeraxs.h"
#include "annoyingdialog.h"
#include "machine.h"

#include <cbdebugger_interfaces.h>

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(DebuggerCommands);

DebuggerDriver::DebuggerDriver(DebuggerAXS* plugin)
    : m_pDBG(plugin),
    m_ProgramIsStopped(true),
    m_ChildPID(0),
    m_CmdSequence(1),
    m_currentFrameNo(0),
    m_userSelectedFrameNo(-1),
    m_CpuState(cpustate_targetdisconnected),
    m_CurrentKey(ProjectTargetOptions::defaultKey),
    m_SyncComm(false)
{
    //ctor
}

DebuggerDriver::~DebuggerDriver()
{
    //dtor
    ClearQueue();
    ClearRunQueue();
}

void DebuggerDriver::Log(const wxString& msg)
{
    m_pDBG->Log(msg);
}

void DebuggerDriver::DebugLog(const wxString& msg)
{
    m_pDBG->DebugLog(msg);
}

void DebuggerDriver::ClearDirectories()
{
    m_Dirs.Clear();
}

void DebuggerDriver::AddDirectory(const wxString& dir)
{
    if (m_Dirs.Index(dir) == wxNOT_FOUND)
    {
        m_Dirs.Add(dir);
        Log(_("Adding source dir: ") + dir);
    }
}

void DebuggerDriver::SetWorkingDirectory(const wxString& dir)
{
    m_WorkingDir = dir;
}

wxString DebuggerDriver::GetDebuggersWorkingDirectory() const
{
    wxString oldDir = wxGetCwd();
    wxSetWorkingDirectory(m_WorkingDir);
    wxString newDir = wxGetCwd();
    wxSetWorkingDirectory(oldDir);
    return newDir;
}

void DebuggerDriver::SetArguments(const wxString& args)
{
    m_Args = args;
}

void DebuggerDriver::Prepare(bool isConsole, bool liveUpdate, bool isSyncComm)
{
    m_SyncComm = isSyncComm;
    Prepare(isConsole, liveUpdate);
}

void DebuggerDriver::ShowFile(const wxString& file, int line)
{
    wxCommandEvent event(DEBUGGER_SHOW_FILE_LINE);
    event.SetString(file);
    event.SetInt(line);
    m_pDBG->ProcessEvent(event);
}

#include <iostream>

void DebuggerDriver::NotifyCursorChanged(bool editor_only)
{
    std::cerr << "NotifyCursorChanged: changed " << (m_Cursor.changed ? "true" : "false")
              << " addr: " << (const char *)m_Cursor.address.mb_str()
              << " file: " << (const char *)m_Cursor.file.mb_str() << ':' << m_Cursor.line;
    if (m_Cursor.line_end > m_Cursor.line)
        std::cerr << '-' << m_Cursor.line_end;
    std::cerr << " func: " << (const char *)m_Cursor.function.mb_str() << std::endl;
    if (!m_Cursor.changed || m_LastCursorAddress == m_Cursor.address)
        return;
    m_LastCursorAddress = m_Cursor.address;
    wxCommandEvent event(DEBUGGER_CURSOR_CHANGED);
    event.SetInt(editor_only);
    m_pDBG->ProcessEvent(event);
}

void DebuggerDriver::NotifyDebuggeeContinued()
{
    m_pDBG->DebuggeeContinued();
    ResetCursor();
}

void DebuggerDriver::ResetCursor()
{
    m_LastCursorAddress.Clear();
    m_Cursor.address.Clear();
    m_Cursor.file.Clear();
    m_Cursor.function.Clear();
    m_Cursor.line = -1;
    m_Cursor.line_end = -1;
    m_Cursor.changed = false;
}

void DebuggerDriver::QueueCommand(DebuggerCmd* dcmd, QueuePriority prio)
{
    if (prio == Low)
        m_DCmds.Add(dcmd);
    else
        m_DCmds.Insert(dcmd, 0);
    RunQueue();
}

bool DebuggerDriver::IsQueueBusy() const
{
    if (!m_RunDCmds.IsEmpty() && !m_RunDCmds.Last().CanOverlap())
        return true;
    for (int i = 0; i < (int)m_DCmds.GetCount(); ++i)
        if (!m_DCmds[i].CanOverlap())
            return true;
    return false;
}

bool DebuggerDriver::RunQueue()
{
    bool runqdone = true;
    for (int i = 0; i < (int)m_RunDCmds.GetCount(); ++i)
    {
        if (!m_RunDCmds[i].IsDone())
        {
            runqdone = false;
            break;
        }
    }
    bool work = false;
    while (!m_DCmds.IsEmpty() && (runqdone || (!m_SyncComm && m_RunDCmds.Last().CanOverlap() && !m_DCmds[0].IsBarrier())))
    {
        m_RunDCmds.Add(m_DCmds.Detach(0));
        m_RunDCmds.Last().RunAction(GetCpuState());
        runqdone = runqdone && m_RunDCmds.Last().IsDone();
        work = true;
    }
    return work;
}

bool DebuggerDriver::ParseOutput(const wxString& output)
{
    Opt cmd(output);
    return ParseOutput(cmd);
}

bool DebuggerDriver::ParseOutput(const Opt& cmd)
{
    if (m_RunDCmds.IsEmpty())
        return false;
    unsigned int seq(cmd.get_cmdseq());
    if (!seq)
        return false;
    bool consumed = false;
    for (int i = 0; i < (int)m_RunDCmds.GetCount(); ++i)
        if (!m_RunDCmds[i].IsDone())
            consumed = m_RunDCmds[i].ParseAllOutput(cmd, seq) || consumed;
    PruneRunQueue();
    return consumed;
}

void DebuggerDriver::PruneRunQueue()
{
    // in-order retire, otherwise barriers will not work
    while (!m_RunDCmds.IsEmpty() && m_RunDCmds[0].IsDone())
    {
        m_RunDCmds.RemoveAt(0);
    }
}

void DebuggerDriver::ClearQueue()
{
    m_DCmds.Clear();
}

void DebuggerDriver::ClearRunQueue()
{
    m_RunDCmds.Clear();
}

void DebuggerDriver::MarkRunQueueDone()
{
    for (int i = 0; i < (int)m_RunDCmds.GetCount(); ++i)
        m_RunDCmds[i].Done();
}

unsigned int DebuggerDriver::CommandAddSeq(Opt& cmd)
{
    unsigned int seq(m_CmdSequence);
    cmd.set_cmdseq(seq, true);
    ++m_CmdSequence;
    if (!m_CmdSequence)
        m_CmdSequence = 1;
    return seq;
}

void DebuggerDriver::SendCommand(const Opt& cmd, bool barrier, bool overlapped, bool debugLog, bool logToNormalLog)
{
    QueueCommand(new DebuggerCmd_Simple(this, cmd, barrier, overlapped, debugLog, logToNormalLog));
}

void DebuggerDriver::DoSendCommand(const Opt& cmd, bool debugLog)
{
    m_pDBG->DoSendCommand(cmd, debugLog);
}

DebuggerDriver::StackFrameContainer const & DebuggerDriver::GetStackFrames() const
{
    return m_backtrace;
}

DebuggerDriver::StackFrameContainer& DebuggerDriver::GetStackFrames()
{
    return m_backtrace;
}

const DebuggerDriver::ThreadsContainer & DebuggerDriver::GetThreads() const
{
    return m_threads;
}

DebuggerDriver::ThreadsContainer & DebuggerDriver::GetThreads()
{
    return m_threads;
}

void DebuggerDriver::SetCurrentFrame(int number, bool user_selected)
{
    m_currentFrameNo = number;
    if (user_selected)
        m_userSelectedFrameNo = number;
}

void DebuggerDriver::ResetCurrentFrame()
{
    m_currentFrameNo = 0;
    m_userSelectedFrameNo = -1;

    if (Manager::Get()->GetDebuggerManager()->UpdateBacktrace())
        Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
}

void DebuggerDriver::SetCpuState(cpustate_t state)
{
    m_CpuState = state;
    for (int i = 0; i < (int)m_RunDCmds.GetCount(); ++i)
        m_RunDCmds[i].RunStateChange(GetCpuState());
}

void DebuggerDriver::SetCurrentKey(ProjectTargetOptions::key_t key)
{
    m_CurrentKey = key;
}

bool DebuggerDriver::CheckError(const Opt& cmd)
{
    return cmd.is_option("error");
}

bool DebuggerDriver::KillOnError(const Opt& cmd, bool enable_skip, bool enable_ignore)
{
    std::pair<wxString,bool> err(cmd.get_option_wxstring("error"));
    if (!err.second)
        return false;
    {
        wxString msg(_T("Command Error: "));
        msg += cmd.get_cmdwxstring();
        GetDebugger()->Log(msg);
    }
    int key;
    {
        wxArrayString btnlbls;
        if (enable_ignore)
            btnlbls.Add(_T("Ignore Error"));
        if (enable_skip)
            btnlbls.Add(_T("Skip Command"));
        btnlbls.Add(_T("Terminate Session"));
        static const AnnoyingDialog::dStyle styles[3] = { AnnoyingDialog::ONE_BUTTON, AnnoyingDialog::TWO_BUTTONS, AnnoyingDialog::THREE_BUTTONS };
        int dflt = (int)btnlbls.GetCount();
        btnlbls.Add(_T(""), 3 - dflt);
        AnnoyingDialog dlg(_("Debugger Command Error"),
                           _("The Debugger Command \"") + cmd.get_cmdwxname() +_("\" generated an error: ") + err.first +
                           _(".\n\nFull Command: \"") + cmd.get_cmdwxstring() + _("\"\n\nWhat do you want to do?"),
                           wxART_ERROR, styles[dflt - 1], (AnnoyingDialog::dReturnType)(AnnoyingDialog::rtONE + dflt - 1), btnlbls[0], btnlbls[1], btnlbls[2]);
        key = dlg.ShowModal();
        if (key >= 1 && key <= btnlbls.GetCount())
        {
            GetDebugger()->Log(_T("Command Error User Response: ") + btnlbls[key - 1]);
        }
    }
    if (enable_ignore)
    {
        if (key == 1)
            return false;
        --key;
    }
    if (enable_skip)
    {
        if (key == 1)
            return true;
        --key;
    }
    KillDebugger(true);
    return true;
}

bool DebuggerDriver::KillOnNotHalt(cpustate_t cs, const wxString& title, bool enable_skip, bool enable_ignore)
{
    if (cs == cpustate_halt)
        return false;
    {
        wxString msg(_T("Command Invalid CPU Status Error: "));
        msg += title + _T(" CPU Status ") + wxString(to_str(cs).c_str(), wxConvUTF8);
        GetDebugger()->Log(msg);
    }
    int key;
    {
        wxArrayString btnlbls;
        if (enable_ignore)
            btnlbls.Add(_T("Ignore Status"));
        if (enable_skip)
            btnlbls.Add(_T("Skip Command"));
        btnlbls.Add(_T("Terminate Session"));
        static const AnnoyingDialog::dStyle styles[3] = { AnnoyingDialog::ONE_BUTTON, AnnoyingDialog::TWO_BUTTONS, AnnoyingDialog::THREE_BUTTONS };
        int dflt = (int)btnlbls.GetCount();
        btnlbls.Add(wxT(""), 3 - dflt);
        AnnoyingDialog dlg(_("Debugger Status Error"),
                           _("The Debugger Command \"") + title +_("\" needs to be issued, but the debuggee is in incompatible state ") +
                           wxString(to_str(cs).c_str(), wxConvUTF8) + _("\n\nWhat do you want to do?"),
                           wxART_ERROR, styles[dflt - 1], (AnnoyingDialog::dReturnType)(AnnoyingDialog::rtONE + dflt - 1), btnlbls[0], btnlbls[1], btnlbls[2]);
        key = dlg.ShowModal();
        if (key >= 1 && key <= btnlbls.GetCount())
        {
            GetDebugger()->Log(_T("Command Error User Response: ") + btnlbls[key - 1]);
        }
    }
    if (enable_ignore)
    {
        if (key == 1)
            return false;
        --key;
    }
    if (enable_skip)
    {
        if (key == 1)
            return true;
        --key;
    }
    KillDebugger(false);
    return true;
}

bool DebuggerDriver::KillOnNotHaltRun(cpustate_t cs, const wxString& title, bool enable_skip, bool enable_ignore)
{
    if (cs == cpustate_run || cs == cpustate_halt)
        return false;
    return KillOnNotHalt(cs, title, enable_skip, enable_ignore);
}
