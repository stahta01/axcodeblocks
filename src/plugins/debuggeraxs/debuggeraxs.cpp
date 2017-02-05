/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include <algorithm> // std::remove_if
#include "axspipedprocess.h"

#ifndef CB_PRECOMP
    #include <wx/txtstrm.h>
    #include <wx/regex.h>
    #include <wx/msgdlg.h>
    #include <wx/frame.h> // GetMenuBar

    #include "cbproject.h"
    #include "manager.h"
    #include "configmanager.h"
    #include "projectmanager.h"
    #include "pluginmanager.h"
    #include "editormanager.h"
    #include "macrosmanager.h"
    #include "cbeditor.h"
    #include "projectbuildtarget.h"
    #include "sdk_events.h"
    #include "compilerfactory.h"
    #include "xtra_res.h"

    #include "scrollingdialog.h"
    #include "globals.h"
#endif

#ifdef __WXMSW__
    #include <winbase.h>
#else
    int GetShortPathName(const void*, void*, int){/* bogus */ return 0; };
#endif

#include <wx/tokenzr.h>
#include "editarraystringdlg.h"
#include "projectloader_hooks.h"
#include "annoyingdialog.h"
#include "cbstyledtextctrl.h"

#include <cbdebugger_interfaces.h>
#include "editbreakpointdlg.h"

#include "databreakpointdlg.h"
#include "debuggerdriver.h"
#include "debuggeraxs.h"
#include "debuggeroptionsdlg.h"
#include "debuggeroptionsprjdlg.h"
#include "editwatchdlg.h"
#include "editaxsemkeydlg.h"
#include "axs_driver.h"
#include "machine.h"


#define implement_debugger_toolbar

// function pointer to DebugBreakProcess under windows (XP+)
#if (_WIN32_WINNT >= 0x0501)
#include "tlhelp32.h"
typedef BOOL WINAPI   (*DebugBreakProcessApiCall)       (HANDLE);
typedef HANDLE WINAPI (*CreateToolhelp32SnapshotApiCall)(DWORD  dwFlags,   DWORD             th32ProcessID);
typedef BOOL WINAPI   (*Process32FirstApiCall)          (HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
typedef BOOL WINAPI   (*Process32NextApiCall)           (HANDLE hSnapshot, LPPROCESSENTRY32W lppe);

DebugBreakProcessApiCall        DebugBreakProcessFunc = 0;
CreateToolhelp32SnapshotApiCall CreateToolhelp32SnapshotFunc = 0;
Process32FirstApiCall           Process32FirstFunc = 0;
Process32NextApiCall            Process32NextFunc = 0;

HINSTANCE kernelLib = 0;

#endif

#ifdef __WXMSW__
// disable the CTRL_C event
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    return TRUE;
}
#endif

// valid debugger command constants
enum DebugCommandConst
{
    CMD_CONTINUE,
    CMD_STEP,
    CMD_STEPIN,
    CMD_STEPOUT,
    CMD_STEP_INSTR,
    CMD_STEP_INTO_INSTR,
    CMD_STOP,
    CMD_PAUSE,
    CMD_BACKTRACE,
    CMD_DISASSEMBLE,
    CMD_REGISTERS,
    CMD_MEMORYDUMP,
    CMD_RUNNINGTHREADS,
    CMD_HWR,
    CMD_SWR,
    CMD_PINEMULATION,
    CMD_DEBUGLINK
};

const wxString g_EscapeChar = wxChar(26);

int idMenuInfoFrame = XRCID("idDebuggerCurrentFrame");
int idMenuInfoDLL = XRCID("idDebuggerLoadedDLLs");
int idMenuInfoFiles = XRCID("idDebuggerFiles");
int idMenuInfoFPU = XRCID("idDebuggerFPU");
int idMenuInfoSignals = XRCID("idDebuggerSignals");
int idMenuInfoCPUTrace = XRCID("idCPUTrace");
int idMenuInfoProfiler = XRCID("idProfiler");

int idGDBProcess = wxNewId();
int idTimerPollDebugger = wxNewId();
int idMenuSettings = wxNewId();

int idMenuWatchDereference = wxNewId();

// this auto-registers the plugin
namespace
{
    PluginRegistrant<DebuggerAXS> reg(_T("AXSEM Debugger"));
}

BEGIN_EVENT_TABLE(DebuggerAXS, cbDebuggerPlugin)
    EVT_MENU(idMenuInfoFrame, DebuggerAXS::OnInfoFrame)
    EVT_MENU(idMenuInfoDLL, DebuggerAXS::OnInfoDLL)
    EVT_MENU(idMenuInfoFiles, DebuggerAXS::OnInfoFiles)
    EVT_MENU(idMenuInfoFPU, DebuggerAXS::OnInfoFPU)
    EVT_MENU(idMenuInfoSignals, DebuggerAXS::OnInfoSignals)
    EVT_MENU(idMenuInfoCPUTrace, DebuggerAXS::OnInfoCPUTrace)
    EVT_MENU(idMenuInfoProfiler, DebuggerAXS::OnInfoProfiler)

    EVT_MENU(idMenuWatchDereference, DebuggerAXS::OnMenuWatchDereference)

    EVT_PIPEDPROCESS_STDOUT(idGDBProcess, DebuggerAXS::OnGDBOutput)
    EVT_PIPEDPROCESS_STDERR(idGDBProcess, DebuggerAXS::OnGDBError)
    EVT_PIPEDPROCESS_TERMINATED(idGDBProcess, DebuggerAXS::OnGDBTerminated)

    EVT_IDLE(DebuggerAXS::OnIdle)
    EVT_TIMER(idTimerPollDebugger, DebuggerAXS::OnTimer)

    EVT_COMMAND(-1, DEBUGGER_CURSOR_CHANGED, DebuggerAXS::OnCursorChanged)
    EVT_COMMAND(-1, DEBUGGER_SHOW_FILE_LINE, DebuggerAXS::OnShowFile)

    //EVT_APP_START_SHUTDOWN(DebuggerAXS::OnAppStartShutdown)
END_EVENT_TABLE()

DebuggerAXS::DebuggerAXS() :
    cbDebuggerPlugin(wxT("AXSEM debugger"), wxT("axs_debugger")),
    m_State(this),
    m_pProcess(0L),
    m_LastExitCode(0),
    m_Pid(0),
    m_IsAttached(false),
    m_pProject(0),
    m_HookId(0),
    m_bIsConsole(false),
    m_nConsolePid(0),
    m_Canceled(false),
    m_TemporaryBreak(false),
    m_EnabledTools(DebuggerToolbarTools::Debug |
                   DebuggerToolbarTools::RunToCursor |
                   DebuggerToolbarTools::Step),
    m_DriverLockDepth(0),
    m_DriverTerminate(false)
{
    if(!Manager::LoadResource(_T("debuggeraxs.zip")))
    {
        NotifyMissingFile(_T("debuggeraxs.zip"));
    }

    // get a function pointer to DebugBreakProcess under windows (XP+)
    #if (_WIN32_WINNT >= 0x0501)
    kernelLib = LoadLibrary(TEXT("kernel32.dll"));
    if (kernelLib)
    {
        DebugBreakProcessFunc = (DebugBreakProcessApiCall)GetProcAddress(kernelLib, "DebugBreakProcess");
        //Windows XP
        CreateToolhelp32SnapshotFunc = (CreateToolhelp32SnapshotApiCall)GetProcAddress(kernelLib, "CreateToolhelp32Snapshot");
        Process32FirstFunc = (Process32FirstApiCall)GetProcAddress(kernelLib, "Process32First");
        Process32NextFunc = (Process32NextApiCall)GetProcAddress(kernelLib, "Process32Next");
    }
    #endif
}

DebuggerAXS::~DebuggerAXS()
{
    #if (_WIN32_WINNT >= 0x0501)
    if (kernelLib)
        FreeLibrary(kernelLib);
    #endif
}

void DebuggerAXS::OnAttachReal()
{
    m_TimerPollDebugger.SetOwner(this, idTimerPollDebugger);

    // hook to project loading procedure
    ProjectLoaderHooks::HookFunctorBase* myhook = new ProjectLoaderHooks::HookFunctor<DebuggerAXS>(this, &DebuggerAXS::OnProjectLoadingHook);
    m_HookId = ProjectLoaderHooks::RegisterHook(myhook);

    // register event sink
    Manager::Get()->RegisterEventSink(cbEVT_BUILDTARGET_SELECTED, new cbEventFunctor<DebuggerAXS, CodeBlocksEvent>(this, &DebuggerAXS::OnBuildTargetSelected));
    Manager::Get()->RegisterEventSink(cbEVT_APP_START_SHUTDOWN, new cbEventFunctor<DebuggerAXS, CodeBlocksEvent>(this, &DebuggerAXS::OnAppStartShutdown));
}

void DebuggerAXS::OnReleaseReal(bool /*appShutDown*/)
{
    ProjectLoaderHooks::UnregisterHook(m_HookId, true);

    //Close debug session when appShutDown
    if (LockDriver())
    {
        Stop();
        UnlockDriver();
        wxYieldIfNeeded();
    }

    m_State.CleanUp();
    KillConsole();
}

