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

#ifdef __linux__ 
		cOSHandleSet hset(m_h);
		return hset.WaitForObjects(dwMilliseconds);
#elif defined(_WIN32)
		return HResult::FromWaitRet(::WaitForSingleObject(m_h, (DWORD)dwMilliseconds));
#else
#error NOOS
#endif
	}
};

//******************************************************************

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cLogMgr.h"

UNITTEST_CLASS(cOSHandle)
{
	UNITTEST_METHOD(cOSHandle)
	{
		cOSHandle h1;
		cOSHandle h2;
#ifdef _WIN32
		// Test dupe handle.
#endif
	}
};
UNITTEST_REGISTER(cOSHandle, UNITTEST_LEVEL_Core);
#endif
