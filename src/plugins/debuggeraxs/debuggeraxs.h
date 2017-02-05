/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DEBUGGERAXS_H
#define DEBUGGERAXS_H

#include <map>
#include <tr1/memory>

#include <settings.h> // much of the SDK is here
#include <sdk_events.h>
#include <cbplugin.h>
#include <loggers.h>
#include <pipedprocess.h>
#include <wx/regex.h>

#include "projtargetoptions.h"
#include "debuggerstate.h"
#include "debugger_defs.h"

extern const wxString g_EscapeChar;

class cbProject;
class TiXmlElement;
class DebuggerDriver;
class DebuggerCmd;
class Compiler;
struct TestIfBelogToProject;
class DebuggerConfiguration;

class DebuggerAXS : public cbDebuggerPlugin
{
        DebuggerState m_State;
    public:
        DebuggerAXS();
        ~DebuggerAXS();

        cbConfigurationPanel* GetProjectConfigurationPanel(wxWindow* parent, cbProject* project);
        void OnAttachReal(); // fires when the plugin is attached to the application
        void OnReleaseReal(bool appShutDown); // fires when the plugin is released from the application

        bool SupportsFeature(cbDebuggerFeature::Flags flag);

        cbDebuggerConfiguration* LoadConfig(const ConfigManagerWrapper &config);

        DebuggerConfiguration& GetActiveConfigEx();

        void RunCommand(int cmd);

