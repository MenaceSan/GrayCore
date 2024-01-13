//
//! @file cMemPage.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cMemPage_H
#define _INC_cMemPage_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySortRef.h"
#include "cSingleton.h"
#include "cSystemInfo.h"

namespace Gray {
/// <summary>
/// Track a protected memory page.
/// _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
/// </summary>
class cMemPage : public cRefBase {
    friend class cMemPageMgr;

 public:
    UINT_PTR m_nPageStart;         /// Always aligned to dwPageSize.
    size_t m_nPageSize;            /// SystemInfo::dwPageSize
    DWORD m_dwOldProtectionFlags;  /// original flags used/returned by _WIN32 VirtualProtect()
    int m_nRefCount2;              /// ProtectPages count.

 public:
    cMemPage(UINT_PTR nPageStart, size_t nPageSize) : m_nPageStart(nPageStart), m_nPageSize(nPageSize), m_dwOldProtectionFlags(0), m_nRefCount2(1) {
        ASSERT(get_SortValue() != 0);
        ASSERT((get_SortValue() % m_nPageSize) == 0);
    }
    virtual ~cMemPage() noexcept {}

    UINT_PTR get_SortValue() const noexcept {
        return m_nPageStart;
    }
    bool IsOverlapped(UINT_PTR p, size_t n) const noexcept {
        if ((p + n) <= m_nPageStart) return false;
        if ((m_nPageStart + m_nPageSize) <= p) return false;
        return true;
    }
    bool SetProtect(bool bProtect) noexcept {
        //! @arg bProtect = false = allow PAGE_EXECUTE_READWRITE
#ifdef __linux__
        ::mprotect((void*)m_nPageStart, m_nPageSize - 1, PROT_READ | PROT_WRITE | PROT_EXEC);
#else
        DWORD dwNewProtectionFlags = bProtect ? m_dwOldProtectionFlags : PAGE_EXECUTE_READWRITE;
        return ::VirtualProtect((void*)m_nPageStart, m_nPageSize - 1, dwNewProtectionFlags, &m_dwOldProtectionFlags);
#endif
    }
};
typedef cRefPtr<cMemPage> cMemPagePtr;

/// <summary>
/// Track my protected memory pages.
/// _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
/// </summary>
class cMemPageMgr : public cSingleton<cMemPageMgr> {
 public:
    DWORD m_dwPageSize;
    cArraySortValue<cMemPage, UINT_PTR> m_aPages;

 public:
    cMemPageMgr() noexcept : cSingleton<cMemPageMgr>(this, typeid(cMemPageMgr)), m_dwPageSize(0) {}
    virtual ~cMemPageMgr() noexcept {
        // Make sure this stuff doesnt get destroyed too early.
    }

    HRESULT ProtectPages(const void* p, size_t nSize, bool bProtect) {
        //! Protect or un-protect these pages.
        if (m_dwPageSize == 0) {
            m_dwPageSize = CastN(DWORD, cSystemInfo::I().get_PageSize());
        }

        const UINT_PTR nStart = PtrCastToNum(p);
        const UINT_PTR nEnd = nStart + nSize;
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
                    m_aPages.Add(pPage);
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
