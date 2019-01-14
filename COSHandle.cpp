//
//! @file COSHandle.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "COSHandle.h"
#include "COSHandleSet.h"
#include "CTimeVal.h"
#ifdef __linux__
#include <sys/ioctl.h>
#endif

namespace Gray
{

#ifdef __linux__
	int COSHandle::IOCtl(int nCmd, void* pArgs) const
	{
		return ::ioctl(m_h, nCmd, pArgs);
	}
	int COSHandle::IOCtl(int nCmd, int nArgs) const
	{
		return ::ioctl(m_h, nCmd, nArgs);
	}
#endif

	HRESULT COSHandle::WaitForSingleObject(TIMESYSD_t dwMilliseconds) const
	{
		//! Wait for the handle m_h to be signaled.
		//! HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT) = after dwMilliseconds

#ifdef __linux__
		CTimeVal timeWait(dwMilliseconds);
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(m_h, &fds);
		int iRet = ::select(m_h + 1, &fds, nullptr, nullptr, &timeWait);
		if (iRet == 0)
		{
			return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT);
		}
		if (iRet < 0)
		{
			return HResult::GetLastDef();
		}
		return S_OK;	// object is signaled.
#elif defined(_WIN32)
		return HResult::FromWaitRet(::WaitForSingleObject(m_h, (DWORD)dwMilliseconds));
#endif
	}
};

//******************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CLogMgr.h"

UNITTEST_CLASS(COSHandle)
{
	UNITTEST_METHOD(COSHandle)
	{
		COSHandle h1;
		COSHandle h2;
#ifdef _WIN32
		// Test dupe handle.
#endif
	}
};
UNITTEST_REGISTER(COSHandle, UNITTEST_LEVEL_Core);
#endif