bool DebuggerAXS::SupportsFeature(cbDebuggerFeature::Flags flag)
{
    switch (flag)
    {
    case cbDebuggerFeature::Breakpoints:
    case cbDebuggerFeature::CPURegisters:
    case cbDebuggerFeature::Disassembly:
    case cbDebuggerFeature::Watches:
    case cbDebuggerFeature::ValueTooltips:
    case cbDebuggerFeature::ExamineMemory:
    case cbDebuggerFeature::HardwareReset:
    case cbDebuggerFeature::SoftwareReset:
    case cbDebuggerFeature::DebugLink:
    case cbDebuggerFeature::PinEmulation:
    case cbDebuggerFeature::ScopedWatch:
    case cbDebuggerFeature::Callstack:
    case cbDebuggerFeature::RunToCursor:
    case cbDebuggerFeature::CanAttach:
        return true;
    case cbDebuggerFeature::Threads:
    case cbDebuggerFeature::SetNextStatement:
    case cbDebuggerFeature::StepIntoInstr:
    default:
        return false;
    }
}

cbDebuggerConfiguration* DebuggerAXS::LoadConfig(const ConfigManagerWrapper &config)
{
    return new DebuggerConfiguration(config);
}

DebuggerConfiguration& DebuggerAXS::GetActiveConfigEx()
{
    return static_cast<DebuggerConfiguration&>(GetActiveConfig());
}

cbConfigurationPanel* DebuggerAXS::GetProjectConfigurationPanel(wxWindow* parent, cbProject* project)
{
    DebuggerOptionsProjectDlg* dlg = new DebuggerOptionsProjectDlg(parent, this, project);
    return dlg;
}

void DebuggerAXS::OnConfigurationChange(bool isActive)
{
}

wxArrayString& DebuggerAXS::GetSearchDirs(cbProject* prj)
{
    SearchDirsMap::iterator it = m_SearchDirs.find(prj);
    if (it == m_SearchDirs.end())
    {
        // create an empty set for this project
        it = m_SearchDirs.insert(m_SearchDirs.begin(), std::make_pair(prj, wxArrayString()));
    }

    return it->second;
}

const wxArrayString& DebuggerAXS::GetSearchDirs(cbProject* prj) const
{
    static const wxArrayString EmptyArrayString;
    SearchDirsMap::const_iterator it = m_SearchDirs.find(prj);
    if (it == m_SearchDirs.end())
        return EmptyArrayString;
    return it->second;
}

void DebuggerAXS::RemoveSearchDirs(cbProject* prj)
{
    SearchDirsMap::iterator it = m_SearchDirs.find(prj);
    if (it != m_SearchDirs.end())
        m_SearchDirs.erase(it);
}

ProjectTargetOptionsMap& DebuggerAXS::GetProjectTargetOptionsMap(cbProject* project)
{
    if (!project)
        project = m_pProject;
    ProjectProjectTargetOptionsMap::iterator it = m_ProjectTargetOptions.find(project);
    if (it == m_ProjectTargetOptions.end())
    {
        // create an empty set for this project
        it = m_ProjectTargetOptions.insert(m_ProjectTargetOptions.begin(), std::make_pair(project, ProjectTargetOptionsMap()));
    }
    return it->second;
}

void DebuggerAXS::RemoveProjectTargetOptionsMap(cbProject* project)
{
    if (!project)
        project = m_pProject;
    ProjectProjectTargetOptionsMap::iterator it = m_ProjectTargetOptions.find(project);
    if (it != m_ProjectTargetOptions.end())
        m_ProjectTargetOptions.erase(it);
}

void DebuggerAXS::OnProjectLoadingHook(cbProject* project, TiXmlElement* elem, bool loading)
{
    bool init(m_SearchDirs.find(project) == m_SearchDirs.end());
    wxArrayString& pdirs = GetSearchDirs(project);
    ProjectTargetOptionsMap& ptoprj = GetProjectTargetOptionsMap(project);

    {
        if (loading)
            ptoprj.clear();

        // Hook called when loading project file.
        TiXmlElement* conf = elem->FirstChildElement("debuggeraxs");
        if (!loading && conf)
            conf = conf->FirstChildElement("scriptadd");
        if (conf)
        {
            if (loading)
                init = false;
            TiXmlElement* pathsElem = conf->FirstChildElement("search_path");
            while (pathsElem)
            {
                if (pathsElem->Attribute("add"))
                {
                    wxString dir = cbC2U(pathsElem->Attribute("add"));
                    if (pdirs.Index(dir) == wxNOT_FOUND)
                        pdirs.Add(dir);
                    init = false;
                }

                pathsElem = pathsElem->NextSiblingElement("search_path");
            }
            TiXmlElement* ptoElem = conf->FirstChildElement("advanced");
            while (ptoElem) {
                wxString targetName = cbC2U(ptoElem->Attribute("target"));
                ProjectBuildTarget* bt = project->GetBuildTarget(targetName);

                ProjectTargetOptions pto;
                TiXmlElement* ptoOpt = ptoElem->FirstChildElement("options");
                if (ptoOpt) {
                    if (ptoOpt->Attribute("flash_erase"))
                        pto.flashErase = (ProjectTargetOptions::FlashEraseType)atol(ptoOpt->Attribute("flash_erase"));
                    pto.fillBreakpoints = ptoOpt->Attribute("fill_breakpoints") && cbC2U(ptoOpt->Attribute("fill_breakpoints")) != _T("0");
                    if (ptoOpt->Attribute("key")) {
                        std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(cbC2U(ptoOpt->Attribute("key"))));
                        if (k.second)
                            pto.key = k.first;
                    }
                }
                TiXmlElement* addKeyElem = ptoElem->FirstChildElement("additional_key");
                while (addKeyElem) {
                    if (addKeyElem->Attribute("key")) {
                        std::pair<uint64_t,bool> k(EditAxsemKeyDlg::str_to_key(cbC2U(addKeyElem->Attribute("key"))));
                        if (k.second)
                            pto.additionalKeys.insert(k.first);
                    }
                    addKeyElem = addKeyElem->NextSiblingElement("additional_key");
                }
                ptoprj.insert(ptoprj.end(), std::make_pair(bt, pto));
                ptoElem = ptoElem->NextSiblingElement("advanced");
            }
            if (!loading)
                conf->Clear();
        }
        if (init)
        {
            elem->InsertEndChild(TiXmlElement("debuggeraxs"));
            // defaults if empty
            Compiler* actualCompiler = CompilerFactory::GetCompiler(project->GetCompilerID());
            if (!actualCompiler)
                actualCompiler = CompilerFactory::GetDefaultCompiler();
            if (actualCompiler)
            {
                const wxArrayString& incdirs(actualCompiler->GetIncludeDirs());
                if (incdirs.GetCount() > 0)
                {
                    for (size_t i = 0; i < incdirs.GetCount(); ++i)
                    {
                        wxFileName fn(incdirs[i], wxEmptyString);
                        if (wxFileName::DirExists(fn.GetPath()) && pdirs.Index(fn.GetPath()) == wxNOT_FOUND)
                            pdirs.Add(fn.GetPath());
                        fn.RemoveLastDir();
                        {
                            wxFileName fn2(fn);
                            fn2.AppendDir(wxT("source"));
                            if (wxFileName::DirExists(fn2.GetPath()) && pdirs.Index(fn2.GetPath()) == wxNOT_FOUND)
                                pdirs.Add(fn2.GetPath());
                        }
                        {
                            wxFileName fn2(fn);
                            fn2.AppendDir(wxT("builtsource"));
                            if (wxFileName::DirExists(fn2.GetPath()) && pdirs.Index(fn2.GetPath()) == wxNOT_FOUND)
                                pdirs.Add(fn2.GetPath());
                        }
                    }
                }
            }
        }
    }
    if (!loading)
    {
        // Hook called when saving project file.

        // since rev4332, the project keeps a copy of the <Extensions> element
        // and re-uses it when saving the project (so to avoid losing entries in it
        // if plugins that use that element are not loaded atm).
        // so, instead of blindly inserting the element, we must first check it's
        // not already there (and if it is, clear its contents)
        TiXmlElement* node = elem->FirstChildElement("debuggeraxs");
        if (!node)
            node = elem->InsertEndChild(TiXmlElement("debuggeraxs"))->ToElement();
        node->Clear();

        if (pdirs.GetCount() > 0)
        {
            for (size_t i = 0; i < pdirs.GetCount(); ++i)
            {
                TiXmlElement* path = node->InsertEndChild(TiXmlElement("search_path"))->ToElement();
                path->SetAttribute("add", cbU2C(pdirs[i]));
            }
        }

        if (ptoprj.size()) {
            for (ProjectTargetOptionsMap::iterator it = ptoprj.begin(); it != ptoprj.end(); ++it) {
//                // valid targets only
//                if (!it->first)
//                    continue;

                ProjectTargetOptions& pto = it->second;

                // if no different than defaults, skip it
                if (pto.flashErase == ProjectTargetOptions::BulkErase &&
                    pto.key == ProjectTargetOptions::defaultKey &&
                    pto.additionalKeys.empty() &&
                    !pto.fillBreakpoints)
                    continue;

                TiXmlElement* ptonode = node->InsertEndChild(TiXmlElement("advanced"))->ToElement();
                if (it->first)
                    ptonode->SetAttribute("target", cbU2C(it->first->GetTitle()));

                TiXmlElement* tgtnode = ptonode->InsertEndChild(TiXmlElement("options"))->ToElement();
                if (pto.flashErase != ProjectTargetOptions::BulkErase)
                    tgtnode->SetAttribute("flash_erase", (int)pto.flashErase);
                if (pto.fillBreakpoints)
                    tgtnode->SetAttribute("fill_breakpoints", "1");
                if (pto.key != ProjectTargetOptions::defaultKey)
                    tgtnode->SetAttribute("key", cbU2C(EditAxsemKeyDlg::key_to_str(pto.key)));
                for (ProjectTargetOptions::additionalKeys_t::const_iterator ki(pto.additionalKeys.begin()), ke(pto.additionalKeys.end()); ki != ke; ++ki) {
                    TiXmlElement* addknode = ptonode->InsertEndChild(TiXmlElement("additional_key"))->ToElement();
                    addknode->SetAttribute("key", cbU2C(EditAxsemKeyDlg::key_to_str(*ki)));
                }
            }
        }
    }
}

