//
//! @file cPtrTraceMgr.h
//! Attempt to trace use of pointers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrTraceMgr_H
#define _INC_cPtrTraceMgr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySort.h"
#include "cPtrTrace.h"
#include "cSingleton.h"
#include "cThreadLock.h"

namespace Gray {
struct cLogProcessor;

/// <summary>
/// a shared object reference being traced.
/// </summary>
struct GRAYCORE_LINK cPtrTraceEntry {
    const TYPEINFO_t* m_TypeInfo = nullptr;  /// for __typeof(TYPEINFO_t).name() for _pIUnk
    IUnknown* _pIUnk = nullptr;              /// Pointer to my shared object. never nullptr!
    UINT_PTR _TraceId = 0;                   /// Unique id for this trace reference. NEVER 0
    cDebugSourceLine m_Src;                  /// where (in code) was _pIUnk set? NOT always available!

 public:
    cPtrTraceEntry() noexcept {}
    cPtrTraceEntry(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, UINT_PTR traceId, const cDebugSourceLine& src) noexcept : m_TypeInfo(&typeInfo), _pIUnk(pIUnk), _TraceId(traceId), m_Src(src) {
        ASSERT_NN(pIUnk);
        ASSERT(traceId != 0);
    }
    cPtrTraceEntry(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, UINT_PTR traceId) noexcept : m_TypeInfo(&typeInfo), _pIUnk(pIUnk), _TraceId(traceId) {
        ASSERT_NN(pIUnk);
        ASSERT(traceId != 0);
    }
    UINT_PTR get_HashCode() const noexcept {
        return _TraceId;
    }
};

/// <summary>
/// USE_PTRTRACE_IUNK = We are tracing all calls to cIUnkPtr or cRefPtr so we can figure out who is not releasing their ref.
/// </summary>
class GRAYCORE_LINK cPtrTraceMgr : public cSingleton<cPtrTraceMgr> {
    friend class cSingleton<cPtrTraceMgr>;
    friend cPtrTrace;

    mutable cThreadLockCount m_Lock;
    UINT_PTR _TraceIdLast = 0;
    cArraySortStructHash<cPtrTraceEntry> m_aTraces;  /// may be up-cast cPtrTrace to cIUnkBasePtr or cRefPtr

 protected:
    cPtrTraceMgr() noexcept : cSingleton<cPtrTraceMgr>(this, typeid(cPtrTraceMgr)) {}
    ~cPtrTraceMgr() override {}

 public:
    ITERATE_t GetSize() const noexcept {
        return m_aTraces.GetSize();
    }
    virtual int TraceDump(cLogProcessor* pLog, ITERATE_t iCountExpected);
    CHEAPOBJECT_IMPL;
};
}  // namespace Gray
#endif
