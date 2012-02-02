/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>

#ifndef CB_PRECOMP
    #include <wx/string.h>

    #include <cbproject.h>
    #include <projectfile.h>
#endif

#include "parserthreadedtask.h"

#include "cclogger.h"
#include "parser.h"
#include "tokenstree.h"

#define CC_PARSERTHREADEDTASK_DEBUG_OUTPUT 0

#if CC_GLOBAL_DEBUG_OUTPUT == 1
    #undef CC_PARSERTHREADEDTASK_DEBUG_OUTPUT
    #define CC_PARSERTHREADEDTASK_DEBUG_OUTPUT 1
#elif CC_GLOBAL_DEBUG_OUTPUT == 2
    #undef CC_PARSERTHREADEDTASK_DEBUG_OUTPUT
    #define CC_PARSERTHREADEDTASK_DEBUG_OUTPUT 2
#endif

#if CC_PARSERTHREADEDTASK_DEBUG_OUTPUT == 1
    #define TRACE(format, args...) \
        CCLogger::Get()->DebugLog(F(format, ##args))
    #define TRACE2(format, args...)
#elif CC_PARSERTHREADEDTASK_DEBUG_OUTPUT == 2
    #define TRACE(format, args...)                            \
        do                                                    \
        {                                                     \
            if (g_EnableDebugTrace)                           \
                CCLogger::Get()->DebugLog(F(format, ##args)); \
        }                                                     \
        while (false)
    #define TRACE2(format, args...) \
        CCLogger::Get()->DebugLog(F(format, ##args))
#else
    #define TRACE(format, args...)
    #define TRACE2(format, args...)
#endif

// class ParserThreadedTask

ParserThreadedTask::ParserThreadedTask(Parser* parser, wxCriticalSection& parserCS) :
    m_Parser(parser),
    m_ParserCritical(parserCS)
{
}

int ParserThreadedTask::Execute()
{
    CC_LOCKER_TRACK_CS_ENTER(m_ParserCritical)

    wxString   preDefs(m_Parser->m_PredefinedMacros);
    StringList priorityHeaders(m_Parser->m_PriorityHeaders);
    StringList batchFiles(m_Parser->m_BatchParseFiles);

    CC_LOCKER_TRACK_CS_LEAVE(m_ParserCritical);

    if (!preDefs.IsEmpty())
        m_Parser->ParseBuffer(preDefs, false, false);

    CC_LOCKER_TRACK_CS_ENTER(m_ParserCritical)

    m_Parser->m_PredefinedMacros.Clear();
    m_Parser->m_IsPriority = true;

    CC_LOCKER_TRACK_CS_LEAVE(m_ParserCritical);

    while (!priorityHeaders.empty())
    {
        m_Parser->Parse(priorityHeaders.front());
        priorityHeaders.pop_front();
    }

    CC_LOCKER_TRACK_CS_ENTER(m_ParserCritical)

    m_Parser->m_PriorityHeaders.clear();
    m_Parser->m_IsPriority = false;

    if (m_Parser->m_IgnoreThreadEvents)
        m_Parser->m_IsFirstBatch = true;

    CC_LOCKER_TRACK_CS_LEAVE(m_ParserCritical);

    while (!batchFiles.empty())
    {
        m_Parser->Parse(batchFiles.front());
        batchFiles.pop_front();
    }

    CC_LOCKER_TRACK_CS_ENTER(m_ParserCritical)

    m_Parser->m_BatchParseFiles.clear();

    if (m_Parser->m_IgnoreThreadEvents)
    {
        m_Parser->m_IgnoreThreadEvents = false;
        m_Parser->m_IsParsing = true;
    }

    CC_LOCKER_TRACK_CS_LEAVE(m_ParserCritical);

    return 0;
}

// class MarkFileAsLocalThreadedTask

MarkFileAsLocalThreadedTask::MarkFileAsLocalThreadedTask(Parser* parser, cbProject* project) :
    m_Parser(parser), m_Project(project)
{
}

int MarkFileAsLocalThreadedTask::Execute()
{
    // mark all project files as local
    for (FilesList::iterator it  = m_Project->GetFilesList().begin();
                             it != m_Project->GetFilesList().end(); ++it)
    {
        ProjectFile* pf = *it;
        if (!pf)
            continue;

        if (ParserCommon::FileType(pf->relativeFilename) != ParserCommon::ftOther)
        {
            CC_LOCKER_TRACK_CS_ENTER(s_TokensTreeCritical)

            m_Parser->GetTokensTree()->MarkFileTokensAsLocal(pf->file.GetFullPath(), true, m_Project);

            CC_LOCKER_TRACK_CS_LEAVE(s_TokensTreeCritical);
        }
    }

    return 0;
}