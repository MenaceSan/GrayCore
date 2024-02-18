//! @file cThreadLockRW.h
//! Read/Write declarative thread locking. similar to __linux__ pthread_rwlock_t
//! more flexible/efficient than cThreadLock as most lockers are just readers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadLockRW_H
#define _INC_cThreadLockRW_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cThreadLock.h"

namespace Gray {
/// <summary>
/// Read/Write thread locking mechanism.
/// Only one thread may write lock something.
/// @note multiple read locks may be released out of  order, i.e first read locker releases with other readers => unknown last read thread.
/// similar to : Linux thread locking pthread_rwlock_wrlock() or std::shared_mutex
/// </summary>
class GRAYCORE_LINK cThreadLockRW : public cThreadLockFast {
    friend struct cThreadGuardRead;
    typedef cThreadLockFast SUPER_t;
    static const THREADID_t kReaderThreadId = 1;  // shared unique thread id for all readers.
    cThreadLockFast m_ReaderLock;                 // Since this is shared with multiple threads it must be locked from itself.
    /// <summary>
    /// lock this for Read
    /// </summary>
    inline void LockRead() noexcept {
        if (this->isThreadLockedByCurrent()) {
            // special case where i can allow this. downgrade.
            SUPER_t::IncLockCount();
            return;
        }
        auto guard(m_ReaderLock.Lock());
        LockThread(kReaderThreadId, cTimeSys::k_INF);
        ASSERT(IsThreadLockOwner(kReaderThreadId));
    }

    /// <summary>
    /// unlock this for Read
    /// </summary>
    inline void UnlockRead() noexcept {
        if (this->isThreadLockedByCurrent()) {
            // special case where i can allow this. downgrade.
            SUPER_t::Unlock();
            return;
        }
        // we may not be the same real thread that locked this originally!
        auto guard(m_ReaderLock.Lock());
        ASSERT(IsThreadLockOwner(kReaderThreadId));
        UnlockThread();
    }

 public:
    int get_ReaderCount() const noexcept {
        if (!IsThreadLockOwner(kReaderThreadId)) return 0;
        return this->get_LockCount();
    }
};

/// <summary>
/// I only want to read from this. Prevent writer from breaking me. Allow other concurrent readers.
/// TODO: NOT upgradeable on the same thread to Write
/// </summary>
struct cThreadGuardRead : public cPtrFacade<cThreadLockRW> {
    typedef cPtrFacade<cThreadLockRW> SUPER_t;
    cThreadGuardRead(cThreadLockRW& rLock) : SUPER_t(&rLock) {
        rLock.LockRead();
    }
    ~cThreadGuardRead() {
        if (SUPER_t::isValidPtr()) {
            SUPER_t::get_Ptr()->UnlockRead();
        }
    }
    cLockerT<cThreadLockFast> Upgrade() {
        // TODO
        if (SUPER_t::isValidPtr()) {
            SUPER_t::get_Ptr()->UnlockRead();  // release my read first.
        }
        return SUPER_t::get_Ptr()->Lock();  // now get full access lock.
    }
    /// <summary>
    /// return the cRefBaseRW as a const* ONLY!!
    /// </summary>
    const cThreadLockRW* get_Ptr() const {
        return SUPER_t::get_Ptr();
    }
};
}  // namespace Gray
#endif  // _INC_cThreadLockRW_H
