//! @file cMemPage.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cMemPage.h"

#ifdef __linux__
#include <sys/mman.h>
#endif

namespace Gray {
cSingleton_IMPL(cMemPageMgr);

HRESULT cMemPageMgr::ProtectPages(const cMemSpan& m, bool bProtect) {
    if (_dwPageSize == 0) {
        _dwPageSize = CastN(DWORD, cSystemInfo::I().get_PageSize());
    }

    const UINT_PTR nStart = CastPtrToNum(m);
    const UINT_PTR nEnd = nStart + m.get_SizeBytes();
    const UINT_PTR nPageOver = nStart % _dwPageSize;
    UINT_PTR nPageStart = nStart - nPageOver;

    for (; nPageStart < nEnd; nPageStart += _dwPageSize) {
        cMemPagePtr pPage = _aPages.FindArgForKey(nPageStart);
        if (bProtect) {
            if (pPage == nullptr) {
                // odd
                // DEBUG_ERR(("ProtectPages bProtect = nullptr"));
                continue;
            }
            if (--pPage->_nRefCount2) continue;
            _aPages.RemoveArg(pPage);
            if (!pPage->SetProtect(true)) {
                // DEBUG_ERR(("ProtectPages SetProtect true"));
                return E_FAIL;
            }
        } else if (pPage == nullptr) {
            pPage = new cMemPage(nPageStart, _dwPageSize);
            ASSERT(pPage);
            if (!pPage->SetProtect(false)) {
                // DEBUG_ERR(("ProtectPages SetProtect false"));
                return E_FAIL;
            }
            _aPages.AddSort(pPage, 1);
        } else {
            pPage->_nRefCount2++;
        }
    }
    return S_OK;
}

bool cMemPage::SetProtect(bool bProtect) noexcept {
    //! @arg bProtect = false = allow PAGE_EXECUTE_READWRITE
#ifdef __linux__
    int ret = ::mprotect((void*)_nPageStart, _nPageSize - 1, PROT_READ | PROT_WRITE | PROT_EXEC);
    return ret == 0;
#else
    const DWORD dwNewProtectionFlags = bProtect ? _dwOldProtectionFlags : PAGE_EXECUTE_READWRITE;
    return ::VirtualProtect((void*)_nPageStart, _nPageSize - 1, dwNewProtectionFlags, &_dwOldProtectionFlags);
#endif
}
}  // namespace Gray
