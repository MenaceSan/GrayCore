//
//! @file cLogMgr.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cLogMgr.h"
#include "cLogEvent.h"
#include "cString.h"
#include "cCodeProfiler.h"
#include "cAppState.h"
#include "cStream.h"

namespace Gray
{
	TIMESECD_t cLogMgr::sm_TimePrevException; //!< doesn't actually matter what this value is at init time.

	//************************************************************************

	cLogNexus::cLogNexus(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel)
		: m_LogFilter(uAttrMask, eLogLevel)
	{
	}

	cLogNexus::~cLogNexus() noexcept
	{
	}

	HRESULT cLogNexus::addEvent(cLogEvent* pEvent) noexcept // virtual
	{
		//! add a new log event and send/route it to all applicable Appenders.
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.

		CODEPROFILEFUNC();
		if (pEvent == nullptr)
			return E_POINTER;
		if (!cLogMgr::isSingleCreated())
		{
			DEBUG_CHECK(cLogMgr::isSingleCreated());
			return HRESULT_WIN32_C(ERROR_EMPTY);
		}
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
		cStringL sTextFormatted; // produce a default final formatted string.
		cLogEventPtr pEventHolder(pEvent);	// one more reference on this.
		HRESULT hResAdd = S_OK;

		for (int i = 0;; i++)
		{
			cLogAppender* pAppender;
			{
				cThreadGuard lock(m_LockLog); // sync multiple threads.
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
		if (cAppState::isDebuggerPresent())
		{
			FlushLogs();
		}
#endif

		return (iUsed > 0) ? iUsed : hResAdd;	// we do any work?
	}

	HRESULT cLogNexus::FlushLogs() // virtual
	{
		cThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			cLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				break;
			pAppender->FlushLogs();
		}
		return S_OK;
	}

	bool cLogNexus::HasAppender(cLogAppender* pAppenderFind, bool bDescend) const
	{
		//! Does this cLogNexus contain this cLogAppender ?
		//! will descend into child cLogNexus as well.
		//! @arg pAppenderFind = what are we trying to find?

		if (pAppenderFind == nullptr)
			return false;
		cThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			const cLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				return false;
			if (pAppenderFind == pAppender)
				return true;
			if (bDescend)
			{
				const cLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr && pLogNexus->HasAppender(pAppenderFind, true))
					return true;
			}
		}
	}

	HRESULT cLogNexus::AddAppender(cLogAppender* pAppenderAdd)
	{
		//! Newest first.
		if (pAppenderAdd == nullptr)
			return E_POINTER;
		if (HasAppender(pAppenderAdd, true))	// no dupes.
			return S_FALSE;
		cThreadGuard lock(m_LockLog);
		m_aAppenders.AddHead(pAppenderAdd);
		return S_OK;
	}

	bool cLogNexus::RemoveAppender(cLogAppender* pAppenderRemove, bool bDescend)
	{
		//! will descend into child cLogNexus as well.
		if (pAppenderRemove == nullptr)
			return false;
		bool bRemoved = false;
		cThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			cLogAppender* pAppender = EnumAppender(i);
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
				const cLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr)
				{
					bRemoved |= const_cast<cLogNexus*>(pLogNexus)->RemoveAppender(pAppenderRemove, true);
				}
			}
		}
	}

	cLogAppender* cLogNexus::FindAppenderType(const TYPEINFO_t& rType, bool bDescend) const
	{
		//! is there an appender of this type already installed?
		cThreadGuard lock(m_LockLog);
		for (ITERATE_t i = 0;; i++)
		{
			const cLogAppender* pAppender = EnumAppender(i);
			if (pAppender == nullptr)
				break;
			if (typeid(*pAppender) == rType)	// already here.
				return const_cast<cLogAppender*>(pAppender);
			if (bDescend)
			{
				const cLogNexus* pLogNexus = pAppender->get_ThisLogNexus();
				if (pLogNexus != nullptr)
				{
					pAppender = pLogNexus->FindAppenderType(rType, true);
					if (pAppender != nullptr)
						return const_cast<cLogAppender*>(pAppender);
				}
			}
		}
		return nullptr;
	}

	//************************************************************************

	cLogMgr::cLogMgr()
		: cSingleton<cLogMgr>(this, typeid(cLogMgr))
#ifdef _DEBUG
		, cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLEV_INFO)
#else
		, cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLEV_INFO)
#endif
	{
		//! ideally this is in the very first static initialize.
#ifdef _DEBUG
		if (cAppState::isDebuggerPresent())
		{
			cLogAppendDebug::AddAppenderCheck(this);	// send logs to the debugger.
		}
#endif
	}
	cLogMgr::~cLogMgr()
	{
	}

#ifdef _CPPUNWIND
	void cLogMgr::LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs) noexcept
	{
		//! An exception occurred. record it.
		//! if ( this == nullptr ) may be OK?

		CODEPROFILEFUNC();

		TIMESECD_t tNowSec = cTimeSys::GetTimeNow() / cTimeSys::k_FREQ;
		if (sm_TimePrevException == tNowSec)	// prevent message floods. 1 per sec.
			return;
		sm_TimePrevException = tNowSec;

		if (!cMem::IsValidApp(this))
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

	void _cdecl cLogMgr::LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...) noexcept
	{
		va_list vargs;
		va_start(vargs, pszCatchContext);
		LogExceptionV(pEx, pszCatchContext, vargs);
		va_end(vargs);
	}
#endif

	HRESULT cLogMgr::WriteString(const LOGCHAR_t* pszStr)	// virtual
	{
		this->addEventS(LOG_ATTR_PRINT, LOGLEV_INFO, pszStr, "");
		return S_OK;
	}
	HRESULT cLogMgr::WriteString(const wchar_t* pszStr) // virtual
	{
		this->addEventS(LOG_ATTR_PRINT, LOGLEV_INFO, pszStr, "");
		return S_OK;
	}


	//************************************************************************

	cLogSubject::cLogSubject(const char* pszSubject)
		: m_pszSubject(pszSubject)
	{
	}

	cLogSubject::~cLogSubject()
	{
	}

	HRESULT cLogSubject::addEvent(cLogEvent* pEvent) noexcept // virtual
	{
		//! Prefix the event with the subject.
		//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
		if (pEvent == nullptr)
			return E_POINTER;
		pEvent->m_pszSubject = m_pszSubject;	// categorize with this subject.
		return cLogMgr::I().addEvent(pEvent);
	}

}
 