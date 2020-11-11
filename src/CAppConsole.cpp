//
//! @file cAppConsole.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cAppConsole.h"
#include "cLogMgr.h"
#include "cOSHandleSet.h"
#include "cAppState.h"
#include "cFileText.h"
#include "HResult.h"

#if ! defined(UNDER_CE) 

#if defined(_WIN32) && USE_CRT
#include <io.h>
#include <conio.h>
// #include <ios>	// std::ios::sync_with_stdio
#elif defined(__linux__)
#include <termios.h>
#include <sys/ioctl.h>
#endif
#include <fcntl.h>		// _O_RDONLY

namespace Gray
{
	cAppConsole::cAppConsole()
		: cSingleton<cAppConsole>(this, typeid(cAppConsole))
		, m_bKeyEchoMode(true)
		, m_bKeyEnterMode(true)
		, m_eConsoleType(AppCon_UNKNOWN)
		, m_bConsoleParent(false)
		, m_iAllocConsoleCount(0)
	{
#ifdef _WIN32
		for (int i = 0; i < AppStd_QTY; i++)
		{
			m_hStd[i] = INVALID_HANDLE_VALUE;
		}
#endif
	}

	cAppConsole::~cAppConsole()
	{
		// Is m_iAllocConsoleCount = 0 ?
	}

	void cAppConsole::CheckConsoleMode() noexcept
	{
		//! Is the process already running from a console window? _CONSOLE
		//! Was process started by a console ?
		//! e.g. Linux applications started from GNOME desktop have no console window.
		//! @note printf() might not work until i call RedirectIOToConsole()
		//! @note "GetConsoleWindow();" returns null if Windows 10 and i was a windows app started in console.

		if (m_eConsoleType != AppCon_UNKNOWN)
			return;

		cStringF sPrompt = cAppState::GetEnvironStr(_FN("PROMPT"));	// Good for Linux and Windows.
		m_bConsoleParent = !sPrompt.IsEmpty();

#if defined(_WIN32)
		HWND hWnd = ::GetConsoleWindow();
		if (hWnd != WINHANDLE_NULL)
#else // __linux__
		if (stdout != nullptr) // TODO: detect if Linux app is in a console!?
#endif
		{
			m_eConsoleType = AppCon_Proc;	// My parent is build using _CONSOLE
			AttachConsoleSync();
		}
		else
		{
			m_eConsoleType = AppCon_NONE;	// i have no console. Assume I'm a GUI app or headless service.
		}
	}

	bool cAppConsole::AttachConsoleSync()
	{
		//! Synchronize the C std* buffers (needs USE_CRT) with _WIN32 Console.
		//! NOTE: Not sure why this isn't just handled by AllocConsole()
		//! Similar to std::ios::sync_with_stdio()

		ASSERT(isConsoleMode());

#if defined(_WIN32) && USE_CRT   
		for (int i = 0; i < AppStd_QTY; i++)
		{
			// redirect un-buffered STDOUT to the console
			DWORD nStdHandle;
			FILE* pFileDest;
			OF_FLAGS_t nFileFlags = OF_WRITE | OF_TEXT;
			switch (i)
			{
			case AppStd_stdin:
				nStdHandle = STD_INPUT_HANDLE;	// CONIN$
				pFileDest = stdin;
				nFileFlags = OF_READ | OF_TEXT;
				break;
			case AppStd_stdout:
				nStdHandle = STD_OUTPUT_HANDLE;	// CONOUT$
				pFileDest = stdout;
				break;
			case AppStd_stderr:
				nStdHandle = STD_ERROR_HANDLE;
				pFileDest = stderr;
				break;
			default:
				ASSERT(0);
				return false;
			}

			m_hStd[i] = ::GetStdHandle(nStdHandle);

			if (m_eConsoleType != AppCon_Proc)
			{
				// Now attach it to the appropriate std FILE*, 
				cFileText fileStd;
				HRESULT hRes = fileStd.OpenFileHandle(m_hStd[i], nFileFlags);
				if (FAILED(hRes))
				{
					return false;
				}
				*pFileDest = *fileStd.DetachFileStream();	// copy FILE struct contents! NOT Just pointer.
			}
		}

		if (m_eConsoleType != AppCon_Proc)
		{
			// set the screen buffer to be big enough to let us scroll text
			CONSOLE_SCREEN_BUFFER_INFO coninfo;
			if (!::GetConsoleScreenBufferInfo(m_hStd[AppStd_stdout], &coninfo))
			{
				return false;
			}

			coninfo.dwSize.Y = k_MAX_CONSOLE_LINES;
			if (!::SetConsoleScreenBufferSize(m_hStd[AppStd_stdout], coninfo.dwSize))
			{
				return false;
			}
#if 0
			// Synchronize STL iostreams. if anyone cares.
			std::ios::sync_with_stdio();
#endif
		}
#endif

		return true;
	}

