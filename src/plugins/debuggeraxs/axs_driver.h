/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef AXS_DRIVER_H
#define AXS_DRIVER_H

#include "debuggerdriver.h"
#include "projtargetoptions.h"
#include <wx/dynarray.h>
#include <wx/regex.h>
#include <wx/progdlg.h>
#include "filefilters.h"

class Opt;

class AXS_driver : public DebuggerDriver
{
    public:
        AXS_driver(DebuggerAXS* plugin);
        virtual ~AXS_driver();

        virtual bool IsQueueBusy() const { return m_IsInitializing || DebuggerDriver::IsQueueBusy(); }

        virtual wxString GetCommandLine(const wxString& debugger, const wxString& debuggee);
        virtual wxString GetCommandLine(const wxString& debugger, int pid);
        virtual void SetTarget(ProjectBuildTarget* target);
        virtual void Prepare(bool isConsole, bool liveUpdate);
        virtual void Start(bool breakOnEntry);
        virtual void Stop();
        virtual void Pause();

        virtual void Continue();
        virtual void Step();
        virtual void StepInstruction();
        virtual void StepIntoInstruction();
        virtual void StepIn();
        virtual void StepOut();
        virtual void SetNextStatement(const wxString& filename, int line);
        virtual void Backtrace();
        virtual void Disassemble();
        virtual void CPURegisters();
        virtual void SwitchToFrame(size_t number);
        virtual void SetVarValue(const wxString& var, const wxString& value);
        virtual void SetRegValue(const wxString& addrspace, unsigned int addr, const wxString& value);
        virtual void MemoryDump();
        virtual void Attach();
        virtual void Detach();
        virtual void RunningThreads();
        virtual void Poll();

        void InfoFrame();
        void InfoDLL();
        void InfoFiles();
        void InfoFPU();
        void InfoSignals();
        void InfoCPUTrace();
        void InfoProfiler();

        void OnCPUTraceChange();
        void OnProfilerChange();

        virtual void HWR();
        virtual void SWR();
        virtual void PinEmulation();
        virtual void DebugLink();

        bool IsHalted() const { return GetCpuState() == cpustate_halt; }

        virtual void SwitchThread(size_t threadIndex);

        virtual void AddBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp);
        virtual void RemoveBreakpoint(cb::shared_ptr<DebuggerBreakpoint> bp);
        virtual void EvaluateSymbol(const wxString& symbol, const wxRect& tipRect);
        virtual void UpdateWatches(WatchesContainer &watches);
        virtual void UpdateWatch(cb::shared_ptr<GDBWatch> const &watch);
        virtual void ExpandWatch(cb::shared_ptr<GDBWatch> watch);
        virtual void CollapseWatch(cb::shared_ptr<GDBWatch> watch);
        virtual void ExpandRegister(cb::shared_ptr<AXSRegister> reg);
        virtual void CollapseRegister(cb::shared_ptr<AXSRegister> reg);
        virtual void UpdateRegister(cb::shared_ptr<AXSRegister> reg = cb::shared_ptr<AXSRegister>());
        virtual void SetChip(const wxString& chip);
        virtual void MarkAllRegistersAsUnchanged();

        virtual bool ParseOutput(const Opt& output);
        virtual bool IsDebuggingStarted() const { return m_IsStarted; }
#ifdef __WXMSW__
        virtual bool UseDebugBreakProcess();
