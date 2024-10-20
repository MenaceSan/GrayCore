//! @file cOSProcess.cpp
//! @note Launching processes is a common basic feature for __linux__
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cFile.h"
#include "cOSModule.h"
#include "cOSProcess.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifdef _WIN32

#pragma comment(lib, "user32.lib")  // GetWindow, etc.

#pragma pack(push, 1)
struct CATTR_PACKED __PEB {
    DWORD dwFiller[4];
    DWORD_PTR dwInfoBlockAddress;
};
struct CATTR_PACKED __INFOBLOCK {
    DWORD dwFiller[16];
    WORD wLength;
    WORD wMaxLength;
    DWORD_PTR dwCmdLineAddress;
};
enum _PROCESSINFOCLASS { ProcessBasicInformation = 0, ProcessWow64Information = 26 };
typedef LONG(WINAPI NTQIP_t)(HANDLE, _PROCESSINFOCLASS, PVOID, ULONG, SIZE_T*);
struct CATTR_PACKED _PROCESS_BASIC_INFORMATION {
    void* Reserved1;
    __PEB* PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    void* Reserved3;
};
#pragma pack(pop)
#endif

namespace Gray {
cOSProcess::cOSProcess() noexcept : _nPid(kPROCESSID_BAD) {
    // _WIN32 = ::GetCurrentProcess() = 0xFFFFFFFF as a shortcut.
}

cOSProcess::~cOSProcess() {
#ifdef _WIN32
    CloseProcessHandle();
#endif
}

#ifdef _WIN32
void cOSProcess::CloseProcessHandle() {
    //! The process may continue running of course.
    if (!isValidProcess()) return;
    _hThread.CloseHandle();
    _hProcess.CloseHandle();
}
#endif

HRESULT cOSProcess::CreateProcessX(const FILECHAR_t* pszExeName, const FILECHAR_t* pszArgs, SHOWWINDOW_t nShowCmd, const FILECHAR_t* pszCurrentDir, cFile* pFileOutPipe) {
    //! Create/launch/spawn the child process file and get handle to it.
    //! @note DOES NOT expand things like %programFiles% . use ExpandEnvironmentStrings()
    //! @arg pszExeName = the app file to start.
    //! @arg pszCurrentDir = cFilePath::GetFileDir( pszExeName )
    //! similar to POSIX execvp()

    if (StrT::IsWhitespace(pszExeName)) return E_INVALIDARG;

#ifdef _WIN32
    _FNF(STARTUPINFO) startInfo;
    cMem::Zero(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
#ifndef UNDER_CE
    startInfo.dwFlags = STARTF_USESHOWWINDOW;  // what fields are used?
#endif
    startInfo.wShowWindow = (WORD)nShowCmd;

    bool bInheritHandles = false;
    if (pFileOutPipe != nullptr) {
        bInheritHandles = true;
#ifndef UNDER_CE
        startInfo.dwFlags |= STARTF_USESTDHANDLES;  // what fields are used?
#endif
        startInfo.hStdOutput = startInfo.hStdError = pFileOutPipe->get_Handle();
    }

    ::PROCESS_INFORMATION procInf;
    cMem::Zero(&procInf, sizeof(procInf));

    // @note The Unicode version of this function, CreateProcessW, can modify the contents of lpCommandLine!?
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
    FILECHAR_t sCommandLine[cFilePath::k_MaxLen];
    sCommandLine[0] = '\0';
    if (!StrT::IsWhitespace(pszArgs)) {
        cSpanX sp(TOSPAN(sCommandLine));
        sp.SetSkip(StrT::CopyPtr(sp, pszExeName));
        sp.SetSkip(StrT::CopyPtr(sp, _FN(" ")));
        StrT::CopyPtr(sp, pszArgs);
        pszArgs = sCommandLine;
    }

    BOOL bRet = _FNF(::CreateProcess)(pszExeName,                                   // lpApplicationName, = pszExeName
                                      pszArgs != nullptr ? sCommandLine : nullptr,  //  lpCommandLine,
                                      nullptr,                                      // LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                      nullptr,                                      // LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                      bInheritHandles,                              // BOOL bInheritHandles,
                                      0,                                            // DWORD dwCreationFlags, CREATE_SUSPENDED
                                      nullptr,                                      // LPVOID lpEnvironment,
                                      const_cast<FILECHAR_t*>(pszCurrentDir),       // lpCurrentDirectory,
                                      &startInfo,                                   // LPSTARTUPINFO lpStartupInfo,
                                      &procInf                                      // LPPROCESS_INFORMATION lpProcessInformation
    );
    if (!bRet) {
        HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
        // 2 = ERROR_FILE_NOT_FOUND
        // E_ACCESSDENIED
        // 740 = ERROR_ELEVATION_REQUIRED = Vista may return this
        return hRes;
    }

    _nPid = procInf.dwProcessId;
    ASSERT(_nPid > 0);
    _hProcess.AttachHandle(procInf.hProcess);
    _ThreadId = procInf.dwThreadId;
    _hThread.AttachHandle(procInf.hThread);  // procInf.dwThreadId

#elif defined(__linux__)

    // similar to system()?
    // similar to _spawnl() _spawnvpe, etc. these are POSIX or M# ?
    // http://linux.die.net/man/3/posix_spawn posix_spawnp
    //

    _nPid = ::fork();  // Split this process into 2 processes.
    if (_nPid == 0) {
        // we ARE the new process now. replace with truly new process from disk.

        const char* args[k_ARG_ARRAY_MAX];  // arbitrary max.
        char szTmp[StrT::k_LEN_Default];
        int iArgs = StrT::ParseArrayTmp<FILECHAR_t>(TOSPAN(szTmp), pszArgs, ToSpan(args + 1, _countof(args) - 2), " ", STRP_DEF);
        args[0] = (char*)pszExeName;
        args[iArgs + 1] = nullptr;  // terminated.

        // p = searches the directories listed in the PATH environment variable
        int iRet = ::execvp(pszExeName, args);
        if (iRet == -1) {
            return HResult::GetPOSIXLastDef();  // Likely this error is lost ?
        }
        // Does NOT return on success!
    }
#endif

    return S_OK;
}

cFilePath cOSProcess::get_ProcessPath() const {  // override
    //! Get the full file path for this process EXE. MUST be loaded by this process for _WIN32.
    //! e.g. "c:\Windows\System32\smss.exe" or "\Device\HarddiskVolume2\Windows\System32\smss.exe"
    //! @note _WIN32 must have the PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights.
    FILECHAR_t szProcessName[cFilePath::k_MaxLen];

#ifdef _WIN32
    // NOTE: GetModuleFileName doesn't work for external processes. GetModuleFileNameEx does but its in psapi.dll
    ::HINSTANCE hInst = (::HINSTANCE)_hProcess.get_Handle();
    if (hInst == (::HINSTANCE)-1) hInst = NULL;  // special case from GetCurrentProcess()
    const DWORD dwRet = _FNF(::GetModuleFileName)(hInst, szProcessName, _countof(szProcessName));
    if (dwRet <= 0) {
        // HRESULT hRes = HResult::GetLast(); // GetLastError is set.
        return "";  // I don't have PROCESS_QUERY_INFORMATION or PROCESS_VM_READ rights.
    }
    return cStringF(ToSpan(szProcessName, dwRet));
#elif defined(__linux__)
    if (_sFilePath.IsEmpty()) {
        cStringF sFileName = cStringF::GetFormatf(_FN("/proc/%d/cmdline"), this->get_ProcessId());
        cFile file;
        HRESULT hRes = file.OpenX(sFileName);
        if (FAILED(hRes)) return _FN("");
        hRes = file.ReadX(TOSPAN(szProcessName));
        // TODO __linux__  chop args?
        const_cast<cOSProcess*>(this)->_sFilePath = cFilePath(szProcessName);
    }
    return _sFilePath;
#else
#error NOOS
#endif
}

cStringF cOSProcess::get_ProcessName() const {
    //! Get a process name from a handle. like GetModuleBaseName()
    //! _WIN32 must have the PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights.
    cFilePath sProcessPath = get_ProcessPath();
    return cFilePath::GetFileName(sProcessPath.get_SpanStr());
}

HRESULT cOSProcess::OpenProcessId(PROCESSID_t nProcessId, DWORD dwDesiredAccess, bool bInheritHandle) {
    //! get a handle to a process by its PROCESSID_t.
    //! @arg dwDesiredAccess = PROCESS_TERMINATE | PROCESS_VM_READ

    if (nProcessId == kPROCESSID_BAD) return E_INVALIDARG;
    if (isValidProcess() && nProcessId == get_ProcessId()) return S_OK;

    _nPid = nProcessId;

#ifdef _WIN32
    _hProcess.AttachHandle(::OpenProcess(dwDesiredAccess, bInheritHandle, nProcessId));
    if (!isValidProcess()) return HResult::GetLastDef(E_HANDLE);  // E_ACCESSDENIED

    // Validate PID.
    PROCESSID_t nProcessId2 = ::GetProcessId(_hProcess);
    if (nProcessId2 != nProcessId) return HResult::GetLastDef(E_HANDLE);  // this should not happen!!

#else
    // Just make sure the PID is valid ?
    _sFilePath = get_ProcessPath();
    if (_sFilePath.IsEmpty()) return E_FAIL;
#endif

    return S_OK;
}

#ifdef _WIN32

HRESULT cOSProcess::CloseProcess() {
    //! Attempt to politely close the process first. More polite than direct call to TerminateThread()
    //! send ::PostThreadMessage(WM_CLOSE) and wait for a bit, to allow programs to save.
    //! https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postthreadmessagea

    if (!_ThreadId.isValidId()) return E_HANDLE;  // Didnt get an id.

    if (!_GTN(::PostThreadMessage)(_ThreadId.GetThreadId(), WM_CLOSE, 0, 0)) {
        return HResult::GetLastDef(E_HANDLE);
    }
    return S_OK;
}

HRESULT cOSProcess::CreateRemoteThread(THREAD_FUNC_t pvFunc, const void* pvArgs, OUT cOSHandle& thread) {
    //! Create a thread (and run it) in the context of some other process
    //! https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread
    //! PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION
    //! NOTE: ASSUME pvArgs is a valid pointer in the apps context. i.e. VirtualAlloc etc.

    // Load our DLL
    DWORD dwThreadId = 0;
    thread.AttachHandle(::CreateRemoteThread(_hProcess, nullptr, 0, pvFunc, (LPVOID)pvArgs, 0, &dwThreadId));
    if (!thread.isValidHandle()) return HResult::GetLast();

    return S_OK;
}

HRESULT cOSProcess::GetProcessCommandLine(cSpanX<wchar_t> ret) const {
    //! Get the command line that invoked this process. "App.exe Args"
    //! http://www.codeproject.com/threads/CmdLine.asp
    //! ReadProcessMemory(); GetCommandLine()
    //! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_READ access AND cOSUserToken with SE_DEBUG_NAME

    SIZE_T dwSize = 0;
    ::_PROCESS_BASIC_INFORMATION pbi;
    pbi.PebBaseAddress = CastNumToPtrT<::__PEB>(0x7ffdf000);  // Default for most process?

    // we'll default to the above address, but newer OSs might have a different
    // base address for the PEB
    cOSModule hLibrary(_FN("ntdll.dll"), cOSModule::k_Load_NoRefCount | cOSModule::k_Load_ByName);
    if (hLibrary.isValidModule()) {
        NTQIP_t* lpfnNtQueryInformationProcess = CastFPtrTo<NTQIP_t*>(hLibrary.GetSymbolAddress("ZwQueryInformationProcess"));
        if (nullptr != lpfnNtQueryInformationProcess) {
            (*lpfnNtQueryInformationProcess)(get_ProcessHandle(), ProcessBasicInformation, &pbi, sizeof(pbi), &dwSize);
        }
    }

    ::__PEB peb;
    HRESULT hRes = ReadProcessMemory(pbi.PebBaseAddress, TOSPANT(peb));
    if (FAILED(hRes)) return hRes;

    ::__INFOBLOCK block;
    hRes = ReadProcessMemory(CastNumToPtr(peb.dwInfoBlockAddress), TOSPANT(block));
    if (FAILED(hRes)) return hRes;

    return ReadProcessMemory(CastNumToPtr(block.dwCmdLineAddress), ret.GetSpanLimit(block.wMaxLength));
}
#endif

cStringF cOSProcess::get_CommandLine() const {
    //! Get the full command line arguments for the process by id.
    //! "App.exe Args"
    //! like _WIN32 "::GetCommandLine()"
#ifdef _WIN32
    wchar_t szCmdLine[cFilePath::k_MaxLen * 2];
    HRESULT hRes = GetProcessCommandLine(TOSPAN(szCmdLine));
    if (FAILED(hRes)) return "";  // Failed to get command line for some reason.

#elif defined(__linux__)
    cStringF sFileName = cStringF::GetFormatf(_FN("/proc/%d/cmdline"), get_ProcessId());
    cFile file;
    HRESULT hRes = file.OpenX(sFileName);
    if (FAILED(hRes)) return "";

    // Read it.
    char szCmdLine[cFilePath::k_MaxLen * 2];
    hRes = file.ReadSpan(TOSPAN(szCmdLine));
#endif
    return szCmdLine;
}

HRESULT cOSProcess::WaitForProcessExit(TIMESYSD_t nTimeWait, APP_EXITCODE_t* pnExitCode) {
    //! Wait for a process to exit.
    //! @arg nTimeWait = how long to wait in seconds. 0 = dont wait. (Does NOT terminate app)
    //! It is generally assumed this is a child process of the current PROCESSID_t (Linux)
    //! @note this does not CAUSE the process to exit.

    if (nTimeWait == 0) return S_FALSE;

#ifdef _WIN32
    // Wait for the app to complete?
    const HRESULT hRes = _hProcess.WaitForSingleObject(nTimeWait);  // wait until its done!
    if (FAILED(hRes)) return hRes;                                   // i waited too long? handles are still open tho?

    // app has exited.
    // Get the exit code from the app
    if (pnExitCode != nullptr) {
        if (!GetExitCodeProcess(*pnExitCode)) {
            *pnExitCode = APP_EXITCODE_t::_UNK;  // no code ? process still running ?
            return E_FAIL;
        }
    }

#elif defined(__linux__)
    // http://linux.die.net/man/2/waitpid
    // or just wait() ?

    int iStatus = 0;
    PROCESSID_t nPidRet = ::waitpid(this->get_ProcessId(), &iStatus, 0);
    if (nPidRet < 0) return E_FAIL;  // Failed to wait!
    if (!WIFEXITED(iStatus)) return E_FAIL;

    if (pnExitCode != nullptr) {
        *pnExitCode = (APP_EXITCODE_t)WEXITSTATUS(iStatus);
    }
#endif
    return S_OK;
}

#ifdef _WIN32
HWND cOSProcess::FindWindowForProcessID(PROCESSID_t nProcessId, DWORD dwStyleFlags, const GChar_t* pszClassName) {  // static
    //! look through all the top level windows for the window that has this PROCESSID_t.
    //! @note there may be more than 1. just take the first/best one.
    //! @arg
    //!  nProcessId = kPROCESSID_BAD = 0 = don't care what process.
    //!  dwStyleFlags = WS_VISIBLE = only accept visible windows.
    //!  pszClassName = must be this window class name.

    if (nProcessId == kPROCESSID_BAD) {  // don't care about pid, just use pszClassName
        return _GTN(::FindWindow)(pszClassName, nullptr);
    }

    // Get the first top window handle. HWND_DESKTOP
    HWND hWnd = _GTN(::FindWindow)(nullptr, nullptr);  // ::GetDesktopWindow() ?
    ASSERT(hWnd != WINHANDLE_NULL);                    // must be a desktop!

    HWND hWndBest = WINHANDLE_NULL;
    int iBestScore = -3;

    // Loop until we find the target or we run out of windows.
    for (; hWnd != WINHANDLE_NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT)) {
        // does it have the PID?
        PROCESSID_t nProcessIdTest = kPROCESSID_BAD;
        const THREADID_t dwThreadID = ::GetWindowThreadProcessId(hWnd, &nProcessIdTest);
        UNREFERENCED_PARAMETER(dwThreadID);
        if (nProcessIdTest != nProcessId) continue;

        // WS_VISIBLE = only accept visible windows.
        if (dwStyleFlags) {
            DWORD dwStyle = (DWORD)_GTN(::GetWindowLong)(hWnd, GWL_STYLE);
            if (!(dwStyle & dwStyleFlags)) continue;
        }
        if (pszClassName != nullptr) {  // must be class name.
            GChar_t szClassNameTmp[cFilePath::k_MaxLen];
            const int iLen = _GTN(::GetClassName)(hWnd, szClassNameTmp, _countof(szClassNameTmp));
            if (iLen <= 0) continue;
            if (StrT::CmpI(szClassNameTmp, pszClassName)) continue;  // MatchRegEx?
                
        }

        // See if this window has a parent. If not,
        // it is a top-level window.
        int iScore = -1;
        const ::HWND hWndParent = ::GetParent(hWnd);
        if (hWndParent == HWND_DESKTOP) {
            iScore = -2;
        } else {
            ::RECT rect;
            if (::GetWindowRect(hWnd, &rect)) {
                iScore = (rect.right - rect.left) * (rect.bottom - rect.top);  // Biggest.
            }
        }
        if (iScore > iBestScore) {
            iBestScore = iScore;
            hWndBest = hWnd;
        }
    }
    return hWndBest;
}
#endif

#if 0  // _WIN32
PROCESSID_t cOSProcess::GetProcessIDFromHandle() {
	//! Get the PROCESSID_t from a process handle.
	//! The handle must have the PROCESS_QUERY_INFORMATION access right
	//! 0 = invalid kPROCESSID_BAD
	if (_nPid == kPROCESSID_BAD && _hProcess.isValidHandle()) {
		_nPid = ::GetProcessId(_hProcess.get_Handle());	// XP SP1 Function.
	}
}
#endif
}  // namespace Gray
