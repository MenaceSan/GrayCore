//
//! @file CAppImpl.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CAppImpl_H
#define _INC_CAppImpl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CAppState.h"
#include "CObject.h"
#include "COSModule.h"

#ifndef _MFC_VER
namespace Gray
{
	class GRAYCORE_LINK CAppImpl
		: public CSingletonStatic < CAppImpl > // use static theApp
	{
		//! @class Gray::CAppImpl
		//! Entry point for my implemented application.
		//! like (CWinApp for MFC) (maybe windowed or console)
		//! I am NOT a library/DLL. I am an application implementation. NOT the same as (or to be merged with) CAppState.
		//! Basic framework for my application I implement. Assume a static like CAppImpl theApp is defined some place.

	public:
		static const char* k_HelpText;

		const FILECHAR_t* m_pszAppName;		//!< Specifies the name of my application. (display friendly)
		TIMESYSD_t m_nMinTickTime;			//!< Minimum amount of time to spend in the OnTickApp() (mSec). CThreadId::SleepCurrent() if there is extra time.
		CAppState& m_State;					//!< Quick reference to CAppState singleton.
		bool m_bCloseSignal;				//!< Polite request to close the application. checked in Run() and OnTickApp()

	public:
		CAppImpl(const FILECHAR_t* pszAppName = nullptr);
		virtual ~CAppImpl();

		static inline HINSTANCE get_HInstance()
		{
			// Similar to MFC?
			return CAppState::get_HModule();
		}

		virtual cString get_HelpText() const;
		virtual bool ShowHelp();
		virtual bool CheckHelpArgs();

		virtual BOOL InitInstance();
		virtual bool OnTickApp();
		virtual int Run();
		virtual int ExitInstance();

		APP_EXITCODE_t Main(HMODULE hInstance = HMODULE_NULL);
	};
}
#endif
#endif
