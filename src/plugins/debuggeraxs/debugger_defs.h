/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DEBUGGER_DEFS_H
#define DEBUGGER_DEFS_H

#include <wx/string.h>
#include <wx/dynarray.h>
#include <deque>
#include <vector>

#include "debuggermanager.h"
#include "machine.h"

class DebuggerDriver;

class Opt;

extern const int DEBUGGER_CURSOR_CHANGED; ///< wxCommandEvent ID fired when the cursor has changed.
extern const int DEBUGGER_SHOW_FILE_LINE; ///< wxCommandEvent ID fired to display a file/line (w/out changing the cursor)

/** Debugger cursor info.
  *
  * Contains info about the debugger's cursor, i.e. where it currently is at
  * (file, line, function, address).
  */
struct Cursor
{
    Cursor() : line(-1), line_end(-1), changed(false) {}
    wxString file;
    wxString address;
    wxString function;
    long int line; ///< If -1, no line info
    long int line_end;
    bool changed;
};

typedef enum {
	cpustate_targetdisconnected,
	cpustate_disconnected,
	cpustate_locked,
	cpustate_halt,
	cpustate_run,
	cpustate_busy
} cpustate_t;

extern const std::string& to_str(cpustate_t cs);

/** Basic interface for debugger commands.
  *
  * Each command sent to the debugger, is a DebuggerCmd.
  * It encapsulates the call and parsing of return values.
  * The most important function is ParseOutput() inside
  * which it must parse the commands output.
  * ParseOutput() can be called multiple times if the debugger
  * outputs multiple lines in response to a command.
  *
  * @remarks This is not an abstract interface, i.e. you can
  * create instances of it. The default implementation just
  * logs the command's output. This way you can debug new commands.
  */
class DebuggerCmd
{
    public:
        DebuggerCmd(DebuggerDriver* driver, bool logToNormalLog = false);
        virtual ~DebuggerCmd();

        /** Executes an action.
          * @param The state.
          *
          * This allows for "dummy" debugger commands to enter the commands queue.
          * You can, for example, leave m_Cmd empty and just have Action()
          * do something GUI-related (like the watches command does).
          * Action() is called when the debugger command becomes current in the
          * commands queue. It is called after sending m_Cmd to the debugger (if not empty).
          */
        virtual void Action();

        /** Parses the command's output.
          * @param The output.
          */
        virtual void ParseOutput(const Opt& output);

        /** Notifies CPU state changes.
          * @param The state.
          */
        virtual void StateChange() {}

        /** Tells if the next command can start running
          * @return true if the command can be executed overlapped with the next
          */
        virtual bool CanOverlap() const { return false; }

        /** If true, this command only starts running if all previous commands have completed
          * @return true if the command requires previous commands to complete before starting
          */
        virtual bool IsBarrier() const { return true; }

        /** Queue a command to the debugger
          * @param opt The command to be queued.
          * @param debugLog Whether to log to debug or normal log.
          * @return the command sequence number
          */
        unsigned int SendCommand(const Opt& opt, bool debugLog = true);

        /** Marks the command as done.
          */
        void Done();

        /** Marks the current subcommand as done.
          */
        void CurrentDone();

	/** Tells if the command is done executing
          * @return true if the command is done
          */
        bool IsDone() const;

	/** Tells if the current subcommand is the last one
          * @return true if the command is done
          */
        bool IsLast() const;

	/** Tells if other subcommands are pending
          * @return true if other subcommands are pending
          */
        bool IsPending() const;

	/** Tells the current cpu state
          * @return the cpu state
          */
        cpustate_t GetState() const { return m_State; }

        /** Parses the command's output.
          * @param The output.
          * @param The preparsed sequence number (for efficiency)
          * @return true if output was consumed
          */
        bool ParseAllOutput(const Opt& output, unsigned int seq);

        /** Executes an action. Keep state locally.
          * @param The state.
          */
        void RunAction(cpustate_t state);

        /** Notifies CPU state changes. Keep state locally.
          * @param The state.
          */
        void RunStateChange(cpustate_t state);

        /** Debugging Info (describe the state)
          */
        wxString DebugInfo();

    protected:
        DebuggerDriver* m_pDriver; ///< the driver
        typedef std::set<unsigned int> cmdseqset_t;
        cmdseqset_t m_CmdSeq; ///< Command Queued Sequence Numbers
        unsigned int m_CurSeq;  ///< current Command Sequence Number (reset to zero if that is all expected command output)
        cpustate_t m_State;  ///< CPU State
        bool m_LogToNormalLog;  ///< if true, log to normal log, else the debug log
        bool m_Running;  ///< if true, the command is running
};

