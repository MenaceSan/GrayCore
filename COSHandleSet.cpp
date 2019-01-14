//
//! @file COSHandleSet.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "COSHandleSet.h"
#include "HResult.h"
#include "CTimeVal.h"	// must include this for _WIN32 DLL instantiate.

namespace Gray
{
	HRESULT COSHandleSet::WaitForObjects(TIMESYSD_t dwMilliseconds, bool bWaitForAll)
	{
		//! Wait for any or all of these handles to be signaled.
		//! @return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT) after dwMilliseconds
#ifdef __linux__
		CTimeVal timeWait(dwMilliseconds);
		int iRet = ::select(m_hHandleMax + 1, &m_fds, nullptr, nullptr, &timeWait);
		if (iRet == 0)
		{
			return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT);
		}
		if (iRet < 0)
		{
			return HResult::GetLastDef();
		}
		// FD_ISSET(0,&read_fd) to find out which ?
		return S_OK;	// One of these objects is signaled.
#else
		DWORD dwRet = ::WaitForMultipleObjects((DWORD)m_fds.GetSize(), m_fds.GetData(), bWaitForAll, dwMilliseconds);
		return HResult::FromWaitRet(dwRet);
#endif
	}
}
