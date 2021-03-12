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

	bool cAppCommand::IsMatch(cStringF sArg) const
	{
		// assume switch prefix (-/) has already been stripped.
		if (m_pszSwitch != nullptr) // optional
		{
			if (sArg.Compare(m_pszSwitch) == 0)	// case sensative.
				return true;
		}
		return sArg.CompareNoCase(m_pszName) == 0;
	}

	class cAppCmdHelp : public cAppCommand
	{
		//! requesting help text via the command line
		//! All apps should support "-help" "-?" requests for assistance.
		//! Is this arg present on the command line ? like FindCommandArg()
	public:
		cAppCmdHelp()
			: cAppCommand(_FN("?"), "help", nullptr, "Get a general description of this program.")	// CATOM_N()
		{
		}

		HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) override
		{
			//! Show my help text via console or dialog .
			//! pszArg = nullptr or extra arg for specific help.

			cString sText = cAppState::get_AppFileTitle() + STR_NL;
			sText += cAppImpl::I().get_HelpText();

			// is console mode or have a parent in console mode ? 
			cLogProcessor& log = cLogMgr::I();

			// Break into multi lines.
			log.addEventS(LOG_ATTR_PRINT, LOGLEV_MAJOR, sText, "");

			return 0;	// consume no more arguments.
		}
	};

	static cAppCmdHelp k_Help;			//!< basic help command.

	class cAppCmdWaitForDebug : public cAppCommand
	{
		// I want to debug something in startup code.
	public:
		cAppCmdWaitForDebug()
			: cAppCommand(_FN("wfd"), "waitfordebugger", nullptr, "Wait for the debugger to attach.") // CATOM_N()
		{
		}

		HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) override
		{
			// TODO
			return E_NOTIMPL;
		}
	};

	static cAppCmdWaitForDebug k_WaitForDebug;	//!< wait for the debugger to attach.

	cAppImpl::cAppImpl(const FILECHAR_t* pszAppName)
		: cSingletonStatic<cAppImpl>(this)
		, m_pszAppName(pszAppName)	// to be set later.
		, m_nMinTickTime(10)	// mSec for OnTickApp
		, m_State(cAppState::I())
		, m_bCloseSignal(false)
	{
		m_aCommands.Add(&k_Help);
		// m_aCommands.Add(&k_WaitForDebug);

		if (StrT::IsWhitespace(m_pszAppName))
		{
			// Set to the app file name?? !!
			m_pszAppName = _FN("App");	// default name.
		}
	}

	cAppImpl::~cAppImpl()
	{
	}

	cString cAppImpl::get_HelpText() const // virtual override
	{
		// Get help for all the m_aCommands we support.

		cString sHelp = _GT("");
		for (int i = 0; i < m_aCommands.GetSize(); i++)
		{
			const cAppCommand* pCmd = m_aCommands[i];
			if (pCmd->m_pszHelp == nullptr)	// hidden
			{
				continue;
			}

			if (pCmd->m_pszSwitch != nullptr)
			{
				sHelp += _GT("-");
				sHelp += StrArg<GChar_t>(pCmd->m_pszSwitch);
				sHelp += _GT(", ");
			}

			sHelp += _GT("-");
			sHelp += StrArg<GChar_t>(pCmd->m_pszName);
			sHelp += _GT(", ");

			if (pCmd->m_pszHelpArgs != nullptr)
			{
				sHelp += StrArg<GChar_t>(pCmd->m_pszHelpArgs);
				sHelp += _GT(", ");
			}

			sHelp += StrArg<GChar_t>(pCmd->m_pszHelp);
			sHelp += _GT(STR_NL);
		}

		return sHelp;
	}

	cAppCommand* cAppImpl::AddCommand(cAppCommand& cmd)
	{
		// Add or override existing command.
		// is it an override ?

		for (int i = 0; i < m_aCommands.GetSize(); i++)	// collision?
		{
			cAppCommand* pCmd2 = m_aCommands[i];
			if (StrT::CmpI(pCmd2->m_pszName, cmd.m_pszName) == 0)
			{

			}
			if (StrT::Cmp(pCmd2->m_pszSwitch, cmd.m_pszSwitch) == 0)
			{

			}
		}

		m_aCommands.Add(&cmd);
		return nullptr;
	}

	BOOL cAppImpl::InitInstance() // virtual 
	{
		//! APPSTATE_RunInit
		//! Override this to make the application do something at start.
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

	HRESULT cAppImpl::RunCommands()
	{
		// NOTE: some commands may be run ahead of this. They effect init.

		cAppArgs& args = m_State.m_Args;
		for (int i = 1; i < args.get_ArgsQty(); i++)
		{
			if (m_State.m_ArgsValid.IsSet((BIT_ENUM_t)i))	// already processed this argument. (out of order ?). don't process it again. no double help.
				continue;

			cStringF sArg = args.GetArgEnum(i);
			const FILECHAR_t* pszCmd = sArg;
			while (cAppArgs::IsArgSwitch(*pszCmd))
			{
				pszCmd++;
			}

			// use "Cmd=Val" syntax?
			// args can come from parse for = sign else get from the next arg in sequence (that isnt starting with IsSwitch(-/)) 
			cStringF sCmd;	// dont use sArg
			const FILECHAR_t* pszArgEq = StrT::FindChar(pszCmd, '=');
			if (pszArgEq != nullptr)
			{
				sCmd = cStringF(pszCmd, pszArgEq - pszCmd);
				pszCmd = sCmd;
			}

			cAppCommand* pCmd = nullptr;
			for (int j = 0; j < m_aCommands.GetSize(); j++)
			{
				cAppCommand* pCmd2 = m_aCommands[j];
				if (pCmd2->IsMatch(pszCmd))
				{
					pCmd = pCmd2;
					break;
				}
			}
			if (pCmd == nullptr)	// no idea how to process this switch. might be an erorr
				continue;

			m_State.m_ArgsValid.SetBit(i);	// found it anyhow.

			HRESULT hRes = pCmd->DoCommand(i, pszArgEq != nullptr ? pszArgEq : args.GetArgEnum(i + 1));
			if (FAILED(hRes))
			{
				// Stop processing. report error.
				cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLEV_CRIT, "Command line '%s' failed '%s'", LOGSTR(sArg), LOGERR(hRes));
				return hRes;
			}

			if (pszArgEq != nullptr)
			{
				// use "Cmd=Val" syntax.
				if (hRes > 1)
					i += hRes - 1;
			}
			else
			{
				i += hRes;	// Process more arguments.
			}
		}

		return S_OK;
	}

	int cAppImpl::Run() // virtual 
	{
		//! APPSTATE_Run
		//! Override this to make the application do something. Main loop of main thread.
		//! Like CWinApp for MFC
		//! @return APP_EXITCODE_t return app exit code. APP_EXITCODE_OK (NOT THREAD_EXITCODE_t?)
		//! @note In _WIN32, If my parent is a console, the console will return immediately! Not on first message loop like old docs say.

		HRESULT hRes = RunCommands();
		if (FAILED(hRes))
		{
			m_bCloseSignal = true;
			return APP_EXITCODE_FAIL;
		}

		// Log a message if there were command line arguments that did nothing. unknown.
		cStringF sInvalidArgs = m_State.get_InvalidArgs();
		if (!sInvalidArgs.IsEmpty())
		{
			// Check m_ArgsValid. Show Error for any junk/unused arguments.
			cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLEV_CRIT, "Unknown command line args. '%s'", LOGSTR(sInvalidArgs));
			// return APP_EXITCODE_FAIL;
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

		m_bCloseSignal = true;
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

		m_State.put_AppState(APPSTATE_RunInit);

#ifdef _MFC_VER
		// Probably calls AfxWinInit() and assume will call InitInstance()
		return (APP_EXITCODE_t) ::AfxWinMain(hInstance, HMODULE_NULL, LPTSTR lpCmdLine, nCmdShow);
#else
		APP_EXITCODE_t iRet = APP_EXITCODE_FAIL;
		if (InitInstance())
		{
			// Run loop until told to stop.
			m_State.put_AppState(APPSTATE_Run);
			iRet = (APP_EXITCODE_t)Run();
			m_State.put_AppState(APPSTATE_RunExit);
			APP_EXITCODE_t iRetExit = (APP_EXITCODE_t)ExitInstance();
			if (iRet == APP_EXITCODE_OK) // allow exit to make this fail.
				iRet = iRetExit;
		}

		// Exit.
		m_State.put_AppState(APPSTATE_Exit);
		return iRet;
#endif
	}

#endif	// MFC
}
