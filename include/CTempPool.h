//! @file cTempPool.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTempPool_H
#define _INC_cTempPool_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cBlob.h"
#include "cThreadLocalSys.h"

namespace Gray {

/// Temp string pool for a single thread.
class GRAYCORE_LINK cTempPool1 {
    static const int k_nBlocksMax = 16;  /// Assume nested functions won't use more than m_aBlocks in a single thread. (e.g. This is the max number of args on a single sprintf)
    int m_nBlockCur;                     /// rotate this count to re-use buffers in m_aBlocks.
    cArray<cBlob> m_aBlocks;  /// Temporary blocks to be used on a single thread.

 public:
    cTempPool1() noexcept : m_nBlockCur(0) {}
    virtual ~cTempPool1() {}

    /// <summary>
    /// Get a temporary/scratch memory space for random uses on this thread. Non leaking pointer return. beware of k_iCountMax.
    /// Typically used to hold "%s" argument conversions for StrT::sprintfN() type operations.
    /// Ideally we use should CString(x).get_CPtr() instead (to control allocation lifetime)?
    /// </summary>
    /// <param name="nLenNeed">exact size (in bytes) including space for '\0'</param>
    /// <returns></returns>
    cMemSpan GetMemSpan(size_t nLenNeed);  // get void/bytes.

    template <typename TYPE>
    cSpanX<TYPE> GetSpan(StrLen_t nLenNeed) {
        //! get space. will add an extra space for '\0'
        return cSpanX<TYPE>(GetMemSpan((nLenNeed + 1) * sizeof(TYPE)));
    }
    template <typename TYPE>
    TYPE* GetT(const cSpanX<TYPE>& src) {
        if (src.isEmpty()) return nullptr;
        cSpanX<TYPE> dst = GetSpan<TYPE>(src.get_MaxLen());
        dst.SetCopySpan(src);
        dst.get_DataWork()[src.get_MaxLen()] = '\0';
        return dst.get_DataWork();
    }
};

/// <summary>
/// A set of thread local temporary strings/spaces for function arguments and Unicode/UTF8 conversions. Used by StrArg .
/// Pool of re-used strings/spaces after k_nBlocksMax uses.
/// use a new set for each thread. Thread Local/Safe.
/// @note This is a bit of a hack as it assumes the strings are not in use when the rollover occurs !
///  beware of using more than k_nBlocksMax strings on one line.
///  We can never be sure we are not re-using strings before they are ready.
///  Just use cString is you want to always be safe?
/// </summary>
class GRAYCORE_LINK cTempPool {
    static cThreadLocalSysNew<cTempPool1> sm_ThreadLocal;

 public:
    /// <summary>
    /// Get thread local cTempPool. create it if its not already allocated.
    /// </summary>
    static cTempPool1* GRAYCALL GetTempPool();
    static void GRAYCALL FreeThreadManually();
 
    /// <summary>
    /// Get thread local temp space.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nLenNeed">will add a space for '\0'</param>
    /// <returns></returns>
    template <typename TYPE>
    static cSpanX<TYPE> GRAYCALL GetSpan(StrLen_t nLenNeed) {
        return GetTempPool()->GetSpan<TYPE>(nLenNeed);
    }
    /// <summary>
    /// Get thread local temp space.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="src">will add a space for '\0'</param>
    /// <returns></returns>
    template <typename TYPE>
    static TYPE* GRAYCALL GetT(const cSpanX<TYPE>& src) {
        return GetTempPool()->GetT<TYPE>(src);
    }
};
}  // namespace Gray
#endif
