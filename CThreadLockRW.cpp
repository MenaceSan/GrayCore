//
//! @file CThreadLockRW.cpp
//! http://www.codeproject.com/KB/threads/ReadWriteLock.aspx?msg=1772196
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//! @todo CThreadLockRW

#include "StdAfx.h"
#include "CThreadLockRW.h"

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CThreadLocalSys.h"
#include "CLogMgr.h"

UNITTEST_CLASS(CThreadLockRW)
{
	UNITTEST_METHOD(CThreadLockRW)
	{
		//! TODO CThreadLockRW
		//! Test reentrant and upgrade features.

		CThreadLockRW lockRW;
#if 0
		lockRW.IncReadLockCount();
		lockRW.Lock();

		{
		}

		lockRW.Unlock();
		lockRW.DecReadLockCount();
#endif
	}
};
UNITTEST_REGISTER(CThreadLockRW, UNITTEST_LEVEL_Core);
#endif
