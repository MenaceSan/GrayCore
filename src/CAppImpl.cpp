//
//! @file cAppImpl.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAppImpl.h"
#include "cOSModule.h"
#include "cLogMgr.h"
#include "cLogAppendConsole.h"

#ifdef _MFC_VER
extern int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
#endif

namespace Gray
{
#ifndef _MFC_VER
	const char* cAppImpl::k_HelpText = " -help, -? = Get a general description of this program." STR_NL
		" -debugger = Wait to engage the debugger." STR_NL;

	cAppImpl::cAppImpl(const FILECHAR_t* pszAppName)
		: cSingletonStatic<cAppImpl>(this)
		, m_pszAppName(pszAppName)	// to be set later.
		, m_nMinTickTime(10)	// mSec for OnTickApp
		, m_State(cAppState::I())
		, m_bCloseSignal(false)
	{
		if (StrT::IsWhitespace(m_pszAppName))
		{
			// Set to the app file name?? !!
			m_pszAppName = _FN("App");	// default.
		}
	}

	cAppImpl::~cAppImpl()
	{
	}

	cString cAppImpl::get_HelpText() const // virtual 
	{
		// App Name
		// Description.
		return k_HelpText;
	}

	bool cAppImpl::ShowHelp() // virtual 
	{
		//! Show my help text via console or dialog .

		cString sMsg = cAppState::get_AppFileTitle() + STR_NL;
		sMsg += get_HelpText();

		// is console mode or have a parent in console mode ? 
		cLogProcessor& log = cLogMgr::I();

		// Break into multi lines.
		log.addEventS(LOG_ATTR_PRINT, LOGLEV_MAJOR, sMsg, "");

		return false; // don't open app.
	}

	bool cAppImpl::CheckHelpArgs() // virtual
	{
		//! Is the caller requesting help text via the command line?
		//! All apps should support "-help" "-?" requests for assistance.
		//! Is this arg present on the command line ? like FindCommandArg()

		ITERATE_t iArg = m_State.m_Args.FindCommandArgs(true, _FN("help"), _FN("?"), nullptr);
		if (iArg < 0)
			return false;
		if (m_State.m_ArgsValid.IsSet((BIT_ENUM_t)iArg))	// already processed this argument. don't process it again..
			return false;
		m_State.SetArgValid(iArg);
		return true;	// I requested help.
	}

	BOOL cAppImpl::InitInstance() // virtual 
	{
		//! APPSTATE_RunInit
		//! Override this to make the application do something.
		//! Like CWinApp for MFC
		//! @return true = OK. false = exit now.

		// AttachToCurrentThread();
		return true;
	}

	bool cAppImpl::OnTickApp() // virtual 
	{
		//! Override this to make the application do something. Main loop of main thread.
		//! @return false = exit
		return !m_bCloseSignal;	// just keep going.
	}

	int cAppImpl::Run() // virtual 
	{
		//! APPSTATE_Run
		//! Override this to make the application do something. Main loop of main thread.
		//! Like CWinApp for MFC
		//! @return APP_EXITCODE_t return app exit code. APP_EXITCODE_OK (NOT THREAD_EXITCODE_t?)
		//! @note In _WIN32, If my parent is a console, the console will return immediately! Not on first message loop like old docs say.

		// Log a message if there were command line arguments that did nothing. unknown.
		cStringF sInvalidArgs = m_State.get_InvalidArgs();
		if (!sInvalidArgs.IsEmpty())
		{
			// Check m_ArgsValid. Show Error for any junk arguments.
			cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLEV_CRIT, "Unknown command line args. '%s'", LOGSTR(sInvalidArgs));
			// return E_FAIL;
		}

		for (;;)
		{
			TIMESYS_t tStart = cTimeSys::GetTimeNow();	// start of tick.
			if (!OnTickApp())
				break;
			if (m_nMinTickTime > 0)
			{
				// if actual Tick time is less than minimum then wait.
				TIMESYS_t tNow = cTimeSys::GetTimeNow();
				TIMESYSD_t iDiff = tNow - tStart;
				if (iDiff < m_nMinTickTime)
				{
					// Sleep to keep from taking over the CPU when i have nothing to do.
					if (iDiff < 0)
						iDiff = 0;
					cThreadId::SleepCurrent(m_nMinTickTime - iDiff);
				}
			}
		}
		return APP_EXITCODE_OK;
	}

	int cAppImpl::ExitInstance() // virtual 
	{
		//! APPSTATE_RunExit
		//! Override this to make the application do something to clean up.
		//! This should be called if Run() fails. NOT called if InitInstance fails.
		//! Like CWinApp for MFC
		//! @return APP_EXITCODE_t return app exit code. APP_EXITCODE_OK
		return APP_EXITCODE_OK;
	}

	APP_EXITCODE_t cAppImpl::Main(HMODULE hInstance)
	{
		//! The main application entry point and process loop. Like MFC AfxWinMain()
		//! Assume cAppStateMain was used.
		//! @return APP_EXITCODE_t

#ifdef _DEBUG
		DEBUG_MSG(("cAppImpl::Main '%s'", LOGSTR(m_pszAppName)));
		APPSTATE_TYPE_ eAppState = cAppState::I().get_AppState();
		ASSERT(eAppState == APPSTATE_Run);	// Assume cAppStateMain
#endif

#ifdef _WIN32
		if (hInstance != HMODULE_NULL)	// don't clear it if already set.
		{
			ASSERT(hInstance == cAppState::get_HModule());
			cAppState::sm_hInstance = hInstance;
		}
#endif

		if (CheckHelpArgs())
		{
			// Show help text. write to the console if there is a console else open a help dialog.
			if (!ShowHelp())
				return APP_EXITCODE_OK;	// don't open app.
		}

#ifdef _MFC_VER
		// Probably calls AfxWinInit()
		return (APP_EXITCODE_t) ::AfxWinMain(hInstance, HMODULE_NULL, LPTSTR lpCmdLine, nCmdShow);
#else
		APP_EXITCODE_t iRet = APP_EXITCODE_FAIL;
		m_State.put_AppState(APPSTATE_RunInit);

		if (InitInstance())
		{
			// Run loop until told to stop.
			m_State.put_AppState(APPSTATE_Run);
			iRet = (APP_EXITCODE_t)Run();
			m_State.put_AppState(APPSTATE_RunExit);
			APP_EXITCODE_t iRetExit = (APP_EXITCODE_t)ExitInstance();
			if (iRet == APP_EXITCODE_OK)
				iRet = iRetExit;
		}

		// Exit.
		m_State.put_AppState(APPSTATE_Exit);
		return iRet;
#endif
	}

#endif
}
