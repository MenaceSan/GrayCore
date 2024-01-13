//
//! @file cThreadLock.h
//! Locking of objects for access by multiple threads
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cThreadLock_H
#define _INC_cThreadLock_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "cInterlockedVal.h"
#include "cLocker.h"
#include "cNonCopyable.h"
#include "cOSHandle.h"
#include "cTimeSys.h"

#if defined(__linux__)
#include <pthread.h>
#endif

namespace Gray {
#ifdef _WIN32
typedef DWORD THREADID_t;   /// CreateThread uses LPDWORD even in 64 bit code.
#define _SIZEOF_THREADID 4  // sizeof(THREADID_t)
#elif defined(__linux__)
typedef pthread_t THREADID_t;                                                         /// @note old __linux__ gettid() is not compatible with pthreads
#define _SIZEOF_THREADID _SIZEOF_PTR
#else
#error NOOS
#endif

#ifdef _WIN32
typedef DWORD THREAD_EXITCODE_t;                                                                 /// Similar to APP_EXITCODE_t
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_RUNNING = ((THREAD_EXITCODE_t)STILL_ACTIVE);  /// can't get exit code if not exited. STILL_ACTIVE = 0x00000103L
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_ERR = ((THREAD_EXITCODE_t)-1);                /// failure exit.

#elif defined(__linux__)
typedef void* THREAD_EXITCODE_t;                                                      /// Similar to APP_EXITCODE_t
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_RUNNING = ((THREAD_EXITCODE_t)2);  /// can't get exit code if not exited.
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_ERR = ((THREAD_EXITCODE_t)1);      /// failure exit.

#else
#error NOOS
#endif

static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_OK = ((THREAD_EXITCODE_t)0);  /// Similar to APP_EXITCODE_t. NOT running.

typedef THREAD_EXITCODE_t(_stdcall* THREAD_FUNC_t)(void*);  // entry point for a thread. same as _WIN32 PTHREAD_START_ROUTINE. like FARPROC ?

/// <summary>
/// base static namespace for common thread functions.
/// ASSUME all code wants defined(_MT) ?
/// </summary>
class GRAYCORE_LINK cThreadId {
 protected:
    THREADID_t m_dwThreadId;  /// unique thread id. i.e. stack base pointer. (Use the MFC name)

 public:
    static const THREADID_t k_NULL = 0;  /// Not a valid thread Id.

 public:
    cThreadId(THREADID_t nThreadId = k_NULL) noexcept : m_dwThreadId(nThreadId) {}
    THREADID_t GetThreadId() const noexcept {
        //! Similar to the MFC CWorkerThread call.
        //! __linux__ use pthread. don't use the old fashioned gettid(). Also (m_hThread == THREADID_t)
        return m_dwThreadId;
    }
    /// <summary>
    /// Get a unique hash code for the thread.
    /// </summary>
    /// <returns></returns>
    THREADID_t get_HashCode() const noexcept {
        return m_dwThreadId;
    }
    /// <summary>
    /// Is this the current running thread?
    /// </summary>
    bool isCurrentThread() const noexcept {
        return IsEqualId(m_dwThreadId, GetCurrentId());
    }
    bool isValidId() const noexcept {
        return IsValidId(m_dwThreadId);
    }
    void InitCurrentId() noexcept {
        //! set equal to the current thread id.
        m_dwThreadId = GetCurrentId();
    }

    /// <summary>
    /// Get the callers ThreadId.
    /// @note We ASSUME this is VERY fast.
    /// ASSUME IsValidThreadId();
    /// </summary>
    /// <returns></returns>
    static inline THREADID_t GetCurrentId() noexcept {
#ifdef _WIN32
        return ::GetCurrentThreadId();
#else  // __linux__
        return ::pthread_self();
#endif
    }
    static constexpr bool IsValidId(THREADID_t id) noexcept {
        //! Is this thread valid? the system thread is considered valid.
        return id != cThreadId::k_NULL;
    }
    static constexpr bool IsEqualId(THREADID_t a, THREADID_t b) noexcept {
        //! Are these id's the same thread? In Linux this might be similar to _WIN32 HANDLE.
#ifdef _WIN32
        return a == b;
#else
        return ::pthread_equal(a, b);
#endif
    }

