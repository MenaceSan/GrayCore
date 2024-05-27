//! @file cMemPage.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cMemPage_H
#define _INC_cMemPage_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySortRef.h"
#include "cSingleton.h"
#include "cSystemInfo.h"
#include "cTypeInfo.h"  // typeid()

namespace Gray {
/// <summary>
/// Track a protected memory page.
/// _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
/// </summary>
class GRAYCORE_LINK cMemPage : public cRefBase {
    friend class cMemPageMgr;

 public:
    UINT_PTR m_nPageStart;             /// Pointer as number. Always aligned to dwPageSize.
    size_t m_nPageSize;                /// SystemInfo::dwPageSize
    DWORD m_dwOldProtectionFlags = 0;  /// original flags used/returned by _WIN32 VirtualProtect()
    REFCOUNT_t m_nRefCount2 = 1;       /// ProtectPages count.

 public:
    cMemPage(UINT_PTR nPageStart, size_t nPageSize) : m_nPageStart(nPageStart), m_nPageSize(nPageSize) {
        ASSERT(get_SortValue() != 0);
        ASSERT((get_SortValue() % m_nPageSize) == 0);
    }
    virtual ~cMemPage() {}

    UINT_PTR get_SortValue() const noexcept {
        return m_nPageStart;
    }
    bool IsOverlapped(UINT_PTR p, size_t n) const noexcept {
        if ((p + n) <= m_nPageStart) return false;
        if ((m_nPageStart + m_nPageSize) <= p) return false;
        return true;
    }
    bool SetProtect(bool bProtect) noexcept;
};
typedef cRefPtr<cMemPage> cMemPagePtr;

/// <summary>
/// Track my protected memory pages.
/// _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
/// </summary>
class cMemPageMgr final : public cSingleton<cMemPageMgr> {
    SINGLETON_IMPL(cMemPageMgr);

 public:
    DWORD m_dwPageSize;
    cArraySortValue<cMemPage, UINT_PTR> m_aPages;

 protected:
    cMemPageMgr() noexcept : cSingleton<cMemPageMgr>(this, typeid(cMemPageMgr)), m_dwPageSize(0) {}

 public:
    HRESULT ProtectPages(const cMemSpan& m, bool bProtect) {
        //! Protect or un-protect these pages.
        if (m_dwPageSize == 0) {
            m_dwPageSize = CastN(DWORD, cSystemInfo::I().get_PageSize());
        }

        const UINT_PTR nStart = CastPtrToNum(m);
        const UINT_PTR nEnd = nStart + m.get_SizeBytes();
        const UINT_PTR nPageOver = nStart % m_dwPageSize;
        UINT_PTR nPageStart = nStart - nPageOver;
        for (; nPageStart < nEnd; nPageStart += m_dwPageSize) {
            cMemPagePtr pPage = m_aPages.FindArgForKey(nPageStart);
            if (bProtect) {
                if (pPage == nullptr) {
                    // odd
                    // DEBUG_ERR(("ProtectPages bProtect = nullptr"));
                    continue;
                }
                if (--pPage->m_nRefCount2) continue;
                m_aPages.RemoveArg(pPage);
                if (!pPage->SetProtect(true)) {
                    // DEBUG_ERR(("ProtectPages SetProtect true"));
                    return E_FAIL;
                }
            } else {
                if (pPage == nullptr) {
                    pPage = new cMemPage(nPageStart, m_dwPageSize);
                    ASSERT(pPage);
                    if (!pPage->SetProtect(false)) {
                        // DEBUG_ERR(("ProtectPages SetProtect false"));
                        return E_FAIL;
                    }
                    m_aPages.AddSort(pPage, 1);
                } else {
                    pPage->m_nRefCount2++;
                }
            }
        }
        return S_OK;
    }
};
}  // namespace Gray
#endif
