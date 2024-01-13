//
//! @file cPtrTrace.h
//! Attempt to trace use of pointers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrTrace_H
#define _INC_cPtrTrace_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"
#include "cDebugAssert.h"
#include "cNonCopyable.h"
#include "cPtrFacade.h"
#include "cTypeInfo.h"

namespace Gray {
/// <summary>
/// Trace each use/reference of the a pointer in cPtrFacade/cIUnkPtr/cRefPtr for _DEBUG purposes.
/// If the lock count fails to go to 0 we know who the leaker was. or if the object is deleted but still has refs we can detect that as well.
/// Add myself to the cPtrTraceMgr table if the m_p pointer is set.
/// </summary>
struct GRAYCORE_LINK cPtrTrace {
    static bool sm_bActive;  /// Turn on/off global tracing via cPtrTraceMgr. be fast.
    UINT_PTR _TraceId = 0;   /// Unique id for this trace reference. 0 = no reference

    static UINT_PTR GRAYCALL TraceAttachX(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, const cDebugSourceLine* src = nullptr);
    static void GRAYCALL TraceUpdateX(UINT_PTR id, const cDebugSourceLine& src) noexcept;
    static void GRAYCALL TraceReleaseX(UINT_PTR id);

    inline void TraceAttach(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, const cDebugSourceLine* src = nullptr) {
        ASSERT(_TraceId == 0);
        _TraceId = TraceAttachX(typeInfo, pIUnk, src);
    }
    inline void TraceUpdate(const cDebugSourceLine& src) noexcept {
        if (_TraceId) TraceUpdateX(_TraceId, src);
    }
    inline void TraceRelease() {
        if (_TraceId) {
            TraceReleaseX(_TraceId);
            _TraceId = 0;
        }
    }
};
}  // namespace Gray
#endif