    /// <summary>
    /// Sleep current thread for n Milliseconds. cTimeSys::k_FREQ.
    /// Let the OS schedule something else during this time.
    /// </summary>
    /// <param name="uMs"></param>
    static inline void SleepCurrent(TIMESYS_t uMs = cTimeSys::k_FREQ) noexcept {
#ifdef _WIN32
        ::Sleep(static_cast<DWORD>(uMs));
#else
        ::usleep((uMs)*1000);                  // Sleep current thread.
#endif
    }
};

/// <summary>
/// Query the status/state of a thread/job and possibly attempt to cancel it.
/// Similar to ICancellable and useful with IStreamProgressCallback
/// </summary>
class GRAYCORE_LINK cThreadState {
 protected:
    bool m_bThreadRunning;            /// called CreateThread() onThreadCreate(), and inside Run(), until onThreadExit()
    VOLATILE bool m_bThreadStopping;  /// trying to stop the thread nicely. Do this before TerminateThread()

 public:
    cThreadState() noexcept : m_bThreadRunning(false), m_bThreadStopping(false) {}

    /// <summary>
    /// is Running? though may be stopping/sleeping/suspended/etc.
    /// </summary>
    bool isThreadRunning() const noexcept {
        return m_bThreadRunning;
    }
    /// <summary>
    /// Thread MUST periodically check this !
    /// </summary>
    /// <returns></returns>
    bool isThreadStopping() const noexcept {
        return m_bThreadStopping;
    }

    virtual bool WaitForThreadExit(TIMESYSD_t iTimeMSec) noexcept;

    virtual bool RequestStopThread(bool bWillWait = false) noexcept {
        UNREFERENCED_PARAMETER(bWillWait);
        m_bThreadStopping = true;
        return isThreadRunning();
    }
};

/// <summary>
/// ASSUME sizeof(THREADID_t) -lte- sizeof(UINT_PTR) _SIZEOF_PTR for __DECL_ALIGN.
/// </summary>
class GRAYCORE_LINK cThreadLockBase : public cLockableBase, protected cNonCopyable {
 protected:
    THREADID_t __DECL_ALIGN(_SIZEOF_THREADID) m_nLockThreadID;  /// The thread that has the lock. cThreadId:k_NULL is not locked.
 public:
    cThreadLockBase() noexcept : m_nLockThreadID(cThreadId::k_NULL) {}
    /// <summary>
    /// Only thread safe way to test this is to look at m_nLockThreadID, NOT m_nLockCount
    /// </summary>
    /// <returns></returns>
    inline bool isLocked() const noexcept {
        return m_nLockThreadID != cThreadId::k_NULL;
    }
    inline THREADID_t get_ThreadLockOwner() const {
        //! @return cThreadId::k_NULL = not locked.
        const THREADID_t nTid1 = m_nLockThreadID;
        ASSERT(nTid1 == cThreadId::k_NULL || cThreadId::IsValidId(nTid1));
        return nTid1;
    }
    inline bool isThreadLockedByCurrent() const noexcept {
        const THREADID_t nTid1 = m_nLockThreadID;
        const THREADID_t nTid2 = cThreadId::GetCurrentId();
        return cThreadId::IsEqualId(nTid1, nTid2);
    }
};

/// <summary>
/// used with any data structure that may be locked for multi threaded access.
/// These are fairly cheap and fast. Slow on actual collision. (but collide assumed to be infrequent)
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// </summary>
class GRAYCORE_LINK cThreadLockFast : public cThreadLockBase {
    typedef cThreadLockBase SUPER_t;

 private:
    cThreadLockFast(const cThreadLockFast& a) noexcept {
        //! Copy constructor should never actually be called!
        //! but if it is, just make a new copy that is not locked!
        //! pretend we copied stuff?
        UNREFERENCED_REFERENCE(a);
    }

 public:
    cThreadLockFast() noexcept {}
    ~cThreadLockFast() noexcept {}

    /// <summary>
    /// Special case if a thread is hard terminated.
    /// Only clear lock if nThreadId is the current owner.
    /// </summary>
    /// <param name="nTid"></param>
    /// <returns></returns>
    bool ClearThreadLockOwner(THREADID_t nThreadId) {
        const THREADID_t nTidOwner = InterlockedN::CompareExchange(&m_nLockThreadID, cThreadId::k_NULL, nThreadId);
        return cThreadId::IsEqualId(nTidOwner, nThreadId);
    }