void DebuggerAXS::DoWatches()
{
    if (!m_pProcess)
        return;
    if (!LockDriver())
        return;
    m_State.GetDriver()->UpdateWatches(m_watches);
    UnlockDriver();
}

void DebuggerAXS::DoRegisters()
{
    if (!m_pProcess)
        return;
    if (!LockDriver())
        return;
    m_State.GetDriver()->UpdateRegister();
    UnlockDriver();
}

int DebuggerAXS::LaunchProcess(const wxString& cmd, const wxString& cwd)
{
    if (m_pProcess)
        return -1;

    // start the axsdb process
    m_pProcess = new AXSPipedProcess(&m_pProcess, this, idGDBProcess, true, cwd);
    Log(_("Starting debugger: ") + cmd);
    m_Pid = wxExecute(cmd, wxEXEC_ASYNC, m_pProcess);

#ifdef __WXMAC__
    if (m_Pid == -1)
    {
        // Great! We got a fake PID. Time to Go Fish with our "ps" rod:

        m_Pid = 0;
        pid_t mypid = getpid();
        wxString mypidStr;
        mypidStr << mypid;

        long pspid = 0;
        wxString psCmd;
        wxArrayString psOutput;
        wxArrayString psErrors;

        psCmd << wxT("/bin/ps -o ppid,pid,command");
        DebugLog(wxString::Format( _("Executing: %s"), psCmd.c_str()) );
        int result = wxExecute(psCmd, psOutput, psErrors, wxEXEC_SYNC);

        mypidStr << wxT(" ");

        for (int i = 0; i < psOutput.GetCount(); ++i)
        { //  PPID   PID COMMAND
           wxString psLine = psOutput.Item(i);
           if (psLine.StartsWith(mypidStr) && psLine.Contains(wxT("gdb")))
           {
               wxString pidStr = psLine.Mid(mypidStr.Length());
               pidStr = pidStr.BeforeFirst(' ');
               if (pidStr.ToLong(&pspid))
               {
                   m_Pid = pspid;
                   break;
               }
           }
         }

        for (int i = 0; i < psErrors.GetCount(); ++i)
            DebugLog(wxString::Format( _("PS Error:%s"), psErrors.Item(i).c_str()) );
    }
#endif

    if (!m_Pid)
    {
        delete m_pProcess;
        m_pProcess = 0;
        Log(_("failed"), Logger::error);
        return -1;
    }
    else if (!m_pProcess->GetOutputStream())
    {
        delete m_pProcess;
        m_pProcess = 0;
        Log(_("failed (to get debugger's stdin)"), Logger::error);
        return -2;
    }
    else if (!m_pProcess->GetInputStream())
    {
        delete m_pProcess;
        m_pProcess = 0;
        Log(_("failed (to get debugger's stdout)"), Logger::error);
        return -2;
    }
    else if (!m_pProcess->GetErrorStream())
    {
        delete m_pProcess;
        m_pProcess = 0;
        Log(_("failed (to get debugger's stderr)"), Logger::error);
        return -2;
    }
    Log(_("done"));
    return 0;
}

bool DebuggerAXS::IsStopped() const
{
    if (!LockDriver())
        return false;
    bool stopped = m_State.GetDriver()->IsProgramStopped();
    UnlockDriver();
    return stopped;
}

bool DebuggerAXS::IsBusy() const
{
    if (!LockDriver())
        return false;
    bool busy = m_State.GetDriver()->IsQueueBusy();
    UnlockDriver();
    return busy;
}

int DebuggerAXS::GetEnabledTools()
{
    int r = m_EnabledTools;
    if (!IsRunning())
        r = cbDebuggerPlugin::DebuggerToolbarTools::Debug |
            cbDebuggerPlugin::DebuggerToolbarTools::RunToCursor |
            cbDebuggerPlugin::DebuggerToolbarTools::Step;
    return r;
}

bool DebuggerAXS::IsHalted()
{
    if (!LockDriver())
        return false;
    bool halt = m_State.GetDriver()->IsHalted();
    UnlockDriver();
    return halt;
}

bool DebuggerAXS::Debug(bool breakOnEntry)
{
    // if already running, return
    if (m_pProcess || WaitingCompilerToFinish())
        return false;

    m_pProject = 0;

    // clear the debug log
    ClearLog();

    // can only debug projects or attach to processes
    ProjectManager* prjMan = Manager::Get()->GetProjectManager();
    cbProject* project = prjMan->GetActiveProject();
    if (!project && !m_IsAttached)
        return false;

    m_pProject = project;
    if (m_pProject && m_ActiveBuildTarget.IsEmpty())
        m_ActiveBuildTarget = m_pProject->GetActiveBuildTarget();

    m_Canceled = false;
    if (!EnsureBuildUpToDate(breakOnEntry ? StartTypeStepInto : StartTypeRun))
        return false;

    // if not waiting for the compiler, start debugging now
    // but first check if the driver has already been started:
    // if the build process was ultra-fast (i.e. nothing to be done),
    // it may have already called DoDebug() and m_WaitingCompilerToFinish
    // would already be set to false
    // by checking the driver availability, we avoid calling DoDebug
    // a second consecutive time...
    // the same applies for m_Canceled: it is true if DoDebug() was launched but
    // returned an error
    if (!WaitingCompilerToFinish() && !m_State.HasDriver() && !m_Canceled)
    {
        return DoDebug(breakOnEntry) == 0;
    }

    return true;
}

