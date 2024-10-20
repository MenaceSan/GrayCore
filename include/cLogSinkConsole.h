//! @file cLogSinkConsole.h
//! specific log sink/destinations/appenders
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cLogSinkConsole_H
#define _INC_cLogSinkConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayString.h"
#include "cLogSink.h"

#if !defined(UNDER_CE)

namespace Gray {
/// <summary>
/// Forward debug statements to the console (cAppConsole) (if i have one)
/// No filter and take default formatted string
/// </summary>
class GRAYCORE_LINK cLogSinkConsole : public cLogSink, public cRefBase {
    typedef cLogSink SUPER_t;

 protected:
    cLogSinkConsole() {}
 
 public:
    /// <summary>
    /// write raw log/debug string to the console stderr. but maybe stdout ??
    /// http://www.jstorimer.com/blogs/workingwithcode/7766119-when-to-use-stderr-instead-of-stdout
    /// </summary>
    /// <param name="pszMsg"></param>
    /// <returns>StrLen_t</returns>
    HRESULT WriteString(const LOGCHAR_t* pszMsg) override;
    HRESULT FlushLogs() override;

    HRESULT addEvent(cLogEvent& rEvent) noexcept override;

    static cLogSinkConsole* GRAYCALL AddSinkCheck(cLogNexus* pLogger = nullptr, bool bAttachElseAlloc = false);
    static bool GRAYCALL RemoveSinkCheck(cLogNexus* pLogger, bool bOnlyIfParent);
    static HRESULT GRAYCALL WaitForDebugger();

    IUNKNOWN_DISAMBIG(cRefBase)
};

/// <summary>
/// Just put the log messages in an array of strings in memory.
/// </summary>
struct GRAYCORE_LINK cLogSinkTextArray : public cLogSink, public cRefBase {
    cArrayString<LOGCHAR_t> _aMsgs;
    const ITERATE_t _nMaxMsgs = SHRT_MAX;  /// Store max this many messages.

    cLogSinkTextArray(ITERATE_t nMaxMsgs = SHRT_MAX) noexcept : _nMaxMsgs(nMaxMsgs) {}
 
    HRESULT WriteLine(const LOGCHAR_t* pszMsg) {
        if (StrT::IsNullOrEmpty(pszMsg)) return 0;
        if (_aMsgs.GetSize() >= _nMaxMsgs) return 0;
        _aMsgs.Add(pszMsg);
        return 1;
    }
    IUNKNOWN_DISAMBIG(cRefBase)
};
}  // namespace Gray
#endif
#endif  // _INC_cLogSinkConsole_H
