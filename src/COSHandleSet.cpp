//! @file cOSHandleSet.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "cOSHandleSet.h"
#include "cTimeVal.h"  // must include this for _WIN32 DLL instantiate.

namespace Gray {
HRESULT cOSHandleSet::WaitForObjects(TIMESYSD_t nMilliseconds, bool bWaitForAll) {
    //! Wait for any or all of these handles to be signaled. 			//! MAX of 0x80 items. STATUS_ABANDONED_WAIT_0 MAXIMUM_WAIT_OBJECTS
    //! @return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT) after nMilliseconds
    //! @note WIN32 does not use select() here because it is in a strange ws2_32.dll AND MUST CALL ::WSAStartup()
#ifdef __linux__
    cTimeVal timeWait(nMilliseconds);
    int iRet = ::select(m_hHandleMax + 1, &m_fds, nullptr, nullptr, &timeWait);
    if (iRet == 0) {
        return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT);
    }
    if (iRet < 0) {
        return HResult::GetLastDef();
    }
    // FD_ISSET(0,&read_fd) to find out which ?
    return S_OK;  // One of these objects is signaled.
#else
    DWORD dwRet = ::WaitForMultipleObjects((DWORD)m_fds.GetSize(), m_fds.get_DataConst(), bWaitForAll, static_cast<DWORD>(nMilliseconds));
    return HResult::FromWaitRet(dwRet);
#endif
}
}  // namespace Gray
