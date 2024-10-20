//! @file cCodeProfiler.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cCodeProfiler.h"
#include "cDebugAssert.h"
#include "cFile.h"
#include "cString.h"
#include "cThreadLock.h"

namespace Gray {

bool cCodeProfileFunc::sm_bActive = false;  // static

#pragma pack(push, 2)
/// <summary>
/// profile log file item struct
/// </summary>
struct CATTR_PACKED cCodeProfilerItem {
    WORD _SizeThis;          /// Number of bytes in this record
    WORD _uLine;          /// Line number in source file
    UINT_PTR _nProcessId;  /// Current process ID PROCESSID_t
    UINT_PTR _nThreadId;   /// Current thread ID THREADID_t
    UINT64 _nCycles;       /// how many cycles inside this? TIMEPERF_t
                           // ?? INT64 _TimePerf;		/// What did this start absolutely ? (so sub stuff can be removed.)

    // File name string. 0 term
    // Func/Function name string. 0 term
};
#pragma pack(pop)

/// <summary>
/// Thread locked singleton stream to write out to. Can be shared by multiple threads.
/// </summary>
class GRAYCORE_LINK cCodeProfilerControl final : public cSingleton<cCodeProfilerControl> {
    friend class cCodeProfileFunc;

 protected:
    cFile _File;
    mutable cThreadLockableX _Lock;
    PROCESSID_t _nProcessId;

 protected:
    DECLARE_cSingleton(cCodeProfilerControl);
    cCodeProfilerControl() : cSingleton<cCodeProfilerControl>(this), _nProcessId(cAppState::get_CurrentProcessId()) {}

    bool StartTime() {
        const auto guard(_Lock.Lock());
        cCodeProfileFunc::sm_bActive = true;
        if (_File.isValidHandle()) return true;
        // open it. PCP file.
        HRESULT hRes = _File.OpenX(_FN("profile.pcp"), OF_CREATE | OF_SHARE_DENY_NONE | OF_WRITE | OF_BINARY);  // append
        if (FAILED(hRes)) return false;
        _File.SeekToEnd();  // Start at end of file
        return true;
    }

    void StopTime() {
        const auto guard(_Lock.Lock());
        cCodeProfileFunc::sm_bActive = false;
        _File.Close();
    }

    void WriteTime(const cTimePerf& nTimeEnd, const class cCodeProfileFunc& Func) noexcept {
        DEBUG_ASSERT(get_Active(), "Active");
        if (!_File.isValidHandle()) return;
        const auto guard(_Lock.Lock());
        if (!_File.isValidHandle()) return;

        const StrLen_t iLenFile = StrT::Len(Func._Src._pszFile);
        const StrLen_t iLenFunc = StrT::Len(Func._Src._pszFunction);
        const StrLen_t iLenTotal = sizeof(cCodeProfilerItem) + iLenFile + iLenFunc + 2;

        BYTE tmp[sizeof(cCodeProfilerItem) + 1024];  // pack binary blob.
        DEBUG_ASSERT(iLenTotal < (StrLen_t)sizeof(tmp), "LenTotal");

        cCodeProfilerItem* pLogItem = reinterpret_cast<cCodeProfilerItem*>(tmp);
        pLogItem->_SizeThis = CastN(WORD, iLenTotal);
        pLogItem->_uLine = Func._Src._uLine;
        pLogItem->_nProcessId = _nProcessId;
        pLogItem->_nThreadId = cThreadId::GetCurrentId();
        pLogItem->_nCycles = nTimeEnd._nTimePerf - Func._nTimeStart._nTimePerf;
        // pLogItem->_TimePerf = nTime;

        cMem::Copy(tmp + sizeof(cCodeProfilerItem), Func._Src._pszFile, iLenFile + 1);                     // include '\0'
        cMem::Copy(tmp + sizeof(cCodeProfilerItem) + iLenFile + 1, Func._Src._pszFunction, iLenFunc + 1);  // include '\0'

        _File.WriteX(cMemSpan(tmp, iLenTotal));
    }

 public:
    ~cCodeProfilerControl() {
        StopTime();
    }
    bool get_Active() const {
        return cCodeProfileFunc::sm_bActive;
    }

    bool put_Active(bool bActive) {
        if (bActive == get_Active()) return true;
        if (bActive) return StartTime();
        StopTime();
        return true;
    }
};
cSingleton_IMPL(cCodeProfilerControl);

void cCodeProfileFunc::StopTime() noexcept {
    //! Record time when this object is destroyed.
    cTimePerf nTimeEnd(true);  // End cycle count
    DEBUG_ASSERT(sm_bActive, "bActive");
    DEBUG_ASSERT(_nTimeStart.isTimeValid(), "nTimeStart");
    // Write to log file
    cCodeProfilerControl* pController = cCodeProfilerControl::get_Single();
    pController->WriteTime(nTimeEnd, *this);
}
}  // namespace Gray
