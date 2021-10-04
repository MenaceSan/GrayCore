//
//! @file cAppImpl.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAppImpl_H
#define _INC_cAppImpl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cAppState.h"
#include "cObject.h"
#include "cOSModule.h"

#ifndef _MFC_VER
namespace Gray
{
	typedef HRESULT(GRAYCALL* AppCommandF_t)(int iArgN, const FILECHAR_t* pszArg); // FARPROC

	struct GRAYCORE_LINK cAppCommand
	{
		//! @struct Gray::cAppCommand
		//! a command line switch that does something. similar to MFC CCmdTarget ?
		//! Abstract Base class for a command handler (plugin).
		//! typically static allocated.

	public:
		const FILECHAR_t* m_pszSwitch;		//!< abbreviated -switch or /switch (case sensitive) optional, nullptr allowed
		const ATOMCHAR_t* m_pszName;		//!< symbolic name for -switch or /switch (case insensitive). MUST be unique.
		const char* m_pszHelpArgs;			//!< describe any extra args this function might take. "[optional arg]. nullptr = takes none.
		const char* m_pszHelp;				//!< help description.
		AppCommandF_t m_pCommand;	//!< we can override or use this function pointer to implement.

	public:
		cAppCommand(const FILECHAR_t* pszSwitch, const ATOMCHAR_t* pszName
			, const char* pszHelpArgs, const char* pszHelp
			, AppCommandF_t pCommand = nullptr) noexcept
			: m_pszSwitch(pszSwitch), m_pszName(pszName)
			, m_pszHelpArgs(pszHelpArgs), m_pszHelp(pszHelp)
			, m_pCommand(pCommand)
		{
		}
		virtual ~cAppCommand()
		{
		}

		bool IsMatch(cStringF sArg) const;

		// return # of EXTRA args consumed. or <0 = error.
		virtual HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) 	//!< call this if we see the m_pszCmd switch. can consume more arguments (or not).
		{
			if (m_pCommand == nullptr)
				return E_NOTIMPL;
			return m_pCommand(iArgN, pszArg);
		}
	};

	class GRAYCORE_LINK cAppImpl
		: public cSingletonStatic < cAppImpl > // use static theApp
	{
		//! @class Gray::cAppImpl
		//! Entry point for my implemented application. I am not a _WINDLL.
		//! like (CWinApp for MFC) (maybe windowed or console)
		//! I am NOT a library/DLL. I am an application implementation. NOT the same as (or to be merged with) cAppState.
		//! Basic framework for my application I implement. Assume a static like cAppImpl theApp is defined some place.

	public:
		const FILECHAR_t* m_pszAppName;		//!< Specifies the name of my application. (display friendly)
		TIMESYSD_t m_nMinTickTime;			//!< Minimum amount of time to spend in the OnTickApp() (mSec). cThreadId::SleepCurrent() if there is extra time.
		cAppState& m_State;					//!< Quick reference to cAppState singleton.
		bool m_bCloseSignal;				//!< Polite request to close the application. checked in Run() and OnTickApp() >= APPSTATE_RunExit APPSTATE_Exit

		cArrayPtr<cAppCommand> m_aCommands;		//! built a list of possible commands. Dynamically add new command handlers to the app. to process cAppArgs.

	public:
		cAppImpl(const FILECHAR_t* pszAppName);
		virtual ~cAppImpl();

		cAppCommand* RegisterCommand(cAppCommand& cmd);
		HRESULT RunCommand(ITERATE_t i, const FILECHAR_t* pszCmd);
		HRESULT RunCommands();

		static inline HINSTANCE get_HInstance()
		{
			// Similar to MFC?
			return cAppState::get_HModule();
		}

		virtual cString get_HelpText() const;

		virtual BOOL InitInstance();
		virtual bool OnTickApp();
		virtual int Run();
		virtual int ExitInstance();

		APP_EXITCODE_t Main(HMODULE hInstance = HMODULE_NULL);
	};
}
#endif
#endif