	bool cAppConsole::AttachOrAllocConsole(bool bAttachElseAlloc)
	{
		//! 1. Do i already have a console. use it. if _CONSOLE app.
		//! 2. Attach to my parents console if there is one.
		//! 3. allocate a new console for this app.
		//! http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-application/494000#494000
		//! https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/

		if (isConsoleMode())	// I'm already in a console.
		{
			m_iAllocConsoleCount++;	// Must have same number of closes with ReleaseConsole().
			return true;
		}

		ASSERT(m_iAllocConsoleCount == 0);
		ASSERT(m_eConsoleType == AppCon_UNKNOWN || m_eConsoleType == AppCon_NONE);

#if defined(_WIN32) 

		// We must specifically attach to the console.
		// A process can be associated with only one console,
		// so the AllocConsole function fails if the calling process already has a console

		// HasConsoleParent()
		if (::AttachConsole(ATTACH_PARENT_PROCESS)) // try to use my parents console.
		{
			m_eConsoleType = AppCon_Attach;
		}
		else
		{
			if (!bAttachElseAlloc)
				return false;
			if (!::AllocConsole())	// Make my own private console.
			{
				// Failed to get or create a console. i probably already have one?
				return false;
			}
			m_eConsoleType = AppCon_Create;
		}

#ifdef _DEBUG
		DWORD adwProcessList[32];
		const DWORD dwRet = ::GetConsoleProcessList(adwProcessList, _countof(adwProcessList));
		ASSERT(dwRet >= 1);
		const HWND hWnd = ::GetConsoleWindow();
		ASSERT(hWnd != WINHANDLE_NULL);
#endif		

		m_iAllocConsoleCount = 1;

#if USE_CRT
		if (!AttachConsoleSync())
		{
			m_iAllocConsoleCount = 0;
			m_eConsoleType = AppCon_NONE;
			return false;
		}
#endif

		return true;
#else
		// AppCon_Proc
		ASSERT(0);
		return false;		// this should NEVER be called.
#endif
	}

	void cAppConsole::ReleaseConsole()
	{
		// Release my console. free it if i created it.
		m_iAllocConsoleCount--;
#if defined(_WIN32)
		if (m_iAllocConsoleCount <= 0 && m_eConsoleType > AppCon_Proc)
		{
			// I called AllocConsole
			::FreeConsole();
			m_eConsoleType = AppCon_NONE;
		}
#endif
	}

	HRESULT cAppConsole::WriteStrErr(const char* pszText)
	{
		//! Does not support UNICODE ?
		//! @return EOF = (-1) error
		//! >=0 = success
		if (!isConsoleMode())
		{
			return true;
		}
		cThreadGuard guard(m_Lock);

#if defined(_WIN32) 
		// @note we must do this to get the dual windows/console stuff to work.
		DWORD dwLengthWritten;
		DWORD dwDataSize = StrT::Len(pszText);
		bool bRet = ::WriteFile(m_hStd[AppStd_stderr], pszText, dwDataSize, &dwLengthWritten, nullptr); // AppStd_stdout
		if (!bRet)
		{
			// GetLastError code ERROR_IO_PENDING is not a failure. Async complete.
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			return hRes;
		}
#else // POSIX
		FILE* pFile = stderr;
		int iRet = ::fputs(pszText, pFile);
		if (iRet == EOF)
		{
			// failed.
			HRESULT hRes = HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			return hRes;
		}
#endif
		return S_OK;	// we are good.
	}

