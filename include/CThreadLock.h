//! @file cThreadLock.h
//! Locking of objects for access by multiple threads
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadLock_H
#define _INC_cThreadLock_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "cInterlockedVal.h"
#include "cLocker.h"
#include "cNonCopyable.h"
#include "cThreadBase.h"
#include "cTimeSys.h"

#if defined(__linux__)
#include <pthread.h>
#endif

namespace Gray {
/// <summary>
/// Base class for thread lockable object.
/// ASSUME sizeof(THREADID_t) -lte- sizeof(UINT_PTR) _SIZEOF_PTR for __DECL_ALIGN.
/// </summary>
class GRAYCORE_LINK cThreadLockable : public cLockableBase, protected cNonCopyable {
 protected:
    THREADID_t __DECL_ALIGN(_SIZEOF_THREADID) _ThreadLockOwner;  /// The thread that has the lock. cThreadId:k_NULL is not locked (or locker thread is not relevant).

 protected:
    inline void ClearThreadLockOwner() {
        // _ThreadLockOwner = cThreadId::k_NULL;
        InterlockedN::Exchange(&_ThreadLockOwner, cThreadId::k_NULL);
    }

    inline void UnlockThread() {
        if (DecLockCount() == 0) ClearThreadLockOwner();
    }

    /// <summary>
    /// i already acquired this lock. multiple ways to get here. record it. we are assumed to be thread safe now.
    /// </summary>
    inline void LockThreadCurrent() noexcept {
        _ThreadLockOwner = cThreadId::GetCurrentId();
        IncLockCount();
        DEBUG_CHECK(isThreadLockedByCurrent());  // may have several locks on the same thread.
    }

 public:
    cThreadLockable() noexcept : _ThreadLockOwner(cThreadId::k_NULL) {}

    /// <summary>
    /// What thread owns this lock? cThreadId::k_NULL = not locked.
    /// </summary>
    inline THREADID_t get_ThreadLockOwner() const {
        const THREADID_t threadLockOwner = _ThreadLockOwner;
        return threadLockOwner;
    }
    inline bool IsThreadLockOwner(const THREADID_t nTid) const noexcept {
        const THREADID_t threadLockOwner = get_ThreadLockOwner();
        return cThreadId::IsEqualId(threadLockOwner, nTid);
    }

    /// <summary>
    /// Only thread safe way to test this is to look at _ThreadLockOwner, NOT m_nLockCount
    /// </summary>
    /// <returns></returns>
    inline bool isLocked() const noexcept {
        return !IsThreadLockOwner(cThreadId::k_NULL);
    }
    inline bool isThreadLockedByCurrent() const noexcept {
        return IsThreadLockOwner(cThreadId::GetCurrentId());
    }
    inline bool isIdle() const noexcept {
        return !isLocked() && !isLockCount();
    }

    /// <summary>
    /// release the lock. ASSUME I own the lock. so this call doesn't really need to be thread safe.
    /// </summary>
    inline void Unlock() noexcept {
        DEBUG_CHECK(isThreadLockedByCurrent());
        UnlockThread();
    }

    /// <summary>
    /// Take ownership if the lock is free or owned by the calling thread
    /// </summary>
    /// <param name="nDelayMS"></param>
    /// <returns></returns>
    bool LockThread(const THREADID_t nTid, TIMESYSD_t nDelayMS = cTimeSys::k_INF);

    /// <summary>
    /// Special case if a thread is hard terminated.
    /// Only clear lock if nThreadId is the current owner.
    /// </summary>
    /// <param name="nTid"></param>
    /// <returns></returns>
    bool ClearThreadLockOwner(THREADID_t nThreadId) {
        const THREADID_t nTidOwnerPrev = InterlockedN::CompareExchange(&_ThreadLockOwner, cThreadId::k_NULL, nThreadId);
        return cThreadId::IsEqualId(nTidOwnerPrev, nThreadId);
    }
};

/// <summary>
/// Implement simple thread lockable mechanism. lock this for multi threaded access.
/// These are fairly cheap and fast (if no collide). Slow on actual collision. (but collide assumed to be infrequent)
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// </summary>
struct GRAYCORE_LINK cThreadLockFast : public cThreadLockable {
    typedef cThreadLockable SUPER_t;

    /// <summary>
    /// Take ownership if the lock is free or owned by the calling thread. e.g. single thread can lock multiple times.
    /// Wait forever for a collision to clear.
    /// @note don't use inline. inline version of this made bad code? _MSC_VER
    /// </summary>
    [[nodiscard]] cLockerT<cThreadLockFast> Lock() noexcept;
    [[nodiscard]] cLockerT<cThreadLockFast> LockTry(TIMESYSD_t nDelayMS = 0);
};

/// <summary>
/// system mutex. similar to std::mutex
/// </summary>
struct cMutex {
#ifdef _WIN32
    cOSHandle m_h;
#else  // __linux__
    ::pthread_mutex_t m_h;                // PTHREAD_MUTEX_INITIALIZER
    static const ::pthread_mutex_t k_Init;  // PTHREAD_MUTEX_INITIALIZER

