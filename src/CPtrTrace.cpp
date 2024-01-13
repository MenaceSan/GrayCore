//
//! @file cPtrTrace.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAppState.h"
#include "cIUnkPtr.h"
#include "cLogMgr.h"
#include "cPtrTrace.h"
#include "cPtrTraceMgr.h"
#include "cSingleton.h"
#include "cThreadArray.h"

#ifdef __linux__
// _uuidof(IUnknown) = "00000000-0000-0000-C000-000000000046"
GRAYCORE_LINK GUID IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};  // access the GUID value via ref to this.
#endif

namespace Gray {
int cPtrTraceMgr::TraceDump(cLogProcessor* pLog, ITERATE_t iCountExpected) {  // virtual
    //! Dump all the IUnks that are left not released !!!

    cThreadGuard threadguard(m_Lock);  // thread sync critical section.
    const ITERATE_t iCount = m_aTraces.GetSize();
    int iLockCountTotal = 0;

    for (ITERATE_t i = 0; i < iCount; i++) {
        const cPtrTraceEntry& entry = m_aTraces.GetAt(i);

        IUnknown* p1 = entry._pIUnk;  // The stored pointer. should be valid!
        ASSERT(p1 != nullptr);

        // pTrace -> cPtrFacade
        const int iLockCount2 = p1->AddRef() - 1;
        ASSERT(iLockCount2 >= 1);
        p1->Release();

        if (pLog != nullptr) {
            pLog->addInfoF("IUnknown=0%x, Locks=%d, Type=%s, File='%s',%d", PtrCastToNum(p1), iLockCount2, LOGSTR(entry.m_TypeInfo->name()), LOGSTR(entry.m_Src.m_pszFile), entry.m_Src.m_uLine);
        }

        iLockCountTotal += iLockCount2;
    }
    if (pLog != nullptr) {
        pLog->addEventF(LOG_ATTR_DEBUG, (iCount == iCountExpected) ? LOGLVL_t::_INFO : LOGLVL_t::_ERROR, "IUnk Dump of %d objects %d locks (of %d expected).", iCount, iLockCountTotal, iCountExpected);
    }
    return iLockCountTotal;
}

//****************************************************

bool cPtrTrace::sm_bActive = false;

UINT_PTR GRAYCALL cPtrTrace::TraceAttachX(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, const cDebugSourceLine* src) {  // static
    if (!sm_bActive)                                                                                                   // not tracking this now.
        return 0;
    if (cAppState::isInCExit())  // can't track this here. Must release all before app destruction.
        return 0;
    ASSERT_NN(pIUnk);
    auto& mgr = cPtrTraceMgr::I();
    cThreadGuard threadguard(mgr.m_Lock);  // thread sync critical section.

    UINT_PTR id = ++mgr._TraceIdLast;
    if (src)
        mgr.m_aTraces.Add(cPtrTraceEntry(typeInfo, pIUnk, id, *src));
    else
        mgr.m_aTraces.Add(cPtrTraceEntry(typeInfo, pIUnk, id));
    return id;
}

void GRAYCALL cPtrTrace::TraceUpdateX(UINT_PTR id, const cDebugSourceLine& src) noexcept {
    ASSERT(id);
    auto& mgr = cPtrTraceMgr::I();
    cThreadGuard threadguard(mgr.m_Lock);  // thread sync critical section.
    const ITERATE_t index = mgr.m_aTraces.FindIForKey(id);
    if (index < 0) return;
    mgr.m_aTraces.ElementAt(index).m_Src = src;
}

void GRAYCALL cPtrTrace::TraceReleaseX(UINT_PTR id) {
    //! Called when cPtrTrace is destroyed.
    ASSERT(id);
    if (cAppState::isInCExit()) {  // can't track this here.
        sm_bActive = false;
        return;
    }
    auto& mgr = cPtrTraceMgr::I();
    cThreadGuard threadguard(mgr.m_Lock);  // thread sync critical section.
    bool ret = mgr.m_aTraces.RemoveKey(id);
    ASSERT(ret);
}
}  // namespace Gray