int DebuggerAXS::DoDebug(bool breakOnEntry)
{
    // set this to true before every error exit point in this function
    m_Canceled = false;
    ProjectManager* prjMan = Manager::Get()->GetProjectManager();

    // select the build target to debug
    ProjectBuildTarget* target = 0;
    Compiler* actualCompiler = 0;
    if (m_pProject && (true || !m_IsAttached))
    {
        Log(_("Selecting target: "));
        if (!m_pProject->BuildTargetValid(m_ActiveBuildTarget, false))
        {
            int tgtIdx = m_pProject->SelectTarget();
            if (tgtIdx == -1)
            {
                Log(_("canceled"));
                m_Canceled = true;
                return 3;
            }
            target = m_pProject->GetBuildTarget(tgtIdx);
            m_ActiveBuildTarget = target->GetTitle();
        }
        else
            target = m_pProject->GetBuildTarget(m_ActiveBuildTarget);

        // make sure it's not a commands-only target
        if (target->GetTargetType() == ttCommandsOnly)
        {
            cbMessageBox(_("The selected target is only running pre/post build step commands\n"
                        "Can't debug such a target..."), _("Information"), wxICON_INFORMATION);
            Log(_("aborted"));
            return 3;
        }
        Log(target->GetTitle());

        // find the target's compiler (to see which debugger to use)
        actualCompiler = CompilerFactory::GetCompiler(target ? target->GetCompilerID() : m_pProject->GetCompilerID());
    }
    else
        actualCompiler = CompilerFactory::GetDefaultCompiler();

    if (!actualCompiler)
    {
        wxString msg;
        msg.Printf(_("This %s is configured to use an invalid debugger.\nThe operation failed..."), target ? _("target") : _("project"));
        cbMessageBox(msg, _("Error"), wxICON_ERROR);
        m_Canceled = true;
        return 9;
    }

    // is axsdb accessible, i.e. can we find it?
    wxString cmdexe;
    cmdexe = GetActiveConfigEx().GetDebuggerExecutable();
    cmdexe.Trim();
    cmdexe.Trim(true);
    if(cmdexe.IsEmpty())
    {
        Log(_("ERROR: You need to specify a debugger program in the debuggers's settings."), Logger::error);
        m_Canceled = true;
        return -1;
    }

    cmdexe << _(" --machineinterface");

    // start debugger driver based on target compiler, or default compiler if no target
    m_DebuggerOutputQueue.clear();
    if (!m_State.StartDriver(target))
    {
        cbMessageBox(_T("Could not decide which debugger to use!"), _T("Error"), wxICON_ERROR);
        m_Canceled = true;
        return -1;
    }


    // Notify debugger plugins so they could start a server process
    PluginManager *plm = Manager::Get()->GetPluginManager();
    CodeBlocksEvent evt(cbEVT_DEBUGGER_STARTED);
    plm->NotifyPlugins(evt);
    int nRet = evt.GetInt();
    if (nRet < 0)
    {
        cbMessageBox(_T("A plugin interrupted the debug process."));
        Log(_("Aborted by plugin"));
        m_Canceled = true;
        return -1;
    }

    // add strings to DebugLink dialog
    {
        axs_cbDbgLink *dlg = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
        if (dlg)
        {
            for (axs_cbDbgLink::FuncKey_t f = axs_cbDbgLink::FuncKey_First; f <= axs_cbDbgLink::FuncKey_Last; f = (axs_cbDbgLink::FuncKey_t)(f + 1))
            {
                dlg->SetFunctionKey(f, GetActiveConfigEx().GetDebugLinkFunctionKey(f));
            }
        }
    }

    // Add Directory Search Paths
    {
        if (LockDriver())
        {
            m_State.GetDriver()->ClearDirectories();
            UnlockDriver();
        }
        // add other open projects dirs as search dirs (only if option is enabled)
        if (GetActiveConfigEx().GetFlag(DebuggerConfiguration::AddOtherProjectDirs))
        {
            // add as include dirs all open project base dirs
            ProjectsArray* projects = prjMan->GetProjects();
            for (unsigned int i = 0; i < projects->GetCount(); ++i)
            {
                cbProject* it = projects->Item(i);
                // skip if it's THE project (added last)
                if (it == m_pProject)
                    continue;
                {
                    wxFileName fn(it->GetBasePath() + wxFILE_SEP_PATH);
                    if (fn.IsOk())
                        AddSourceDir(fn.GetPath());
                }
                {
                    wxFileName fn(it->GetCommonTopLevelPath() + wxFILE_SEP_PATH);
                    if (fn.IsOk())
                        AddSourceDir(fn.GetPath());
                }
            }
        }
        // now add all per-project user-set search dirs
        const wxArrayString& pdirs = GetSearchDirs(m_pProject);
        for (size_t i = 0; i < pdirs.GetCount(); ++i)
        {
            wxFileName fn(pdirs[i] + wxFILE_SEP_PATH);
            if (!fn.IsOk())
                continue;
            if (fn.IsRelative())
                if (!fn.MakeAbsolute(m_pProject->GetBasePath()))
                    continue;
            AddSourceDir(fn.GetPath());
        }
        if (m_pProject)
        {
            // add file paths of project files
            for (int i = 0, n = m_pProject->GetFilesCount(); i < n; ++i)
            {
                const ProjectFile *pf(m_pProject->GetFile(i));
                if (!pf)
                    continue;
                AddSourceDir(pf->file.GetPath());
            }

            // lastly, add THE project as source dir
            {
                wxFileName fn(m_pProject->GetBasePath() + wxFILE_SEP_PATH);
                if (fn.IsOk())
                    AddSourceDir(fn.GetPath());
            }
            {
                wxFileName fn(m_pProject->GetCommonTopLevelPath() + wxFILE_SEP_PATH);
                if (fn.IsOk())
                    AddSourceDir(fn.GetPath());
            }
        }
    }

    // start the axsdb process
//    wxString wdir = m_State.GetDriver()->GetDebuggersWorkingDirectory();
//    Manager::Get()->GetLogManager()->DebugLog(_T("########## BBB222"));
//    if (wdir.empty())
    wxString wdir = m_pProject ? m_pProject->GetBasePath() : _T(".");
    DebugLog(_T("Command-line: ") + cmdexe);

    int ret = LaunchProcess(cmdexe, wdir);

    if (ret != 0)
    {
        m_Canceled = true;
        return ret;
    }

    // although I don't really like these do-nothing loops, we must wait a small amount of time
    // for gdb to see if it really started: it may fail to load shared libs or whatever
    // the reason this is added is because I had a case where gdb would error and bail out
    // *while* the driver->Prepare() call was running below and hell broke loose...
    int i = 50;
    while (i)
    {
        wxMilliSleep(1);
        Manager::Yield();
        --i;
    }
    if (!LockDriver())
        return -1;
    m_State.GetDriver()->Prepare(target && target->GetTargetType() == ttConsoleOnly,
                                 GetActiveConfigEx().GetFlag(DebuggerConfiguration::LiveUpdate),
                                 GetActiveConfigEx().GetFlag(DebuggerConfiguration::CommandsSynchronous));
    UnlockDriver();

   #ifndef __WXMSW__
    // create xterm and issue tty "/dev/pts/#" to GDB where
    // # is the tty for the newly created xterm
    m_bIsConsole = target && target->GetUseConsoleRunner();
    if (m_bIsConsole)
    {
        wxString consoleTty;
        m_nConsolePid = RunNixConsole(consoleTty);
        if (m_nConsolePid > 0)
        {   wxString gdbTtyCmd;
            gdbTtyCmd << wxT("tty ") << consoleTty;
//            m_State.GetDriver()->QueueCommand(new DebuggerCmd(m_State.GetDriver(), gdbTtyCmd, true));
//            DebugLog(wxString::Format( _("Queued:[%s]"), gdbTtyCmd.c_str()) );
        }
    }//if
   #endif//ndef __WXMSW__

    if (!LockDriver())
        return -1;
    if (!m_IsAttached)
        m_State.GetDriver()->Start(breakOnEntry);
    else
        m_State.GetDriver()->Attach();
    UnlockDriver();

    // start polling
    m_TimerPollDebugger.Start(1000);

    // switch to the user-defined layout for debugging
    if (m_pProcess)
        SwitchToDebuggingLayout();

    return 0;
} // Debug

void DebuggerAXS::AddSourceDir(const wxString& dir)
{
    if (dir.IsEmpty())
        return;
    wxString filename = dir;
    Manager::Get()->GetMacrosManager()->ReplaceEnvVars(filename); // apply env vars
    if (!LockDriver())
        return;
    //Log(_("Adding source dir: ") + filename);
    ConvertToGDBDirectory(filename, _T(""), false);
    m_State.GetDriver()->AddDirectory(filename);
    UnlockDriver();
}

// static
void DebuggerAXS::StripQuotes(wxString& str)
{
    if (str.GetChar(0) == _T('\"') && str.GetChar(str.Length() - 1) == _T('\"'))
        str = str.Mid(1, str.Length() - 2);
}

// static
void DebuggerAXS::ConvertToGDBFriendly(wxString& str)
{
    if (str.IsEmpty())
        return;

    str = UnixFilename(str);
    while (str.Replace(_T("\\"), _T("/")))
        ;
    while (str.Replace(_T("//"), _T("/")))
        ;
//    str.Replace("/", "//");
    if (str.Find(_T(' ')) != -1 && str.GetChar(0) != _T('"'))
        str = _T("\"") + str + _T("\"");
}

// static
void DebuggerAXS::ConvertToGDBFile(wxString& str)
{
    wxFileName fname = str;
    str = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
    DebuggerAXS::ConvertToGDBDirectory(str);
    str << fname.GetFullName();
}

void DebuggerAXS::ConvertDirectory(wxString& str, wxString base, bool relative)
{
    ConvertToGDBDirectory(str, base, relative);
}

