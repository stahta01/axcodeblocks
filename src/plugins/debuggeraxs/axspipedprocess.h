/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 */

#ifndef AXSPIPEDPROCESS_H
#define AXSPIPEDPROCESS_H

#include "pipedprocess.h"

/*
 * No description
 */
class AXSPipedProcess : public PipedProcess
{
    public:
        // class constructor
        AXSPipedProcess(PipedProcess** pvThis, wxEvtHandler* parent, int id = wxID_ANY, bool pipe = true, const wxString& dir = wxEmptyString);
        // class destructor
        ~AXSPipedProcess();
        virtual bool HasInput();
        virtual void SendString(const wxString& text);
    protected:

        class ChopLines {
            public:
                ChopLines();
                bool Process(wxInputStream& is, wxEventType evt, wxEvtHandler *evth, int id);

            protected:
                wxString m_data;
                bool m_lastcr;
	};

        ChopLines m_stderr;
        ChopLines m_stdout;
        std::string m_outputbuffer;

        void DoSendString();
};

#endif // AXSPIPEDPROCESS_H
