//
//! @file CThreadLock.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CThreadLock.h"

namespace Gray
{
#ifdef __linux__
	const pthread_mutex_t CThreadLockMutex::k_MutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

	void CThreadLockFast::Lock()
	{
		//! Take ownership if the lock is free or owned by the calling thread.
		//! Wait forever for a collision to clear.
		//! @note don't use inline. inline version of this made bad code? _MSC_VER

#ifdef _DEBUG
		TIMESYSD_t dwWaitCount = 0;
#endif
		TIMESYSD_t dwWaitTimeMS = 0;
		THREADID_t nTid = CThreadId::GetCurrentId(); // get my current thread id.
		for (;;)
		{
			THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, CThreadId::k_NULL);
			if (nTidowner == CThreadId::k_NULL || CThreadId::IsEqualId(nTidowner, nTid))
				break;	// i got it. or already had it.
			// Some other thread owns the lock. Wait.
			ASSERT(CThreadId::IsValidId(nTidowner));
			CThreadId::SleepCurrent(dwWaitTimeMS); // Give up the rest of the time slice. just wait for it to free.
			dwWaitTimeMS = 1;
#ifdef _DEBUG
			dwWaitCount++;	// Count how long i had to wait.
#endif
		}
		SUPER_t::IncLockCount();
#ifdef _DEBUG
		ASSERT(isThreadLockedByCurrent()); // may have several locks on the same thread.
		if (dwWaitCount)
		{
			// DEBUG_CHECK(0);	// collide cleared.
		}
#endif
	}
	bool CThreadLockFast::LockTry(TIMESYSD_t dwDelayMS)
	{
		//! inline version of this made bad code?
		//! Take ownership if the lock is free or owned by the calling thread
#ifdef _DEBUG
		TIMESYSD_t dwWaitCount = 0;
#endif
		TIMESYSD_t dwWaitTimeMS = 0;
		THREADID_t nTid = CThreadId::GetCurrentId(); //  get my current thread id.
		for (;;)
		{
			THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, CThreadId::k_NULL);
			if (nTidowner == CThreadId::k_NULL || CThreadId::IsEqualId(nTidowner, nTid))
				break;	// i got it. or already had it.

			// Some other thread owns the lock. Wait.
			ASSERT(CThreadId::IsValidId(nTidowner));
			if (dwDelayMS <= 0)
			{
#ifdef _DEBUG
				if (dwWaitCount)
				{
					// DEBUG_CHECK(0);	// collide cleared.
				}
#endif
				return false; // FAILED to lock
			}
			CThreadId::SleepCurrent(dwWaitTimeMS); // wait for a tick.
			if (dwWaitTimeMS == 0)
			{
				dwWaitTimeMS = 1;
			}
			else
			{
				dwDelayMS--;
			}
#ifdef _DEBUG
			dwWaitCount++;	// Count how long i had to wait.
#endif
		}
		SUPER_t::IncLockCount();
		return true;
	}
}

//*******************************************************************

#if USE_UNITTESTS
#include "CUnitTest.h"
#include "CLogMgr.h"

UNITTEST_CLASS(CThreadId)
{
	UNITTEST_METHOD(CThreadId)
	{
		// NOTE: See CThread UnitTest for better testing of locks.
		THREADID_t idCurrent = CThreadId::GetCurrentId();
		UNITTEST_TRUE(CThreadId::IsValidId(idCurrent));

		CThreadLockFast lockFast;
		lockFast.Lock();
		lockFast.Unlock();

		CThreadLockCrit lockCrit;
		lockCrit.Lock();
		lockCrit.Unlock();

		CThreadLockMutex lockMutex;
		lockMutex.Lock();
		lockMutex.Unlock();
	}
};
UNITTEST_REGISTER(CThreadId, UNITTEST_LEVEL_Core);
#endif
