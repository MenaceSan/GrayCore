//
//! @file cOSHandle.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cOSHandle.h"
#include "cOSHandleSet.h"
#include "cTimeVal.h"

#ifdef __linux__
#include <sys/ioctl.h>
#endif
#if defined(_WIN32)
#pragma comment(lib, "kernel32.lib")	 // CloseHandle, etc
#endif

namespace Gray
{

#ifdef __linux__
	int cOSHandle::IOCtl(int nCmd, void* pArgs) const
	{
		return ::ioctl(m_h, nCmd, pArgs);
	}
	int cOSHandle::IOCtl(int nCmd, int nArgs) const
	{
		return ::ioctl(m_h, nCmd, nArgs);
	}
#endif

	HRESULT cOSHandle::WaitForSingleObject(TIMESYSD_t dwMilliseconds) const
	{
		//! Wait for the handle m_h to be signaled.
		//! HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT) = after dwMilliseconds
		if (!isValidHandle())
			return E_HANDLE;
#ifdef __linux__ 
		cOSHandleSet hset(m_h);
		return hset.WaitForObjects(dwMilliseconds);
#elif defined(_WIN32)
		return HResult::FromWaitRet(::WaitForSingleObject(m_h, (DWORD)dwMilliseconds));
#else
#error NOOS
#endif
	}
} 
 