    inline ~cMutex() noexcept {
        ::pthread_mutex_destroy(&m_h);
    }
#endif

    /// If the mutex is already locked (by diff thread), the calling thread blocks until the mutex becomes available.
    /// Fail if mutex was destroyed ?
    bool Lock() noexcept {
#ifdef _WIN32
        return m_h.WaitForSingleObject(cTimeSys::k_INF) == S_OK;
#else
        return ::pthread_mutex_lock(&m_h) == 0;  // error on lock. EINVAL
#endif
    }

    // not the same as isLocked
    bool isInit() const noexcept {
#ifdef _WIN32
        return m_h.isValidHandle();
#else
        return m_h != 0;
#endif
    }

    /// <summary>
    /// Create a new system mutex.
    /// </summary>
    /// <param name="pszMutexName"></param>
    /// <param name="bInitialOwner">owned by the thread that created this.</param>
    bool InitLock(const FILECHAR_t* pszMutexName = nullptr, bool bInitialOwner = false) noexcept {
#ifdef _WIN32
        m_h.AttachHandle(_FNF(::CreateMutex)(nullptr, bInitialOwner, pszMutexName));
        return m_h.isValidHandle();
#else  // __linux__
       // Allow this mutex to be opened multiple times in the same thread.
        m_h = k_Init;  // same as ::pthread_mutex_init() ??
        if (bInitialOwner) return Lock();
        return true;
#endif
    }
    inline bool Unlock() noexcept {
#ifdef _WIN32
        return ::ReleaseMutex(m_h) ? true : false;
#else
        ::pthread_mutex_unlock(&m_h);
        return true;
#endif
    }

    bool LockTry(TIMESYSD_t nDelayMS = 0) noexcept {
#ifdef _WIN32
        return m_h.WaitForSingleObject(nDelayMS) == S_OK;
#elif defined(__USE_XOPEN2K)  // __linux__
        cTimeSpec tSpec(nDelayMS);
        return ::pthread_mutex_timedlock(&m_h, &tSpec) == 0;
#else
        // if pthread_mutex_timedlock() is not available.
        TIMESYSD_t dwWaitTimeMS = 0;
        for (;;) {
            if (::pthread_mutex_trylock(&m_h) == 0) break;
            if (nDelayMS <= 0) {  // EBUSY
                return false;      // FAILED to lock
            }
            cThreadId::SleepCurrent(dwWaitTimeMS);  // wait for a tick.
            if (dwWaitTimeMS == 0) {
                dwWaitTimeMS = 1;
            } else {
                nDelayMS--;
            }
        }
        return true;
#endif
    }
};

/// <summary>
/// Base class for data structure that may be locked for multi threaded/process access.
/// These are expensive size wise but fast in contention.
/// lock something and wait for it to be freed.
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// fast collision resolution.
/// Mutex = 1 bathroom door 1 lock. Semaphore = a set of X keys to X bathrooms.
/// _ThreadLockOwner = API wont tell me if it is locked. i have to track this myself.
/// @note I've seen use of this in Windows Service (init?) on W10 cause app exit !? 2015
/// </summary>
class GRAYCORE_LINK cThreadLockMutex : public cThreadLockable {
    typedef cThreadLockable SUPER_t;

 protected:
    cMutex m_Mutex;
    const bool m_bInitialOwner;  /// I also lock this myself. unlock on destruct.

 public:
    /// <summary>
    /// Copy constructor should never actually be used. but if it is  just make a new copy that is not locked!
    /// DUMMY = pretend we copied stuff??
    /// </summary>
    /// <param name="a"></param>
    cThreadLockMutex(const cThreadLockMutex& a) noexcept : m_bInitialOwner(false) {
        UNREFERENCED_REFERENCE(a);
        m_Mutex.InitLock();
    }
    cThreadLockMutex(const FILECHAR_t* pszMutexName = nullptr, bool bInitialOwner = false) noexcept : m_bInitialOwner(bInitialOwner) {
        if (m_Mutex.InitLock(pszMutexName, bInitialOwner) && bInitialOwner) {
            LockThreadCurrent();
        }
    }
    inline ~cThreadLockMutex() noexcept {
        // DEBUG_CHECK( ! IsLocked());
        if (m_bInitialOwner && isLocked()) {
            this->Unlock();
        }
    }

