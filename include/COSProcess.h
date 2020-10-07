//
//! @file COSProcess.h
//! @note Launching processes is a common basic feature for __linux__
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_COSProcess_H
#define _INC_COSProcess_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSHandle.h"
#include "CFilePath.h"
#include "CThreadLock.h"

UNITTEST_PREDEF(COSProcess)

namespace Gray
{
	class cFile;

#ifdef _WIN32
	typedef int SHOWWINDOW_t;	//!< show window mode for _WIN32. enum SW_SHOW, SW_HIDE etc.
#elif defined(__linux__)
	enum SHOWWINDOW_t
	{
		//! @enum SHOWWINDOW_t
		//! mode to show or hide a HWND window.
		SW_HIDE = 0,
		SW_SHOWNORMAL = 1,
		SW_SHOWMINIMIZED,
		SW_SHOWMAXIMIZED,	//!< Normal window maximized with top bar showing.
		SW_SHOWDEFAULT = 10,
		//!< SW_FULLSCREEN = 12 = NON Standard. Full screen mode (for games/videos). NOT the same as Maximized. No bar.
	};
#else
#error NOOS
#endif

#ifdef _WIN32
	typedef DWORD PROCESSID_t;		//!< EnumProcesses uses DWORD even in 64 bit code.
	typedef DWORD APP_EXITCODE_t;	//!< main() return value. @note its DWORD in GetExitCodeProcess() but its 'int' from main(), _tmain() and WinMain()
#elif defined(__linux__)
	typedef pid_t PROCESSID_t;		//!< pid_t getpid() for __linux__
	typedef int APP_EXITCODE_t;		//!< main() return value.
#endif
	const PROCESSID_t PROCESSID_BAD = 0;	//!< Invalid process id.

	enum APP_EXITCODE_TYPE
	{
		//! @enum Gray::APP_EXITCODE_TYPE
		//! http://en.wikipedia.org/wiki/Exit_status = 'errorlevel' return from POSIX process.

		APP_EXITCODE_ERRNO = -1,			//!< See Posix 'errno' for why this process never started.
		APP_EXITCODE_OK = EXIT_SUCCESS,		//!< 0=EXIT_SUCCESS (stdlib.h).  App closed.
		APP_EXITCODE_FAIL = EXIT_FAILURE,	//!< 1=EXIT_FAILURE = generic error. App closed.
		APP_EXITCODE_ABORT = 3,				//!< 3=Default error returned if "abort()" used (arbitrary?)  App closed.
		
		// .. some other condition.
#ifdef _WIN32
		APP_EXITCODE_STILL_ACTIVE = STILL_ACTIVE,	//!< _WIN32 Process has not exited yet. STATUS_PENDING
#elif defined(__linux__)
		APP_EXITCODE_STILL_ACTIVE = 0x103,			//!< Process has not exited yet.
#endif
		APP_EXITCODE_UNK = SHRT_MAX,		//!< handle not valid ?
	};

	class GRAYCORE_LINK COSProcess 
	{
		//! @class Gray::COSProcess
		//! A running process in the system. May or may not be the current process. 
		//! handle to some active process I started (me or my child).
		//! Related to MIME_EXT_exe PROCESSID_t. Related to COSModule.

	protected:
		PROCESSID_t m_nPid;		//!< Process ID, 0 = PROCESSID_BAD = un-init.

#ifdef _WIN32
		COSHandle m_hProcess;	//!< open handle to the process. COSModule
	public:
		COSHandle m_hThread;	//!< may have this or not. Only if i launched this myself.
#elif defined(__linux__)
	protected:
		CStringF m_sPath;		//!< cached file path for the EXE/ELF/etc file.
#endif

	public:
		COSProcess();
		virtual ~COSProcess();

#ifdef _WIN32
		HANDLE get_ProcessHandle() const noexcept
		{
			return m_hProcess.get_Handle();
		}
		void CloseProcessHandle();
#endif
		HRESULT CreateProcessX(const FILECHAR_t* pszExeName, const FILECHAR_t* pszArgs = nullptr, SHOWWINDOW_t nShowCmd = SW_SHOWNORMAL, const FILECHAR_t* pszCurrentDir = nullptr, cFile* pFileOutPipe = nullptr);

		static inline bool GRAYCALL IsSystemPID(PROCESSID_t nProcessID) noexcept
		{
			if (nProcessID == 0)	// PROCESSID_BAD
				return true;
#ifdef _WIN32
			if (nProcessID == 4)
				return true;
#endif
			return false;
		}

		bool isValidProcess() const noexcept
		{
			// Is the process in memory/valid/active now ?
#ifdef _WIN32
			return m_hProcess.isValidHandle();
#elif defined(__linux__)
			return m_nPid != 0;
#endif
		}

		PROCESSID_t get_ProcessId() const noexcept
		{
			return m_nPid;
		}
		virtual CStringF get_ProcessPath() const;
		cString get_ProcessName() const;
		HRESULT OpenProcessId(PROCESSID_t dwProcessID, DWORD dwDesiredAccess = 0, bool bInheritHandle = false);

#ifdef _WIN32
		HRESULT GetProcessCommandLine( OUT wchar_t* pwText, _Inout_ size_t* pdwTextSize) const;
#endif
		CStringF get_CommandLine() const;

		HRESULT WaitForProcessExit(TIMESYSD_t nTimeWait, APP_EXITCODE_t* pnExitCode = nullptr);

