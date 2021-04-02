//
//! @file cThreadLock.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cThreadLock.h"

namespace Gray
{
	bool cThreadState::WaitForThreadExit(TIMESYSD_t iTimeMSec) noexcept // virtual
	{
		// similar to ::pthread_join() but with a timer.
		const cTimeSys tStart(cTimeSys::GetTimeNow());
		for (;;)
		{
			if (!m_bThreadRunning)
				return true;
			UINT tDiff = (UINT)tStart.get_AgeSys();
			if (tDiff > (UINT)iTimeMSec)	// -1 = INFINITE
				break;
			cThreadId::SleepCurrent((tDiff > 400) ? 200 : 10);	// milliseconds
		}
		return false;	// didn't stop in time! may have to hard terminate
	}

#ifdef __linux__
	const pthread_mutex_t cThreadLockMutex::k_MutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

	void cThreadLockFast::Lock()
	{
		//! Take ownership if the lock is free or owned by the calling thread.
		//! Wait forever for a collision to clear.
		//! @note don't use inline. inline version of this made bad code? _MSC_VER

#ifdef _DEBUG
		TIMESYSD_t dwWaitCount = 0;
#endif
		TIMESYSD_t dwWaitTimeMS = 0;
		THREADID_t nTid = cThreadId::GetCurrentId(); // get my current thread id.
		for (;;)
		{
			THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, cThreadId::k_NULL);
			if (nTidowner == cThreadId::k_NULL || cThreadId::IsEqualId(nTidowner, nTid))
				break;	// i got it. or already had it.
			// Some other thread owns the lock. Wait.
			ASSERT(cThreadId::IsValidId(nTidowner));
			cThreadId::SleepCurrent(dwWaitTimeMS); // Give up the rest of the time slice. just wait for it to free.
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

	bool cThreadLockFast::LockTry(TIMESYSD_t dwDelayMS)
	{
		//! inline version of this made bad code?
		//! Take ownership if the lock is free or owned by the calling thread
#ifdef _DEBUG
		TIMESYSD_t dwWaitCount = 0;
#endif
		TIMESYSD_t dwWaitTimeMS = 0;
		THREADID_t nTid = cThreadId::GetCurrentId(); //  get my current thread id.
		for (;;)
		{
			THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, cThreadId::k_NULL);
			if (nTidowner == cThreadId::k_NULL || cThreadId::IsEqualId(nTidowner, nTid))
				break;	// i got it. or already had it.

			// Some other thread owns the lock. Wait.
			ASSERT(cThreadId::IsValidId(nTidowner));
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
			cThreadId::SleepCurrent(dwWaitTimeMS); // wait for a tick.
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
 
