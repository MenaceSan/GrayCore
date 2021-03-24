//
//! @file cOSProcess.cpp
//! @note Launching processes is a common basic feature for __linux__
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cOSProcess.h"
#include "cOSModule.h"
#include "cFile.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifdef _WIN32

#pragma pack(push,1)
struct CATTR_PACKED __PEB
{
	DWORD   dwFiller[4];
	DWORD_PTR   dwInfoBlockAddress;
};
struct CATTR_PACKED __INFOBLOCK
{
	DWORD   dwFiller[16];
	WORD    wLength;
	WORD    wMaxLength;
	DWORD_PTR   dwCmdLineAddress;
};
enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
	ProcessWow64Information = 26
};
typedef LONG(WINAPI NTQIP_t)(HANDLE, _PROCESSINFOCLASS, PVOID, ULONG, SIZE_T*);
struct CATTR_PACKED _PROCESS_BASIC_INFORMATION
{
	void* Reserved1;
	__PEB* PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	void* Reserved3;
};
#pragma pack(pop)
#endif

namespace Gray
{
	cOSProcess::cOSProcess() noexcept
		: m_nPid(PROCESSID_BAD)
	{
		// _WIN32 = ::GetCurrentProcess() = 0xFFFFFFFF as a shortcut.
	}

	cOSProcess::~cOSProcess()
	{
#ifdef _WIN32
		CloseProcessHandle();
#endif
	}

#ifdef _WIN32
	void cOSProcess::CloseProcessHandle()
	{
		//! The process may continue running of course.
		if (!isValidProcess())
			return;
		m_hThread.CloseHandle();
		m_hProcess.CloseHandle();
	}
#endif

	HRESULT cOSProcess::CreateProcessX(const FILECHAR_t* pszExeName, const FILECHAR_t* pszArgs, SHOWWINDOW_t nShowCmd, const FILECHAR_t* pszCurrentDir, cFile* pFileOutPipe)
	{
		//! Create/launch/spawn the child process file and get handle to it.
		//! @note DOES NOT expand things like %programFiles% . use ExpandEnvironmentStrings()
		//! @arg pszExeName = the app file to start. 
		//! @arg pszCurrentDir = cFilePath::GetFileDir( pszExeName )
		//! similar to POSIX execvp()
 
		if (StrT::IsWhitespace(pszExeName))
			return E_INVALIDARG;

#ifdef _WIN32
		_FNF(STARTUPINFO) startInfo;
		cMem::Zero(&startInfo, sizeof(startInfo));
		startInfo.cb = sizeof(startInfo);
#ifndef UNDER_CE
		startInfo.dwFlags = STARTF_USESHOWWINDOW;	// what fields are used?
#endif
		startInfo.wShowWindow = (WORD)nShowCmd;

		bool bInheritHandles = false;
		if (pFileOutPipe != nullptr)
		{
			bInheritHandles = true;
#ifndef UNDER_CE
			startInfo.dwFlags |= STARTF_USESTDHANDLES;	// what fields are used?
#endif
			startInfo.hStdOutput = startInfo.hStdError = pFileOutPipe->m_hFile;
		}

		PROCESS_INFORMATION procInf;
		cMem::Zero(&procInf, sizeof(procInf));

		// @note The Unicode version of this function, CreateProcessW, can modify the contents of lpCommandLine!?
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
		FILECHAR_t sCommandLine[_MAX_PATH];
		sCommandLine[0] = '\0';
		if (!StrT::IsWhitespace(pszArgs))
		{
			StrLen_t len = StrT::CopyLen(sCommandLine, pszExeName, STRMAX(sCommandLine));
			len += StrT::CopyLen<FILECHAR_t>(sCommandLine + len, _FN(" "), STRMAX(sCommandLine) - len);
			StrT::CopyLen(sCommandLine + len, pszArgs, STRMAX(sCommandLine) - len);
			pszArgs = sCommandLine;
		}

		BOOL bRet = _FNF(::CreateProcess)(
			pszExeName,		// lpApplicationName, = pszExeName
			pszArgs != nullptr ? sCommandLine : nullptr,	//  lpCommandLine,
			nullptr,		// LPSECURITY_ATTRIBUTES lpProcessAttributes,
			nullptr,		// LPSECURITY_ATTRIBUTES lpThreadAttributes,
			bInheritHandles,		// BOOL bInheritHandles,
			0,			// DWORD dwCreationFlags, CREATE_SUSPENDED
			nullptr,		// LPVOID lpEnvironment,
			const_cast<FILECHAR_t*>(pszCurrentDir),		// lpCurrentDirectory,
			&startInfo,		// LPSTARTUPINFO lpStartupInfo,
			&procInf		// LPPROCESS_INFORMATION lpProcessInformation
			);
		if (!bRet)
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
			// 2 = ERROR_FILE_NOT_FOUND
			// E_ACCESSDENIED
			// 740 = ERROR_ELEVATION_REQUIRED = Vista may return this
			return hRes;
		}

