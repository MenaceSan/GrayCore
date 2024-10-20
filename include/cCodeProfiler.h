//! @file cCodeProfiler.h
//! Declare entry/exit from a function such that it will build a profile.
//! Write out a profile PCP file.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cCodeProfiler_H
#define _INC_cCodeProfiler_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include "cTimeSys.h"

namespace Gray {
/// <summary>
/// profile the entry/exit for a function.
/// This is ALWAYS stack based so its thread safe.
/// </summary>
class GRAYCORE_LINK cCodeProfileFunc {
    friend class cCodeProfilerControl;

    const cDebugSourceLine _Src;  /// Record source location of this function.
    const cTimePerf _nTimeStart;  /// Function enter Start time in system clock ticks

    static bool sm_bActive;  /// are we actively measuring? Thread Safe read.

 private:
    void StopTime() noexcept;

 public:
    cCodeProfileFunc(cDebugSourceLine src) noexcept : _Src(src), _nTimeStart(sm_bActive) {  // Cheat a little and burn off 4 instructions inside counted function time.
        //! record Start/Record cycle count on object construct.
    }
    ~cCodeProfileFunc() {
        if (sm_bActive) {  // inline check for maximum speed.
            StopTime();
        }
    }
};

// cCodeProfileFunc usage requires only single declaration at beginning of function
#if 0  // defined(_DEBUG)
#define CODEPROFILEFUNC() cCodeProfileFunc _tagPROFILE_CLASS(DEBUGSOURCELINE)
#else
#define CODEPROFILEFUNC() __noop  // compile out profiling. Do nothing.
#endif
}  // namespace Gray
#endif
