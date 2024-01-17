//
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
    WORD m_wSize;          /// Number of bytes in this record
    WORD m_uLine;          /// Line number in source file
    UINT_PTR m_ProcessID;  /// Current process ID PROCESSID_t
    UINT_PTR m_ThreadID;   /// Current thread ID THREADID_t
    UINT64 m_Cycles;       /// how many cycles inside this? TIMEPERF_t
                           // ?? INT64 m_Time;		/// What did this start absolutely ? (so sub stuff can be removed.)

    // File name string. 0 term
    // Func/Function name string. 0 term
};
#pragma pack(pop)

/// <summary>
/// Thread locked singleton stream to write out to. Can be shared by multiple threads.
/// </summary>
class GRAYCORE_LINK cCodeProfilerControl : public cSingleton<cCodeProfilerControl> {
    friend class cCodeProfileFunc;
    friend class cSingleton<cCodeProfilerControl>;

 protected:
    cFile m_File;
    mutable cThreadLockCount m_Lock;
    PROCESSID_t m_ProcessId;

 public:
    cCodeProfilerControl() : cSingleton<cCodeProfilerControl>(this, typeid(cCodeProfilerControl)), m_ProcessId(cAppState::get_CurrentProcessId()) {}
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

 protected:
    bool StartTime() {
        cThreadGuard lock(m_Lock);
        cCodeProfileFunc::sm_bActive = true;
        if (m_File.isValidHandle()) return true;
        // open it. PCP file.
        HRESULT hRes = m_File.OpenX(_FN("profile.pcp"), OF_CREATE | OF_SHARE_DENY_NONE | OF_WRITE | OF_BINARY);  // append
        if (FAILED(hRes)) return false;
        m_File.SeekToEnd();  // Start at end of file
        return true;
    }

    void StopTime() {
        cThreadGuard lock(m_Lock);
        cCodeProfileFunc::sm_bActive = false;
        m_File.Close();
    }

    void WriteTime(const cTimePerf& nTimeEnd, const class cCodeProfileFunc& Func) noexcept {
        // cThreadLockableRef
        DEBUG_ASSERT(get_Active(), "Active");
        if (!m_File.isValidHandle())  
            return;       
        cThreadGuard lock(m_Lock);
        if (!m_File.isValidHandle())  
            return;
        
        const StrLen_t iLenFile = StrT::Len(Func.m_src.m_pszFile);
        const StrLen_t iLenFunc = StrT::Len(Func.m_src.m_pszFunction);
        const StrLen_t iLenTotal = sizeof(cCodeProfilerItem) + iLenFile + iLenFunc + 2;

        BYTE Tmp[sizeof(cCodeProfilerItem) + 1024]; // pack binary blob.
        DEBUG_ASSERT(iLenTotal < (StrLen_t)sizeof(Tmp), "LenTotal");

        cCodeProfilerItem* pLogItem = reinterpret_cast<cCodeProfilerItem*>(Tmp);
        pLogItem->m_wSize = CastN(WORD, iLenTotal);
        pLogItem->m_uLine = Func.m_src.m_uLine;
        pLogItem->m_ProcessID = m_ProcessId;
        pLogItem->m_ThreadID = cThreadId::GetCurrentId();
        pLogItem->m_Cycles = nTimeEnd.m_nTime - Func.m_nTimeStart.m_nTime;
        // pLogItem->m_Time = nTime;

        cMem::Copy(Tmp + sizeof(cCodeProfilerItem), Func.m_src.m_pszFile, iLenFile + 1);    // include '\0'
        cMem::Copy(Tmp + sizeof(cCodeProfilerItem) + iLenFile + 1, Func.m_src.m_pszFunction, iLenFunc + 1); // include '\0'

        m_File.WriteX(Tmp, iLenTotal);
    }
};

void cCodeProfileFunc::StopTime() noexcept {
    //! Record time when this object is destroyed.
    cTimePerf nTimeEnd(true);  // End cycle count
    DEBUG_ASSERT(sm_bActive, "bActive");
    DEBUG_ASSERT(m_nTimeStart.isTimeValid(), "nTimeStart");
    // Write to log file
    cCodeProfilerControl* pController = cCodeProfilerControl::get_Single();
    pController->WriteTime(nTimeEnd, *this);
}
}  // namespace Gray