/** Simple command class. */
class DebuggerCmd_Simple : public DebuggerCmd
{
    public:
        DebuggerCmd_Simple(DebuggerDriver* driver, const Opt& opt, bool barrier, bool overlapped, bool debugLog, bool logToNormalLog = false)
            : DebuggerCmd(driver, logToNormalLog), m_cmd(opt), m_debuglog(debugLog), m_barrier(barrier), m_overlapped(overlapped) { }

        void Action() { SendCommand(m_cmd, m_debuglog); }
        bool CanOverlap() const { return m_overlapped; }
        bool IsBarrier() const { return m_barrier; }
        void ParseOutput(const Opt& output) { Done(); }

    protected:
        Opt m_cmd;
        bool m_debuglog;
        bool m_barrier;
        bool m_overlapped;
};

/** Action-only debugger comand to signal the watches tree to update. */
class DbgCmd_UpdateWatchesTree : public DebuggerCmd
{
    public:
        DbgCmd_UpdateWatchesTree(DebuggerDriver* driver);
        virtual ~DbgCmd_UpdateWatchesTree(){}
        virtual void Action();
        bool CanOverlap() const { return true; }
        bool IsBarrier() const { return true; }
};

/** Debugger breakpoint interface.
  *
  * This is the struct used for debugger breakpoints.
  */
////////////////////////////////////////////////////////////////////////////////
struct DebuggerBreakpoint : cbBreakpoint
{
    /** Constructor.
      * Sets default values for members.
      */
    DebuggerBreakpoint()
        : line(0),
        index(-1),
        temporary(false),
        enabled(true),
        active(true),
        useIgnoreCount(false),
        ignoreCount(0),
        useCondition(false),
        wantsCondition(false),
        address(0),
        alreadySet(false),
        userData(0)
    {}

    // from cbBreakpoint
    virtual void SetEnabled(bool flag);
    virtual wxString GetLocation() const;
    virtual int GetLine() const;
    virtual wxString GetLineString() const;
    virtual wxString GetType() const;
    virtual wxString GetInfo() const;
    virtual bool IsEnabled() const;
    virtual bool IsVisibleInEditor() const;
    virtual bool IsTemporary() const;

    wxString filename; ///< The filename for the breakpoint (kept as relative).
    wxString filenameAsPassed; ///< The filename for the breakpoint as passed to the debugger (i.e. full filename).
    int line; ///< The line for the breakpoint.
    long int index; ///< The breakpoint number. Set automatically. *Don't* write to it.
    bool temporary; ///< Is this a temporary (one-shot) breakpoint?
    bool enabled; ///< Is the breakpoint enabled?
    bool active; ///< Is the breakpoint active? (currently unused)
    bool useIgnoreCount; ///< Should this breakpoint be ignored for the first X passes? (@c x == @c ignoreCount)
    int ignoreCount; ///< The number of passes before this breakpoint should hit. @c useIgnoreCount must be true.
    bool useCondition; ///< Should this breakpoint hit only if a specific condition is met?
    bool wantsCondition; ///< Evaluate condition for pending breakpoints at first stop !
    wxString condition; ///< The condition that must be met for the breakpoint to hit. @c useCondition must be true.
    wxString func; ///< The function to set the breakpoint. If this is set, it is preferred over the filename/line combination.
    unsigned long int address; ///< The actual breakpoint address. This is read back from the debugger. *Don't* write to it.
    bool alreadySet; ///< Is this already set? Used to mark temporary breakpoints for removal.
    wxString lineText; ///< Optionally, the breakpoint line's text (used by GDB for setting breapoints on ctors/dtors).
    void* userData; ///< Custom user data.
};
typedef std::deque<cb::shared_ptr<DebuggerBreakpoint> > BreakpointsList;

/** Watch variable format.
  *
  * @note not all formats are implemented for all debugger drivers.
  */
enum WatchFormat
{
    Undefined = 0, ///< Format is undefined (whatever the debugger uses by default).
    Decimal, ///< Variable should be displayed as decimal.
    Unsigned, ///< Variable should be displayed as unsigned.
    Hex, ///< Variable should be displayed as hexadecimal (e.g. 0xFFFFFFFF).
    Binary, ///< Variable should be displayed as binary (e.g. 00011001).
    Char, ///< Variable should be displayed as a single character (e.g. 'x').
    Float, ///< Variable should be displayed as floating point number (e.g. 14.35)
    String, ///< Variable should be displayed as string

    // do not remove these
    Last, ///< used for iterations
    Any ///< used for watches searches
};

enum WatchType
{
    TypeVoid = 0,
    TypeSimple,
    TypeBitfield,
    TypeStruct,
    TypePointer,
    TypeArray
};

class GDBWatch : public cbWatch
{
    public:
        GDBWatch(wxString const &symbol);
        virtual ~GDBWatch();
    public:

