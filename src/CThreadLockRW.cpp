//
//! @file cThreadLockRW.cpp
//! http://www.codeproject.com/KB/threads/ReadWriteLock.aspx?msg=1772196
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! @todo cThreadLockRW

#include "pch.h"
#include "cThreadLockRW.h"

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cThreadLocalSys.h"
#include "cLogMgr.h"

UNITTEST_CLASS(cThreadLockRW)
{
	UNITTEST_METHOD(cThreadLockRW)
	{
		//! TODO cThreadLockRW
		//! Test reentrant and upgrade features.

		cThreadLockRW lockRW;
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
UNITTEST_REGISTER(cThreadLockRW, UNITTEST_LEVEL_Core);
#endif
