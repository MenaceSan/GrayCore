//
//! @file cTempPool.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrConst.h"
#include "StrT.h"
#include "StrU.h"
#include "cTempPool.h"

namespace Gray {
cThreadLocalSysNew<cTempPool> cTempPool::sm_ThreadLocal;  // Thread Local Mechanism.

cTempPool* GRAYCALL cTempPool::GetTempPool() {  // static
    return sm_ThreadLocal.GetDataNew();
}
void GRAYCALL cTempPool::FreeThreadManually() {  // static
    // It is useful to free the main thread early on exit to avoid leak detection.
    sm_ThreadLocal.FreeDataManually();
}

void* cTempPool::GetTempV(size_t nLenNeed) {
    if (m_aBlocks.isEmpty()) {  // first time alloc.
        m_aBlocks.SetSize(k_nBlocksMax);
        m_nBlockCur = 0;
    } else {
        if (++m_nBlockCur >= k_nBlocksMax) m_nBlockCur = 0;
    }
    auto& r = m_aBlocks[m_nBlockCur];
    r.ReAllocSize(nLenNeed);  // alloc to the size we need.
    return r.get_DataW();
}

void* cTempPool::GetTempV(size_t nLenNeed, const void* pData) {
    void* pDst = GetTempV(nLenNeed);
    if (pData != nullptr && pDst != nullptr) {
        cMem::Copy(pDst, pData, nLenNeed);
    }
    return pDst;
}
}  // namespace Gray