#endif
        virtual wxString GetDisassemblyFlavour(void);

        wxString GetScriptedTypeCommand(const wxString& gdb_type, wxString& parse_func);

        void KillDebugger(bool terminate = true);

        // used by AxsCmd_ConnectTargetDownload
        typedef enum {
            compilervendor_unknown,
            compilervendor_sdcc,
            compilervendor_keil,
            compilervendor_iar,
            compilervendor_wickenhaeuser,
            compilervendor_noice
        } compilervendor_t;

        static const std::string& to_str(compilervendor_t cv);

        compilervendor_t GetCompilerVendor() const { return m_compilervendor; }
        const wxFileName& GetBinFile() const { return m_BinFile; }
        const wxFileName& GetSymFile() const { return m_SymFile; }
        void UpdateProjectTargetOptions();
        void FindProgramFiles();
        void CommandAddKeys(Opt& cmd);
        void NotifyInitDone();
        wxString FilePathSearch(const wxString& filename);
        const ProjectTargetOptions& GetProjTargetOpt() const { return m_ProjTargetOpt; }

    protected:
        typedef enum {
            filetype_omf51,
            filetype_ihex,
            filetype_cdb,
            filetype_ubrof,
            filetype_unknown
        } filetype_t;

        static const std::string& to_str(filetype_t ft);
        static filetype_t determine_filetype(std::istream& is);
        static filetype_t determine_filetype(const std::string& filename);
        static filetype_t determine_filetype(const wxFileName& filename);

    private:
        void InitializeScripting();
        void RegisterType(const wxString& name, const wxString& regex, const wxString& eval_func, const wxString& parse_func);

        // win/Cygwin platform checking
        void DetectCygwinMount(void);
        void CorrectCygwinPath(wxString& path);

        // Process one Command
        void ProcessCommand(const Opt& optline);

        // Status Handling
        bool ParseStatus(const Opt& optline);
        void UpdateEnabledTools();


        bool m_CygwinPresent;
        wxString m_CygdrivePrefix;

        // Program is "running": after a "run" or a "start", and before "kill" or a "quit"
        bool m_IsStarted;

        // for project target options
        ProjectBuildTarget* m_pTarget;

        ProjectTargetOptions m_ProjTargetOpt;
        compilervendor_t m_compilervendor;
        wxFileName m_BinFile;
        wxFileName m_SymFile;

        bool m_IsInitializing;
        bool m_LiveUpdate;

        cb::shared_ptr<AXSRegister> m_registers;

        wxString m_AxsdbVersion;
        cb::shared_ptr<wxProgressDialog> m_ProgDlg;

        class CPUTracePanel : public wxPanel {
        public:
            CPUTracePanel(wxWindow *parent, AXS_driver *driver);
            wxWindow *GetWindow() { return this; }
            void Add(const wxString& type, time_t sec, unsigned int usec);
            void Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr);
            void Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
                     const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm);
            void Add(const wxString& type, time_t sec, unsigned int usec, const wxString& chr);
            unsigned int GetMode(void);
            unsigned int GetBuffer(void) const;

        protected:
            void OnModeChange(wxCommandEvent& event);
            void OnHistoryChange(wxSpinEvent& event);
            void OnFetchClicked(wxCommandEvent& event);
            void OnClearClicked(wxCommandEvent& event);
            void OnSaveClicked(wxCommandEvent& event);
            void OnListDoubleClick(wxListEvent& event);

        private:
            DECLARE_EVENT_TABLE();
        private:
            class myListCtrl : public wxListCtrl {
            public:
		myListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize, long style = wxLC_ICON,
                           const wxValidator& validator = wxDefaultValidator, const wxString& name = wxListCtrlNameStr);
                virtual wxString OnGetItemText(long item, long column) const;
                void Add(const wxString& type, time_t sec, unsigned int usec);
                void Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr);
                void Add(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
                         const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm);
                void Add(const wxString& type, time_t sec, unsigned int usec, const wxString& chr);
                int SaveCSV(const wxString& filename) const;
                void Clear(void);
                void SetBufferLength(unsigned int len);
                unsigned int GetBufferLength(void) const;
                std::pair<wxString,unsigned int> GetSourceInfo(long item) const;

            protected:
                class Entry {
                public:
                    Entry(const wxString& type, time_t sec, unsigned int usec);
                    Entry(const wxString& type, time_t sec, unsigned int usec, unsigned int addr);
                    Entry(const wxString& type, time_t sec, unsigned int usec, unsigned int addr,
                          const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm);
                    Entry(const wxString& type, time_t sec, unsigned int usec, const wxString& chr);
                    wxString GetCol(unsigned int col) const;
                    wxString GetCSV(void) const;
                    const wxString& get_type(void) const { return m_type; }
                    const wxString& get_name(void) const { return m_name; }
                    time_t get_sec(void) const { return m_sec; }
                    unsigned int get_usec(void) const { return m_usec; }
                    unsigned int get_addr(void) const { return m_addr; }
                    unsigned int get_line(void) const { return m_line; }
                    unsigned int get_level(void) const { return m_level; }
                    unsigned int get_block(void) const { return m_block; }
                    bool has_addr(void) const { return m_mode == mode_addr || m_mode == mode_addrsourceline; }
                    bool has_sourceline(void) const { return m_mode == mode_addrsourceline; }
                    bool has_char(void) const { return m_mode == mode_char; }
                    bool is_asm(void) const { return m_isasm; }
                protected:
                    wxString m_type;
                    wxString m_name;
                    time_t m_sec;
                    unsigned int m_usec;
                    unsigned int m_addr;
                    unsigned int m_line;
                    unsigned int m_level;
                    unsigned int m_block;
                    typedef enum {
                        mode_addr,
                        mode_addrsourceline,
                        mode_char,
                        mode_misc
                    } mode_t;
                    mode_t m_mode;
                    bool m_isasm;
                };

                void Add(const Entry& e);
                void OnTimer(wxTimerEvent& evt);

                DECLARE_EVENT_TABLE();

                typedef std::vector<Entry> buffer_t;
                buffer_t m_buffer;
                unsigned int m_bufwr;
                unsigned int m_bufrd;
                wxTimer m_timer;
            };

            static const wxString modechoices[];

            AXS_driver *m_driver;
            myListCtrl *m_list;
            wxRadioBox *m_enable;
            wxSpinCtrl *m_history;
            wxButton *m_fetch;
            wxButton *m_clear;
            wxButton *m_save;
            bool m_dofetch;
        };

        CPUTracePanel *m_cputracepanel;

        class ProfilerPanel : public wxPanel {
        public:
            typedef enum {
                mode_off = 0,
                mode_c = 1,
                mode_asm = 2,
                mode_c_asm = mode_c | mode_asm
            } mode_t;

            ProfilerPanel(wxWindow *parent, AXS_driver *driver);
            wxWindow *GetWindow() { return this; }
            mode_t GetMode(void) const;
            unsigned int GetSamples(void) const;
            void Add(const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm,
                     unsigned int addr, unsigned int hitcount);
            void Add();

        protected:
            void OnModeChange(wxCommandEvent& event);
            void OnSamplesChange(wxSpinEvent& event);
            void OnListDoubleClick(wxListEvent& event);

        private:
            DECLARE_EVENT_TABLE();
        private:
            class ProfileEntry {
            public:
                ProfileEntry(const wxString& name, unsigned int line, unsigned int level, unsigned int block, bool isasm,
                             unsigned int addr, unsigned int hitcount)
                        : m_name(name), m_line(line), m_level(level), m_block(block), m_addr(addr), m_hitcount(hitcount), m_isasm(isasm) {}
                int compare(const ProfileEntry& x) const;
                bool operator<(const ProfileEntry& x) const { return compare(x) < 0; }
                const wxString& get_name() const { return m_name; }
                unsigned int get_line() const { return m_line; }
                unsigned int get_level() const { return m_level; }
                unsigned int get_block() const { return m_block; }
                unsigned int get_addr() const { return m_addr; }
                unsigned int get_hitcount() const { return m_hitcount; }
                bool is_asm() const { return m_isasm; }
                wxString get_locstr() const;
            protected:
                wxString m_name;
                unsigned int m_line;
                unsigned int m_level;
                unsigned int m_block;
                unsigned int m_addr;
                unsigned int m_hitcount;
                bool m_isasm;
            };

            class SortDescHitcount;

            static const wxString modechoices[];
            typedef std::set<ProfileEntry> profileset_t;
            typedef std::vector<ProfileEntry> profilevector_t;
            profileset_t m_profile;
            profilevector_t m_sortedprofile;
            AXS_driver *m_driver;
            wxListCtrl *m_list;
            wxRadioBox *m_enable;
            wxSpinCtrl *m_samples;
        };

        ProfilerPanel *m_profilerpanel;
}; // AXS_driver

#endif // AXS_DRIVER_H