// static
//if relative == false, try to leave as an absolute path
void DebuggerAXS::ConvertToGDBDirectory(wxString& str, wxString base, bool relative)
{
    if (str.IsEmpty())
        return;

    ConvertToGDBFriendly(str);
    ConvertToGDBFriendly(base);
    StripQuotes(str);
    StripQuotes(base);

    if(platform::windows)
    {
        int ColonLocation = str.Find(_T(':'));
        wxChar buf[255];
        if(ColonLocation != -1)
        {
            //If can, get 8.3 name for path (Windows only)
            if (str.Contains(_T(' '))) // only if has spaces
            {
                GetShortPathName(str.c_str(), buf, 255);
                str = buf;
            }
        }
        else if(!base.IsEmpty() && str.GetChar(0) != _T('/'))
        {
            if(base.GetChar(base.Length()) == _T('/')) base = base.Mid(0, base.Length() - 2);
            while(!str.IsEmpty())
            {
                base += _T("/") + str.BeforeFirst(_T('/'));
                if(str.Find(_T('/')) != -1) str = str.AfterFirst(_T('/'));
                else str.Clear();
            }
            if (base.Contains(_T(' '))) // only if has spaces
            {
                GetShortPathName(base.c_str(), buf, 255);
                str = buf;
            }
        }

        if(ColonLocation == -1 || base.IsEmpty())
            relative = false;        //Can't do it
    }
    else
    {
        if((str.GetChar(0) != _T('/') && str.GetChar(0) != _T('~')) || base.IsEmpty())
            relative = false;
    }

    if(relative)
    {
        if(platform::windows)
        {
            if(str.Find(_T(':')) != -1) str = str.Mid(str.Find(_T(':')) + 2, str.Length());
            if(base.Find(_T(':')) != -1) base = base.Mid(base.Find(_T(':')) + 2, base.Length());
        }
        else
        {
            if(str.GetChar(0) == _T('/')) str = str.Mid(1, str.Length());
            else if(str.GetChar(0) == _T('~')) str = str.Mid(2, str.Length());
            if(base.GetChar(0) == _T('/')) base = base.Mid(1, base.Length());
            else if(base.GetChar(0) == _T('~')) base = base.Mid(2, base.Length());
        }

        while(!base.IsEmpty() && !str.IsEmpty())
        {
            if(str.BeforeFirst(_T('/')) == base.BeforeFirst(_T('/')))
            {
                if(str.Find(_T('/')) == -1) str.Clear();
                else str = str.AfterFirst(_T('/'));

                if(base.Find(_T('/')) == -1) base.Clear();
                else base = base.AfterFirst(_T('/'));
            }
            else break;
        }
        while (!base.IsEmpty())
        {
            str = _T("../") + str;
            if(base.Find(_T('/')) == -1) base.Clear();
            else base = base.AfterFirst(_T('/'));
        }
    }
    ConvertToGDBFriendly(str);
}

void DebuggerAXS::SendCommand(const wxString& cmd, bool debugLog)
{
    if (!LockDriver())
        return;
    Opt cmd1(cmd);
    m_State.GetDriver()->SendCommand(cmd1, true, false, debugLog, !debugLog);
    UnlockDriver();
}

void DebuggerAXS::DoSendCommand(const Opt& cmd, bool debugLog)
{
    if (!m_pProcess)
        return;
    wxString cmd1(cmd.get_cmdwxstring());
    if (!debugLog)
        Log(wxT(">> ") + cmd1);
    else if (HasDebugLog())
        DebugLog(wxT(">> ") + cmd1);
    m_pProcess->SendString(cmd1);
}

void DebuggerAXS::RequestUpdate(DebugWindows window)
{
    switch (window)
    {
        case Backtrace:
            RunCommand(CMD_BACKTRACE);
            break;
        case CPURegisters:
            RunCommand(CMD_REGISTERS);
            break;
        case Disassembly:
            RunCommand(CMD_DISASSEMBLE);
            break;
        case ExamineMemory:
            RunCommand(CMD_MEMORYDUMP);
            break;
        case Threads:
            RunCommand(CMD_RUNNINGTHREADS);
            break;
        case Watches:
            if (IsWindowReallyShown(Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->GetWindow()))
                DoWatches();
            break;
        case DebugLink:
            RunCommand(CMD_DEBUGLINK);
            break;
        case PinEmulation:
            RunCommand(CMD_PINEMULATION);
            break;
    }
}

void DebuggerAXS::RunCommand(int cmd)
{
    // just check for the process
    if (!m_pProcess)
        return;

    switch (cmd)
    {
        case CMD_CONTINUE:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                Log(_("Continuing..."));
                m_State.GetDriver()->Continue();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
            }
            break;
        }

        case CMD_STEP:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->Step();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
            }
            break;
        }

        case CMD_STEP_INSTR:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->StepInstruction();
                m_State.GetDriver()->ResetCurrentFrame();
                //m_State.GetDriver()->NotifyCursorChanged();
                UnlockDriver();
            }
            break;
        }

        case CMD_STEP_INTO_INSTR:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->StepIntoInstruction();
                m_State.GetDriver()->ResetCurrentFrame();
                //m_State.GetDriver()->NotifyCursorChanged();
                UnlockDriver();
            }
            break;
        }

        case CMD_STEPIN:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->StepIn();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
            }
            break;
        }

        case CMD_STEPOUT:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->StepOut();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
            }
            break;
        }

        case CMD_STOP:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->Stop();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
                MarkAsStopped();
            }
            break;
        }

        case CMD_PAUSE:
        {
            ClearActiveMarkFromAllEditors();
            if (LockDriver())
            {
                m_State.GetDriver()->Pause();
                m_State.GetDriver()->ResetCurrentFrame();
                UnlockDriver();
                MarkAsStopped();
            }
            break;
        }

        case CMD_BACKTRACE:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->Backtrace();
                UnlockDriver();
            }
            break;
        }

        case CMD_DISASSEMBLE:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->Disassemble();
                UnlockDriver();
            }
            break;
        }

        case CMD_REGISTERS:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->CPURegisters();
                UnlockDriver();
            }
            break;
        }

        case CMD_MEMORYDUMP:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->MemoryDump();
                UnlockDriver();
            }
            break;
        }

        case CMD_RUNNINGTHREADS:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->RunningThreads();
                UnlockDriver();
            }
            break;
        }

        case CMD_HWR:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->HWR();
                UnlockDriver();
            }
            break;
        }

        case CMD_SWR:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->SWR();
                UnlockDriver();
            }
            break;
        }

        case CMD_PINEMULATION:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->PinEmulation();
                UnlockDriver();
            }
            break;
        }

        case CMD_DEBUGLINK:
        {
            if (LockDriver())
            {
                m_State.GetDriver()->DebugLink();
                UnlockDriver();
            }
            break;
        }

        default: break;
    }
}

int DebuggerAXS::GetStackFrameCount() const
{
    if (!LockDriver())
        return 0;
    int sfc = m_State.GetDriver()->GetStackFrames().size();
    UnlockDriver();
    return sfc;
}

cb::shared_ptr<const cbStackFrame> DebuggerAXS::GetStackFrame(int index) const
{
    cb::shared_ptr<const cbStackFrame> sf;
    if (LockDriver())
    {
        sf = m_State.GetDriver()->GetStackFrames()[index];
        UnlockDriver();
    }
    return sf;
}

void DebuggerAXS::SwitchToFrame(int number)
{
    if (LockDriver())
    {
        m_State.GetDriver()->SetCurrentFrame(number, true);
        m_State.GetDriver()->SwitchToFrame(number);
        UnlockDriver();

        if(Manager::Get()->GetDebuggerManager()->UpdateBacktrace())
           Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
    }
}

int DebuggerAXS::GetActiveStackFrame() const
{
    if (!LockDriver())
        return 0;
    int curframe = m_State.GetDriver()->GetCurrentFrame();
    UnlockDriver();
    return curframe;
}

int DebuggerAXS::GetThreadsCount() const
{
    if (!LockDriver())
        return 0;
    int thr = m_State.GetDriver()->GetThreads().size();
    UnlockDriver();
    return thr;
}

cb::shared_ptr<const cbThread> DebuggerAXS::GetThread(int index) const
{
    cb::shared_ptr<const cbThread> thr;
    if (LockDriver())
    {
        thr = m_State.GetDriver()->GetThreads()[index];
        UnlockDriver();
    }
    return thr;
}

bool DebuggerAXS::SwitchToThread(int thread_number)
{
    if (!LockDriver())
        return false;
    DebuggerDriver *driver = m_State.GetDriver();
    DebuggerDriver::ThreadsContainer const &threads = driver->GetThreads();

    bool ret(false);
    for (DebuggerDriver::ThreadsContainer::const_iterator it = threads.begin(); it != threads.end(); ++it)
    {
        if ((*it)->GetNumber() == thread_number)
        {
            if (!(*it)->IsActive())
                driver->SwitchThread(thread_number);
            ret = true;
            break;
        }
    }
    UnlockDriver();
    return ret;
}

cb::shared_ptr<cbBreakpoint> DebuggerAXS::AddBreakpoint(const wxString& filename, int line)
{
    bool debuggerIsRunning = !IsStopped();
    if (debuggerIsRunning)
        DoBreak(true);

    cb::shared_ptr<DebuggerBreakpoint> bp = m_State.AddBreakpoint(filename, line, false);

    if (debuggerIsRunning)
        Continue();

    return bp;
}

cb::shared_ptr<cbBreakpoint> DebuggerAXS::AddDataBreakpoint(const wxString& dataExpression)
{
    return cb::shared_ptr<cbBreakpoint>();
}

int DebuggerAXS::GetBreakpointsCount() const
{
    return m_State.GetBreakpoints().size();
}

cb::shared_ptr<cbBreakpoint> DebuggerAXS::GetBreakpoint(int index)
{
    BreakpointsList::const_iterator it = m_State.GetBreakpoints().begin();
    std::advance(it, index);
    cbAssert(it != m_State.GetBreakpoints().end());
    return *it;
}

cb::shared_ptr<const cbBreakpoint> DebuggerAXS::GetBreakpoint(int index) const
{
    BreakpointsList::const_iterator it = m_State.GetBreakpoints().begin();
    std::advance(it, index);
    cbAssert(it != m_State.GetBreakpoints().end());
    return *it;
}

