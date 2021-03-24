//
//! @file cLogAppender.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cLogAppender.h"
#include "cLogMgr.h"
#include "cLogEvent.h"
#include "cBits.h"
#include "cCodeProfiler.h"
#include "StrBuilder.h"

#ifdef UNDER_CE
#include <dbgapi.h>	// OutputDebugStringA
#endif

namespace Gray
{
	cStringL cLogEvent::get_FormattedDefault() const
	{
		//! Format the text for the event in the default way. adds FILE_EOL.

		LOGCHAR_t szTemp[StrT::k_LEN_MAX];	// assume this magic number is big enough. Logging is weird and special so dont use dynamic memory.
		StrBuilder<LOGCHAR_t> s(szTemp, STRMAX(szTemp));

		if (get_LogLevel() >= LOGLEV_WARN)
		{
			s.AddStr(cLogLevel::GetPrefixStr(get_LogLevel()));
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
		if (!cBits::HasMask(get_LogAttrMask(), (LOG_ATTR_MASK_t)LOG_ATTR_NOCRLF) && !bHasCRLF)
		{
			s.AddStr(FILE_EOL);
		}
		return s.get_Str();
	}

	//**************************************************************

	cLogThrottle::cLogThrottle()
		: m_fLogThrottle(2000.0f)
		, m_TimeLogLast(cTimeSys::k_CLEAR)
		, m_nQtyLogLast(0)
	{
	}

	cLogThrottle::~cLogThrottle()
	{
	}

	//************************************************************************

	HRESULT cLogProcessor::addEventS(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, cStringL sMsg, cStringL sContext /* = "" */) //
	{
		//! Dispatch the event to all matching appenders.
		//! ASSUME new line.
		//! @note This can be called by multiple threads!
		//! @note This could be called in odd interrupt context so don't use dynamic stuff
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.

		if (!IsLogged(uAttrMask, eLogLevel))
		{
			// Pre-Check if anyone cares before creating cLogEvent
			return HRESULT_WIN32_C(ERROR_EMPTY); // no appenders care about this.
		}

		cLogEventPtr pEvent(new cLogEvent(uAttrMask, eLogLevel, sMsg, sContext));
		return addEvent(pEvent);
	}

	HRESULT cLogProcessor::addEventV(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, const LOGCHAR_t* pszFormat, va_list vargs)	// , cStringL sContext /* = "" */
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

	cLogAppender::cLogAppender()
	{
	}

	cLogAppender::~cLogAppender()
	{
		RemoveAppenderThis();
	}

	HRESULT cLogAppender::WriteString(const wchar_t* pszMsg) // virtual 
	{
		//! Support loggers that want to write unicode to the log.
		return WriteString(StrArg<LOGCHAR_t>(pszMsg));
	}

	bool cLogAppender::RemoveAppenderThis()
	{
		//! Remove myself from the list of valid appenders in cLogMgr.
		//! will descend into child cLogNexus as well.
		//! called on destruct.

		if (!cLogMgr::isSingleCreated())
		{
			return false;
		}
		return cLogMgr::I().RemoveAppender(this, true);
	}

	//************************************************************************

	cLogAppendDebug::cLogAppendDebug()
	{
	}
	cLogAppendDebug::~cLogAppendDebug() // virtual
	{
	}

	HRESULT cLogAppendDebug::WriteString(const LOGCHAR_t* pszText) // virtual
	{
		//! Do NOT assume new line.
		//! default OutputDebugString event if no other handler. (this == nullptr)
#ifdef _WIN32
		cThreadGuard threadguard(m_Lock);
#if 0 // def _DEBUG
		if (m_Lock.get_ThreadLockOwner() != cAppState::I().get_MainThreadId())
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

	HRESULT GRAYCALL cLogAppendDebug::AddAppenderCheck(cLogNexus* pLogger) // static
	{
		//! Apps should call this in main() or in some static init.
		//! default logger = cLogMgr
		if (pLogger == nullptr)
		{
			pLogger = cLogMgr::get_Single();
		}
		if (pLogger->FindAppenderType(typeid(cLogAppendDebug)) != nullptr)
			return S_FALSE;
		pLogger->AddAppender(new cLogAppendDebug);
		return S_OK;
	}
}
