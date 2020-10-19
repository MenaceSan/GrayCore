//
//! @file CLogAppendConsole.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CLogMgr.h"
#include "CLogAppendConsole.h"
#include "CAppConsole.h"
#include "CAppState.h"
#include "COSProcess.h"

#if !defined(UNDER_CE) && USE_CRT

namespace Gray
{
	CLogAppendConsole::CLogAppendConsole()
		: CSingletonSmart<CLogAppendConsole>(this, typeid(CLogAppendConsole))
	{
		// CLogAppendConsole::AddAppenderCheck()
	}

	CLogAppendConsole::~CLogAppendConsole() // virtual 
	{
	}

	HRESULT CLogAppendConsole::WriteString(const LOGCHAR_t* pszText) // virtual
	{
		//! write log/debug string to the console stderr. but maybe stdout ??
		//! http://www.jstorimer.com/blogs/workingwithcode/7766119-when-to-use-stderr-instead-of-stdout
		//! @return StrLen_t

		// LOG_ATTR_PRINT? // WriteStrErr or WriteStrOut?
		HRESULT hRes = CAppConsole::I().WriteStrOut(pszText);
		if (FAILED(hRes))
			return hRes;
		return(1);	// something was written.
	}

	HRESULT GRAYCALL CLogAppendConsole::AddAppenderCheck(CLogNexus* pLogger, bool bAttachElseAlloc) // static
	{
		//! Make sure CLogAppendConsole singleton is attached to CLogMgr
		//! Push log messages to the console (or my parent console) if there is one.
		//! Add this console appender to CLogMgr/pLogger if not already added.
		//! @arg bAttachOrCreate = create my own console if there is no parent to attach to. 

		if (pLogger == nullptr)
		{
			pLogger = CLogMgr::get_Single();
		}
		if (pLogger->FindAppenderType(typeid(CLogAppendConsole)) != nullptr) // already has this appender.
			return S_FALSE;

		CAppConsole& ac = CAppConsole::I();
		if (!ac.isConsoleMode())	// no console ?
		{
			// Attach to parent console or create my own.
			if (!ac.AttachOrAllocConsole(bAttachElseAlloc))
				return E_NOTIMPL;
		}

		// attach new console appender to logging system.
		pLogger->AddAppender(new CLogAppendConsole);
		return S_OK;
	}

	bool GRAYCALL CLogAppendConsole::RemoveAppenderCheck(CLogNexus* pLogger, bool bOnlyIfParent) // static
	{
		//! remove this appender if there is a parent console. leave it if i created the console.
		//! We only created it for start up status and errors.
		//! @arg bOnlyIfParent = only remove the console appender if its my parent process console. NOT if I'm _CONSOLE or I created the console.
		if (pLogger == nullptr)
		{
			pLogger = CLogMgr::get_Single();
		}

		CLogAppender* pAppender = pLogger->FindAppenderType(typeid(CLogAppendConsole));
		if (pAppender == nullptr) 
			return false;
		if (bOnlyIfParent)
		{
			// Detach from parent console.
			CAppConsole& ac = CAppConsole::I();
			if (ac.get_ConsoleMode() != CAppCon_Attach)
			{
				return false;
			}
			ac.ReleaseConsole();
		}
		return pLogger->RemoveAppender(pAppender, true);
	}

	HRESULT GRAYCALL CLogAppendConsole::ShowMessageBox(cString sMsg, UINT uFlags) // static
	{
		//! Display a message that needs user feedback. This is something very important that the user should see.
		//! Use the console if it exists else put up a dialog if i can.
		//! @arg uFlags = 1 = MB_OKCANCEL;
		//! TODO Show message in GUI with MessageBox if no console available.
		//! like AfxMessageBox, MessageBox, etc
		//! http://unix.stackexchange.com/questions/144924/creating-a-messagebox-using-commandline
		//! @return 1 = IDOK, 2=IDCANCEL

		CAppConsole& ac = CAppConsole::I();
		if (ac.isConsoleMode())
		{
			// Show prompt message.
			ac.WriteStrOut(StrArg<char>(sMsg));	// WriteStrOut WriteStrErr

			// Wait for user response.
			int iKey = ac.ReadKeyWait();	// 
			UNREFERENCED_PARAMETER(iKey);
			return 1;
		}

		// NOT in console. So we must use a pop up.

#ifdef _WIN32
		int iRet = _GTN(MessageBox)(HWND_DESKTOP, sMsg, CAppState::get_AppFileTitle(), uFlags);		// wait to attach debug.

#elif defined( __linux__)
		COSProcess proc;
		int iRet = 1;
		// TODO launch sub COSProcess to create message box.
		// notify-send "My name is bash and I rock da house"
		// notify-send -t 0 'hi there!' // does not expire.
		//

#endif
		return iRet;	// IDOK = 1
	}

	HRESULT GRAYCALL CLogAppendConsole::WaitForDebugger() // static
	{
		// Wait for the debugger to attach. -debugger command line arg.

		// CAppState::isDebuggerPresent()
		ShowMessageBox("Waiting for debugger", 0);
		return S_OK;
	}

}
#endif
