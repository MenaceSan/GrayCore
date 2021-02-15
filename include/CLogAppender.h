//
//! @file cLogAppender.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogAppender_H
#define _INC_cLogAppender_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogLevel.h"
#include "cRefPtr.h"
#include "cString.h"
#include "cStream.h"
#include "cThreadLock.h"

namespace Gray
{
	class cLogEvent;

	typedef cStringT<LOGCHAR_t> cStringL;	//!< Log string.
#define LOGERR(hRes) LOGSTR(cStringL::GetErrorString(hRes))		//!< Used to supply "ERR='%s'"

	enum LOG_ATTR_TYPE_
	{
		//! @enum Gray::LOG_ATTR_TYPE_
		//! Log event attributes. special controls for odd logged events.
		//! bitmask so event can have multi attributes

		LOG_ATTR_0 = 0,

		//! Adding new attributes (low values) here can be used to add more categories.
		//!  but use cLogSubject for that for much more flexibility.

		LOG_ATTR_INIT = 0x00100000,	//!< startup/exit stuff. category.
		LOG_ATTR_SCRIPT = 0x00200000,	//!< from some sort of scripted code exec. category.
		LOG_ATTR_NET = 0x00400000,	//!< from network activity. category. (watch out that this is not also sent on the network as it might cause feedback)
		LOG_ATTR_ODD = 0x01000000,	//!< This is odd/unusual behavior for client. category. Probably not a code problem but a user/security/integrity problem.
		LOG_ATTR_DEBUG = 0x02000000,	//!< Unclassified debug stuff. category.

		// Bit Flags to control appenders
		LOG_ATTR_PRINT = 0x04000000,	//!< The equiv of a printf() to console.
		LOG_ATTR_PRINTBACK = 0x08000000,	//!< Back up over the last LOG_ATTR_PRINT. append to the last.
		LOG_ATTR_TEMP = 0x10000000,	//!< Real time status (don't bother to log permanently)
		LOG_ATTR_INTERNAL = 0x20000000,	//!< Do not echo this message as it may relate to my own logging internals (i.e.feedback loop)
		LOG_ATTR_NOCRLF = 0x40000000,	//!< Don't add a FILE_EOL (CR NL) to the end of this string. this is a partial message.
		LOG_ATTR_FILTERED = 0x80000000,	//!< Filter already checked so don't check it again.

		// LOG_ATTR_QUESTION = 0x123123,	//!< This is a question that we hope will get an answer through some async callback. cLogAppendConsole

		LOG_ATTR_CUST_MASK = 0x000FFFFF,
		LOG_ATTR_BASE_MASK = 0xFFF00000,
		LOG_ATTR_ALL_MASK = 0xFFFFFFFF
	};
	typedef UINT32 LOG_ATTR_MASK_t;	// mask of LOG_ATTR_TYPE_

#if 0
	typedef UINT32 LOG_FIELD_t;
	enum LOG_FIELD_MASK_
	{
		LOG_FIELD_MSG,		//!< The actual message text part of the log event.
		LOG_FIELD_TIME,		//!< The time stamp part of a message.
		LOG_FIELD_LEVEL,	//!< The critical level message prefix. LOGLEV_TYPE
		LOG_FIELD_CONTEXT,	//!< The context label. (what script/source is this from)
	};
#endif

	//***********************************************************************************

	class GRAYCORE_LINK cLogEventParams
	{
		//! @class Gray::cLogEventParams
		//! Filterable Parameters associated with a particular log event instance.

	protected:
		LOG_ATTR_MASK_t m_uAttrMask;		//!< Special attributes for the event. (regardless of level) similar to pszSubject?
		LOGLEV_TYPE m_eLogLevel;			//!< Min Importance level to see. 0 = LOGLEV_ANY = not important.

	public:
		cLogEventParams(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_0, LOGLEV_TYPE eLogLevel = LOGLEV_TRACE) noexcept
			: m_uAttrMask(uAttrMask)		// Level of log detail messages. IsLogMsg()
			, m_eLogLevel(eLogLevel)		// Importance level.
		{
		}

		LOG_ATTR_MASK_t get_LogAttrMask() const noexcept
		{
			return m_uAttrMask;
		}
		void put_LogAttrMask(LOG_ATTR_MASK_t uAttrMask) noexcept
		{
			//! What types of info do we want to filter for.
			m_uAttrMask = uAttrMask;
		}
		bool IsLogAttrMask(LOG_ATTR_MASK_t uAttrMask) const noexcept
		{
			return (m_uAttrMask & uAttrMask) ? true : false ;
		}

		LOGLEV_TYPE get_LogLevel() const noexcept
		{
			//! Min level to show.
			return m_eLogLevel;
		}
		void put_LogLevel(LOGLEV_TYPE eLogLevel) noexcept
		{
			//! What level of importance do we want to filter for.
			m_eLogLevel = eLogLevel;
		}
		bool IsLoggedLevel(LOGLEV_TYPE eLogLevel) const noexcept
		{
			//! level = LOGLEV_INFO (higher is more important
			return eLogLevel >= m_eLogLevel ;
		}

		bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel) const noexcept
		{
			// Would this message be logged?
			if (!IsLoggedLevel(eLogLevel))
				return false;
			if (uAttrMask != 0 && !IsLogAttrMask(uAttrMask))
				return false;
			return true;
		}
	};

	class GRAYCORE_LINK cLogThrottle
	{
		//! @class Gray::cLogThrottle
		//! Parameters for time throttle of log messages. Queue messages up if they are coming too fast.

	public:
		// throttle.
		float m_fLogThrottle;				//!< how fast sent to me? messages/sec
		mutable TIMESYS_t m_TimeLogLast;	//!< Last time period for throttling (1 sec).
		mutable UINT m_nQtyLogLast;			//!< Qty of messages since m_TimeLogLast.

	public:
		cLogThrottle();
		~cLogThrottle();

		float get_LogThrottle() const noexcept
		{
			//! messages/sec
			return m_fLogThrottle;
		}
	};

	// CLogFormat = what should each event line contain from cLogEvent?

	//***********************************************************************

	DECLARE_INTERFACE(ILogProcessor)
	{
		//! @interface Gray::ILogProcessor
		//! All events funnel through addEvent().
		IGNORE_WARN_INTERFACE(ILogProcessor);
		virtual bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel) const = 0; // fast pre-check. can call before building message.
		virtual HRESULT addEvent(cLogEvent* pEvent) = 0;
	};

	class cLogNexus;

	class GRAYCORE_LINK cLogProcessor
		: public ILogProcessor
		, public cStreamOutput	// for WriteString raw dump messages into the system.
	{
		//! @class Gray::cLogProcessor
		//! Build/submit a log message to be submitted to the log system.

	public:
		virtual ~cLogProcessor()
		{
		}
		virtual const cLogNexus* get_ThisLogNexus() const
		{
			//! Is this a cLogNexus or just a cLogProcessor? like dynamic_cast<>
			return nullptr;
		}
		bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel) const override // fast pre-check.
		{
			//! would this message be logged? should i bother building it ? fast.
			UNREFERENCED_PARAMETER(uAttrMask);
			UNREFERENCED_PARAMETER(eLogLevel);
			return true;
		}
		virtual HRESULT FlushLogs()
		{
			//! Override this to flush logs for this processor.
			return S_OK;
		}

		// Helpers.
		HRESULT addEventS(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, cStringL sMsg, cStringL sContext="");
		HRESULT addEventV(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, const LOGCHAR_t* pszFormat, va_list vargs);

		HRESULT _cdecl addEventF(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel, const LOGCHAR_t* pszFormat, ...)
		{
			//! @arg uAttrMask = LOG_ATTR_DEBUG|LOG_ATTR_INTERNAL
			//! @arg eLogLevel = LOGLEV_TRACE, LOGLEV_ERROR
			//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(uAttrMask, eLogLevel, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}

		HRESULT _cdecl addInfoF(const LOGCHAR_t* pszFormat, ...)
		{
			//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(LOG_ATTR_0, LOGLEV_INFO, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}

		HRESULT _cdecl addDebugErrorF(const LOGCHAR_t* pszFormat, ...)
		{
			//! Add message with LOG_ATTR_DEBUG
			//! @return <0 = failed, 0=not processed by anyone, # = number of processors.
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLEV_ERROR, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}
		HRESULT _cdecl addDebugWarnF(const LOGCHAR_t* pszFormat, ...)
		{
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLEV_WARN, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}
		HRESULT _cdecl addDebugInfoF(const LOGCHAR_t* pszFormat, ...)
		{
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLEV_INFO, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}
		HRESULT _cdecl addDebugTraceF(const LOGCHAR_t* pszFormat, ...)
		{
			va_list vargs;
			va_start(vargs, pszFormat);
			const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLEV_TRACE, pszFormat, vargs);
			va_end(vargs);
			return(hRes);
		}
	};

	class GRAYCORE_LINK cLogAppender : public IUnknown, public cLogProcessor
	{
		//! @class Gray::cLogAppender
		//! Base class for the destination for a log message.
		//! Messages can be routed to several different places depending on filtering, app config, etc.
		//! overload these to implement which messages go here.
		friend class cLogNexus;
	protected:
		HRESULT WriteString(const LOGCHAR_t* pszMsg) override
		{
			//! override this. should never call this directly. for optimizing use of get_FormattedDefault()
			//! Do not assume FILE_EOL.
			UNREFERENCED_PARAMETER(pszMsg);
			ASSERT(0);
			return E_NOTIMPL;
		}

		HRESULT WriteString(const wchar_t* pszMsg) override;

	public:
		cLogAppender();
		virtual ~cLogAppender();

		//! Remove myself from the list of valid appenders.
		bool RemoveAppenderThis();

		HRESULT addEvent(cLogEvent* pEvent) override
		{
			//! Push the message where it is supposed to go.
			//! ILogProcessor
			//! @return
			//!  > 0 = i handled this.
			//!  0 = just pass the default string to WriteString(); No-one handled this.
			//!  <0 = failed = don't process this appender anymore.
			UNREFERENCED_PARAMETER(pEvent);
			return 0;	// just pass the default string to WriteString() below;
		}
	};

	class GRAYCORE_LINK cLogAppendDebug : public cLogAppender, public cRefBase
	{
		//! @class Gray::cLogAppendDebug
		//! Send logged messages out to the debug system. OutputDebugString()
		//! No filter and take default formatted string

	private:
		mutable cThreadLockCount m_Lock;	// prevent multi thread mixing of messages.
	public:
		cLogAppendDebug();
		virtual ~cLogAppendDebug();

		static HRESULT GRAYCALL AddAppenderCheck(cLogNexus* pLogger = nullptr);
		HRESULT WriteString(const LOGCHAR_t* pszMsg) override;

		IUNKNOWN_DISAMBIG(cRefBase);
	};

	class GRAYCORE_LINK cLogAppendCache : public cLogAppender, public cRefBase
	{
		//! @class Gray::CLogAppendCache
		//! @todo Append (or cache) detailed messages here and hold them until some error triggers them.
		//! Once some error triggers, then we can emit all these detail messages to some file for processing.
		//! If no trigger occurs in time then trash these messages.

		TIMESYS_t m_nCacheHold;	// How long to hold messages.

	};
}

#endif
