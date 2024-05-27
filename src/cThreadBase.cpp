//! @file cThreadBase.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cThreadBase.h"
#include "cExceptionSystem.h"

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

void cThreadRef::onThreadCreate() {  // virtual
    ASSERT(isThreadRunning());
    const THREADID_t nThreadId = this->GetThreadId();
    ASSERT(nThreadId != cThreadId::k_NULL);  // ASSUME set.
    UNREFERENCED_PARAMETER(nThreadId);
    ASSERT(isCurrentThread());               // GetThreadId() is set already in CreateThread
    m_nExitCode = THREAD_EXITCODE_RUNNING;
#if defined(_CPPUNWIND)
    cExceptionSystem::InitForCurrentThread();  // must be called for each thread.
#endif
}

void cThreadRef::onThreadExit(THREAD_EXITCODE_t nExitCode) {  // virtual
    ASSERT(nExitCode != THREAD_EXITCODE_RUNNING);
    DEBUG_CHECK(isThreadRunning());
    DEBUG_CHECK(isCurrentThread());
    m_nExitCode = nExitCode;
    m_bThreadStopping = true;  // probably already set anyhow.
    m_bThreadRunning = false;
}

THREAD_EXITCODE_t cThreadRef::Run() {  // virtual
    //! implementation should virtual override this (or ThreadTick) for body.
    while (ThreadTick()) {
        // Sleep if no time was taken in ThreadTick()?
    }
    return THREAD_EXITCODE_OK;
}

THREAD_EXITCODE_t cThreadRef::RunDirectly() {
    //! Run cThread in the current system thread.
    ASSERT_NN(this);
    cRefPtr<cThreadRef> pThis(this);  // prevent destruct from occurring in the onThreadExit() call.
    onThreadCreate();
    ASSERT(isCurrentThread());  // already call AttachToCurrentThread
    ASSERT(isValidCheck());
    const THREAD_EXITCODE_t nExitCode = this->Run();  // call virtual
    onThreadExit(nExitCode);                          // This fake thread has exited.
    return nExitCode;
}

THREAD_EXITCODE_t _stdcall cThreadRef::EntryProc(void* pThisThread) {  // static
    return reinterpret_cast<cThreadRef*>(pThisThread)->RunDirectly();
}

HRESULT cThreadRef::CreateThread(void* pArgs, THREAD_FUNC_t pEntryProc, DWORD dwCreationFlags) {
    if (pEntryProc == nullptr) {
        DEBUG_CHECK(0);
        return E_POINTER;
    }
    if (pArgs == nullptr) pArgs = this;
    m_bThreadRunning = true;           // assume success.
    m_bThreadStopping = false;
    m_dwThreadId = cThreadId::k_NULL;  // we are about to query this.

    // Assume onThreadCreate() will be called.
#ifdef _WIN32
    // same as _beginthreadex()
    // ASSUME: m_dwThreadId is set before actually run thread code.
    m_hThread = ::CreateThread(nullptr,     // LPSECURITY_ATTRIBUTES lpThreadAttributes,
                               0,           // SIZE_T dwStackSize,
                               pEntryProc,  // LPTHREAD_START_ROUTINE lpStartAddress,
                               pArgs,       // LPVOID lpParameter,
                               dwCreationFlags, &m_dwThreadId);
    if (m_hThread == THREADHANDLE_NULL) {
        m_bThreadRunning = false;  // never did anything.
        return HResult::GetLastDef();
    }
#elif defined(__linux__)
    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // ASSUME POSIX default

    const int nErrNo = ::pthread_create(&m_hThread, nullptr, pEntryProc, pArgs);
    // ::pthread_attr_destroy(&attr);
    if (nErrNo != 0) {
        m_hThread = THREADHANDLE_NULL;
        m_bThreadRunning = false;  // never did anything.
        return HResult::GetDef(HResult::FromPOSIX(nErrNo));
    }

    m_dwThreadId = m_hThread;
#endif

    ASSERT(m_dwThreadId != cThreadId::k_NULL);
    return S_OK;
}

THREAD_EXITCODE_t cThreadRef::GetExitCodeThread() {
    //! Get the value set by the thread when it returned.
    //! @return THREAD_EXITCODE_RUNNING = the thread is still running.
    //! @return THREAD_EXITCODE_ERR = the thread was hard terminated.
#ifdef _WIN32
    if (m_nExitCode == THREAD_EXITCODE_RUNNING) {
        // AttachToCurrentThread will not have handle!
        if (m_hThread == nullptr) return THREAD_EXITCODE_RUNNING;
        if (!::GetExitCodeThread(m_hThread, &m_nExitCode)) {
            // the thread handle is bad!
            HRESULT hRes = HResult::GetLastDef();
            UNREFERENCED_PARAMETER(hRes);
            return THREAD_EXITCODE_ERR;
        }
    }
#endif
    return m_nExitCode;  // No OS support for this in __linux__
}
bool cThreadRef::ExitCurrentThread(THREAD_EXITCODE_t nExitCode) {
    //! Exit the current thread.
    //! Must be called on the current cThread.
    //! Same as just returning from entrypoint()

    ASSERT(nExitCode != THREAD_EXITCODE_RUNNING);
    if (!isCurrentThread()) {
        DEBUG_CHECK(0);  // we should not do this!
        return false;
    }
    ASSERT(isThreadRunning());
    onThreadExit(nExitCode);
    // cThread might already be destroyed!
#ifdef _WIN32
    ::ExitThread(nExitCode);
#else
    ::pthread_exit(nExitCode);
#endif
    UNREACHABLE_CODE(__noop);
}
}  // namespace Gray
