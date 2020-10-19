//
//! @file CAppConsole.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CAppConsole_H
#define _INC_CAppConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSingleton.h"

#if ! defined(UNDER_CE) && USE_CRT	// fix USE_CRT ? 
#include "CStream.h"
#include "CUnitTestDecl.h"
#include "StrArg.h"

UNITTEST_PREDEF(CAppConsole)

namespace Gray
{
	enum CAppStd_TYPE
	{
		//! @enum Gray::CAppStd_TYPE
		//! Standard streams/handles. True for both __linux__ and _WIN32. (though __linux__ implementation is hidden)
		CAppStd_stdin = 0,	//!< stdin  (&__iob_func()[0]) -> GetStdHandle(STD_INPUT_HANDLE) = STDIN_FILENO
		CAppStd_stdout = 1,	//!< stdout (&__iob_func()[1]) -> GetStdHandle(STD_OUTPUT_HANDLE) = STDOUT_FILENO
		CAppStd_stderr = 2,	//!< stderr (&__iob_func()[2]) -> GetStdHandle(STD_ERROR_HANDLE) = STDERR_FILENO
		CAppStd_QTY,
	};
	enum CAppCon_TYPE
	{
		//! @enum Gray::CAppCon_TYPE
		//! What type of console is connected?

		CAppCon_UNKNOWN = -1,
		CAppCon_NONE = 0,
		CAppCon_Proc = 1,	//!< Process was build as _CONSOLE mode. stdin,stdout already setup.
		CAppCon_Attach = 2,	//!< Attached to parent console. must call FreeConsole()
		CAppCon_Create = 3,	//!< Created my own console. must call FreeConsole()
	};

	class GRAYCORE_LINK CAppConsole
		: public CSingleton < CAppConsole >
		, public CStreamOutput
	{
		//! @class Gray::CAppConsole
		//! Manage console output/input for this app. use of printf() etc
		//! This allows apps not compiled in _CONSOLE mode to attach to a console if they are started in one (or create one if not).

		friend class CSingleton < CAppConsole >;

	public:
		static const COUNT_t k_MAX_CONSOLE_LINES = 500; //!< arbitrary max lines shown at once.

#if	defined(_WIN32)
		HANDLE m_hStd[CAppStd_QTY];	//!< stdin,stdout,stderr as COSHandle. But i don't need to close these ? ::GetStdHandle()
#elif defined(__linux__)
		// __iob_func
#endif

	private:
		bool m_bKeyEchoMode;		//!< default true = echo the keys to the display.
		bool m_bKeyEnterMode;		//!< default true = wait for enter before return. false = get each key char as it comes in (raw).

		CAppCon_TYPE m_eConsoleType;	//!< 2 = I called AttachConsole(), 3 = I called AllocConsole() and must call FreeConsole()
		bool m_bConsoleParent;			//!< My parent process is a console. I may attach to it.
		int m_iAllocConsoleCount;		//!< I had to create my own console. must call FreeConsole() this many times.

		mutable CThreadLockCount m_Lock;			//!< serialize multiple threads.

	protected:
		CAppConsole();
		virtual ~CAppConsole();

		void CheckConsoleMode() noexcept;
#if defined(_WIN32) && ! defined(UNDER_CE) && USE_CRT  
		bool AttachConsoleSync();
#endif

	public:
		bool HasConsoleParent() noexcept
		{
			//! started from command line ? Call AllocConsole to start using console.
			CheckConsoleMode();
			return m_bConsoleParent;
		}
		CAppCon_TYPE get_ConsoleMode() noexcept
		{
			CheckConsoleMode();
			return m_eConsoleType;
		}
		bool isConsoleMode() noexcept
		{
			//! Is the app already running in console mode? can i use printf() ?
			//! 1. I am _CONSOLE app, 2. I attached to my parent. 3. I created a console.
			return get_ConsoleMode() != CAppCon_NONE;
		}

		//! make printf() go to the console. create console if needed.
		bool AttachOrAllocConsole(bool bAttachElseAlloc = true);
		void ReleaseConsole();

		HRESULT WriteStrErr(const char* pszText);
		HRESULT WriteStrOut(const char* pszText);

		HRESULT SetKeyModes(bool bEchoMode = true, bool bEnterMode = true);
		int get_KeyReadQty() const;
		int ReadKeyWait();		//!< Get a single char. -1 = block/wait for char failed.
		int ReadKey();			//!< Get a single char. -1 = none avail. non blocking,.

		virtual HRESULT WriteString(const char* pszStr) override
		{
			//! support CStreamOutput
			//! Do not assume line termination with \n
			HRESULT hRes = WriteStrOut(pszStr);
			if (FAILED(hRes))
				return hRes;
			return 1;
		}
		virtual HRESULT WriteString(const wchar_t* pszStr) override
		{
			//! support CStreamOutput
			//! Do not assume line termination with \n
			HRESULT hRes = WriteStrOut(StrArg<char>(pszStr));
			if (FAILED(hRes))
				return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
			return 1;
		}

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(CAppConsole);
	};
}
#endif	// UNDER_CE
#endif	// _INC_CAppConsole_H
