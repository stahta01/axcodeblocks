/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk_precomp.h"

#include "axspipedprocess.h"

#include <iostream>

AXSPipedProcess::ChopLines::ChopLines()
    : m_lastcr(false)
{
}

bool AXSPipedProcess::ChopLines::Process(wxInputStream& is, wxEventType evt, wxEvtHandler *evth, int id)
{
    while (is.CanRead())
    {
        char buf[4096];
        is.Read(&buf, sizeof(buf));
        size_t len(is.LastRead());
        m_data.Alloc(m_data.Len() + len);
        for (size_t i = 0; i < len; ++i)
            m_data += buf[i];
    }
    if (!evth)
        return false;
    size_t l(m_data.Len());
    if (!l)
        return false;
    size_t i1(0);
    if (m_lastcr && m_data[i1] == '\n')
	++i1;
    size_t i2(i1);
    bool eol(false);
    for (; i2 < l; ++i2)
    {
        if (m_data[i2] == '\r')
        {
       	    m_lastcr = true;
            eol = true;
            break;
        }
        if (m_data[i2] == '\n')
        {
       	    m_lastcr = false;
            eol = true;
            break;
        }
    }
    if (!eol)
        return false;
    CodeBlocksEvent event(evt, id);
    event.SetString(m_data.Mid(i1, i2 - i1));
    if (false)
        std::cout << "debugger rx: \"" << (char *)event.GetString().char_str() << "\"" << std::endl;
#if 0 && defined(__WXMSW__)
    OutputDebugStringA((wxT("< ") + event.GetString()).char_str());
#endif
    wxPostEvent(evth, event);
//    m_Parent->ProcessEvent(event);
    m_data.Remove(0, i2 + 1);
    return true;
}

// class constructor
AXSPipedProcess::AXSPipedProcess(PipedProcess** pvThis, wxEvtHandler* parent, int id, bool pipe, const wxString& dir)
    : PipedProcess(pvThis, parent, id, pipe, dir)
{
}

// class destructor
AXSPipedProcess::~AXSPipedProcess()
{
    // insert your code here
}

bool AXSPipedProcess::HasInput()
{
    DoSendString();
    if (m_stderr.Process(*GetErrorStream(), cbEVT_PIPEDPROCESS_STDERR, m_Parent, m_Id))
        return true;
    if (m_stdout.Process(*GetInputStream(), cbEVT_PIPEDPROCESS_STDOUT, m_Parent, m_Id))
        return true;
    return false;
}

void AXSPipedProcess::SendString(const wxString& text)
{
    //Manager::Get()->GetLogManager()->Log(m_PageIndex, cmd);
    m_outputbuffer += std::string(text.mb_str()) + "\n";
    DoSendString();
}

void AXSPipedProcess::DoSendString()
{
    if (m_outputbuffer.empty())
        return;
    wxOutputStream* pOut = GetOutputStream();
    if (!pOut)
    {
        m_outputbuffer.clear();
        return;
    }
    pOut->Write(m_outputbuffer.c_str(), m_outputbuffer.length());
    if (pOut->GetLastError() != wxSTREAM_NO_ERROR)
    {
#if 0 && defined(__WXMSW__)
        OutputDebugStringA(">> ERROR: Clearing Queue");
#endif
        if (false)
            std::cout << "debugger tx: clearing queue due error: " << pOut->GetLastError() << std::endl;
        m_outputbuffer.clear();
        return;
    }
    size_t sz(pOut->LastWrite());
    if (sz)
    {
#if 0 && defined(__WXMSW__)
        OutputDebugStringA(("> " + std::string(m_outputbuffer, 0, sz)).c_str());
#endif
        if (false)
            std::cout << "debugger tx: \"" << std::string(m_outputbuffer, 0, sz) << "\"" << std::endl;
        m_outputbuffer.erase(m_outputbuffer.begin(), m_outputbuffer.begin() + sz);
    }
}
