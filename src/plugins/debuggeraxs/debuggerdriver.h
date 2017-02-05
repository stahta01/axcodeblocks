/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DEBUGGERDRIVER_H
#define DEBUGGERDRIVER_H

#ifndef WX_PRECOMP
#   ifdef __WXMSW__
#       include <wx/msw/wrapwin.h>  // Needed to prevent GetCommandLine define bug.
#   endif
#endif

#include "debugger_defs.h"
#include "projtargetoptions.h"
#include <wx/regex.h>
#include <wx/dynarray.h>
#include <globals.h>

#define NOT_IMPLEMENTED()   \
    do {                    \
        DebugLog(wxString(cbC2U(__PRETTY_FUNCTION__)) + _T(": Not implemented in driver"));     \
        Log(wxString(cbC2U(__PRETTY_FUNCTION__)) + _T(": Not implemented in driver"));           \
    } while(0)

class DebuggerAXS;
class Compiler;
class ProjectBuildTarget;
class Opt;

class DebuggerCmd;

WX_DECLARE_OBJARRAY(DebuggerCmd, DebuggerCommands);

class DebuggerDriver
{
    public:
        /** The priority used when adding a command in the queue.
            Low appends the new command in the queue, i.e. last in the queue.
            High inserts the command at the top of the queue.
        */
        enum QueuePriority
        {
            Low = 0,
            High
        };
        typedef std::vector<cb::shared_ptr<cbStackFrame> > StackFrameContainer;
        typedef std::vector<cb::shared_ptr<cbThread> > ThreadsContainer;

        DebuggerDriver(DebuggerAXS* plugin);
        virtual ~DebuggerDriver();

        void Log(const wxString& msg);
        void DebugLog(const wxString& msg);

        DebuggerAXS* GetDebugger() { return m_pDBG; }

        ////////////////////////////////
        // BEFORE PROCESS STARTS - BEGIN
        ////////////////////////////////

        /** Add a directory in search list. */
        virtual void AddDirectory(const wxString& dir);

        /** Clear directories search list. */
        virtual void ClearDirectories();

        /** Set the working directory. */
        virtual void SetWorkingDirectory(const wxString& dir);

        /** Set the execution arguments. */
        virtual void SetArguments(const wxString& args);

        /** Get the command-line to launch the debugger. */
        virtual wxString GetCommandLine(const wxString& debugger, const wxString& debuggee) = 0;

        /** Get the command-line to launch the debugger. */
        virtual wxString GetCommandLine(const wxString& debugger, int pid) = 0;

        /** Sets the target */
        virtual void SetTarget(ProjectBuildTarget* target) = 0;

        /** Prepares the debugging process by setting up search dirs etc.
            @param target The build target to debug.
            @param isConsole If true, the debuggee is a console executable.
            @param isSyncComm If true, the debugger communication is synchronous.
        */
        virtual void Prepare(bool isConsole, bool liveUpdate, bool isSyncComm);

        /** Prepares the debugging process by setting up search dirs etc.
            @param target The build target to debug.
            @param isConsole If true, the debuggee is a console executable.
        */
        virtual void Prepare(bool isConsole, bool liveUpdate) = 0;

        /** Begin the debugging process by launching a program. */
        virtual void Start(bool breakOnEntry) = 0;

        ////////////////////////////////
        // BEFORE PROCESS STARTS - END
        ////////////////////////////////

        /** Stop debugging. */
        virtual void Stop() = 0;

        virtual void Pause() = 0;

        virtual void Continue() = 0;
        virtual void Step() = 0;
        virtual void StepInstruction() = 0;
        virtual void StepIntoInstruction() = 0;
        virtual void StepIn() = 0;
        virtual void StepOut() = 0;
        virtual void SetNextStatement(const wxString& filename, int line) = 0;
        virtual void Backtrace() = 0;
        virtual void Disassemble() = 0;
        virtual void CPURegisters() = 0;
        virtual void SwitchToFrame(size_t number) = 0;
        virtual void SetVarValue(const wxString& var, const wxString& value) = 0;
        virtual void SetRegValue(const wxString& addrspace, unsigned int addr, const wxString& value) = 0;
        virtual void MemoryDump() = 0;
        virtual void RunningThreads() = 0;
        virtual void Poll() = 0;

        /// AXSEM reset functions:
        virtual void HWR() = 0;
        virtual void SWR() = 0;
        virtual void PinEmulation() = 0;
        virtual void DebugLink() = 0;

        virtual void InfoFrame() = 0;
        virtual void InfoDLL() = 0;
        virtual void InfoFiles() = 0;
        virtual void InfoFPU() = 0;
        virtual void InfoSignals() = 0;
        virtual void InfoCPUTrace() = 0;
        virtual void InfoProfiler() = 0;