		void AttachCurrentProcess() noexcept
		{
			//! No need to close this handle! 0xFFFFFFFF = current process.
#ifdef _WIN32
			m_nPid = ::GetCurrentProcessId();
			m_hProcess.AttachHandle(::GetCurrentProcess());
#elif defined(__linux__)
			m_nPid = ::getpid();
#endif
		}

		HRESULT TerminateProcess(APP_EXITCODE_t uExitCode)
		{
			//! terminate some process. inject uExitCode.
			//! m_nPid may be invalid after this!

			if (!isValidProcess())
				return S_FALSE;
#ifdef _WIN32
			if (!::TerminateProcess(m_hProcess.get_Handle(), uExitCode))
#elif defined(__linux__)
			if (::kill(get_ProcessId(), SIGTERM) != 0) // send a signal(SIGTERM) to the process.
#endif
			{
				return HResult::GetLastDef();
			}
			return S_OK;
		}

		//! CPU priority level for scheduling.
		DWORD get_PriorityClass() const noexcept
		{
			//! ABOVE_NORMAL_PRIORITY_CLASS
#if defined(_WIN32) && ! defined(UNDER_CE)
			return ::GetPriorityClass(m_hProcess.get_Handle());
#elif defined(__linux__)
			ASSERT(0);
			// TODO __linux__
			return 0;
#endif
		}
		bool put_PriorityClass(DWORD dwPriorityClass) noexcept
		{
			//! @arg dwPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS
#if defined(_WIN32) && ! defined(UNDER_CE)
			return ::SetPriorityClass(m_hProcess.get_Handle(), dwPriorityClass) ? true : false;
#elif defined(__linux__)
			ASSERT(0);
			// TODO __linux__
			return false;
#endif
		}

#ifdef _WIN32

		HRESULT CreateRemoteThread(const void* pvFunc, const void* pvArgs);

		HRESULT WriteProcessMemory(void* pBaseAddress, const void* pData, size_t nSize)
		{
			//! Write to memory inside some other process address space.
			//! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_WRITE access AND COSUserToken with SE_DEBUG_NAME

			SIZE_T nSizeWrite = nSize;
			BOOL bSuccess = ::WriteProcessMemory(m_hProcess.get_Handle(), pBaseAddress, pData, nSize, &nSizeWrite);
			if (!bSuccess)
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			}
			return (HRESULT)nSizeWrite;
		}

		HRESULT ReadProcessMemory(const void* pBaseAddress, void* pDataIn, size_t nSize) const
		{
			//! Read memory from inside some other process address space.
			//! Need permissions to do this.
			//! ASSUME: PROCESS_QUERY_INFORMATION | PROCESS_VM_READ access AND COSUserToken with SE_DEBUG_NAME

			SIZE_T nSizeRead = nSize;
			BOOL bSuccess = ::ReadProcessMemory(m_hProcess.get_Handle(), pBaseAddress, pDataIn, nSize, &nSizeRead);
			if (!bSuccess)
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_READ_FAULT));
			}
			return (HRESULT)nSizeRead;
		}

		bool GetExitCodeProcess(OUT APP_EXITCODE_t* pnExitCode)
		{
			//! The exit value specified in the ExitProcess or TerminateProcess function.
			//! @arg pnExitCode = APP_EXITCODE_STILL_ACTIVE = process is running.
			//! @return false = process handle is bad ?
			return ::GetExitCodeProcess(m_hProcess.get_Handle(), pnExitCode) ? true : false;
		}

		static PROCESSID_t GRAYCALL FindProcessIdForWindow(HWND hWnd)
		{
			//! Find process Id for the hWnd.
			//! @return 0 = can't find it. PROCESSID_BAD
			PROCESSID_t dwProcessIDTest = PROCESSID_BAD;
			THREADID_t dwThreadID = ::GetWindowThreadProcessId(hWnd, &dwProcessIDTest);
			UNREFERENCED_PARAMETER(dwThreadID);
			return dwProcessIDTest;
		}

		// Stats
#ifndef UNDER_CE
		bool GetStatTimes(OUT FILETIME* pCreationTime, OUT FILETIME* pExitTime, OUT FILETIME* pKernelTime, OUT FILETIME* pUserTime) const
		{
			//! How much time has this process run ?
			return ::GetProcessTimes(m_hProcess.get_Handle(), pCreationTime, pExitTime, pKernelTime, pUserTime) ? true : false;
		}

		bool GetStatIoCounters(OUT IO_COUNTERS* pIoCounters) const
		{
			return ::GetProcessIoCounters(m_hProcess.get_Handle(), pIoCounters) ? true : false;
		}
#endif // UNDER_CE

#if ( _WIN32_WINNT >= 0x0501 ) && ! defined(UNDER_CE)
		bool GetStatHandleCount(OUT DWORD* pdwHandleCount) const
		{
			//! How many open handles does this process have ?
			return ::GetProcessHandleCount(m_hProcess.get_Handle(), pdwHandleCount);
		}
#endif
		static HWND GRAYCALL FindWindowForProcessID(PROCESSID_t nProcessID, DWORD dwStyleFlags, const GChar_t* pszClassName = nullptr); // WS_VISIBLE
#endif // _WIN32

		UNITTEST_FRIEND(COSProcess);
	};
}

#endif // _INC_COSProcess_H

