/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DEBUGGEROPTIONSDLG_H
#define DEBUGGEROPTIONSDLG_H

#include <debuggermanager.h>
#include <cbdebugger_interfaces.h>

class ConfigManagerWrapper;

class DebuggerConfiguration : public cbDebuggerConfiguration
{
    public:
        explicit DebuggerConfiguration(const ConfigManagerWrapper &config);

        virtual cbDebuggerConfiguration* Clone() const;
        virtual wxPanel* MakePanel(wxWindow *parent);
        virtual bool SaveChanges(wxPanel *panel);
    public:
        enum Flags
        {
            CommandsSynchronous,
            EvalExpression,
            AddOtherProjectDirs,
            DoNotRun,
            LiveUpdate
        };

        bool GetFlag(Flags flag);
        wxString GetDebuggerExecutable(bool expandMarco = true);
        wxString GetInitCommands();
        wxString GetDebugLinkFunctionKey(axs_cbDbgLink::FuncKey_t key, bool expandEscape = true);

    private:
        static const wxString FuncKeyXMLName[];
        static const wxString FuncKeyFieldName[];
        static const wxString FuncKeyDefault[];
};

#endif // DEBUGGEROPTIONSDLG_H
