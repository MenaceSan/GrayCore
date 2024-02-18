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
    if (m_aBlocks.isEmpty()) {  // first time alloc.
        m_aBlocks.SetSize(k_nBlocksMax);
        m_nBlockCur = 0;
    } else {
        if (++m_nBlockCur >= k_nBlocksMax) m_nBlockCur = 0;
    }
    auto& r = m_aBlocks.ElementAt(m_nBlockCur);
    r.ReAllocSize(nLenNeed);  // re-alloc to the size we need.
    return cMemSpan(r, nLenNeed);
}
}  // namespace Gray
