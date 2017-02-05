/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef PROJTARGETOPTIONS_H
#define PROJTARGETOPTIONS_H

#include <stdint.h>
#include <map>
#include <set>
#include <wx/string.h>

class ProjectBuildTarget;

// per-target remote debugging support
struct ProjectTargetOptions
{
    enum FlashEraseType
    {
        BulkErase = 0,
        AllSectorErase,
        NeededSectorErase
    };
    typedef uint64_t key_t;
    typedef std::set<key_t> additionalKeys_t;

    ProjectTargetOptions() : flashErase(BulkErase), fillBreakpoints(false), key(defaultKey) {}

    bool IsOk() const
    {
        return true;
    }

    void MergeWith(const ProjectTargetOptions& other)
    {
        if (other.IsOk())
        {
            flashErase = std::min(flashErase, other.flashErase);
            fillBreakpoints = fillBreakpoints || other.fillBreakpoints;
            additionalKeys.insert(other.additionalKeys.begin(), other.additionalKeys.end());
            additionalKeys.insert(key);
            key = other.key;
            additionalKeys_t::iterator ki(additionalKeys.find(key));
            if (ki != additionalKeys.end())
                additionalKeys.erase(ki);
            ki = additionalKeys.find(defaultKey);
            if (ki != additionalKeys.end())
                additionalKeys.erase(ki);
        }
    }

    static const key_t defaultKey = ~0ULL;

    FlashEraseType flashErase;
    bool fillBreakpoints;
    key_t key;
    additionalKeys_t additionalKeys;
};

typedef std::map<ProjectBuildTarget*, ProjectTargetOptions> ProjectTargetOptionsMap;


#endif // PROJTARGETOPTIONS_H

