//! @file cOSProcess.h
//! @note Launching processes is a common basic feature for __linux__
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSProcess_H
#define _INC_cOSProcess_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFilePath.h"
#include "cOSHandle.h"
#include "cThreadLock.h"

namespace Gray {
class cFile;

#ifdef _WIN32
typedef int SHOWWINDOW_t;  /// show window mode for _WIN32. enum SW_SHOW, SW_HIDE etc.
#elif defined(__linux__)
/// <summary>
/// mode to show or hide a HWND window.
/// </summary>
enum SHOWWINDOW_t {
    SW_HIDE = 0,
    SW_SHOWNORMAL = 1,
    SW_SHOWMINIMIZED,
    SW_SHOWMAXIMIZED,  /// Normal window maximized with top bar showing.
    SW_SHOWDEFAULT = 10,
    /// SW_FULLSCREEN = 12 = NON Standard. Full screen mode (for games/videos). NOT the same as Maximized. No bar.
};
#else
#error NOOS
#endif

#ifdef _WIN32
typedef DWORD PROCESSID_t;      /// EnumProcesses uses DWORD even in 64 bit code.
#elif defined(__linux__)
typedef pid_t PROCESSID_t;    /// pid_t getpid() for __linux__
#endif
constexpr PROCESSID_t kPROCESSID_BAD = 0;  /// Invalid process id. 

/// <summary>
/// http://en.wikipedia.org/wiki/Exit_status = 'errorlevel' return from POSIX process.
/// int main() _tmain() and WinMain() return value. BUT its DWORD in GetExitCodeProcess() and UINT for ExitProcess.
/// </summary>
enum class APP_EXITCODE_t : int {
    _ERRNO = -1,           /// See Posix 'errno' for why this process never started.
    _OK = EXIT_SUCCESS,    /// 0=EXIT_SUCCESS (stdlib.h).  App closed.
    _FAIL = EXIT_FAILURE,  /// 1=EXIT_FAILURE = generic error. App closed.
    _ABORT = 3,            /// 3=Default error returned if "abort()" used (arbitrary?)  App closed.

// .. some other condition.
#ifdef _WIN32
    _STILL_ACTIVE = STILL_ACTIVE,  /// _WIN32 Process has not exited yet. STATUS_PENDING
#elif defined(__linux__)
    _STILL_ACTIVE = 0x103,    /// 259 = Process has not exited yet.
#endif
    _UNK = SHRT_MAX,  /// handle not valid ?
};

/// <summary>
/// A running process in the system. May or may not be the current process.
/// handle to some active process I started (me or my child).
/// Related to MIME_EXT_exe PROCESSID_t. Related to cOSModule.
/// </summary>
class GRAYCORE_LINK cOSProcess {
 protected:
    PROCESSID_t m_nPid;  /// Process ID, 0 = kPROCESSID_BAD = un-init.

#ifdef _WIN32
    cOSHandle m_hProcess;  /// open handle to the process. cOSModule
 public:
    cThreadId m_ThreadId;  /// Main thread id for the process. Only if i launched this myself.
    cOSHandle m_hThread;   /// may have this or not. Only if i launched this myself.
#elif defined(__linux__)
 protected:
    cFilePath m_sPath;  /// cached file path for the EXE/ELF/etc file.
#else
#error NOOS
#endif

 public:
    cOSProcess() noexcept;
    virtual ~cOSProcess();

#ifdef _WIN32
    cOSProcess(PROCESSID_t nPid, HANDLE hProc, HANDLE hThread) noexcept : m_nPid(nPid), m_hProcess(hProc), m_hThread(hThread) {
        //! _WIN32 = ::GetCurrentProcess() = 0xFFFFFFFF as a shortcut. HMODULE_CURPROC
    }
    HANDLE get_ProcessHandle() const noexcept {
        return m_hProcess.get_Handle();
    }
    void CloseProcessHandle();
#endif
    HRESULT CreateProcessX(const FILECHAR_t* pszExeName, const FILECHAR_t* pszArgs = nullptr, SHOWWINDOW_t nShowCmd = SW_SHOWNORMAL, const FILECHAR_t* pszCurrentDir = nullptr, cFile* pFileOutPipe = nullptr);

    static inline bool GRAYCALL IsSystemPID(PROCESSID_t nProcessID) noexcept {
        if (nProcessID == 0)  // kPROCESSID_BAD
            return true;
#ifdef _WIN32
        if (nProcessID == 4) return true;
#endif
        return false;
    }

    bool isValidProcess() const noexcept {
        //! Is the process in memory/valid/active now ?
#ifdef _WIN32
        return m_hProcess.isValidHandle();
#elif defined(__linux__)
        return m_nPid != 0;
#endif
    }

    PROCESSID_t get_ProcessId() const noexcept {
        return m_nPid;
    }

    virtual cFilePath get_ProcessPath() const;
    cStringF get_ProcessName() const;
    HRESULT OpenProcessId(PROCESSID_t dwProcessID, DWORD dwDesiredAccess = 0, bool bInheritHandle = false);

#ifdef _WIN32
    HRESULT GetProcessCommandLine(OUT wchar_t* pwText, _Inout_ size_t* pdwTextSize) const;
#endif
    cStringF get_CommandLine() const;

    HRESULT WaitForProcessExit(TIMESYSD_t nTimeWait, APP_EXITCODE_t* pnExitCode = nullptr);

    void AttachCurrentProcess() noexcept {
        //! I represent the current process. No need to close this handle!
#ifdef _WIN32
        // 0xFFFFFFFF = current process. https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocess
        m_nPid = ::GetCurrentProcessId();  // cAppState::get_CurrentProcessId();
        m_hProcess.AttachHandle(::GetCurrentProcess());
        m_ThreadId.InitCurrentId();
#elif defined(__linux__)
        m_nPid = ::getpid();
#endif
    }

