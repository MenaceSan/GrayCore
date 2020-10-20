//
//! @file CLogMgr.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CLogMgr.h"
#include "CLogEvent.h"
#include "CString.h"
#include "CCodeProfiler.h"
#include "CAppState.h"
#include "CStream.h"

namespace Gray
{
	TIMESECD_t CLogMgr::sm_TimePrevException; //!< doesn't actually matter what this value is at init time.

	//************************************************************************

	CLogSubject::CLogSubject(const char* pszSubject)
		: m_pszSubject(pszSubject)
	{
	}

	CLogSubject::~CLogSubject()
	{
	}

	HRESULT CLogSubject::addEvent(CLogEvent* pEvent) // virtual
	{
		//! Prefix the event with the subject.
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
		pEvent->m_pszSubject = m_pszSubject;	// categorize with this subject.
		return CLogMgr::I().addEvent(pEvent);
	}

	//************************************************************************

	CLogNexus::CLogNexus(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel)
		: m_LogFilter(uAttrMask, eLogLevel)
	{
	}

	CLogNexus::~CLogNexus()
	{
	}

	HRESULT CLogNexus::addEvent(CLogEvent* pEvent) // virtual
	{
		//! add a new log event and send it to all applicable Appenders.
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.

		CODEPROFILEFUNC();
		if (pEvent == nullptr)
			return E_POINTER;
		ASSERT(CLogMgr::isSingleCreated());
		if (!IsLogged(pEvent->get_LogAttrMask(), pEvent->get_LogLevel()))	// I don't care about these ?
		{
			return HRESULT_WIN32_C(ERROR_EMPTY); // no appenders care about this.
		}
		if (pEvent->m_sMsg.IsEmpty())
		{
			return E_INVALIDARG;
		}

		//! LOG_ATTR_FILTERED
		m_LogThrottle.m_nQtyLogLast++;

		int iUsed = 0;
		CStringL sTextFormatted; // produce a default final formatted string.
		CLogEventPtr pEventHolder(pEvent);	// one more reference on this.
		HRESULT hResAdd = S_OK;

		for (int i = 0;; i++)
		{
			CLogAppender* pAppender;
			{
				CThreadGuard lock(m_LockLog); // sync multiple threads.
				pAppender = m_aAppenders.GetAtCheck(i);
				if (pAppender == nullptr)
					break;
			}
			ASSERT(pAppender != nullptr);
			hResAdd = pAppender->addEvent(pEvent);
			if (hResAdd > 0)
			{
				iUsed++;	// handled.
				continue;
			}
			if (hResAdd < 0)
			{
				continue;	// another form of filtering. don't process this appender.
			}
			if (sTextFormatted.IsEmpty())
			{
				// build default formatted string only if needed. add FILE_EOL if desired.
				sTextFormatted = pEvent->get_FormattedDefault();
				if (sTextFormatted.IsEmpty())
				{
					return E_INVALIDARG;
				}
			}
			hResAdd = pAppender->WriteString(sTextFormatted);
			if (SUCCEEDED(hResAdd))
			{
				iUsed++;
			}
		}

#ifdef _DEBUG
		if (CAppState::isDebuggerPresent())
		{
			FlushLogs();
		}
#endif

		return (iUsed > 0) ? iUsed : hResAdd;	// we do any work?
	}

	HRESULT CLogNexus::FlushLogs() // virtual
	{
		CThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			CLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				break;
			pAppender->FlushLogs();
		}
		return S_OK;
	}

	bool CLogNexus::HasAppender(CLogAppender* pAppenderFind, bool bDescend) const
	{
		//! Does this CLogNexus contain this CLogAppender ?
		//! will descend into child CLogNexus as well.
		//! @arg pAppenderFind = what are we trying to find?

		if (pAppenderFind == nullptr)
			return false;
		CThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			const CLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				return false;
			if (pAppenderFind == pAppender)
				return true;
			if (bDescend)
			{
				const CLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr && pLogNexus->HasAppender(pAppenderFind, true))
					return true;
			}
		}
	}

	HRESULT CLogNexus::AddAppender(CLogAppender* pAppenderAdd)
	{
		//! Newest first.
		if (pAppenderAdd == nullptr)
			return E_POINTER;
		CThreadGuard lock(m_LockLog);
		m_aAppenders.AddHead(pAppenderAdd);
		return S_OK;
	}

	bool CLogNexus::RemoveAppender(CLogAppender* pAppenderRemove, bool bDescend)
	{
		//! will descend into child CLogNexus as well.
		if (pAppenderRemove == nullptr)
			return false;
		bool bRemoved = false;
		CThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			CLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				return bRemoved;
			if (pAppenderRemove == pAppender)
			{
				m_aAppenders.RemoveAt(i);
				i--;
				bRemoved = true;
			}
			else if (bDescend)
			{
				const CLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr)
				{
					bRemoved |= const_cast<CLogNexus*>(pLogNexus)->RemoveAppender(pAppenderRemove, true);
				}
			}
		}
	}

	CLogAppender* CLogNexus::FindAppenderType(const TYPEINFO_t& rType, bool bDescend) const
	{
		//! is there an appender of this type already installed?
		CThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			const CLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				break;
			if (typeid(*pAppender) == rType)	// already here.
				return const_cast<CLogAppender*>(pAppender);
			if (bDescend)
			{
				const CLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr)
				{
					pAppender = pLogNexus->FindAppenderType(rType, true);
					if (pAppender != nullptr)
						return const_cast<CLogAppender*>(pAppender);
				}
			}
		}
		return nullptr;
	}

	//************************************************************************

	CLogMgr::CLogMgr()
		: CSingleton<CLogMgr>(this, typeid(CLogMgr))
