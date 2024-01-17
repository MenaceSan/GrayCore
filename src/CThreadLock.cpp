//
//! @file cThreadLock.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cThreadLock.h"

namespace Gray {
bool cThreadState::WaitForThreadExit(TIMESYSD_t iTimeMSec) noexcept {  // virtual
    // similar to ::pthread_join() but with a timer. NOT optimal.
    const cTimeSys tStart(cTimeSys::GetTimeNow());
    for (;;) {
        if (!isThreadRunning()) return true;
        UINT tDiff = (UINT)tStart.get_AgeSys();
        if (tDiff > (UINT)iTimeMSec) break;                 // -1 = INFINITE
        cThreadId::SleepCurrent((tDiff > 400) ? 200 : 10);  // milliseconds
    }
    return false;  // didn't stop in time! may have to hard terminate
}

#ifdef __linux__
const pthread_mutex_t cThreadLockMutex::k_MutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

void cThreadLockFast::Lock() {
#ifdef _DEBUG
    TIMESYSD_t dwWaitCount = 0;
#endif
    TIMESYSD_t dwWaitTimeMS = 0;
    THREADID_t nTid = cThreadId::GetCurrentId();  // get my current thread id.
    for (;;) {
        const THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, cThreadId::k_NULL);
        if (nTidowner == cThreadId::k_NULL || cThreadId::IsEqualId(nTidowner, nTid)) break;  // i got it. or already had it.
        // Some other thread owns the lock. Wait.
        ASSERT(cThreadId::IsValidId(nTidowner));
        cThreadId::SleepCurrent(dwWaitTimeMS);  // Give up the rest of the time slice. just wait for it to free.
        dwWaitTimeMS = 1;
#ifdef _DEBUG
        dwWaitCount++;  // Count how long i had to wait.
#endif
    }
    SUPER_t::IncLockCount();
#ifdef _DEBUG
    ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
    if (dwWaitCount) {
        // DEBUG_CHECK(0);	// collide cleared.
    }
#endif
}

bool cThreadLockFast::LockTry(TIMESYSD_t dwDelayMS) {
    //! inline version of this made bad code?
    //! Take ownership if the lock is free or owned by the calling thread
#ifdef _DEBUG
    TIMESYSD_t dwWaitCount = 0;
#endif
    TIMESYSD_t dwWaitTimeMS = 0;
    THREADID_t nTid = cThreadId::GetCurrentId();  //  get my current thread id.
    for (;;) {
        THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, nTid, cThreadId::k_NULL);
        if (nTidowner == cThreadId::k_NULL || cThreadId::IsEqualId(nTidowner, nTid)) break;  // i got it. or already had it.

        // Some other thread owns the lock. Wait.
        ASSERT(cThreadId::IsValidId(nTidowner));
        if (dwDelayMS <= 0) {
#ifdef _DEBUG
            if (dwWaitCount) {
                // DEBUG_CHECK(0);	// collide cleared.
            }
#endif
            return false;  // FAILED to lock
        }
        cThreadId::SleepCurrent(dwWaitTimeMS);  // wait for a tick.
        if (dwWaitTimeMS == 0) {
            dwWaitTimeMS = 1;
        } else {
            dwDelayMS--;
        }
#ifdef _DEBUG
        dwWaitCount++;  // Count how long i had to wait.
#endif
    }
    SUPER_t::IncLockCount();
    return true;
}
}  // namespace Gray