	HRESULT cAppConsole::WriteStrOut(const char* pszText)
	{
		//! Write to console. Does not support UNICODE ?
		//! @return HRESULT_WIN32_C(ERROR_HANDLE_DISK_FULL)
		//! >=0 = success
		//! @note _WIN32 could probably use the m_h directly.

		if (!isConsoleMode())
		{
			return S_OK;
		}
		cThreadGuard guard(m_Lock);

#if defined(_WIN32) 
		// @note we must do this to get the dual windows/console stuff to work.
		DWORD dwLengthWritten;
		DWORD dwDataSize = StrT::Len(pszText);
		bool bRet = ::WriteFile(m_hStd[AppStd_stdout], pszText, dwDataSize, &dwLengthWritten, nullptr); // AppStd_stdout
		if (!bRet)
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			return hRes;
		}
#else // POSIX
		FILE* pFile = stdout;
		int iRet = ::fputs(pszText, pFile);
		if (iRet == EOF)
		{
			// failed. EBADF=9
			HRESULT hRes = HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			return hRes;
		}
#endif
		return S_OK;	// we are good.
	}

	HRESULT cAppConsole::SetKeyModes(bool bEchoMode, bool bEnterMode)
	{
		//! @arg bEchoMode = default true = auto echo for input keys input on/off
		//! @arg bEnterMode = default true = Wait for a enter to be pressed first ? false = get all keys as they come in.
		//! @note The effects are persistent for the console in Linux. remember to undo your changes before exit.

#ifdef _WIN32
		if (bEnterMode && !bEchoMode)
		{
			// NON raw mode must echo ??
			// return E_INVALIDARG;
		}
		::SetConsoleCtrlHandler(nullptr, true);	// Ignore CONTROL-C, CTRL+C
#endif

#ifdef __linux__
		char buf[256];
		cMem::Zero(buf, sizeof(buf));
		int iRet = ::readlink("/proc/self/fd/0", buf, sizeof(buf)); // Is this needed or just use stdin=0 ?
		if (iRet == -1)
		{
			return HResult::GetLastDef();
		}

		cOSHandle fdin;
		fdin.OpenHandle(buf, O_RDWR);	// NOTE: fdin should be 0 instead? stdin = iobuf[0]
		if (!fdin.isValidHandle())
		{
			return HResult::GetLastDef(E_HANDLE);
		}

		// get the current state. fdin = 0?
		struct termios stty;
		iRet = fdin.IOCtl(TCGETS, &stty);
		if (iRet < 0)
		{
			return HResult::GetLastDef();
		}

		if (bEchoMode)	// CLOCAL ??
		{
			stty.c_lflag |= (ECHO);  // echo
		}
		else
		{
			stty.c_lflag &= ~(ECHO);  // suppress echo
		}
		if (bEnterMode)
		{
			stty.c_lflag |= (ICANON);  // must wait for enter to get keys.
		}
		else
		{
			stty.c_lflag &= ~(ICANON);  // one char @ a time
		}

		iRet = fdin.IOCtl(TCSETS, &stty);
		if (iRet < 0)
		{
			return HResult::GetLastDef();
		}
#endif
		m_bKeyEchoMode = bEchoMode;
		m_bKeyEnterMode = bEnterMode;
		return S_OK;
	}

	int cAppConsole::get_KeyReadQty() const
	{
		//! Are there keys to be read ?
		//! see http://www.linuxquestions.org/questions/programming-9/pausing-the-screen-44573/
		//! or http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
		//! or http://www.control.auc.dk/~jnn/c2000/programs/mm5/keyboardhit/msg02541.html

#ifdef _WIN32
#if USE_CRT
		return ::_kbhit() ? 1 : 0;
#else
		return false;
#endif
#else
		// NOTE: can i use FIONREAD ?
		cOSHandleSet hs(0);
		if (hs.WaitForObjects(0) == S_OK)
			return 1;
		return 0;
#endif
	}

	int cAppConsole::ReadKeyWait()
	{
		//! Read a single key from conio stdin. block/wait for char.
		//! Arrows and escape key are sometimes special purpose here.
		//! @return -1 = failed. no char is available. else ASCII key. (like enum VK_TYPE)
		if (!isConsoleMode())
		{
			return -1;
		}

#ifdef _WIN32
		// NOTE: _WIN32 fgetc(stdin) will block until the ENTER key is pressed ! then feed chars until it runs out.
#if USE_CRT
		if (!m_bKeyEnterMode)
		{
			if (m_bKeyEchoMode)
			{
				return ::_getche();
			}
			return ::_getch();	// don't wait for ENTER return as we get them.
		}
		return ::getchar();	// buffer chars and returns when ENTER is pressed for a whole line.
#else
		return -1;
#endif
#elif defined(__linux__)
		// @note NON raw mode always echoes.
		return ::getchar();	// buffer chars and returns when ENTER is pressed for a whole line.
#endif
	}

	int cAppConsole::ReadKey()
	{
		//! Get ASCII_TYPE Key char produced by possibly multiple keys pressed (shift). Don't wait. 
		//! similar to INPUTKEY_TYPE and VK_TYPE -> VK_ESCAPE = INPUTKEY_ESCAPE
		//! @return -1 = no char is available. else ASCII_TYPE character produced by key or keys pressed.
		if (get_KeyReadQty() <= 0)
			return -1;
		return ReadKeyWait();
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "cUnitTest.h"
#include "StrCharAscii.h"

UNITTEST_CLASS(cAppConsole)
{
	UNITTEST_METHOD(cAppConsole)
	{
		cAppConsole& console = cAppConsole::I();
		if (!console.isConsoleMode())
		{
			if (sm_pLog != nullptr)
			{
				sm_pLog->addDebugInfoF("cAppConsole is NOT in CONSOLE MODE");
			}
			// try to create or attach a console using AllocConsole() ??
			console.AttachOrAllocConsole();
		}

		console.WriteString(_GT("cAppConsole in CONSOLE MODE" STR_NL));

		if (cUnitTestCur::IsTestInteractive())
		{
			console.WriteString(_GT("Press ESC to continue." STR_NL));

			console.SetKeyModes(false, false);
			for (int i = 0; i < 20; i++)
			{
				const int iKey = console.ReadKeyWait();
				if (iKey == ASCII_ESC)	// ESC = 27
					break;
				console.Printf(_GT("Got Key %d='%c'." STR_NL), iKey, iKey);
				cThreadId::SleepCurrent(1);
			}
			console.SetKeyModes();	// restore modes to default.
		}
		console.ReleaseConsole();
	}
};
UNITTEST_REGISTER(cAppConsole, UNITTEST_LEVEL_Core);
#endif

#endif	// UNDER_CE