#ifdef _DEBUG
		, CLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLEV_INFO)
#else
		, CLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLEV_INFO)
#endif
	{
		//! ideally this is in the very first static initialize.
#ifdef _DEBUG
		if (CAppState::isDebuggerPresent())
		{
			CLogAppendDebug::AddAppenderCheck(this);	// send logs to the debugger.
		}
#endif
	}
	CLogMgr::~CLogMgr()
	{
	}

#ifdef _CPPUNWIND
	void CLogMgr::LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs)
	{
		//! An exception occurred. record it.
		//! if ( this == nullptr ) may be OK?

		CODEPROFILEFUNC();

		TIMESECD_t tNowSec = CTimeSys::GetTimeNow() / CTimeSys::k_FREQ;
		if (sm_TimePrevException == tNowSec)	// prevent message floods. 1 per sec.
			return;
		sm_TimePrevException = tNowSec;

		if (!CMem::IsValidApp(this))
		{
			// Nothing we can do about this?!
			ASSERT(0);
			return;
		}

		// Keep a record of what we catch.
		try
		{
			LOGCHAR_t szMsg[cExceptionHolder::k_MSG_MAX_SIZE];
			StrLen_t iLen;
			if (pEx == nullptr)
			{
				iLen = StrT::CopyLen(szMsg, "Unknown exception", STRMAX(szMsg));
			}
			else
			{
				pEx->GetErrorMessage(szMsg, STRMAX(szMsg));
				iLen = StrT::Len(szMsg);
			}

			iLen += StrT::CopyLen(szMsg + iLen, ", in ", STRMAX(szMsg) - iLen);
			if (vargs == nullptr)
			{
				iLen += StrT::CopyLen(szMsg + iLen, pszCatchContext, STRMAX(szMsg) - iLen);
			}
			else
			{
				iLen += StrT::vsprintfN(szMsg + iLen, STRMAX(szMsg) - iLen, pszCatchContext, vargs);
			}

			LOGLEV_TYPE eSeverity = (pEx == nullptr) ? LOGLEV_CRIT : (pEx->get_Severity());
			addEventS(LOG_ATTR_DEBUG, eSeverity, szMsg, "");
		}
		catch (...)
		{
			// Not much we can do about this.
		}
	}

	void _cdecl CLogMgr::LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...)
	{
		va_list vargs;
		va_start(vargs, pszCatchContext);
		LogExceptionV(pEx, pszCatchContext, vargs);
		va_end(vargs);
	}
#endif

	HRESULT CLogMgr::WriteString(const LOGCHAR_t* pszStr)	// virtual
	{
		this->addEventS(LOG_ATTR_PRINT, LOGLEV_INFO, pszStr, "");
		return S_OK;
	}
	HRESULT CLogMgr::WriteString(const wchar_t* pszStr) // virtual
	{
		this->addEventS(LOG_ATTR_PRINT, LOGLEV_INFO, pszStr, "");
		return S_OK;
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CLogMgr)
{
	UNITTEST_METHOD(CLogMgr)
	{
		CLogMgr& logmgr = CLogMgr::I();

		CSmartPtr<CLogAppendDebug> pLogDebug(new CLogAppendDebug);
		logmgr.AddAppender(pLogDebug);
		UNITTEST_TRUE(CLogAppendDebug::AddAppenderCheck(&logmgr) == S_FALSE);

		CLogAppender* pAppender1 = logmgr.FindAppenderType(typeid(CLogAppendDebug));
		UNITTEST_TRUE(pAppender1 != nullptr);	// already added

		// This should print twice.
		logmgr.addEventS(LOG_ATTR_DEBUG, LOGLEV_INFO, "Test Event to CLogAppendDebug * 2 (double logged event)", "");
		logmgr.RemoveAppender(pLogDebug);
	}
};
UNITTEST_REGISTER(CLogMgr, UNITTEST_LEVEL_Core);
#endif
