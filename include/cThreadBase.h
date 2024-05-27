//! @file cThreadBase.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cThreadBase_H
#define _INC_cThreadBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cOSHandle.h"
#include "cObject.h"
#include "cRefPtr.h"

#ifdef _MFC_VER
#include <atlutil.h>  // CWorkerThread
#endif
#ifdef __linux__
#include <pthread.h>
#endif

namespace Gray {
#ifdef _WIN32
typedef DWORD THREADID_t;   /// CreateThread uses LPDWORD even in 64 bit code.
#define _SIZEOF_THREADID 4  // sizeof(THREADID_t)
#elif defined(__linux__)
typedef pthread_t THREADID_t;                                                              /// @note old __linux__ gettid() is not compatible with pthreads
#define _SIZEOF_THREADID _SIZEOF_PTR
#else
#error NOOS
#endif

#ifdef _WIN32
typedef DWORD THREAD_EXITCODE_t;                                                                      /// Similar to APP_EXITCODE_t
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_RUNNING = CastN(THREAD_EXITCODE_t, STILL_ACTIVE);  /// can't get exit code if not exited. STILL_ACTIVE = 0x00000103L
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_ERR = CastN(THREAD_EXITCODE_t, -1);                /// failure exit.
static constexpr THREAD_EXITCODE_t THREAD_EXITCODE_OK = CastN(THREAD_EXITCODE_t, 0);                  /// Similar to APP_EXITCODE_t. NOT running.

#elif defined(__linux__)
typedef void* THREAD_EXITCODE_t;                                                           /// Similar to APP_EXITCODE_t
static const THREAD_EXITCODE_t THREAD_EXITCODE_RUNNING = CastNumToPtr(2);  /// can't get exit code if not exited.
static const THREAD_EXITCODE_t THREAD_EXITCODE_ERR = CastNumToPtr(1);      /// failure exit.
static const THREAD_EXITCODE_t THREAD_EXITCODE_OK = CastNumToPtr(0);       /// Similar to APP_EXITCODE_t. NOT running.

#else
#error NOOS
#endif


typedef THREAD_EXITCODE_t(_stdcall* THREAD_FUNC_t)(void*);  // entry point for a thread. same as _WIN32 PTHREAD_START_ROUTINE. like FUNCPTR_t ?

/// <summary>
/// wrapper for id for a thread.
/// ASSUME all code defined(_MT) ?
/// </summary>
class GRAYCORE_LINK cThreadId {
 protected:
    THREADID_t m_dwThreadId;  /// unique thread id. i.e. stack base pointer. (Use the MFC name m_dwThreadId)

 public:
    static const THREADID_t k_NULL = 0;  /// Not a valid thread Id. 1 might be reserved as well. see cThreadLockRW

 public:
    cThreadId(THREADID_t nThreadId = k_NULL) noexcept : m_dwThreadId(nThreadId) {}

    /// <summary>
    /// Get thread id.  Similar to the MFC CWorkerThread call.
    /// __linux__ use pthread. don't use the old fashioned gettid(). Also (m_hThread == THREADID_t)
    /// </summary>
    THREADID_t GetThreadId() const noexcept {
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
    /// <summary>
    /// set equal to the current thread id.
    /// </summary>
    void InitCurrentId() noexcept {
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
    static inline bool IsEqualId(THREADID_t a, THREADID_t b) noexcept {
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
        ::usleep((uMs)*1000);       // Sleep current thread.
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

#ifdef _WIN32
typedef HANDLE THREADHANDLE_t;  // cOSHandle in WIN32 but not in __linux__
static const THREADHANDLE_t THREADHANDLE_NULL = HANDLE_NULL;
#elif defined(__linux__)
typedef THREADID_t THREADHANDLE_t;  // same as THREADID_t pthread_t
static const THREADHANDLE_t THREADHANDLE_NULL = cThreadId::k_NULL;
#else
#error NOOS
#endif

#ifdef _MFC_VER
typedef CWorkerThread<> cThreadBase;  // GetThreadId
#else
/// <summary>
/// Basic emulation of MFC CWorkerThread.
/// similar to std::thread
/// @note not very MFC friendly!
/// NO auto CloseHandle() on destruct.
/// </summary>
class GRAYCORE_LINK cThreadBase : public cObject, public cThreadId {
 protected:
    THREADHANDLE_t m_hThread;  /// there may be many handles to the same THREADID_t in _WIN32. I must call CloseThread() on this. _WIN32 same as cOSHandle.

 public:
    cThreadBase() noexcept : m_hThread(THREADHANDLE_NULL) {}
    ~cThreadBase() override {}
};
#endif  // _MFC_VER

/// <summary>
/// Minimal implementation of a thread. Not managed externally.
/// Threads similar to MFC CWorkerThread.
/// Not exactly the same as MFC CWinThread. no message pump.
/// @note Use cRefBase so threads can prevent delete (or hold a reference to) themselves until they cleanly stop. Fire and forget.
///
/// </summary>
class GRAYCORE_LINK cThreadRef : public cThreadBase, public cRefBase, public cThreadState {
 protected:
    THREAD_EXITCODE_t m_nExitCode = THREAD_EXITCODE_OK;  /// __linux__ doesn't seem to have a way to query this after thread close. so we have to store it.

 protected:
    virtual void onThreadCreate();                           /// OnCreate for MFC
    virtual void onThreadExit(THREAD_EXITCODE_t nExitCode);  /// OnExit

    /// <summary>
    /// override this to do a chunk of work then return. (the caller will deal with sleep).
    /// Called periodically inside default implementation of Run()
    /// This thread declares itself alive.
    /// </summary>
    /// <returns>true = thread should keep running. false = thread should exit.</returns>
    virtual bool ThreadTick() {
        ASSERT(isCurrentThread());  // only call from self thread.
        return !isThreadStopping();
    }

    /// <summary>
    /// thread should override this for body. AKA WorkerThreadProc
    /// </summary>
    /// <returns></returns>
    virtual THREAD_EXITCODE_t Run();

    THREAD_EXITCODE_t RunDirectly();

    /// optional internal implementation of THREAD_FUNC_t. 
    /// We created a system thread and it calls this as THREAD_FUNC_t. _WorkerThreadProc
    /// will call RunDirectly().
    static THREAD_EXITCODE_t _stdcall EntryProc(void* pThisThread);

 public:
#ifdef _MFC_VER
    bool isCurrentThread() const {
        //! Is this the current running thread? Normally defined in cThreadId
        return cThreadId::IsEqualId(this->get_HashCode(), cThreadId::GetCurrentId());
    }
#endif

    /// <summary>
    /// Handle Is Valid?
    /// @note Doesn't mean its running. may have stopped.
    /// </summary>
    /// <returns></returns>
    bool isValidThreadHandle() const noexcept {
        return this->m_hThread != THREADHANDLE_NULL;
    }

    /// <summary>
    /// Get a unique hash code for the thread. disambiguate.
    /// </summary>
    THREADID_t get_HashCode() const noexcept {
        return this->m_dwThreadId;
    }

    virtual HRESULT CreateThread(void* pArgs = nullptr, THREAD_FUNC_t pEntryProc = EntryProc, DWORD dwCreationFlags = 0);

    THREAD_EXITCODE_t GetExitCodeThread();
    bool ExitCurrentThread(THREAD_EXITCODE_t nExitCode = THREAD_EXITCODE_OK);
};
}  // namespace Gray
#endif
