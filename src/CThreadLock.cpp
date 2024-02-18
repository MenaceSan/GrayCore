//! @file cThreadLock.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cThreadLock.h"

namespace Gray {
#ifdef __linux__
const pthread_mutex_t cMutex::k_Init = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

bool cLockableBase::WaitUnique(TIMESYSD_t nDelayMS) {
#ifdef _DEBUG
    TIMESYSD_t nWaitCount = 0;
#endif
    TIMESYSD_t nWaitTimeMS = 0;
    while (!this->isLockCount()) {
        if (nDelayMS <= 0) {
            return false;  // FAILED to lock
        }
        cThreadId::SleepCurrent(nWaitTimeMS);  // wait for a tick.
        if (nWaitTimeMS == 0) nWaitTimeMS = 1;
        if (nDelayMS != cTimeSys::k_INF) nDelayMS--;

#ifdef _DEBUG
        nWaitCount++;  // Count how long i had to wait.
#endif
    }
    return true;
}

bool cThreadLockable::LockThread(const THREADID_t nTid, TIMESYSD_t nDelayMS) {
    ASSERT(cThreadId::IsValidId(nTid));
    ASSERT(nDelayMS == cTimeSys::k_INF || nDelayMS >= 0);
#ifdef _DEBUG
    TIMESYSD_t nWaitCount = 0;
#endif
    TIMESYSD_t nWaitTimeMS = 0;
    for (;;) {
        const THREADID_t nTidOwnerPrev = InterlockedN::CompareExchange(&_ThreadLockOwner, nTid, cThreadId::k_NULL);
        if (nTidOwnerPrev == cThreadId::k_NULL || cThreadId::IsEqualId(nTidOwnerPrev, nTid)) {  // i got it. (or already had it)
            IncLockCount();
            ASSERT(IsThreadLockOwner(nTid));  // i locked it!
            return true;
        }

        // Some other thread owns the lock. Wait.
        ASSERT(cThreadId::IsValidId(nTidOwnerPrev));

        if (nDelayMS == 0) {
#ifdef _DEBUG
            if (nWaitCount) {
                // DEBUG_CHECK(0);	// collide cleared.
            }
#endif
            return false;  // FAILED to lock in time.
        }
        cThreadId::SleepCurrent(nWaitTimeMS);  // wait for a tick.
        if (nWaitTimeMS == 0) nWaitTimeMS = 1;
        if (nDelayMS != cTimeSys::k_INF) nDelayMS--;

#ifdef _DEBUG
        nWaitCount++;  // Count how long i had to wait.
#endif
    }
}

cLockerT<cThreadLockFast> cThreadLockFast::Lock() noexcept {
    LockThread(cThreadId::GetCurrentId(), cTimeSys::k_INF);
#ifdef _DEBUG
    ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
#endif
    return cLockerT<cThreadLockFast>(this, true);
}

cLockerT<cThreadLockFast> cThreadLockFast::LockTry(TIMESYSD_t nDelayMS) {
    if (LockThread(cThreadId::GetCurrentId(), nDelayMS)) {
#ifdef _DEBUG
        ASSERT(get_LockCount() == 1);
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
#endif
        return cLockerT<cThreadLockFast>(this, true);
    }
    return cLockerT<cThreadLockFast>(nullptr, false);
}
}  // namespace Gray
