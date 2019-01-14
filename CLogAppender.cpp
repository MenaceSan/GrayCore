//
//! @file CLogAppender.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CLogAppender.h"
#include "CLogMgr.h"
#include "CLogEvent.h"
#include "CBits.h"
#include "CCodeProfiler.h"
#include "StrBuilder.h"

#ifdef UNDER_CE
#include <dbgapi.h>	// OutputDebugStringA
#endif

namespace Gray
{
	CStringL CLogEvent::get_FormattedDefault() const
	{
		//! Format the text for the event in the default way. adds FILE_EOL.

		LOGCHAR_t szTemp[StrT::k_LEN_MAX];	// assume this magic number is big enough.
		StrBuilder s(szTemp, STRMAX(szTemp));

		if (get_LogLevel() >= LOGLEV_WARN)
		{
			s.AddStr(CLogLevel::GetPrefixStr(get_LogLevel()));
		}
#ifdef _DEBUG
		if (get_LogLevel() >= LOGLEV_ERROR)
		{
			s.AddStr("!");
		}
#endif
		if (!m_sContext.IsEmpty())
		{
			s.AddStr(m_sContext);
		}

		s.AddStr(m_sMsg);
		StrLen_t iLen = m_sMsg.GetLength();
		ASSERT(iLen > 0);

		bool bHasCRLF = (m_sMsg[iLen - 1] == '\r' || m_sMsg[iLen - 1] == '\n');
		if (!CBits::HasMask(get_LogAttrMask(), (LOG_ATTR_MASK_t)LOG_ATTR_NOCRLF) && !bHasCRLF)
		{
			s.AddStr(FILE_EOL);
		}
		return s.get_Str();
	}

	//**************************************************************

	CLogThrottle::CLogThrottle()
		: m_fLogThrottle(2000.0f)
		, m_TimeLogLast(CTimeSys::k_CLEAR)
		, m_nQtyLogLast(0)
	{
	}

	CLogThrottle::~CLogThrottle()
	{
	}

	//************************************************************************

	HRESULT CLogProcessor::addEventS(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, CStringL sMsg, CStringL sContext /* = "" */) //
	{
		//! Dispatch the event to all matching appenders.
		//! ASSUME new line.
		//! @note This can be called by multiple threads!
		//! @note This could be called in odd interrupt context so don't use dynamic stuff
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.

		if (!IsLogged(uAttrMask, eLogLevel))
		{
			// Pre-Check if anyone cares before creating CLogEvent
			return HRESULT_WIN32_C(ERROR_EMPTY); // no appenders care about this.
		}

		CLogEventPtr pEvent(new CLogEvent(uAttrMask, eLogLevel, sMsg, sContext));
		return addEvent(pEvent);
	}

	HRESULT CLogProcessor::addEventV(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, const LOGCHAR_t* pszFormat, va_list vargs)	// , CStringL sContext /* = "" */
	{
		//! Add a log message (line) to the system in the sprintf() format (with arguments)
		//! ASSUME new line.
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
		CODEPROFILEFUNC();
		if (StrT::IsNullOrEmpty(pszFormat))
		{
			return E_INVALIDARG;
		}

		LOGCHAR_t szTemp[StrT::k_LEN_MAX];	// assume this magic number is big enough.
		StrLen_t iLen = StrT::vsprintfN(szTemp, STRMAX(szTemp), pszFormat, vargs);
		if (iLen <= 0)
		{
			return E_INVALIDARG;
		}

		return addEventS(uAttrMask, eLogLevel, szTemp, "");
	}

	//************************************************************************

	CLogAppender::CLogAppender()
	{
	}

	CLogAppender::~CLogAppender()
	{
		RemoveAppenderThis();
	}

	HRESULT CLogAppender::WriteString(const wchar_t* pszMsg) // virtual 
	{
		//! Support loggers that want to write unicode to the log.
		return WriteString(StrArg<LOGCHAR_t>(pszMsg));
	}

	bool CLogAppender::RemoveAppenderThis()
	{
		//! Remove myself from the list of valid appenders in CLogMgr.
		//! will descend into child CLogNexus as well.
		if (!CLogMgr::isSingleCreated())
		{
			return false;
		}
		return CLogMgr::I().RemoveAppender(this, true);
	}

	//************************************************************************

	CLogAppendDebug::CLogAppendDebug()
	{
	}
	CLogAppendDebug::~CLogAppendDebug() // virtual
	{
	}

	HRESULT CLogAppendDebug::WriteString(const LOGCHAR_t* pszText) // virtual
	{
		//! Do NOT assume new line.
		//! default OutputDebugString event if no other handler. (this == nullptr)
#ifdef _WIN32
		CThreadGuard threadguard(m_Lock);
#if 0 // def _DEBUG
		if (m_Lock.get_ThreadLockOwner() != CAppState::I().get_MainThreadId())
		{
			ASSERT(0);
		}
#endif
#ifdef UNDER_CE
		::OutputDebugStringW(StrArg<wchar_t>(pszText));
#else
		::OutputDebugStringA(pszText);
#endif
#endif
		return S_OK;
	}

	HRESULT GRAYCALL CLogAppendDebug::AddAppenderCheck(CLogNexus* pLogger) // static
	{
		//! Apps should call this in main() or in some static init.
		//! default logger = CLogMgr
		if (pLogger == nullptr)
		{
			pLogger = CLogMgr::get_Single();
		}
		if (pLogger->FindAppenderType(typeid(CLogAppendDebug)) != nullptr)
			return S_FALSE;
		pLogger->AddAppender(new CLogAppendDebug);
		return S_OK;
	}
}
