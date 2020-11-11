//
//! @file cAppConsole.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAppConsole_H
#define _INC_cAppConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSingleton.h"

#if ! defined(UNDER_CE) 
#include "cStream.h"
#include "cUnitTestDecl.h"
#include "StrArg.h"

UNITTEST_PREDEF(cAppConsole)

namespace Gray
{
	enum AppStd_TYPE
	{
		//! @enum Gray::AppStd_TYPE
		//! Standard streams/handles. True for both __linux__ and _WIN32. (though __linux__ implementation is hidden)
		AppStd_stdin = 0,	//!< stdin  (&__iob_func()[0]) -> GetStdHandle(STD_INPUT_HANDLE) = STDIN_FILENO
		AppStd_stdout = 1,	//!< stdout (&__iob_func()[1]) -> GetStdHandle(STD_OUTPUT_HANDLE) = STDOUT_FILENO
		AppStd_stderr = 2,	//!< stderr (&__iob_func()[2]) -> GetStdHandle(STD_ERROR_HANDLE) = STDERR_FILENO
		AppStd_QTY,
	};
	enum AppCon_TYPE
	{
		//! @enum Gray::AppCon_TYPE
		//! What type of console is connected?

		AppCon_UNKNOWN = -1,
		AppCon_NONE = 0,
		AppCon_Proc = 1,	//!< Process was build as _CONSOLE mode. stdin,stdout already setup.
		AppCon_Attach = 2,	//!< Attached to parent console. must call FreeConsole()
		AppCon_Create = 3,	//!< Created my own console. must call FreeConsole()
	};

	class GRAYCORE_LINK cAppConsole
		: public cSingleton < cAppConsole >
		, public cStreamOutput
	{
		//! @class Gray::cAppConsole
		//! Manage console output/input for this app. use of printf() etc
		//! This allows apps not compiled in _CONSOLE mode to attach to a console if they are started in one (or create one if not).

		friend class cSingleton < cAppConsole >;

	public:
		static const COUNT_t k_MAX_CONSOLE_LINES = 500; //!< arbitrary max lines shown at once.

#if	defined(_WIN32)
		HANDLE m_hStd[AppStd_QTY];	//!< stdin,stdout,stderr as cOSHandle. But i don't need to close these ? ::GetStdHandle()
#elif defined(__linux__)
		// __iob_func
#endif

	private:
		bool m_bKeyEchoMode;		//!< default true = echo the keys to the display.
		bool m_bKeyEnterMode;		//!< default true = wait for enter before return. false = get each key char as it comes in (raw).

		AppCon_TYPE m_eConsoleType;	//!< 2 = I called AttachConsole(), 3 = I called AllocConsole() and must call FreeConsole()
		bool m_bConsoleParent;			//!< My parent process is a console. I may attach to it.
		int m_iAllocConsoleCount;		//!< I had to create my own console. must call FreeConsole() this many times.

		mutable cThreadLockCount m_Lock;			//!< serialize multiple threads.

	protected:
		cAppConsole();
		virtual ~cAppConsole();

		void CheckConsoleMode() noexcept;
		bool AttachConsoleSync();

	public:
		bool HasConsoleParent() noexcept
		{
			//! started from command line ? Call AllocConsole to start using console.
			CheckConsoleMode();
			return m_bConsoleParent;
		}
		AppCon_TYPE get_ConsoleMode() noexcept
		{
			CheckConsoleMode();
			return m_eConsoleType;
		}
		bool isConsoleMode() noexcept
		{
			//! Is the app already running in console mode? can i use printf() ?
			//! 1. I am _CONSOLE app, 2. I attached to my parent. 3. I created a console.
			return get_ConsoleMode() != AppCon_NONE;
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
			//! support cStreamOutput
			//! Do not assume line termination with \n
			HRESULT hRes = WriteStrOut(pszStr);
			if (FAILED(hRes))
				return hRes;
			return 1;
		}
		virtual HRESULT WriteString(const wchar_t* pszStr) override
		{
			//! support cStreamOutput
			//! Do not assume line termination with \n
			HRESULT hRes = WriteStrOut(StrArg<char>(pszStr));
			if (FAILED(hRes))
				return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
			return 1;
		}

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(cAppConsole);
	};
}
#endif	// UNDER_CE
#endif	// _INC_cAppConsole_H