        cb::shared_ptr<cbBreakpoint> AddBreakpoint(const wxString& filename, int line);
        cb::shared_ptr<cbBreakpoint> AddDataBreakpoint(const wxString& dataExpression);
        int GetBreakpointsCount() const;
        cb::shared_ptr<cbBreakpoint> GetBreakpoint(int index);
        cb::shared_ptr<const cbBreakpoint> GetBreakpoint(int index) const;
        void UpdateBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint);
        void DeleteBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint);
        void DeleteAllBreakpoints();
        void ShiftBreakpoint(int index, int lines_to_shift);
        void EnableBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint, bool enable);

        // stack frame calls;
        int GetStackFrameCount() const;
        cb::shared_ptr<const cbStackFrame> GetStackFrame(int index) const;
        void SwitchToFrame(int number);
        int GetActiveStackFrame() const;

        // threads
        int GetThreadsCount() const;
        cb::shared_ptr<const cbThread> GetThread(int index) const;
        bool SwitchToThread(int thread_number);

        bool Debug(bool breakOnEntry);
        void Continue();
        void Next();
        void NextInstruction();
        void StepIntoInstruction();
        void Step();
        void StepOut();
        bool RunToCursor(const wxString& filename, int line, const wxString& line_text);
        void SetNextStatement(const wxString& filename, int line);
        void Break();
        void Stop();
        bool Validate(const wxString& line, const char cb);
        bool IsRunning() const { return m_pProcess; }
        bool IsStopped() const;
        bool IsBusy() const;
        bool IsTemporaryBreak() const {return m_TemporaryBreak;}
        int GetExitCode() const { return m_LastExitCode; }

        bool IsHalted();
        int GetEnabledTools();
        void SetEnabledTools(int t) { m_EnabledTools = t; }

        void HWReset();
        void SWReset();

        cb::shared_ptr<cbWatch> AddWatch(const wxString& symbol);
        void DeleteWatch(cb::shared_ptr<cbWatch> watch);
        bool HasWatch(cb::shared_ptr<cbWatch> watch);
        void ShowWatchProperties(cb::shared_ptr<cbWatch> watch);
        bool SetWatchValue(cb::shared_ptr<cbWatch> watch, const wxString &value);
        void ExpandWatch(cb::shared_ptr<cbWatch> watch);
        void CollapseWatch(cb::shared_ptr<cbWatch> watch);
        void UpdateWatch(cb::shared_ptr<cbWatch> watch);

        void AddWatchNoUpdate(const cb::shared_ptr<GDBWatch> &watch);

        void OnWatchesContextMenu(wxMenu &menu, const cbWatch &watch, wxObject *property, int &disabledMenus);

        void SetWatchesDisabled(bool flag);

        void ExpandRegister(cb::shared_ptr<cbRegister> reg);
        void CollapseRegister(cb::shared_ptr<cbRegister> reg);
        bool SetRegisterValue(cb::shared_ptr<cbRegister> reg, const wxString &value);
        void UpdateRegister(cb::shared_ptr<cbRegister> reg = cb::shared_ptr<cbRegister>());
        void SetChip(const wxString& chip);

        void GetCurrentPosition(wxString &filename, int &line);
        void RequestUpdate(DebugWindows window);

        void AttachToProcess();
        void DetachFromProcess();
        bool IsAttachedToProcess() const;

        void SendCommand(const wxString& cmd, bool debugLog);
        void DoSendCommand(const Opt& cmd, bool debugLog);

        DebuggerState& GetState() { return m_State; }

        void OnConfigurationChange(bool isActive);

        wxArrayString& GetSearchDirs(cbProject* prj);
        const wxArrayString& GetSearchDirs(cbProject* prj) const;
        void RemoveSearchDirs(cbProject* prj);
        ProjectTargetOptionsMap& GetProjectTargetOptionsMap(cbProject* project = 0);
        void RemoveProjectTargetOptionsMap(cbProject* project = 0);
        void OnProjectLoadingHook(cbProject* project, TiXmlElement* elem, bool loading);

        void OnValueTooltip(const wxString &token, const wxRect &evalRect);
        bool ShowValueTooltip(int style);

        static void ConvertToGDBFriendly(wxString& str);
        static void ConvertToGDBFile(wxString& str);
        static void ConvertToGDBDirectory(wxString& str, wxString base = _T(""), bool relative = true);
        static void StripQuotes(wxString& str);

        void DebuggeeContinued();

    protected:
        cbProject* GetProject() { return m_pProject; }
        void ResetProject() { m_pProcess = NULL; }
        void ConvertDirectory(wxString& str, wxString base, bool relative);
        void CleanupWhenProjectClosed(cbProject *project);
        bool CompilerFinished(bool compilerFailed, StartType startType);

        unsigned int LockDriver() const;
        void UnlockDriver() const;
        void UnlockDriver();

    protected:
        void AddSourceDir(const wxString& dir);
    private:
        void DoWatches();
        void MarkAllWatchesAsUnchanged();
        int LaunchProcess(const wxString& cmd, const wxString& cwd);
        int DoDebug(bool breakOnEntry);
        void DoBreak(bool temporary);

        void DoRegisters();
        void MarkAllRegistersAsUnchanged();

        void OnAddSymbolFile(wxCommandEvent& event);
        void DeleteAllProjectBreakpoints(cbProject* project);
        void OnBuildTargetSelected(CodeBlocksEvent& event);
        void OnGDBOutput(wxCommandEvent& event);
        void OnGDBError(wxCommandEvent& event);
        void OnGDBTerminated(wxCommandEvent& event);
        void StopDebugger();
        void OnIdle(wxIdleEvent& event);
        void OnTimer(wxTimerEvent& event);
        void OnShowFile(wxCommandEvent& event);
        void OnCursorChanged(wxCommandEvent& event);
        void OnAppStartShutdown(CodeBlocksEvent& event);

        void SetupToolsMenu(wxMenu &menu);
        void KillConsole();

        void OnInfoFrame(wxCommandEvent& event);
        void OnInfoDLL(wxCommandEvent& event);
        void OnInfoFiles(wxCommandEvent& event);
        void OnInfoFPU(wxCommandEvent& event);
        void OnInfoSignals(wxCommandEvent& event);
        void OnInfoCPUTrace(wxCommandEvent& event);
        void OnInfoProfiler(wxCommandEvent& event);

        void OnMenuWatchDereference(wxCommandEvent& event);
    private:
        PipedProcess* m_pProcess;
        bool m_LastExitCode;
        int m_Pid;
        bool m_IsAttached; // for "attach to process"
        wxRect m_EvalRect;
        wxTimer m_TimerPollDebugger;

        // extra dialogs
        cbProject* m_pProject; // keep the currently debugged project handy
        wxString m_ActiveBuildTarget;

        // per-project debugger search-dirs
        typedef std::map<cbProject*, wxArrayString> SearchDirsMap;
        SearchDirsMap m_SearchDirs;

        typedef std::map<cbProject*, ProjectTargetOptionsMap> ProjectProjectTargetOptionsMap;
        ProjectProjectTargetOptionsMap m_ProjectTargetOptions;

        int m_HookId; // project loader hook ID

        // Linux console support
        bool m_bIsConsole;
        int m_nConsolePid;

        bool m_Canceled; // flag to avoid re-entering DoDebug when we shouldn't

        bool m_TemporaryBreak;

        WatchesContainer m_watches;
        wxString m_watchToDereferenceSymbol;
        wxObject *m_watchToDereferenceProperty;

        int m_EnabledTools;

        // exclusion mechanism to prevent the driver from going away under us
        std::list<wxString> m_DebuggerOutputQueue;
        mutable unsigned int m_DriverLockDepth;
        bool m_DriverTerminate;

        friend struct TestIfBelongToProject;

        DECLARE_EVENT_TABLE()
};

#endif // DEBUGGERAXS_H