void DebuggerAXS::UpdateBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint)
{
    const BreakpointsList &breakpoints = m_State.GetBreakpoints();
    BreakpointsList::const_iterator it = std::find(breakpoints.begin(), breakpoints.end(), breakpoint);
    if (it == breakpoints.end())
        return;
    cb::shared_ptr<DebuggerBreakpoint> bp = cb::static_pointer_cast<DebuggerBreakpoint>(breakpoint);
    bool reset = false;
    {
        EditBreakpointDlg dlg(*bp, Manager::Get()->GetAppWindow());
        PlaceWindow(&dlg);
        if (dlg.ShowModal() == wxID_OK)
        {
            *bp = dlg.GetBreakpoint();
            reset = true;
        }
    }

    if (reset)
    {
        bool debuggerIsRunning = !IsStopped();
        if (debuggerIsRunning)
            DoBreak(true);

        m_State.ResetBreakpoint(bp);

        if (debuggerIsRunning)
            Continue();
    }
}

void DebuggerAXS::DeleteBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint)
{
    bool debuggerIsRunning = !IsStopped();
    if (debuggerIsRunning)
        DoBreak(true);

    m_State.RemoveBreakpoint(cb::static_pointer_cast<DebuggerBreakpoint>(breakpoint));

    if (debuggerIsRunning)
        Continue();
}

void DebuggerAXS::DeleteAllBreakpoints()
{
    bool debuggerIsRunning = !IsStopped();
    if (debuggerIsRunning)
        DoBreak(true);
    m_State.RemoveAllBreakpoints();

    if (debuggerIsRunning)
        Continue();
}

void DebuggerAXS::ShiftBreakpoint(int index, int lines_to_shift)
{
    BreakpointsList breakpoints = m_State.GetBreakpoints();
    BreakpointsList::iterator it = breakpoints.begin();
    std::advance(it, index);
    if(it != breakpoints.end())
        m_State.ShiftBreakpoint(*it, lines_to_shift);
}

void DebuggerAXS::EnableBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint, bool enable)
{
    bool debuggerIsRunning = !IsStopped();
    DebugLog(wxString::Format(wxT("DebuggerAXS::EnableBreakpoint(running=%d);"), (int)debuggerIsRunning));
    if (debuggerIsRunning)
        DoBreak(true);

    cb::shared_ptr<DebuggerBreakpoint> bp = cb::static_pointer_cast<DebuggerBreakpoint>(breakpoint);
    bp->enabled = enable;
    m_State.ResetBreakpoint(bp);

    if (debuggerIsRunning)
        Continue();
}

void DebuggerAXS::DeleteAllProjectBreakpoints(cbProject* project)
{
    m_State.RemoveAllProjectBreakpoints(project);
}

void DebuggerAXS::Continue()
{
    RunCommand(CMD_CONTINUE);
}

void DebuggerAXS::Next()
{
    RunCommand(CMD_STEP);
}

void DebuggerAXS::NextInstruction()
{
    RunCommand(CMD_STEP_INSTR);
}

void DebuggerAXS::StepIntoInstruction()
{
    RunCommand(CMD_STEP_INTO_INSTR);
}

void DebuggerAXS::Step()
{
    RunCommand(CMD_STEPIN);
}

bool DebuggerAXS::Validate(const wxString& line, const char cb)
{
    bool bResult = false;

    int bep = line.Find(cb)+1;
    int scs = line.Find(_T('\''))+1;
    int sce = line.Find(_T('\''),true)+1;
    int dcs = line.Find(_T('"'))+1;
    int dce = line.Find(_T('"'),true)+1;
    //No single and double quote
    if(!scs && !sce && !dcs && !dce) bResult = true;
    //No single/double quote in pair
    if(!(sce-scs) && !(dce-dcs)) bResult = true;
    //Outside of single quote
    if((sce-scs) && ((bep < scs)||(bep >sce))) bResult = true;
    //Outside of double quote
    if((dce-dcs) && ((bep < dcs)||(bep >dce))) bResult = true;

    return bResult;
}

void DebuggerAXS::StepOut()
{
    RunCommand(CMD_STEPOUT);
}

bool DebuggerAXS::RunToCursor(const wxString& filename, int line, const wxString& line_text)
{
    if (m_pProcess)
    {
        m_State.AddBreakpoint(filename, line, true, line_text);
        Manager::Get()->GetDebuggerManager()->GetBreakpointDialog()->Reload();
        Continue();
        return true;
    }
    else
    {
        if (!GetActiveConfigEx().GetFlag(DebuggerConfiguration::DoNotRun))
        {
            m_State.AddBreakpoint(filename, line, true, line_text);
            Manager::Get()->GetDebuggerManager()->GetBreakpointDialog()->Reload();
        }
        return Debug(false);
    }
}

void DebuggerAXS::SetNextStatement(const wxString& filename, int line)
{
    if (!IsStopped())
        return;
    if (LockDriver())
    {
        m_State.GetDriver()->SetNextStatement(filename, line);
        UnlockDriver();
    }
}

void DebuggerAXS::Break()
{
    DoBreak(false);
}

void DebuggerAXS::DoBreak(bool temporary)
{
    m_TemporaryBreak = temporary;

    if (LockDriver())
    {
        m_State.GetDriver()->Pause();
        UnlockDriver();
    }

    // Notify debugger plugins for end of debug session
    PluginManager *plm = Manager::Get()->GetPluginManager();
    CodeBlocksEvent evt(cbEVT_DEBUGGER_PAUSED);
    plm->NotifyPlugins(evt);
}

void DebuggerAXS::Stop()
{
    // m_Process is PipedProcess I/O; m_Pid is debugger pid
    if (m_pProcess && m_Pid)
    {
        if (!IsStopped())
            Break();
        RunCommand(CMD_STOP);
    }
}

void DebuggerAXS::OnAppStartShutdown(CodeBlocksEvent& event)
{
    Stop();
    while (m_pProcess && m_Pid)
    {
        // calling HasInput seems to be necessary on windows
        if (!((PipedProcess*)m_pProcess)->HasInput())
            wxMilliSleep(1);
        wxYield();
    }
    event.Skip(); // allow others to process it too
}

void DebuggerAXS::GetCurrentPosition(wxString &filename, int &line)
{
    if (LockDriver())
    {
        const Cursor& cursor = m_State.GetDriver()->GetCursor();
        filename = cursor.file;
        line = cursor.line;
        UnlockDriver();
    }
    else
    {
        filename = wxEmptyString;
        line = -1;
    }
}

// TODO: should reimplement
void DebuggerAXS::OnAddSymbolFile(wxCommandEvent& WXUNUSED(event))
{
    wxString file = wxFileSelector(_("Choose file to read symbols from"),
                                    _T(""),
                                    _T(""),
                                    _T(""),
                                    _("Executables and libraries|*.exe;*.dll"),
                                    wxFD_OPEN | wxFD_FILE_MUST_EXIST | compatibility::wxHideReadonly);
    if (file.IsEmpty())
        return;
//    Manager::Get()->GetLogManager()->Log(m_PageIndex, _("Adding symbol file: %s"), file.c_str());
    ConvertToGDBDirectory(file);
//    QueueCommand(new DbgCmd_AddSymbolFile(this, file));
}

void DebuggerAXS::SetupToolsMenu(wxMenu &menu)
{
//    menu.Append(idMenuInfoFrame,    _("Current stack frame"), _("Displays info about the current (selected) stack frame"));
//    menu.Append(idMenuInfoDLL,      _("Loaded libraries"), _("List dynamically loaded libraries (DLL/SO)"));
    menu.Append(idMenuInfoFiles,    _("Targets and files"), _("Displays info on the targets and files being debugged"));
//    menu.Append(idMenuInfoFPU,      _("FPU status"), _("Displays the status of the floating point unit"));
//    menu.Append(idMenuInfoSignals,  _("Signal handling"), _("Displays how the debugger handles various signals"));
    menu.Append(idMenuInfoCPUTrace, _("CPU Trace"), _("Displays CPU trace information"));
    menu.Append(idMenuInfoProfiler, _("Profiler"), _("Displays the CPU Profiler"));
}

void DebuggerAXS::OnInfoFrame(wxCommandEvent& WXUNUSED(event))
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoFrame();
        UnlockDriver();
    }
}

void DebuggerAXS::OnInfoDLL(wxCommandEvent& WXUNUSED(event))
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoDLL();
        UnlockDriver();
    }
}

void DebuggerAXS::OnInfoFiles(wxCommandEvent& WXUNUSED(event))
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoFiles();
        UnlockDriver();
    }
}

void DebuggerAXS::OnInfoFPU(wxCommandEvent& WXUNUSED(event))
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoFPU();
        UnlockDriver();
    }
}

void DebuggerAXS::OnInfoSignals(wxCommandEvent& WXUNUSED(event))
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoSignals();
        UnlockDriver();
    }
}
 
void DebuggerAXS::OnInfoCPUTrace(wxCommandEvent& event)
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoCPUTrace();
        UnlockDriver();
    }
}