 public:
    bool LockThreadCurrent() noexcept {
        if (!m_Mutex.Lock()) return false;
        SUPER_t::LockThreadCurrent();  // may have several locks on the same thread.
        return true;
     }

    /// <summary>
    /// lock this object on a thread.
    /// This will wait forever for the resource to be available!
    /// @note It should NOT wait if it is in the same thread.
    /// </summary>
    [[nodiscard]] cLockerT<cThreadLockMutex> Lock() noexcept {
        if (LockThreadCurrent()) return cLockerT<cThreadLockMutex>(this, true);
        return cLockerT<cThreadLockMutex>(nullptr, false);        
    }

    /// <summary>
    /// Try to lock the mutex. give up after a certain amount of time if it is locked by another thread.
    /// </summary>
    /// <param name="nDelayMS">amount of time to wait. 0 = don't wait</param>
    /// <returns></returns>
    [[nodiscard]] cLockerT<cThreadLockMutex> LockTry(TIMESYSD_t nDelayMS = 0) {
        if (!m_Mutex.LockTry(nDelayMS)) return cLockerT<cThreadLockMutex>(nullptr, false);
        SUPER_t::LockThreadCurrent();
        return cLockerT<cThreadLockMutex>(this, true);
    }

    void Unlock() noexcept {
        SUPER_t::Unlock();
        m_Mutex.Unlock();
    }
};

#ifdef _WIN32
/// <summary>
/// Base class for data structure that may be locked for multi threaded access.
/// These are fairly expensive size wise but fast.
/// Use a cheaper cThreadLockFast for normal data that wont collide often.
/// Same as MFC CComCriticalSection
/// @note This is essentially the same as a mutex. CPU burn during collision.
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// _ThreadLockOwner = i have to glean this info myself. not sure why the API wont give it to me.
/// </summary>
class GRAYCORE_LINK cThreadLockCrit : public cThreadLockable {
    typedef cThreadLockable SUPER_t;

 protected:
    ::CRITICAL_SECTION m_CritSection;  // RTL_CRITICAL_SECTION, more efficient than a MUTEX. but takes more memory to store.

 private:
    void InitLockCrit() {
        //! RTL_CRITICAL_SECTION
        ::InitializeCriticalSection(&m_CritSection);
    }

 public:
    /// <summary>
    /// Copy constructor should never actually be used. but if it is just make a new copy that is not locked!
    /// DUMMY = pretend we copied stuff.
    /// </summary>
    /// <param name="a"></param>
    cThreadLockCrit(const cThreadLockCrit& a) {
        UNREFERENCED_REFERENCE(a);
        InitLockCrit();
    }
    cThreadLockCrit() {
        InitLockCrit();
    }
    inline ~cThreadLockCrit() {
        //! DEBUG_CHECK( ! IsLocked());
        ::DeleteCriticalSection(&m_CritSection);
    }

 public:
    /// <summary>
    /// lock a thread.
    /// This will wait forever for the resource to be free !
    /// @note It will NOT wait if it is in the same thread.
    /// </summary>
    [[nodiscard]] cLockerT<cThreadLockCrit> Lock() noexcept {
        ::EnterCriticalSection(&m_CritSection);
        LockThreadCurrent();
        return cLockerT<cThreadLockCrit>(this, true);
    }
    [[nodiscard]] cLockerT<cThreadLockCrit> LockTry() {
        //! don't wait.
        if (::TryEnterCriticalSection(&m_CritSection) == 0) return cLockerT<cThreadLockCrit>(nullptr, false);
        LockThreadCurrent();
        return cLockerT<cThreadLockCrit>(this, true);
    }
    void Unlock() noexcept {
        SUPER_t::Unlock();
        ::LeaveCriticalSection(&m_CritSection);
    }
};
#else
typedef cThreadLockMutex cThreadLockCrit;  // just substitute it with Mutex if not _WIN32.
#endif  // _WIN32

/// <summary>
/// Stub that does nothing. For stub out in single thread environments or debug usage.
/// </summary>
struct cThreadLockStub : public cLockableBase {
    inline THREADID_t get_ThreadLockOwner() const noexcept {
        return isLockCount() ? CastN(THREADID_t, 1) : cThreadId::k_NULL;
    }
    cLockerT<cLockableBase> Lock() {
        return cLockerT<cLockableBase>(this, true);
    }
};

//****************************************************************************

/// <summary>
/// C++ can ONLY infer type as arg to function but NOT via constructor! no idea why.
/// </summary>
template <typename T>
constexpr cLockerT<T> ToLock(const T& r) noexcept {
    return r.Lock();
}

#if defined(_MT) || defined(__linux__)
typedef cThreadLockFast cThreadLockCount;
#else
typedef cThreadLockStub cThreadLockCount;
#endif
}  // namespace Gray
#endif  // _INC_cThreadLock_H
