//
//! @file cThreadLockRW.h
//! Read/Write declarative thread locking. similar to __linux__ pthread_rwlock_t
//! more flexible/efficient than cThreadLock as most lockers are just readers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! @todo cThreadLockRW

#ifndef _INC_cThreadLockRW_H
#define _INC_cThreadLockRW_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cInterlockedVal.h"
#include "cNonCopyable.h"
#include "cThreadLock.h"
#include "cThreadLockRef.h"

namespace Gray {
/// <summary>
/// Simple NON recursive, NON upgradeable, read / write locking.
/// from: http://www.viksoe.dk/code/rwmonitor.htm
/// @todo cThreadLockRW NOT complete
/// similar to https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937(v=vs.85).aspx e.g. InitializeSRWLock()
/// </summary>
class GRAYCORE_LINK cThreadLockRWS : protected cNonCopyable {
 private:
    cInterlockedInt m_nReaders;
    cInterlockedInt m_nWriters;

 public:
    void IncReadLockCount() noexcept {
        for (;;) {
            m_nReaders.IncV();
            if (m_nWriters == 0) break;
            m_nReaders.DecV();
            cThreadId::SleepCurrent(0);
        }
    }
    void DecReadLockCount() noexcept {
        m_nReaders.DecV();
    }

    void Lock() noexcept {
        // Write lock
        for (;;) {
            if (m_nWriters.Exchange(1) == 1) {
                cThreadId::SleepCurrent(0);
            } else {
                while (m_nReaders != 0) cThreadId::SleepCurrent(0);
                break;
            }
        }
    }

    void Unlock() noexcept {
        // Write unlock
        m_nWriters.DecV();
    }
};

/// <summary>
/// @todo cThreadLockRW
/// cheap RW declaring thread locking mechanism.
/// Recursive and upgradeable (i.e. i can start with read and upgrade to write permissions on the same thread)
/// Only one thread may write lock something.
/// multiple read locks may be released out of  order, i.e first read locker releases with other readers = unknown read thread.
/// similar to : Linux thread locking pthread_rwlock_wrlock()
///
/// RULES:
///  FR = first thread reader.
///  FW = first thread writer. (just like a normal thread lock)
///  OR = other thread reader
///  OW = other thread writer.
///  if the first locker is write, FR=go, FW=go, OR=wait, OW=wait
///  if the first locker is reader, FR=go, FW=go, OR=go, OW=wait.
/// </summary>
class GRAYCORE_LINK cThreadLockRW : public cThreadLockFast {
    typedef cThreadLockFast SUPER_t;

 public:
    // cThreadLock::m_nLockThreadID;		// First reader or writers .
    // cThreadLock::m_nLockCount;			// How many write locks (for orig m_nLockThreadID)
    cInterlockedInt m_nReadLockCount;       /// How many readers (for orig m_nLockThreadID)
    cInterlockedInt m_nOtherReadLockCount;  /// How many outside (not on orig thread) readers
    bool m_bLostOrder;                      /// can't figure who is thread.

 public:
    cThreadLockRW() noexcept : m_bLostOrder(false) {}
    ~cThreadLockRW() {
        ASSERT(m_nReadLockCount == 0);
        ASSERT(m_nOtherReadLockCount == 0);
    }

    inline void IncReadLockCount() {
        // this is a slightly softer READ lock.
        if (SUPER_t::LockTry()) {
            // I'm first locker or at least the same thread as the first.
            m_nReadLockCount.IncV();
            return;
        }
        // some other thread has the write/read lock before me. can i just read it ?
        m_nOtherReadLockCount.IncV();
        if (m_nReadLockCount == get_LockCount())  // another reader is OK.
        {
        } else {
        }
    }
    inline void DecReadLockCount() {}
};

/// <summary>
/// I only want to read from this. Prevent writers from breaking me.
/// </summary>
class cThreadGuardRead : public cLockerT<cThreadLockRW> {
 public:
    cThreadGuardRead(cThreadLockRW& rLock) : cLockerT<cThreadLockRW>(rLock) {}
    // TODO: call IncReadLockCount
};

typedef cLockerT<cThreadLockRW> cThreadGuardWrite;  // I only want to write to this.

//******************************************************

/// <summary>
/// Abstract base for a smart pointer referenced object that can be locked in read or write mode.
/// similar to cThreadLockableRef
/// </summary>
class GRAYCORE_LINK cThreadLockableRW : public cRefBase, public cThreadLockRW {};

/// <summary>
/// I promise to only read from the cThreadLockableRW based object.
/// If another thread is open writing then we must wait.
/// If any thread has other read opens then it's OK.
/// No need to lock an object if 2 threads are just reading it!
/// we MUST record the read action in case a writer (on another thread) comes along.
/// @note this only returns 'const' pointers of course.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSmartReadPtr : public cRefPtr<TYPE>, public cThreadGuardRead {
    cSmartReadPtr(TYPE* pObj) : cRefPtr<TYPE>(pObj), cThreadGuardRead(*pObj) {}

    const TYPE* get_Ptr() const {
        // return the cThreadLockableRW
        return this->m_pObj;
    }
};

/// <summary>
/// we would like to write to the cThreadLockableRW based object.
/// If another thread has it open (read or write) then we must wait.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSmartWritePtr : public cRefPtr<TYPE>, public cThreadGuardWrite {
    cSmartWritePtr(TYPE* pObj) : cRefPtr<TYPE>(pObj), cThreadGuardWrite(*pObj) {}
};
}  // namespace Gray
#endif  // _INC_cThreadLockRW_H
