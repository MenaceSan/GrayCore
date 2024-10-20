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
    const UINT_PTR _nPageStart;       /// Pointer as number. Always aligned to _nPageSize. cMemSpan
    const size_t _nPageSize;          /// from cSystemInfo::dwPageSize
    DWORD _dwOldProtectionFlags = 0;  /// original flags used/returned by _WIN32 VirtualProtect()
    REFCOUNT_t _nRefCount2 = 1;       /// ProtectPages count.

 public:
    cMemPage(UINT_PTR nPageStart, size_t nPageSize) : _nPageStart(nPageStart), _nPageSize(nPageSize) {
        ASSERT(get_SortValue() != 0);
        ASSERT((get_SortValue() % _nPageSize) == 0);
    }
    virtual ~cMemPage() {}

    UINT_PTR get_SortValue() const noexcept {
        return _nPageStart;
    }
    bool IsOverlapped(UINT_PTR p, size_t n) const noexcept {
        if ((p + n) <= _nPageStart) return false;
        if ((_nPageStart + _nPageSize) <= p) return false;
        return true;
    }
    bool SetProtect(bool bProtect) noexcept;
};
typedef cRefPtr<cMemPage> cMemPagePtr;

/// <summary>
/// Track my protected memory pages.
/// _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
/// </summary>
class GRAYCORE_LINK cMemPageMgr final : public cSingleton<cMemPageMgr> {
 public:
    DECLARE_cSingleton(cMemPageMgr);
    DWORD _dwPageSize = 0;                        /// from cSystemInfo::dwPageSize
    cArraySortValue<cMemPage, UINT_PTR> _aPages;  // Protected pages.

 protected:
    cMemPageMgr() noexcept : cSingleton<cMemPageMgr>(this) {}

 public:
    /// <summary>
    /// Protect or un-protect pages overlapping cMemSpan.
    /// </summary>
    HRESULT ProtectPages(const cMemSpan& m, bool bProtect);
};
}  // namespace Gray
#endif