		m_nPid = procInf.dwProcessId;
		ASSERT(m_nPid > 0);
		m_hProcess.AttachHandle(procInf.hProcess);
		m_hThread.AttachHandle(procInf.hThread); // procInf.dwThreadId

#elif defined(__linux__)

		// similar to system()?
		// similar to _spawnl() _spawnvpe, etc. these are POSIX or M# ? 
		// http://linux.die.net/man/3/posix_spawn posix_spawnp
		//

		m_nPid = ::fork();	// Split this process into 2 processes.
		if (m_nPid == 0)
		{
			// we ARE the new process now. replace with truly new process from disk.

			char* args[k_ARG_ARRAY_MAX];	// arbitrary max.
			char szTmp[StrT::k_LEN_MAX];
			int iArgs = StrT::ParseCmdsTmp<FILECHAR_t>(szTmp, STRMAX(szTmp), pszArgs, args + 1, _countof(args) - 2, " ", STRP_DEF);
			args[0] = (char*)pszExeName;
			args[iArgs + 1] = nullptr;	// terminated.

			// p = searches the directories listed in the PATH environment variable 
			int iRet = ::execvp(pszExeName, args);
			if (iRet == -1)
			{
				return HResult::GetPOSIXLastDef();		// Likely this error is lost ?
			}
			// Does NOT return on success!
		}
#endif

		return S_OK;
	}

	cStringF cOSProcess::get_ProcessPath() const // virtual
	{
		//! Get the full file path for this process EXE. MUST be loaded by this process for _WIN32.
		//! e.g. "c:\Windows\System32\smss.exe" or "\Device\HarddiskVolume2\Windows\System32\smss.exe"
		//! @note _WIN32 must have the PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights.
		FILECHAR_t szProcessName[_MAX_PATH];

#ifdef _WIN32
		// NOTE: GetModuleFileName doesn't work for external processes. GetModuleFileNameEx does but its in psapi.dll		
		HINSTANCE hInst = (HINSTANCE)m_hProcess.get_Handle();
		if (hInst == (HINSTANCE)-1)	// special case from GetCurrentProcess()
			hInst = NULL;
		DWORD dwRet = _FNF(::GetModuleFileName)(hInst, szProcessName, _countof(szProcessName));
		if (dwRet <= 0)
		{
			// HRESULT hRes = HResult::GetLast(); // GetLastError is set.
			return "";		// I don't have PROCESS_QUERY_INFORMATION or PROCESS_VM_READ rights.
		}
		return cStringF(szProcessName, dwRet);
#elif  defined(__linux__)
		if (m_sPath.IsEmpty())
		{
			cStringF sFileName = cStringF::GetFormatf(_FN("/proc/%d/cmdline"), this->get_ProcessId());
			cFile file;
			HRESULT hRes = file.OpenX(sFileName);
			if (FAILED(hRes))
			{
				return _FN("");
			}
			hRes = file.ReadX(szProcessName, sizeof(szProcessName));
			// TODO __linux__  chop args?			 
			m_sPath = cStringF(szProcessName);
		}
		return m_sPath;
#else
#error NOOS
#endif
	}

	cStringF cOSProcess::get_ProcessName() const
	{
		//! Get a process name from a handle. like GetModuleBaseName()
		//! _WIN32 must have the PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights.
		cStringF sProcessPath = get_ProcessPath();
		return cFilePath::GetFileName(sProcessPath);
	}

	HRESULT cOSProcess::OpenProcessId(PROCESSID_t nProcessId, DWORD dwDesiredAccess, bool bInheritHandle)
	{
		//! get a handle to a process by its PROCESSID_t.
		//! @arg dwDesiredAccess = PROCESS_TERMINATE | PROCESS_VM_READ

		if (nProcessId == PROCESSID_BAD)
		{
			return E_INVALIDARG;
		}
		if (isValidProcess() && nProcessId == get_ProcessId())
		{
			return S_OK;
		}

		m_nPid = nProcessId;

#ifdef _WIN32
		m_hProcess.AttachHandle(::OpenProcess(dwDesiredAccess, bInheritHandle, nProcessId));
		if (!isValidProcess())
		{
			// E_ACCESSDENIED
			return HResult::GetLastDef(E_HANDLE);
		}
		// Validate PID.
		PROCESSID_t nProcessId2 = ::GetProcessId(m_hProcess);
		if (nProcessId2 != nProcessId)
		{
			// this should not happen!!
			return HResult::GetLastDef(E_HANDLE);
		}
#else
		// Just make sure the PID is valid ?
		m_sPath = get_ProcessPath();
		if (m_sPath.IsEmpty())
		{
			return E_FAIL;
		}
#endif

		return S_OK;
	}

