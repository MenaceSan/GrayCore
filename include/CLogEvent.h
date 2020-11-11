//
//! @file cLogEvent.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogEvent_H
#define _INC_cLogEvent_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogAppender.h"
#include "cTimeInt.h"

namespace Gray
{
	class GRAYCORE_LINK cLogEvent : public cLogEventParams, public cRefBase
	{
		//! @class Gray::cLogEvent
		//! Store a single event instance for asynchronous processing.

	public:
		const char* m_pszSubject;		//!< general subject matter tag.
		cStringL m_sContext;			//!< extra context info. format ? e.g. What script/class/etc name is this event from? 
		cStringL m_sMsg;				//!< message text
		TIMESEC_t m_time;				//!< cTimeInt. when did this happen? maybe not set until needed. ! isTimeValid()

	public:
		cLogEvent(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_0, LOGLEV_TYPE eLogLevel = LOGLEV_ANY, cStringL sMsg = "", cStringL sContext = "")
		: cLogEventParams(uAttrMask, eLogLevel)
		, m_pszSubject(nullptr)
		, m_sContext(sContext)
		, m_sMsg(sMsg)
		, m_time(0)
		{
		}
		~cLogEvent()
		{
		}

		//! take all my attributes and make a single string.
		cStringL get_FormattedDefault() const;
	};

	typedef cRefPtr<cLogEvent> cLogEventPtr;
}

#endif
