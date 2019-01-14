//
//! @file CLogAppendConsole.h
//! specific log destinations/appenders
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CLogAppendConsole_H
#define _INC_CLogAppendConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CLogAppender.h"
#include "CArrayString.h"
#include "CSingletonPtr.h"

namespace Gray
{
	class GRAYCORE_LINK CLogAppendConsole : public CLogAppender, public CSingletonSmart<CLogAppendConsole>
	{
		//! @class Gray::CLogAppendConsole
		//! Forward debug statements to the console (CAppConsole) (if i have one)
		//! No filter and take default formatted string
	public:
		CLogAppendConsole();
		virtual ~CLogAppendConsole();

		virtual HRESULT WriteString(const LOGCHAR_t* pszMsg) override;

		static HRESULT GRAYCALL AddAppenderCheck(CLogNexus* pLogger = nullptr, bool bAttachElseAlloc = false);
		static bool GRAYCALL RemoveAppenderCheck(CLogNexus* pLogger, bool bOnlyIfParent);

		static HRESULT GRAYCALL ShowMessageBox(cString sMsg, UINT uFlags = 1);	// 1= MB_OKCANCEL
		static HRESULT GRAYCALL WaitForDebugger();

		CHEAPOBJECT_IMPL;
		IUNKNOWN_DISAMBIG(CSingletonSmart<CLogAppendConsole>);
	};

	class GRAYCORE_LINK CLogAppendTextArray : public CLogAppender, public CSmartBase
	{
		//! @class Gray::CLogAppendTextArray
		//! Just put the log messages in an array of strings in memory.
	public:
		CArrayString<LOGCHAR_t> m_aMsgs;
		const ITERATE_t m_iMax;				//!< Store this many messages.

	public:
		CLogAppendTextArray(ITERATE_t iMax = SHRT_MAX)
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
		IUNKNOWN_DISAMBIG(CSmartBase);
	};
}
#endif // _INC_CLogAppendConsole_H
