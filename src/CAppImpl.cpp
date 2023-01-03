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
			if (sArg.Compare(m_pszSwitch) == 0)	// case Sensitive.
				return true;
		}
		return sArg.IsEqualNoCase(StrArg<FILECHAR_t>(m_pszName));
	}

	/// <summary>
	/// request help text via the command line
	/// All apps should support "-help" "-?" requests for assistance.
	/// Is this arg present on the command line ? like FindCommandArg()
	/// </summary>
	class cAppCmdHelp : public cAppCommand
	{
	public:
		cAppCmdHelp()
			: cAppCommand(_FN("?"), "help", nullptr, "Get a general description of this program.")	// CATOM_N()
		{
		}

		/// <summary>
		/// Show my help text via console or dialog .
		/// </summary>
		/// <param name="iArgN"></param>
		/// <param name="pszArg">nullptr or extra arg for specific help.</param>
		/// <returns></returns>
		HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) override
		{
			UNREFERENCED_PARAMETER(iArgN);
			UNREFERENCED_PARAMETER(pszArg);

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

	/// <summary>
	/// I want to debug something in startup code.
	/// </summary>
	class cAppCmdWaitForDebug : public cAppCommand
	{
	public:
		cAppCmdWaitForDebug()
			: cAppCommand(_FN("wfd"), "waitfordebugger", nullptr, "Wait for the debugger to attach.") // CATOM_N()
		{
		}

		HRESULT DoCommand(int iArgN, const FILECHAR_t* pszArg) override
		{
			// TODO  pop message box or use console ? to wait for user input.
			UNREFERENCED_PARAMETER(iArgN);
			UNREFERENCED_PARAMETER(pszArg);
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
		DEBUG_CHECK(!StrT::IsWhitespace(m_pszAppName));
		m_aCommands.Add(&k_Help);
		// m_aCommands.Add(&k_WaitForDebug); 
	}

	cAppImpl::~cAppImpl()
	{
	}

	cString cAppImpl::get_HelpText() const // virtual override
	{
		//! Get help for all the m_aCommands we support.

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

	cAppCommand* cAppImpl::RegisterCommand(cAppCommand& cmd)
	{
		// Add or override existing command.
		// is it an override ?

		for (ITERATE_t i = 0; i < m_aCommands.GetSize(); i++)	// collision?
		{
			cAppCommand* pCmd2 = m_aCommands[i];
			if (StrT::CmpI(pCmd2->m_pszName, cmd.m_pszName) == COMPARE_Equal)
			{
				// collide, replace ?
				DEBUG_ERR(("RegisterCommand collision '%s'", LOGSTR(cmd.m_pszName)));
				return pCmd2;
			}
			if (StrT::Cmp(pCmd2->m_pszSwitch, cmd.m_pszSwitch) == COMPARE_Equal)
			{
				// allow collide ?
			}
		}

		m_aCommands.Add(&cmd);
		return &cmd;
	}

	BOOL cAppImpl::InitInstance() // virtual 
	{
		//! APPSTATE_RunInit
		//! Override this to make the application do something at start.
		//! Like CWinApp for MFC
		//! @return true = OK. false = exit now.

		// AttachToCurrentThread();
		return true;	// Run() will be called.
	}

	bool cAppImpl::OnTickApp() // virtual 
	{
		//! Override this to make the application do something. Main loop of main thread.
		//! @return false = exit
		return !m_bCloseSignal && m_State.isAppStateRun();	// just keep going. not APPSTATE_Exit
	}

	HRESULT cAppImpl::RunCommand(ITERATE_t i, const FILECHAR_t* pszCmd)
	{
		// Run a single command and set up its arguments.
		// @arg i = index in m_Args array.

		while (cAppArgs::IsArgSwitch(*pszCmd))
		{
			pszCmd++;
		}

		// use "Cmd=Val" syntax?
		// args can come from parse for = sign else get from the next arg in sequence (that isnt starting with IsSwitch(-/)) 
		cStringF sCmd2;
		const FILECHAR_t* pszArgEq = StrT::FindChar<FILECHAR_t>(pszCmd, '=');
		if (pszArgEq != nullptr)
		{
			sCmd2 = cStringF(pszCmd, StrT::Diff(pszArgEq, pszCmd));
			pszCmd = sCmd2;
			pszArgEq++;
		}

		cAppCommand* pCmd = nullptr;
		for (int j = 0; j < m_aCommands.GetSize(); j++)	// find handler for this type of command.
		{
			cAppCommand* pCmd2 = m_aCommands[j];
			if (pCmd2->IsMatch(pszCmd))
			{
				pCmd = pCmd2;
				break;
			}
		}

		if (pCmd == nullptr)	// no idea how to process this switch. might be an erorr
		{
			return i;	// just skip this.
		}

		m_State.m_ArgsValid.SetBit(i);	// found it anyhow.

		HRESULT hRes = pCmd->DoCommand(i, pszArgEq != nullptr ? pszArgEq : m_State.m_Args.GetArgEnum(i + 1));
		if (FAILED(hRes))
		{
			// Stop processing. report error.
			LOGF((LOG_ATTR_INIT, LOGLEV_CRIT, "Command line '%s' failed '%s'", LOGSTR(pszCmd), LOGERR(hRes)));
			return hRes;
		}

		int j = i;
		if (pszArgEq != nullptr)
		{
			// use "Cmd=Val" syntax?
			if (hRes > 1)
				j += hRes - 1;
		}
		else
		{
			j += hRes;	// Process more arguments?
		}

		for (; i < j; i++)
		{
			m_State.m_ArgsValid.SetBit(i + 1);	// consumed
		}

		return i;
	}

	HRESULT cAppImpl::RunCommands()
	{
		// NOTE: some commands may be run ahead of this. They effect init.

		const cAppArgs& args = m_State.m_Args;
		ITERATE_t i = 1;
		const ITERATE_t nQty = args.get_ArgsQty();
		for (; i < nQty; i++)
		{
			if (m_State.m_ArgsValid.IsSet((BIT_ENUM_t)i))	// already processed this argument. (out of order ?). don't process it again. no double help.
				continue;
			cStringF sCmd = args.GetArgEnum(i);
			HRESULT hRes = RunCommand(i, sCmd);
			if (FAILED(hRes))
			{
				// Stop processing. report error.
				cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLEV_CRIT, "Command line '%s' failed '%s'", LOGSTR(sCmd), LOGERR(hRes));
				return hRes;
			}
			i = hRes;	// maybe skip some args ?
		}

		return i;
	}

	int cAppImpl::Run() // virtual 
	{
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
		return APP_EXITCODE_OK;
	}

	APP_EXITCODE_t cAppImpl::Main(HMODULE hInstance)
	{
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
