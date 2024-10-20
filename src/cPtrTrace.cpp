//! @file cPtrTrace.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
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
cSingleton_IMPL(cPtrTraceMgr);

int cPtrTraceMgr::TraceDump(cLogProcessor* pLog, ITERATE_t iCountExpected) const {
    // Dump all the IUnks that are left not released !!!

    const auto guard(_Lock.Lock());  // thread sync critical section.
    int iLockCountTotal = 0;

    for (const cPtrTraceEntry& entry : _aTraces) {
        ::IUnknown* p1 = entry._pIUnk;  // The stored pointer. should be valid!
        ASSERT_NN(p1);

        // pTrace -> cPtrFacade
        const int iLockCount2 = p1->AddRef() - 1;  // Get other lock counts.
        ASSERT(iLockCount2 >= 1);
        p1->Release();  // This should never free!

        if (pLog != nullptr) {
            pLog->addInfoF("IUnknown=0%x, Locks=%d, Type=%s, File='%s',%d", CastPtrToNum(p1), iLockCount2, LOGSTR(entry._TypeInfo->name()), LOGSTR(entry._Src._pszFile), entry._Src._uLine);
        }

        iLockCountTotal += iLockCount2;  // we may be double counting the same objects?! or we may see references not traced. maybe not a useful stat.
    }
    if (pLog != nullptr) {
        const ITERATE_t iCount = _aTraces.GetSize();
        pLog->addEventF(LOG_ATTR_DEBUG, (iCount == iCountExpected) ? LOGLVL_t::_INFO : LOGLVL_t::_ERROR, "IUnk Dump of %d objects, with %d locks (of %d expected).", iCount, iLockCountTotal, iCountExpected);
    }
    return iLockCountTotal;
}

cArrayStruct<cPtrTraceEntry> cPtrTraceMgr::FindTraces(::IUnknown* p) const {
    cArrayStruct<cPtrTraceEntry> a;
    const auto guard(_Lock.Lock());  // thread sync critical section.
    for (const cPtrTraceEntry& entry : _aTraces) {
        if (entry._pIUnk == p) {
            a.Add(entry);
        }
    }
    return a;
}

//****************************************************

bool cPtrTrace::sm_bActive = false;

UINT_PTR GRAYCALL cPtrTrace::TraceAttachX(const TYPEINFO_t& typeInfo, ::IUnknown* pIUnk, const cDebugSourceLine* pSrc) {  // static
    if (!sm_bActive) return 0;                                                                                            // not tracking this now.
    if (cAppState::isInCExit()) return 0;                                                                                 // can't track this here. Must release all before app destruction.
    ASSERT_NN(pIUnk);
    auto& mgr = cPtrTraceMgr::I();
    const auto guard(mgr._Lock.Lock());  // thread sync critical section.
    const UINT_PTR id = ++mgr._TraceIdLast;
    mgr._aTraces.AddSort(cPtrTraceEntry(typeInfo, pIUnk, id, pSrc ? *pSrc : cDebugSourceLine()), 1);
    return id;
}

void GRAYCALL cPtrTrace::TraceUpdateX(UINT_PTR id, const cDebugSourceLine& src) noexcept {
    ASSERT(id);
    auto& mgr = cPtrTraceMgr::I();
    const auto guard(mgr._Lock.Lock());  // thread sync critical section.
    const ITERATE_t index = mgr._aTraces.FindIForKey(id);
    if (index < 0) return;
    mgr._aTraces.ElementAt(index)._Src = src;
}

void GRAYCALL cPtrTrace::TraceReleaseX(UINT_PTR id) {
    //! Called when cPtrTrace is destroyed.
    ASSERT(id);
    if (cAppState::isInCExit()) {  // can't track this here.
        sm_bActive = false;
        return;
    }
    auto& mgr = cPtrTraceMgr::I();
    const auto guard(mgr._Lock.Lock());  // thread sync critical section.
    bool ret = mgr._aTraces.RemoveKey(id);
    ASSERT(ret);
    UNREFERENCED_PARAMETER(ret);
}
}  // namespace Gray
