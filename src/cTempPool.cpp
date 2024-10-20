//! @file cTempPool.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrConst.h"
#include "StrT.h"
#include "StrU.h"
#include "cTempPool.h"

namespace Gray {
cThreadLocalSysNew<cTempPool1> cTempPool::sm_ThreadLocal;  // Thread Local Mechanism.

cTempPool1* GRAYCALL cTempPool::GetTempPool() {  // static
    return sm_ThreadLocal.GetDataNew();
}
void GRAYCALL cTempPool::FreeThreadManually() {  // static
    // It is useful to free the main thread early on exit to avoid leak detection.
    sm_ThreadLocal.FreeDataManually();
}

cMemSpan cTempPool1::GetMemSpan(size_t nLenNeed) {
    if (_aBlocks.isEmpty()) {  // first time alloc.
        _aBlocks.SetSize(k_nBlocksMax);
        _nBlockCur = 0;
    } else {
        if (++_nBlockCur >= k_nBlocksMax) _nBlockCur = 0;
    }
    auto& r = _aBlocks.ElementAt(_nBlockCur);
    r.ReAllocSize(nLenNeed);  // re-alloc to the size we need.
    return cMemSpan(r, nLenNeed);
}
}  // namespace Gray