#ifdef _WIN32

	HRESULT cOSProcess::CreateRemoteThread(THREAD_FUNC_t pvFunc, const void* pvArgs, OUT cOSHandle& thread)
	{
		// Create a thread (and run it) in the context of some other process
		// https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread
		// PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION
		// NOTE: ASSUME pvArgs is a valid pointer in the apps context. i.e. VirtualAlloc etc.

		// Load our DLL 
		DWORD dwThreadId = 0;
		thread.AttachHandle(::CreateRemoteThread(m_hProcess, nullptr, 0, pvFunc, (LPVOID)pvArgs, 0, &dwThreadId));
		if (!thread.isValidHandle())
			return HResult::GetLast();

		return S_OK;
	}

	HRESULT cOSProcess::GetProcessCommandLine(OUT wchar_t* pwText, _Inout_ size_t* pdwTextSize) const
	{
		//! Get the command line that invoked this process. "App.exe Args"
		//! http://www.codeproject.com/threads/CmdLine.asp
		//! ReadProcessMemory(); GetCommandLine()
		//! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_READ access AND cOSUserToken with SE_DEBUG_NAME

		SIZE_T dwSize = 0;
		_PROCESS_BASIC_INFORMATION pbi;
		pbi.PebBaseAddress = (__PEB*)0x7ffdf000;	// Default for most process?

		// we'll default to the above address, but newer OSs might have a different
		// base address for the PEB
		cOSModule hLibrary(_FN("ntdll.dll"), cOSModule::k_Load_NoRefCount | cOSModule::k_Load_ByName);
		if (hLibrary.isValidModule())
		{
			NTQIP_t* lpfnNtQueryInformationProcess = (NTQIP_t*)hLibrary.GetSymbolAddress("ZwQueryInformationProcess");
			if (nullptr != lpfnNtQueryInformationProcess)
			{
				(*lpfnNtQueryInformationProcess)(get_ProcessHandle(), ProcessBasicInformation, &pbi, sizeof(pbi), &dwSize);
			}
		}

		__PEB PEB;
		HRESULT hRes = ReadProcessMemory(pbi.PebBaseAddress, &PEB, sizeof(PEB));
		if (FAILED(hRes))
			return hRes;

		__INFOBLOCK Block;
		hRes = ReadProcessMemory((LPVOID)PEB.dwInfoBlockAddress, &Block, sizeof(Block));
		if (FAILED(hRes))
			return hRes;

		hRes = ReadProcessMemory((LPVOID)Block.dwCmdLineAddress, pwText, MIN(Block.wMaxLength, *pdwTextSize));
		if (FAILED(hRes))
			return hRes;
		*pdwTextSize = hRes;
		return S_OK;
	}
#endif

	cStringF cOSProcess::get_CommandLine() const
	{
		//! Get the full command line arguments for the process by id.
		//! "App.exe Args"
		//! like _WIN32 "::GetCommandLine()"
#ifdef _WIN32
		wchar_t szCmdLine[_MAX_PATH * 2];
		size_t dwSize = sizeof(szCmdLine);
		HRESULT hRes = GetProcessCommandLine(szCmdLine, &dwSize);
		if (FAILED(hRes))
		{
			// Failed to get command line for some reason.
			return "";
		}
#elif defined(__linux__)
		cStringF sFileName = cStringF::GetFormatf(_FN("/proc/%d/cmdline"), get_ProcessId());
		cFile file;
		HRESULT hRes = file.OpenX(sFileName);
		if (FAILED(hRes))
		{
			return "";
		}
		// Read it.
		char szCmdLine[_MAX_PATH * 2];
		szCmdLine[0] = '\0';
		hRes = file.ReadX(szCmdLine, STRMAX(szCmdLine));
#endif
		szCmdLine[STRMAX(szCmdLine)] = '\0';	// Extra safe termination.
		return szCmdLine;
		}

	HRESULT cOSProcess::WaitForProcessExit(TIMESYSD_t nTimeWait, APP_EXITCODE_t* pnExitCode)
	{
		//! Wait for a process to exit.
		//! @arg nTimeWait = how long to wait in seconds. 0 = dont wait. (Does NOT terminate app)
		//! It is generally assumed this is a child process of the current PROCESSID_t (Linux)
		//! @note this does not CAUSE the process to exit.

		if (nTimeWait == 0)
		{
			return S_FALSE;
		}

#ifdef _WIN32
		// Wait for the app to complete?
		HRESULT hRes = m_hProcess.WaitForSingleObject(nTimeWait);	// wait until its done!
		if (FAILED(hRes))
		{
			// i waited too long? handles are still open tho?
			return hRes;
		}

		// app has exited.
		// Get the exit code from the app
		if (pnExitCode != nullptr)
		{
			*pnExitCode = SHRT_MAX;
			if (!GetExitCodeProcess(pnExitCode))
			{
				*pnExitCode = APP_EXITCODE_UNK; // no code ? process still running ?
				return E_FAIL;
			}
		}

#elif defined(__linux__)
		// http://linux.die.net/man/2/waitpid
		// or just wait() ?

		int iStatus = 0;
		PROCESSID_t nPidRet = ::waitpid(this->get_ProcessId(), &iStatus, 0);
		if (nPidRet < 0)
		{
			// Failed to wait!
			return E_FAIL;
		}
		if (!WIFEXITED(iStatus))
		{
			return E_FAIL;
		}
		if (pnExitCode != nullptr)
		{
			*pnExitCode = WEXITSTATUS(iStatus);
	}
#endif
		return S_OK;
	}