void DebuggerAXS::OnInfoProfiler(wxCommandEvent& event)
{
    if (LockDriver())
    {
        m_State.GetDriver()->InfoProfiler();
        UnlockDriver();
    }
}

void DebuggerAXS::HWReset()
{
    RunCommand(CMD_HWR);
}

void DebuggerAXS::SWReset()
{
    RunCommand(CMD_SWR);
}

unsigned int DebuggerAXS::LockDriver() const
{
    if (!m_State.HasDriver())
    {
        cbAssert(m_DriverLockDepth == 0);
        return 0;
    }
    ++m_DriverLockDepth;
    return m_DriverLockDepth;
}

void DebuggerAXS::UnlockDriver() const
{
    cbAssert(m_DriverLockDepth == 0);
    --m_DriverLockDepth;
    //if (!m_DriverLockDepth && m_DriverTerminate)
    //    StopDebugger();
}

void DebuggerAXS::UnlockDriver()
{
    cbAssert(m_DriverLockDepth == 0);
    --m_DriverLockDepth;
    if (!m_DriverLockDepth && m_DriverTerminate)
        StopDebugger();
}

void DebuggerAXS::StopDebugger()
{
    m_DriverTerminate = false;

    cbExamineMemoryDlg *MEMdialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
    MEMdialog->Clear();     /// Reset memory dump when debugging session ends
    MEMdialog->SetAddressSpaces();

    axs_cbPinEmDlg *PEMdialog = Manager::Get()->GetDebuggerManager()->GetAXSPinEmDialog();
    PEMdialog->Reset(true);     /// Hard-Reset Pin-Emulation when debugging session ends

    axs_cbDbgLink *DLdialog = Manager::Get()->GetDebuggerManager()->GetAXSDbgLinkDialog();
    DLdialog->SetTransmitBuffer(0, 0);
    DLdialog->TerminalEnable(false);  /// Deactivate prompt of DebuggerLink

    cbCPURegistersDlg *CPURdialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();
    CPURdialog->Clear();
    CPURdialog->SetRootRegister(cb::shared_ptr<cbRegister>());
    CPURdialog->SetChips(wxArrayString());
    CPURdialog->EnableInteraction(0);

    m_IsAttached = false;
    m_EnabledTools = DebuggerToolbarTools::Debug |
                     DebuggerToolbarTools::RunToCursor |
                     DebuggerToolbarTools::Step;

    //the process deletes itself
//    m_pProcess = 0L;

    ClearActiveMarkFromAllEditors();
    m_State.StopDriver();
    Manager::Get()->GetDebuggerManager()->GetBreakpointDialog()->Reload();
    Log(wxString::Format(_("Debugger finished with status %d"), m_LastExitCode));

    // Notify debugger plugins for end of debug session
    PluginManager *plm = Manager::Get()->GetPluginManager();
    CodeBlocksEvent evt(cbEVT_DEBUGGER_FINISHED);
    plm->NotifyPlugins(evt);

    // switch to the user-defined layout when finished debugging
    SwitchToPreviousLayout();
    KillConsole();
    MarkAsStopped();

    ///killerbot : run there the post shell commands ?
}

void DebuggerAXS::OnGDBOutput(wxCommandEvent& event)
{
    wxString msg = event.GetString();
    if (msg.IsEmpty())
        return;
    if (!LockDriver())
        return;
    bool e = m_DebuggerOutputQueue.empty();
    m_DebuggerOutputQueue.push_back(msg);
    if (e)
    {
        do {
            m_State.GetDriver()->ParseOutput(m_DebuggerOutputQueue.front());
            m_DebuggerOutputQueue.pop_front();
        } while (!m_DebuggerOutputQueue.empty());
    }
    UnlockDriver();
}

void DebuggerAXS::OnGDBError(wxCommandEvent& event)
{
    wxString msg = event.GetString();
    if (!msg.IsEmpty())
    {
        msg.Trim(true);
        DebugLog(_T("E>") + msg + _T("\n"));
    }
}

void DebuggerAXS::OnGDBTerminated(wxCommandEvent& event)
{
    m_TimerPollDebugger.Stop();
    m_LastExitCode = event.GetInt();
    if (LockDriver())
    {
        m_DriverTerminate = true;
        UnlockDriver();
    }
}

void DebuggerAXS::KillConsole()
{
#ifdef __WXMSW__
    if (m_bIsConsole)
    {
        // remove the CTRL_C handler
        SetConsoleCtrlHandler(HandlerRoutine, FALSE);
        FreeConsole();
        m_bIsConsole = false;
    }
#else
    // kill any linux console
    if ( m_bIsConsole && (m_nConsolePid > 0) )
    {
        ::wxKill(m_nConsolePid);
        m_nConsolePid = 0;
        m_bIsConsole = false;
    }
#endif
}

bool DebuggerAXS::ShowValueTooltip(int style)
{
    if (!m_pProcess || !IsStopped())
        return false;

    if (!LockDriver())
        return false;
    {
        bool dbgstarted = m_State.GetDriver()->IsDebuggingStarted();
        UnlockDriver();
        if (!dbgstarted)
            return false;
    }

    if (!GetActiveConfigEx().GetFlag(DebuggerConfiguration::EvalExpression))
        return false;

    if (style != wxSCI_C_DEFAULT && style != wxSCI_C_OPERATOR && style != wxSCI_C_IDENTIFIER)
        return false;
    return true;
}

void DebuggerAXS::OnValueTooltip(const wxString &token, const wxRect &evalRect)
{
    if (!LockDriver())
        return;
    m_State.GetDriver()->EvaluateSymbol(token, m_EvalRect);
    UnlockDriver();
}

void DebuggerAXS::CleanupWhenProjectClosed(cbProject *project)
{
    // remove all search dirs stored for this project so we don't have conflicts
    // if a newly opened project happens to use the same memory address
    RemoveSearchDirs(project);

    // the same for remote debugging
    RemoveProjectTargetOptionsMap(project);

    // remove all breakpoints belonging to the closed project
    DeleteAllProjectBreakpoints(project);
    cbBreakpointsDlg *dlg = Manager::Get()->GetDebuggerManager()->GetBreakpointDialog();
    dlg->Reload();
}

void DebuggerAXS::OnIdle(wxIdleEvent& event)
{
    if (m_pProcess && ((PipedProcess*)m_pProcess)->HasInput())
        event.RequestMore();
    else
        event.Skip();
}

void DebuggerAXS::OnTimer(wxTimerEvent& event)
{
    wxWakeUpIdle();
    if (!LockDriver())
        return;
    m_State.GetDriver()->Poll();
    UnlockDriver();
}

void DebuggerAXS::OnShowFile(wxCommandEvent& event)
{
    SyncEditor(event.GetString(), event.GetInt(), false);
}

void DebuggerAXS::DebuggeeContinued()
{
    m_TemporaryBreak = false;
}

#include <iostream>

void DebuggerAXS::OnCursorChanged(wxCommandEvent& event)
{
    {
        const Cursor& cursor = m_State.GetDriver()->GetCursor();
        std::cerr << "OnCursorChanged: changed " << (cursor.changed ? "true" : "false")
                  << " addr: " << (const char *)cursor.address.mb_str()
                  << " file: " << (const char *)cursor.file.mb_str() << ':' << cursor.line
                  << " func: " << (const char *)cursor.function.mb_str()
                  << " tempbkp: " << (m_TemporaryBreak ? "true" : "false") << std::endl;
    }

    if (m_TemporaryBreak)
        return;

    if (LockDriver())
    {
        const Cursor& cursor = m_State.GetDriver()->GetCursor();
        if (event.GetInt())
        {
            if (cursor.changed && cursor.line != -1)
                SyncEditor(cursor.file, cursor.line, true, cursor.line_end);
        }
        else if (cursor.changed)
        {
            bool autoSwitch = cbDebuggerCommonConfig::GetFlag(cbDebuggerCommonConfig::AutoSwitchFrame);

            MarkAllWatchesAsUnchanged();
            MarkAllRegistersAsUnchanged();

            // if the cursor line is invalid and the auto switch is on,
            // we don't sync the editor, because there is no line to sync to
            // and also we are going to execute a backtrace command hoping to find a valid frame.
            if (!autoSwitch || cursor.line != -1)
                SyncEditor(cursor.file, cursor.line, true, cursor.line_end);

            BringCBToFront();
            if (cursor.line != -1)
                Log(wxString::Format(_("At %s:%d"), cursor.file.c_str(), cursor.line));
            else
                Log(wxString::Format(_("In %s (%s)"), cursor.function.c_str(), cursor.file.c_str()));

            // update watches
            DebuggerManager *dbg_manager = Manager::Get()->GetDebuggerManager();

            if (IsWindowReallyShown(dbg_manager->GetWatchesDialog()->GetWindow()))
                DoWatches();

            if (IsWindowReallyShown(dbg_manager->GetCPURegistersDialog()->GetWindow()))
                DoRegisters();

            // update CPU registers
            if (dbg_manager->UpdateCPURegisters())
                RunCommand(CMD_REGISTERS);

            // update callstack
            if (dbg_manager->UpdateBacktrace())
                RunCommand(CMD_BACKTRACE);
            else
            {
                if (cursor.line == -1 && autoSwitch)
                    RunCommand(CMD_BACKTRACE);
            }

            // update disassembly
            if (dbg_manager->UpdateDisassembly())
            {
                unsigned long int addrL;
                bool addrok = cursor.address.ToULong(&addrL, 16);
                if (addrok && !dbg_manager->GetDisassemblyDialog()->SetActiveAddress(addrL))
                    RunCommand(CMD_DISASSEMBLE);
            }

            // update memory examiner
            if (dbg_manager->UpdateExamineMemory())
                RunCommand(CMD_MEMORYDUMP);

            // update running threads
            if (dbg_manager->UpdateThreads())
                RunCommand(CMD_RUNNINGTHREADS);
        }
        UnlockDriver();
    }
}

