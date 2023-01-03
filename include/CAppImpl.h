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

	/// <summary>
	/// define a named command line switch that does something. similar to MFC CCmdTarget ?
	/// Abstract Base class for a command handler (plugin).
	/// typically static allocated.
	/// </summary>
	struct GRAYCORE_LINK cAppCommand
	{
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

		/// <summary>
		/// Execute a command
		/// </summary>
		/// <param name="iArgN"></param>
		/// <param name="pszArg"></param>
		/// <returns># of EXTRA args consumed. or lt 0 = error.</returns>
		virtual HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) 	//!< call this if we see the m_pszCmd switch. can consume more arguments (or not).
		{
			if (m_pCommand == nullptr)
				return E_NOTIMPL;
			return m_pCommand(iArgN, pszArg);
		}
	};

	/// <summary>
	/// Entry point for my implemented application. I am not a _WINDLL.
	/// like CWinApp for MFC (maybe windowed or console)
	/// I am NOT a library/DLL. I am an application implementation. NOT the same as (or to be merged with) cAppState.
	/// Basic framework for my application I implement. Assume a static like cAppImpl theApp is defined some place.
	/// </summary>
	class GRAYCORE_LINK cAppImpl
		: public cSingletonStatic < cAppImpl > // use static theApp
	{
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

		/// <summary>
		/// APPSTATE_Run
		/// Override this to make the application do something. Main loop of main thread.
		/// Like CWinApp for MFC
		/// @note In _WIN32, If my parent is a console, the console will return immediately! Not on first message loop like old docs say.
		/// </summary>
		/// <returns>APP_EXITCODE_t return app exit code. APP_EXITCODE_OK (NOT THREAD_EXITCODE_t?)</returns>
		virtual int Run();

		/// <summary>
		/// APPSTATE_RunExit
		/// Override this to make the application do something to clean up.
		/// This should be called if Run() fails. NOT called if InitInstance fails.
		/// Like CWinApp for MFC
		/// </summary>
		/// <returns>APP_EXITCODE_t return app exit code. APP_EXITCODE_OK</returns>
		virtual int ExitInstance();

		/// <summary>
		/// The main application entry point and process loop. Like MFC AfxWinMain() 
		/// Assume cAppStateMain was used.
		/// </summary>
		/// <param name="hInstance"></param>
		/// <returns>APP_EXITCODE_t</returns>
		APP_EXITCODE_t Main(HMODULE hInstance = HMODULE_NULL);
	};
}
#endif
#endif
