//
//! @file cLogAppendConsole.h
//! specific log destinations/appenders
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogAppendConsole_H
#define _INC_cLogAppendConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogAppender.h"
#include "cArrayString.h"
#include "cSingletonPtr.h"

#if !defined(UNDER_CE) && USE_CRT

namespace Gray
{
	class GRAYCORE_LINK cLogAppendConsole : public cLogAppender, public cSingletonSmart<cLogAppendConsole>
	{
		//! @class Gray::cLogAppendConsole
		//! Forward debug statements to the console (cAppConsole) (if i have one)
		//! No filter and take default formatted string
	public:
		cLogAppendConsole();
		virtual ~cLogAppendConsole();

		virtual HRESULT WriteString(const LOGCHAR_t* pszMsg) override;

		static HRESULT GRAYCALL AddAppenderCheck(cLogNexus* pLogger = nullptr, bool bAttachElseAlloc = false);
		static bool GRAYCALL RemoveAppenderCheck(cLogNexus* pLogger, bool bOnlyIfParent);

		static HRESULT GRAYCALL ShowMessageBox(cString sMsg, UINT uFlags = 1);	// 1= MB_OKCANCEL
		static HRESULT GRAYCALL WaitForDebugger();

		CHEAPOBJECT_IMPL;
		IUNKNOWN_DISAMBIG(cSingletonSmart<cLogAppendConsole>);
	};

	class GRAYCORE_LINK cLogAppendTextArray : public cLogAppender, public cRefBase
	{
		//! @class Gray::cLogAppendTextArray
		//! Just put the log messages in an array of strings in memory.
	public:
		cArrayString<LOGCHAR_t> m_aMsgs;
		const ITERATE_t m_iMax;				//!< Store this many messages.

	public:
		cLogAppendTextArray(ITERATE_t iMax = SHRT_MAX)
			: m_iMax(iMax)
		{
		}
		virtual HRESULT WriteString(const LOGCHAR_t* pszMsg) override
		{
			if (StrT::IsNullOrEmpty(pszMsg))
				return 0;
			if (m_aMsgs.GetSize() >= m_iMax)
				return 0;
			m_aMsgs.Add(pszMsg);
			return 1;
		}
		IUNKNOWN_DISAMBIG(cRefBase);
	};
}
#endif
#endif // _INC_cLogAppendConsole_H