#ifdef _WIN32
	HWND cOSProcess::FindWindowForProcessID(PROCESSID_t nProcessId, DWORD dwStyleFlags, const GChar_t* pszClassName) // static
	{
		//! look through all the top level windows for the window that has this PROCESSID_t.
		//! @note there may be more than 1. just take the first/best one.
		//! @arg
		//!  nProcessId = PROCESSID_BAD = 0 = don't care what process.
		//!  dwStyleFlags = WS_VISIBLE = only accept visible windows.
		//!  pszClassName = must be this window class name.

		if (nProcessId == PROCESSID_BAD) // don't care about pid, just use pszClassName
		{
			return _GTN(::FindWindow)(pszClassName, nullptr);
		}

		// Get the first top window handle. HWND_DESKTOP
		HWND hWnd = _GTN(::FindWindow)(nullptr, nullptr); // ::GetDesktopWindow() ?
		ASSERT(hWnd != WINHANDLE_NULL);	// must be a desktop!

		HWND hWndBest = WINHANDLE_NULL;
		int iBestScore = -3;

		// Loop until we find the target or we run out of windows.
		for (; hWnd != WINHANDLE_NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
		{
			// does it have the PID?
			PROCESSID_t nProcessIdTest = PROCESSID_BAD;
			THREADID_t dwThreadID = ::GetWindowThreadProcessId(hWnd, &nProcessIdTest);
			UNREFERENCED_PARAMETER(dwThreadID);
			if (nProcessIdTest != nProcessId)
				continue;

			// WS_VISIBLE = only accept visible windows.
			if (dwStyleFlags)
			{
				DWORD dwStyle = (DWORD)_GTN(::GetWindowLong)(hWnd, GWL_STYLE);
				if (!(dwStyle & dwStyleFlags))
					continue;
			}
			if (pszClassName != nullptr)	// must be class name.
			{
				GChar_t szClassNameTmp[_MAX_PATH];
				const int iLen = _GTN(::GetClassName)(hWnd, szClassNameTmp, _countof(szClassNameTmp));
				if (iLen <= 0)
					continue;
				if (StrT::CmpI(szClassNameTmp, pszClassName)) // MatchRegEx?
					continue;
			}

			// See if this window has a parent. If not,
			// it is a top-level window.
			int iScore = -1;
			const HWND hWndParent = ::GetParent(hWnd);
			if (hWndParent == HWND_DESKTOP)
			{
				iScore = -2;
			}
			else
			{
				RECT rect;
				if (::GetWindowRect(hWnd, &rect))
				{
					iScore = (rect.right - rect.left) * (rect.bottom - rect.top);	// Biggest.
				}
			}
			if (iScore > iBestScore)
			{
				iBestScore = iScore;
				hWndBest = hWnd;
			}
		}
		return hWndBest;
	}
#endif

#if 0  // _WIN32
	PROCESSID_t cOSProcess::GetProcessIDFromHandle()
	{
		//! Get the PROCESSID_t from a process handle.
		//! The handle must have the PROCESS_QUERY_INFORMATION access right
		//! 0 = invalid PROCESSID_BAD
		if (m_nPid == PROCESSID_BAD && m_hProcess.isValidHandle())
		{
			m_nPid = ::GetProcessId(m_hProcess.get_Handle());	// XP SP1 Function.
		}
}
#endif

}