    /// <summary>
    /// Take ownership if the lock is free or owned by the calling thread.
    /// Wait forever for a collision to clear.
    /// @note don't use inline. inline version of this made bad code? _MSC_VER
    /// </summary>
    void Lock();
    bool LockTry(TIMESYSD_t dwDelayMS = 0);

    inline void Unlock() {
        //! inline version of this made bad code?
        //! ASSUME I own the lock. so this call doesn't really need to be thread safe.
        ASSERT(isThreadLockedByCurrent());
        if (SUPER_t::DecLockCount() == 0) {
            InterlockedN::Exchange(&m_nLockThreadID, cThreadId::k_NULL);
        }
    }
};
typedef cLockerT<cThreadLockFast> cThreadGuardFast;

/// <summary>
/// Base class for data structure that may be locked for multi threaded/process access.
/// similar to std::mutex
/// These are expensive size wise but fast.
/// lock something and wait for it to be freed.
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// fast collision resolution.
/// Mutex = 1 bathroom door 1 lock. Semaphore = a set of X keys to X bathrooms.
/// m_nLockThreadID = API wont tell me if it is locked. i have to track this myself.
/// @note I've seen use of this in Windows Service on W10 cause app exit !? 2015
/// </summary>
class GRAYCORE_LINK cThreadLockMutex : public cThreadLockBase {
    typedef cThreadLockBase SUPER_t;

 public:
#ifdef _WIN32
    cOSHandle m_Mutex;
#else  // __linux__
    pthread_mutex_t m_Mutex;                   // PTHREAD_MUTEX_INITIALIZER
    static const pthread_mutex_t k_MutexInit;  // PTHREAD_MUTEX_INITIALIZER
#endif
 protected:
    bool m_bInitialOwner;  /// I also lock this myself.

 public:
    cThreadLockMutex(const cThreadLockMutex& a) noexcept : m_bInitialOwner(false) {
        //! Copy constructor should never actually be used.
        //! but if it is  just make a new copy that is not locked!
        //! DUMMY = pretend we copied stuff.
        UNREFERENCED_REFERENCE(a);
        InitLockMutex(nullptr, false);
    }
    cThreadLockMutex(const FILECHAR_t* pszMutexName = nullptr, bool bInitialOwner = false) noexcept : m_bInitialOwner(bInitialOwner) {
        InitLockMutex(pszMutexName, bInitialOwner);
    }
    inline ~cThreadLockMutex() noexcept {
        // DEBUG_CHECK( ! IsLocked());
        if (m_bInitialOwner && isLocked())  // m_nLockCount == 1
        {
            this->Unlock();
        }
#if defined(__linux__)
        ::pthread_mutex_destroy(&m_Mutex);
#endif
    }

 protected:
    void InitLockMutex(const FILECHAR_t* pszMutexName, bool bInitialOwner) noexcept {
        //! Create a system mutex.
        //! @arg bInitialOwner = owned by the thread that created this.
#ifdef _WIN32
        m_Mutex.AttachHandle(_FNF(::CreateMutex)(nullptr, bInitialOwner, pszMutexName));
        if (bInitialOwner) {
            LockInternal();
        }
#else  // __linux__
       // Allow this mutex to be opened multiple times in the same thread.
        m_Mutex = k_MutexInit;                 // same as ::pthread_mutex_init() ??
        if (bInitialOwner) {
            Lock();
        }
#endif
    }

    inline void LockInternal() {
        //! i was able to acquire this Mutex. some other way like WaitForMultipleObjects() ?
        m_nLockThreadID = cThreadId::GetCurrentId();
        SUPER_t::IncLockCount();
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
    }

 public:
    inline bool Lock() {
        //! code to lock a thread.
        //! This will wait forever for the resource to be free !
        //! @note It should NOT wait if it is in the same thread.
#ifdef _WIN32
        const HRESULT hRes = m_Mutex.WaitForSingleObject(cTimeSys::k_INF);
        if (hRes != S_OK) {
            return false;  // maybe mutex was destroyed?
        }
#else
        // If the mutex is already locked (by diff thread), the calling thread blocks until the mutex becomes available
        int iRet = ::pthread_mutex_lock(&m_Mutex);
        if (iRet) {
            return false;  // error on lock. EINVAL
        }
#endif
        LockInternal();  // may have several locks on the same thread.
        return true;
    }
    inline bool Unlock() {
        // Assume i own the lock. so thread safe isn't a worry on entry.
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
        if (SUPER_t::DecLockCount() <= 0) {
            m_nLockThreadID = cThreadId::k_NULL;
        }
#ifdef _WIN32
        return ::ReleaseMutex(m_Mutex) ? true : false;
#else
        ::pthread_mutex_unlock(&m_Mutex);
        return true;
#endif
    }

