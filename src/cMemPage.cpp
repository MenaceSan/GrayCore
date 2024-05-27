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
bool cMemPage::SetProtect(bool bProtect) noexcept {
    //! @arg bProtect = false = allow PAGE_EXECUTE_READWRITE
#ifdef __linux__
    int ret = ::mprotect((void*)m_nPageStart, m_nPageSize - 1, PROT_READ | PROT_WRITE | PROT_EXEC);
    return ret == 0;
#else
    const DWORD dwNewProtectionFlags = bProtect ? m_dwOldProtectionFlags : PAGE_EXECUTE_READWRITE;
    return ::VirtualProtect((void*)m_nPageStart, m_nPageSize - 1, dwNewProtectionFlags, &m_dwOldProtectionFlags);
#endif
}
}  // namespace Gray
