//! @file cThreadLock.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cThreadLock.h"

namespace Gray {

bool cLockableBase::WaitUnique(TIMESYSD_t nWaitMS) {
    TIMESYSD_t nWaitCount = 0;
    TIMESYSD_t nWaitTickMS = 0;  // No wait the first tick.
    while (!this->isLockCount()) {
        if (nWaitMS <= 0) return false;        // FAILED to lock in time.
        cThreadId::SleepCurrent(nWaitTickMS);  // wait for a tick.
        nWaitTickMS = 1;
        if (nWaitMS != cTimeSys::k_INF) nWaitMS--;
        nWaitCount++;  // Count how long i had to wait. for _DEBUG.
    }
    return true;
}

bool cThreadLockable::LockThread(const THREADID_t nTid, TIMESYSD_t nWaitMS) {
    ASSERT(cThreadId::IsValidId(nTid));
    ASSERT(nWaitMS == cTimeSys::k_INF || nWaitMS >= 0);  // not negative.
#ifdef _DEBUG
    TIMESYSD_t nWaitCount = 0;
#endif
    TIMESYSD_t nWaitTickMS = 0; // No wait the first tick.
    for (;;) {
        const THREADID_t nTidOwnerPrev = InterlockedN::CompareExchange(&_ThreadLockOwner, nTid, cThreadId::k_NULL);
        if (nTidOwnerPrev == cThreadId::k_NULL || cThreadId::IsEqualId(nTidOwnerPrev, nTid)) {  // i got it. (or already had it)
            IncLockCount();
            ASSERT(IsThreadLockOwner(nTid));  // i locked it!
            return true;
        }

        // Some other thread owns the lock. Wait.
        ASSERT(cThreadId::IsValidId(nTidOwnerPrev));

        if (nWaitMS == 0) {
#ifdef _DEBUG
            if (nWaitCount) {
                // DEBUG_CHECK(0);	// collide cleared.
            }
#endif
            return false;  // FAILED to lock in time.
        }
        cThreadId::SleepCurrent(nWaitTickMS);  // wait for a tick.
        nWaitTickMS = 1;
        if (nWaitMS != cTimeSys::k_INF) nWaitMS--;

#ifdef _DEBUG
        nWaitCount++;  // Count how long i had to wait.
#endif
    }
}

cLockerT<cThreadLockableFast> cThreadLockableFast::Lock() noexcept {
    LockThread(cThreadId::GetCurrentId(), cTimeSys::k_INF);
#ifdef _DEBUG
    ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
#endif
    return cLockerT<cThreadLockableFast>(this, true);
}

cLockerT<cThreadLockableFast> cThreadLockableFast::LockTry(TIMESYSD_t nWaitMS) {
    if (LockThread(cThreadId::GetCurrentId(), nWaitMS)) {
#ifdef _DEBUG
        ASSERT(get_LockCount() == 1);
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
#endif
        return cLockerT<cThreadLockableFast>(this, true);
    }
    return cLockerT<cThreadLockableFast>(nullptr, false);  // failed to lock in time!
}
}  // namespace Gray