    HRESULT TerminateProcess(APP_EXITCODE_t uExitCode) const noexcept {
        //! Hard terminate some process. inject uExitCode. May not save work.
        //! m_nPid may be invalid after this!
        //! in _WIN32 Is would be more polite to call CloseProcess() first. send ::PostThreadMessage(WM_CLOSE) and wait for a bit, to allow programs to save.

        if (!isValidProcess()) return S_FALSE;
#ifdef _WIN32
        if (!::TerminateProcess(get_ProcessHandle(), (UINT)uExitCode))
#elif defined(__linux__)
        if (::kill(get_ProcessId(), SIGTERM) != 0)  // send a signal(SIGTERM) to the process.
#endif
        {
            return HResult::GetLastDef();
        }
        return S_OK;
    }

#ifdef _WIN32
    HRESULT CloseProcess();
#endif

    //! CPU priority level for scheduling.
    DWORD get_PriorityClass() const noexcept {
        //! ABOVE_NORMAL_PRIORITY_CLASS
#if defined(_WIN32) && !defined(UNDER_CE)
        return ::GetPriorityClass(get_ProcessHandle());
#elif defined(__linux__)
        // TODO __linux__ get_PriorityClass
        ASSERT(0);
        return 0;
#endif
    }

    bool put_PriorityClass(DWORD dwPriorityClass) const noexcept {
        //! Set the threads priority.
        //! @arg dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS
#if defined(_WIN32) && !defined(UNDER_CE)
        return ::SetPriorityClass(get_ProcessHandle(), dwPriorityClass) ? true : false;
#elif defined(__linux__)
        ASSERT(0);
        // TODO __linux__
        return false;
#endif
    }

#ifdef _WIN32

    HRESULT CreateRemoteThread(THREAD_FUNC_t pvFunc, const void* pvArgs, OUT cOSHandle& thread);

    void* AllocMemory(size_t nSize) const noexcept {
        //! Allocate space in the process memory.
        return ::VirtualAllocEx(get_ProcessHandle(), NULL, nSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    HRESULT WriteProcessMemory(void* pBaseAddress, const void* pData, size_t nSize) const noexcept {
        //! Write to memory inside some other process address space.
        //! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_WRITE access AND cOSUserToken with SE_DEBUG_NAME

        SIZE_T nSizeWrite = nSize;
        const BOOL bSuccess = ::WriteProcessMemory(get_ProcessHandle(), pBaseAddress, pData, nSize, &nSizeWrite);
        if (!bSuccess) {
            return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        }
        return CastN(HRESULT, nSizeWrite);
    }

    HRESULT ReadProcessMemory(const void* pBaseAddress, void* pDataIn, size_t nSize) const noexcept {
        //! Read memory from inside some other process address space.
        //! Need permissions to do this.
        //! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_READ access AND cOSUserToken with SE_DEBUG_NAME

        SIZE_T nSizeRead = nSize;
        const BOOL bSuccess = ::ReadProcessMemory(get_ProcessHandle(), pBaseAddress, pDataIn, nSize, &nSizeRead);
        if (!bSuccess) {
            return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_READ_FAULT));
        }
        return CastN(HRESULT, nSizeRead);
    }

    bool GetExitCodeProcess(OUT APP_EXITCODE_t& nExitCode) const noexcept {
        //! The exit value specified in the ExitProcess or TerminateProcess function.
        //! @arg pnExitCode = APP_EXITCODE_t::_STILL_ACTIVE = process is running.
        //! @return false = process handle is bad ?
        DWORD dwExitCode = 0;
        if (!::GetExitCodeProcess(get_ProcessHandle(), &dwExitCode)) return false;
        nExitCode = (APP_EXITCODE_t)dwExitCode;
        return true;
    }

    static inline PROCESSID_t FindProcessIdForWindow(HWND hWnd) noexcept {
        //! Find process Id for the hWnd.
        //! @return 0 = can't find it. kPROCESSID_BAD
        PROCESSID_t dwProcessIDRet = kPROCESSID_BAD;
        const THREADID_t dwThreadID = ::GetWindowThreadProcessId(hWnd, &dwProcessIDRet);
        UNREFERENCED_PARAMETER(dwThreadID);
        return dwProcessIDRet;
    }

    // Stats
#ifndef UNDER_CE
    bool GetStatTimes(OUT FILETIME* pCreationTime, OUT FILETIME* pExitTime, OUT FILETIME* pKernelTime, OUT FILETIME* pUserTime) const noexcept {
        //! How much time has this process run ?
        return ::GetProcessTimes(get_ProcessHandle(), pCreationTime, pExitTime, pKernelTime, pUserTime) ? true : false;
    }

    bool GetStatIoCounters(OUT IO_COUNTERS* pIoCounters) const noexcept {
        return ::GetProcessIoCounters(get_ProcessHandle(), pIoCounters) ? true : false;
    }
#endif  // UNDER_CE

#if (_WIN32_WINNT >= 0x0501) && !defined(UNDER_CE)
    bool GetStatHandleCount(OUT DWORD* pdwHandleCount) const noexcept {
        //! How many open handles does this process have ?
        return ::GetProcessHandleCount(get_ProcessHandle(), pdwHandleCount);
    }
#endif
    static HWND GRAYCALL FindWindowForProcessID(PROCESSID_t nProcessID, DWORD dwStyleFlags, const GChar_t* pszClassName = nullptr);  // WS_VISIBLE
#endif                                                                                                                               // _WIN32
};
}  // namespace Gray

#endif  // _INC_cOSProcess_H