        /** Add a breakpoint.
            @param bp The breakpoint to add.
            @param editor The editor this breakpoint is set (might be NULL).
        */
        virtual void AddBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp) = 0;

        /** Remove a breakpoint.
            @param bp The breakpoint to remove. If NULL, all reakpoints are removed.
        */
        virtual void RemoveBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp) = 0;

        /** Evaluate a symbol.
            @param symbol The symbol to evaluate.
            @param tipRect The rect to use for the tip window.
        */
        virtual void EvaluateSymbol(const wxString& symbol, const wxRect& tipRect) = 0;

        /** Update watches.
            @param doLocals Display values of local variables.
            @param doArgs Display values of function arguments.
            @param tree The watches tree control.
        */
        virtual void UpdateWatches(WatchesContainer &watches) = 0;
        virtual void UpdateWatch(cb::shared_ptr<GDBWatch> const &watch) = 0;

        virtual void ExpandWatch(cb::shared_ptr<GDBWatch> watch) = 0;
        virtual void CollapseWatch(cb::shared_ptr<GDBWatch> watch) = 0;

        virtual void ExpandRegister(cb::shared_ptr<AXSRegister> reg) = 0;
        virtual void CollapseRegister(cb::shared_ptr<AXSRegister> reg) = 0;
        virtual void UpdateRegister(cb::shared_ptr<AXSRegister> reg = cb::shared_ptr<AXSRegister>()) = 0;
        virtual void SetChip(const wxString& chip) = 0;
        virtual void MarkAllRegistersAsUnchanged() = 0;


        /** Attach to process */
        virtual void Attach() = 0;
        /** Detach from running process. */
        virtual void Detach() = 0;

        /** Is debugging started */
        virtual bool IsDebuggingStarted() const = 0;
        /** Is the program stopped? */
        virtual bool IsProgramStopped() const { return m_ProgramIsStopped; }
        /** Is the driver processing some commands? */
        virtual bool IsQueueBusy() const;
        /*** Is the target halted? */
        virtual bool IsHalted() const = 0;
        /** Set child PID (debuggee's). Usually set by debugger commands. */
        virtual void SetChildPID(long pid) { m_ChildPID = pid; }
        /** Get the child's (debuggee's) PID. */
        virtual long GetChildPID() const { return m_ChildPID; }
        /** Request to switch to another thread. */
        virtual void SwitchThread(size_t threadIndex) = 0;

#ifdef __WXMSW__
        /** Ask the driver if the debugger should be interrupted with DebugBreakProcess or Ctrl+C event */
        virtual bool UseDebugBreakProcess() = 0;
#endif
        wxString GetDebuggersWorkingDirectory() const;

        /** Show a file/line without changing the cursor */
        void ShowFile(const wxString& file, int line);

        void QueueCommand(DebuggerCmd* dcmd, QueuePriority prio = Low); ///< add a command in the queue. The DebuggerCmd will be deleted automatically when finished.
        bool RunQueue(); ///< runs the next command(s) in the queue, if possible
        bool ParseOutput(const wxString& output); ///< parse command output string
        virtual bool ParseOutput(const Opt& output); ///< parse command output
        void PruneRunQueue(); ///< prunes commands from run queue that are marked done
        void ClearQueue(); ///< clears the queue
        void ClearRunQueue(); ///< clears the run queue
        void MarkRunQueueDone(); ///< marks the run queue as done
        unsigned int CommandAddSeq(Opt& cmd);
        void DoSendCommand(const Opt& cmd, bool debugLog);
        void SendCommand(const Opt& cmd, bool barrier, bool overlapped, bool debugLog, bool logToNormalLog = false);

        const StackFrameContainer & GetStackFrames() const; ///< returns the container with the current backtrace
        StackFrameContainer & GetStackFrames(); ///< returns the container with the current backtrace

        const ThreadsContainer & GetThreads() const; ///< returns the thread container with the current list of threads
        ThreadsContainer & GetThreads(); ///< returns the thread container with the current list of threads

        /** Get debugger's cursor. */
        const Cursor& GetCursor() const { return m_Cursor; }
        /** Set debugger's cursor. */
        void SetCursor(const Cursor& cursor) { m_Cursor = cursor; }

        void ResetCurrentFrame();
        int GetCurrentFrame() const { return m_currentFrameNo; }
        int GetUserSelectedFrame() const { return m_userSelectedFrameNo; }
        void SetCurrentFrame(int number, bool user_selected);

        void NotifyDebuggeeContinued();
        /** Called by implementations to notify cursor changes. */
        void NotifyCursorChanged(bool editor_only = false);

        void SetCpuState(cpustate_t state);
        cpustate_t GetCpuState(void) const { return m_CpuState; }
        void SetCurrentKey(ProjectTargetOptions::key_t key = ProjectTargetOptions::defaultKey);
        ProjectTargetOptions::key_t GetCurrentKey(void) const { return m_CurrentKey; }

        virtual void KillDebugger(bool terminate = true) = 0;
        bool CheckError(const Opt& cmd);
        bool KillOnError(const Opt& cmd, bool enable_skip = true, bool enable_ignore = false);
        bool KillOnNotHalt(cpustate_t cs, const wxString& title, bool enable_skip = true, bool enable_ignore = false);
        bool KillOnNotHaltRun(cpustate_t cs, const wxString& title, bool enable_skip = true, bool enable_ignore = false);

    protected:
        /** Called by implementations to reset the cursor. */
        void ResetCursor();
    protected:
        // the debugger plugin
        DebuggerAXS* m_pDBG;

        // convenience properties for starting up
        wxArrayString m_Dirs;
        wxString m_WorkingDir;
        wxString m_Args;

        // cursor related
        bool m_ProgramIsStopped;
        wxString m_LastCursorAddress;
        Cursor m_Cursor;

        long m_ChildPID;

        // commands
        DebuggerCommands m_DCmds;
        DebuggerCommands m_RunDCmds;
        unsigned int m_CmdSequence;

        StackFrameContainer m_backtrace;
        ThreadsContainer m_threads;
        int m_currentFrameNo;
        int m_userSelectedFrameNo;

        cpustate_t m_CpuState;
        ProjectTargetOptions::key_t m_CurrentKey;

        bool m_SyncComm;
};

#endif // DEBUGGERDRIVER_H