    bool LockTry(TIMESYSD_t dwDelayMS = 0) {
        //! Try to lock the mutex. give up after a certain amount of time if it is locked by another thread.
        //! dwDelayMS = amount of time to wait. 0 = don't wait
#ifdef _WIN32
        const HRESULT hRes = m_Mutex.WaitForSingleObject(dwDelayMS);
        if (hRes != S_OK) {
            return false;
        }
#elif defined(__USE_XOPEN2K)  // __linux__
        cTimeSpec tSpec(dwDelayMS);
        const int iRet = ::pthread_mutex_timedlock(&m_Mutex, &tSpec);
        if (iRet != 0) {
            return false;
        }
#else
        // if pthread_mutex_timedlock() is not available.
        TIMESYSD_t dwWaitTimeMS = 0;
        for (;;) {
            if (::pthread_mutex_trylock(&m_Mutex) == 0) break;
            if (dwDelayMS <= 0)  // EBUSY
            {
                return false;  // FAILED to lock
            }
            cThreadId::SleepCurrent(dwWaitTimeMS);  // wait for a tick.
            if (dwWaitTimeMS == 0) {
                dwWaitTimeMS = 1;
            } else {
                dwDelayMS--;
            }
        }
#endif
        LockInternal();
        return true;
    }
};

typedef cLockerT<cThreadLockMutex> cThreadGuardMutex;

#ifdef _WIN32
/// <summary>
/// Base class for data structure that may be locked for multi threaded access.
/// These are fairly expensive size wise but fast.
/// Use a cheaper cThreadLockFast for normal data that wont collide often.
/// Same as MFC CComCriticalSection
/// @note This is essentially the same as a mutex. CPU burn during collision.
/// @note reentrant, multi locks on a single thread are allowed and counted.
/// m_nLockThreadID = i have to glean this info myself. not sure why the API wont give it to me.
/// </summary>
class GRAYCORE_LINK cThreadLockCrit : public cThreadLockBase {
 protected:
    ::CRITICAL_SECTION m_CritSection;  // RTL_CRITICAL_SECTION, more efficient than a MUTEX. but takes more memory to store.

 private:
    void LockInternal() {
        m_nLockThreadID = cThreadId::GetCurrentId();
        IncLockCount();
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
    }
    void InitLockCrit() {
        //! RTL_CRITICAL_SECTION
        ::InitializeCriticalSection(&m_CritSection);
    }

 public:
    cThreadLockCrit(const cThreadLockCrit& a) {
        //! Copy constructor should never actually be used.
        //! but if it is  just make a new copy that is not locked!
        //! DUMMY = pretend we copied stuff.
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
    inline void Lock() {
        //! code to lock a thread.
        //! This will wait forever for the resource to be free !
        //! @note It will NOT wait if it is in the same thread.
        ::EnterCriticalSection(&m_CritSection);
        LockInternal();
    }
    inline void Unlock() {
        ASSERT(isThreadLockedByCurrent());  // may have several locks on the same thread.
        if (DecLockCount() <= 0) {
            m_nLockThreadID = cThreadId::k_NULL;
        }
        ::LeaveCriticalSection(&m_CritSection);
    }

    bool LockTry() {
        //! don't wait.
        if (::TryEnterCriticalSection(&m_CritSection) == 0) return false;
        LockInternal();
        return true;
    }
};
#else
typedef cThreadLockMutex cThreadLockCrit;  // just substitute it if not _WIN32.
#endif  // _WIN32

/// <summary>
/// Stub that does nothing. For stub out in single thread environments or debug usage.
/// </summary>
struct cThreadLockStub : public cLockableBase {
    inline THREADID_t get_ThreadLockOwner() const {
        //! @return cThreadId::k_NULL = not locked.
        return (THREADID_t)isLocked() ? 1 : 0;
    }
};

//****************************************************************************

#if defined(_MT) || defined(__linux__)
typedef cThreadLockFast cThreadLockCount;
#else
typedef cThreadLockStub cThreadLockCount;
#endif
typedef cLockerT<cThreadLockCount> cThreadGuard;  // instantiated locker.
}  // namespace Gray
#endif  // _INC_cThreadLock_H
