//
//! @file CLogEvent.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CLogEvent_H
#define _INC_CLogEvent_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CLogAppender.h"
#include "CTimeInt.h"

namespace Gray
{
	class GRAYCORE_LINK CLogEvent : public CLogEventParams, public CSmartBase
	{
		//! @class Gray::CLogEvent
		//! Store a single event instance for asynchronous processing.

	public:
		const char* m_pszSubject;		//!< general subject matter tag.
		CStringL m_sContext;			//!< extra context info. format ? e.g. What script/class/etc name is this event from? 
		CStringL m_sMsg;				//!< message text
		TIMESEC_t m_time;				//!< CTimeInt. when did this happen? maybe not set until needed. ! isTimeValid()

	public:
		CLogEvent(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_0, LOGLEV_TYPE eLogLevel = LOGLEV_ANY, CStringL sMsg = "", CStringL sContext = "")
		: CLogEventParams(uAttrMask, eLogLevel)
		, m_pszSubject(nullptr)
		, m_sContext(sContext)
		, m_sMsg(sMsg)
		, m_time(0)
		{
		}
		~CLogEvent()
		{
		}

		//! take all my attributes and make a single string.
		CStringL get_FormattedDefault() const;
	};

	typedef CSmartPtr<CLogEvent> CLogEventPtr;
}

#endif
