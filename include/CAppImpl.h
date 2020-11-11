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
	class GRAYCORE_LINK cAppImpl
		: public cSingletonStatic < cAppImpl > // use static theApp
	{
		//! @class Gray::cAppImpl
		//! Entry point for my implemented application.
		//! like (CWinApp for MFC) (maybe windowed or console)
		//! I am NOT a library/DLL. I am an application implementation. NOT the same as (or to be merged with) cAppState.
		//! Basic framework for my application I implement. Assume a static like cAppImpl theApp is defined some place.

	public:
		static const char* k_HelpText;

		const FILECHAR_t* m_pszAppName;		//!< Specifies the name of my application. (display friendly)
		TIMESYSD_t m_nMinTickTime;			//!< Minimum amount of time to spend in the OnTickApp() (mSec). cThreadId::SleepCurrent() if there is extra time.
		cAppState& m_State;					//!< Quick reference to cAppState singleton.
		bool m_bCloseSignal;				//!< Polite request to close the application. checked in Run() and OnTickApp()

	public:
		cAppImpl(const FILECHAR_t* pszAppName = nullptr);
		virtual ~cAppImpl();

		static inline HINSTANCE get_HInstance()
		{
			// Similar to MFC?
			return cAppState::get_HModule();
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