        virtual void GetSymbol(wxString &symbol) const;
        virtual void GetValue(wxString &value) const;
        virtual bool SetValue(const wxString &value);
        virtual void GetFullWatchString(wxString &full_watch) const;
        virtual void GetType(wxString &type) const;
        virtual void SetType(const wxString &type);
        virtual void GetAddrSpace(wxString &as) const;
        virtual void SetAddrSpace(const wxString &as);
        virtual void GetAddr(wxString &addr) const;
        virtual void SetAddr(const wxString &addr);
        virtual bool IsDisabled() const;
        virtual bool IsReadonly() const;

        virtual wxString const & GetDebugString() const;
    public:
        void SetDebugValue(wxString const &value);
        void SetSymbol(const wxString& symbol);

        void SetFormat(WatchFormat format);
        WatchFormat GetFormat() const;
        void SetWatchType(WatchType type);
        WatchType GetWatchType() const;

        void SetArrayParams(int start, int count);
        int GetArrayStart() const;
        int GetArrayCount() const;

        void SetBitSize(unsigned int bitsize);
        unsigned int GetBitSize() const;

        void SetForTooltip(bool flag = true);
        bool GetForTooltip() const;

        void SetDisabled(bool flag = false);
        void SetDisabledRecursive(bool flag = false);

        void ClearEnums(void);
        void AddEnum(unsigned long val, const wxString& name);
        bool FindEnum(wxString& name, unsigned long val) const;
        bool FindEnum(unsigned long& val, const wxString& name) const;

    protected:
        virtual void DoDestroy();

    private:
        wxString m_symbol;
        wxString m_type;
        wxString m_raw_value;
        wxString m_debug_value;
        wxString m_addrspace;
        wxString m_addr;
        typedef std::map<unsigned long,wxString> enumvalues_t;
        enumvalues_t m_enumvalues;
        WatchFormat m_format;
        WatchType m_watchtype;
        int m_array_start;
        int m_array_count;
        unsigned int m_bitsize;
        bool m_forTooltip;
        bool m_disabled;
    };

typedef std::vector<cb::shared_ptr<GDBWatch> > WatchesContainer;

/** Stack frame.
  *
  * This keeps info about a specific stack frame.
  */
struct oldStackFrame
{
    oldStackFrame() : valid(false), number(0), address(0) {}
    /** Clear everything. */
    void Clear()
    {
        valid = false;
        number = 0;
        address = 0;
        function.Clear();
        file.Clear();
        line.Clear();
    }
    bool valid; ///< Is this stack frame valid?
    unsigned long int number; ///< Stack frame's number (used in backtraces).
    unsigned long int address; ///< Stack frame's address.
    wxString function; ///< Current function name.
    wxString file; ///< Current file.
    wxString line; ///< Current line in file.
};

class AXSRegister : public cbRegister
{
    public:
        AXSRegister(wxString const &name, wxString const &desc = wxEmptyString);
        AXSRegister(wxString const &name, unsigned int bitlength, const wxString &addrspace, uint16_t addr, uint16_t writemask, bool readsafe, wxString const &desc = wxEmptyString);
        virtual ~AXSRegister();
    public:
        void GetName(wxString &name) const;
        void GetDescription(wxString &desc) const;
        void GetValue(wxString &value) const;
        void GetValueAlt(wxString &value) const;
        void GetWriteMask(wxString &mask) const;
        void GetAddrSpace(wxString &as) const;
        void GetAddr(wxString &addr) const;
        bool IsCategory() const { return !m_bitlength; }
        bool IsOutdated() const { return m_outdated; }
        bool IsReadonly() const { return !m_writemask; }
        bool IsReadSafe() const { return m_readsafe; }
        bool IsPC() const { return m_addrspace.IsEmpty(); }

        uint16_t GetAddr() const { return m_addr; }
        uint16_t GetValue() const { return m_value; }
        uint16_t GetWriteMask() const { return m_writemask; }
        unsigned int GetBitLength() const { return m_bitlength; }
        void SetValue(uint16_t val);
        void SetOutdated(bool outdated = true);
        void SetChildrenOutdated(bool outdated = true);
        void SetReadSafe(bool rs = true);
        void SetBitLength(unsigned int bl = 8);
        void SetWriteMask(uint16_t writemask = 0xffff);
        void SetAddrSpace(const wxString &as);
        void SetAddr(uint16_t addr);
        void SetDescription(const wxString &desc);


        wxString const & GetDebugString() const;

    protected:
        wxString m_name;
        wxString m_desc;
        wxString m_addrspace;
        uint16_t m_addr;
        uint16_t m_writemask;
        uint16_t m_value;
        uint8_t m_bitlength;
        bool m_readsafe;
        bool m_outdated;
};

#endif // DEBUGGER_DEFS_H