cb::shared_ptr<cbWatch> DebuggerAXS::AddWatch(const wxString& symbol)
{
    cb::shared_ptr<GDBWatch> watch(new GDBWatch(symbol));
    m_watches.push_back(watch);

    if (m_pProcess)
    {
        if (LockDriver())
        {
            m_State.GetDriver()->UpdateWatch(m_watches.back());
            UnlockDriver();
        }
    }
    return watch;
}

void DebuggerAXS::AddWatchNoUpdate(const cb::shared_ptr<GDBWatch> &watch)
{
    m_watches.push_back(watch);
}

void DebuggerAXS::DeleteWatch(cb::shared_ptr<cbWatch> watch)
{
    m_watches.erase(std::find(m_watches.begin(), m_watches.end(), watch));
}

bool DebuggerAXS::HasWatch(cb::shared_ptr<cbWatch> watch)
{
    return std::find(m_watches.begin(), m_watches.end(), watch) != m_watches.end();
}

void DebuggerAXS::SetWatchesDisabled(bool flag)
{
    for (WatchesContainer::iterator wi(m_watches.begin()), we(m_watches.end()); wi != we; ++wi)
        (*wi)->SetDisabledRecursive(flag);
}

void DebuggerAXS::ShowWatchProperties(cb::shared_ptr<cbWatch> watch)
{
    cb::shared_ptr<GDBWatch> real_watch = cb::static_pointer_cast<GDBWatch>(watch);
    EditWatchDlg dlg(real_watch);
    if(dlg.ShowModal() == wxID_OK)
        DoWatches();
}

bool DebuggerAXS::SetWatchValue(cb::shared_ptr<cbWatch> watch, const wxString &value)
{
    if (!HasWatch(cbGetRootWatch(watch)))
        return false;

    if (watch->IsReadonly())
        return false;

    wxString full_symbol;
    watch->GetFullWatchString(full_symbol);

    wxString value1(value);
    cb::shared_ptr<GDBWatch> real_watch = cb::static_pointer_cast<GDBWatch>(watch);
    {
        unsigned long val(0);
        if (real_watch->FindEnum(val, value))
        {
            value1 = wxString::Format(wxT("%lu"), val);
        }
    }
    if (!LockDriver())
        return false;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->SetVarValue(full_symbol, value1);
    UnlockDriver();

    DoWatches();
    DoRegisters();
    return true;
}

void DebuggerAXS::ExpandWatch(cb::shared_ptr<cbWatch> watch)
{
    if (!watch)
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->ExpandWatch(cb::static_pointer_cast<GDBWatch>(watch));
    UnlockDriver();
}

void DebuggerAXS::CollapseWatch(cb::shared_ptr<cbWatch> watch)
{
    if (!watch)
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->CollapseWatch(cb::static_pointer_cast<GDBWatch>(watch));
    UnlockDriver();
}

void DebuggerAXS::UpdateWatch(cb::shared_ptr<cbWatch> watch)
{
    if (!HasWatch(watch))
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    cb::shared_ptr<GDBWatch> real_watch = cb::static_pointer_cast<GDBWatch>(watch);
    driver->UpdateWatch(real_watch);
    UnlockDriver();
}

void DebuggerAXS::MarkAllWatchesAsUnchanged()
{
    for(WatchesContainer::iterator it = m_watches.begin(); it != m_watches.end(); ++it)
        (*it)->MarkAsChangedRecursive(false);
}

void DebuggerAXS::OnWatchesContextMenu(wxMenu &menu, const cbWatch &watch, wxObject *property, int &disabledMenus)
{
    const GDBWatch &gwatch(static_cast<const GDBWatch &>(watch));

    if (gwatch.GetWatchType() == TypePointer)
    {
        wxString symbol;
        gwatch.GetFullWatchString(symbol);
        if (true || !gwatch.GetParent())
        {
            // disable deref of children for now
            menu.InsertSeparator(0);
            menu.Insert(0, idMenuWatchDereference, _("Dereference ") + symbol);
            if (gwatch.GetParent())
            {
                m_watchToDereferenceSymbol = wxT("(") + symbol + wxT(")");
                m_watchToDereferenceProperty = NULL;
            }
            else
            {
                m_watchToDereferenceSymbol = symbol;
                m_watchToDereferenceProperty = property;
            }
        }
    }

    if (watch.GetParent())
    {
        disabledMenus = WatchesDisabledMenuItems::Rename;
        //disabledMenus |= WatchesDisabledMenuItems::Properties;
        disabledMenus |= WatchesDisabledMenuItems::Delete;
    }
    disabledMenus |= WatchesDisabledMenuItems::AddDataBreak;
}

void DebuggerAXS::OnMenuWatchDereference(wxCommandEvent& event)
{
    cbWatchesDlg *watches = Manager::Get()->GetDebuggerManager()->GetWatchesDialog();
    if (!watches)
        return;
    if (m_watchToDereferenceSymbol.IsEmpty())
        return;
    if (!m_watchToDereferenceProperty)
    {
        cb::shared_ptr<cbWatch> watch = AddWatch(wxT("(*") + m_watchToDereferenceSymbol + wxT(")"));
        if (watch.get())
            watches->AddWatch(watch);
    }
    else
    {
        watches->RenameWatch(m_watchToDereferenceProperty, wxT("*") + m_watchToDereferenceSymbol);
    }
    m_watchToDereferenceProperty = NULL;
    m_watchToDereferenceSymbol = wxEmptyString;
}

void DebuggerAXS::ExpandRegister(cb::shared_ptr<cbRegister> reg)
{
    if (!reg)
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->ExpandRegister(cb::static_pointer_cast<AXSRegister>(reg));
    UnlockDriver();
}

void DebuggerAXS::CollapseRegister(cb::shared_ptr<cbRegister> reg)
{
    if (!reg)
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->CollapseRegister(cb::static_pointer_cast<AXSRegister>(reg));
    UnlockDriver();
}

void DebuggerAXS::MarkAllRegistersAsUnchanged()
{
    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->MarkAllRegistersAsUnchanged();
    UnlockDriver();
}

bool DebuggerAXS::SetRegisterValue(cb::shared_ptr<cbRegister> reg, const wxString &value)
{
    if (!reg)
        return false;

    if (reg->IsReadonly())
        return false;

    wxString as;
    reg->GetAddrSpace(as);
    cb::shared_ptr<AXSRegister> reg1(cb::static_pointer_cast<AXSRegister>(reg));

    if (!LockDriver())
        return false;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->SetRegValue(as, reg1->GetAddr(), value);
    UnlockDriver();
    DoWatches();
    DoRegisters();
    return true;
}

void DebuggerAXS::UpdateRegister(cb::shared_ptr<cbRegister> reg)
{
    if (!reg)
        return;

    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->UpdateRegister(cb::static_pointer_cast<AXSRegister>(reg));
    UnlockDriver();
}

void DebuggerAXS::SetChip(const wxString& chip)
{
    if (!LockDriver())
        return;
    DebuggerDriver* driver = m_State.GetDriver();
    driver->SetChip(chip);
    UnlockDriver();
}

void DebuggerAXS::AttachToProcess()
{
    m_IsAttached = true;
    Debug(false);
}

void DebuggerAXS::DetachFromProcess()
{
    if (!LockDriver())
        return;
    m_State.GetDriver()->Detach();
    m_IsAttached = false;
    m_State.GetDriver()->Stop();
    UnlockDriver();
}

bool DebuggerAXS::IsAttachedToProcess() const
{
    return m_IsAttached;
}

bool DebuggerAXS::CompilerFinished(bool compilerFailed, StartType startType)
{
    if (compilerFailed || startType == StartTypeUnknown)
        return false;
    if (DoDebug(startType == StartTypeStepInto) != 0)
        return false;
    return true;
}

void DebuggerAXS::OnBuildTargetSelected(CodeBlocksEvent& event)
{
    // verify that the project that sent it, is the one we 're debugging
    // and that a project is loaded
    if (m_pProject && event.GetProject() == m_pProject)
        m_ActiveBuildTarget = event.GetBuildTargetName();
}
