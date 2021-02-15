//
//! @file cLogMgr.h
//! Message / Event Log Macros. (Debug Mostly)
//! similar to Log4J (with appenders) formatters per appender to control actual format?
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogMgr_H
#define _INC_cLogMgr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogAppender.h"
#include "cDebugAssert.h"
#include "cTimeSys.h"
#include "IUnknown.h"
#include "cSingleton.h"
#include "cTimeUnits.h"
#include "cArrayRef.h"
#include "cArrayIUnk.h"
#include "cException.h"
#include "HResult.h"
#include "cUnitTestDecl.h"
#include "cTypeInfo.h"

// #define _DEBUG_FAST	// put debug in release mode optimized code.

namespace Gray
{
	class GRAYCORE_LINK cLogSubject : public cLogProcessor
	{
		//! @class Gray::cLogSubject
		//! A logger dedicated to a certain subject matter.
		//! declared statically for each subject we might want to log.
		//! equivalent to the Logger in Log4J
		//! all log messages should enter the log system here ideally. NOT through cLogMgr::I() directly.
		//! ideally subjects are hierarchical ALA Log4J
		//! e.g. "Root.Server.Clients.Login" so they can be filter via hierarchy wildcards.

	public:
		const char* m_pszSubject;	//!< static string. general subject matter tag.
	public:
		cLogSubject(const char* pszSubject);
		virtual ~cLogSubject();

		virtual HRESULT addEvent(cLogEvent* pEvent) override; // ILogProcessor
	};

	class GRAYCORE_LINK cLogNexus
		: public cLogProcessor	// can submit directly instead of using cLogSubject = default
	{
		//! @class Gray::cLogNexus
		//! A nexus for routing log messages. (may have sub appenders)
		//! Like Log4J
		//! Actual cLogEvent may be routed or filtered to multiple destinations/Appenders from here.
		//! Array of attached appenders to say where the logged events go.
		//! addEvent() is multi thread safe.

	public:
		cLogEventParams m_LogFilter;	//!< Union filter what goes out to ALL appenders
		cLogThrottle m_LogThrottle;		//!< Measure how fast messages are going.
		mutable cThreadLockCount m_LockLog;			//!< serialize multiple threads for m_aAppenders

	protected:
		cArrayIUnk<cLogAppender> m_aAppenders;		//!< where do the log messages go? child appenders.

	public:
		cLogNexus(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_ALL_MASK, LOGLEV_TYPE eLogLevel = LOGLEV_ANY);
		virtual ~cLogNexus() noexcept;

		const cLogNexus* get_ThisLogNexus() const override
		{
			//! Is this a log nexus or just a processor?
			return this;
		}
		bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel) const override // fast pre-check.
		{
			//! @note Check IsLogged(x) before generating the message! for speed
			return m_LogFilter.IsLogged(uAttrMask, eLogLevel);
		}
		HRESULT addEvent(cLogEvent* pEvent) override;
		HRESULT FlushLogs() override;

		// manage appenders
		cLogAppender* EnumAppender(ITERATE_t i)
		{
			return m_aAppenders.GetAtCheck(i);
		}
		const cLogAppender* EnumAppender(ITERATE_t i) const
		{
			return m_aAppenders.GetAtCheck(i);
		}
		bool HasAppender(cLogAppender* pAppender, bool bDescend = false) const;
		HRESULT AddAppender(cLogAppender* pAppender);
		bool RemoveAppender(cLogAppender* pAppender, bool bDescend = false);
		cLogAppender* FindAppenderType(const TYPEINFO_t& rType, bool bDescend = false) const;

		bool RemoveAppenderType(const TYPEINFO_t& rType, bool bDescend=true)
		{
			return RemoveAppender(FindAppenderType(rType, bDescend), bDescend);
		}
	};

	class GRAYCORE_LINK cLogMgr
		: public cSingleton < cLogMgr >
		, public cLogNexus
	{
		//! @class Gray::cLogMgr
		//! The root log message nexus. take in all log messages and decide where they go.
		//! Only one default singleton log manager per system.
		friend class cSingleton < cLogMgr >;

	private:
		static TIMESECD_t sm_TimePrevException;	//!< throttle/don't flood log with exceptions().

	protected:
		cLogMgr();
		virtual ~cLogMgr();

	public:

#ifdef _CPPUNWIND
		// Logging of cException.
		// has no arguments
		void LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs);
		void _cdecl LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...);
#endif

		//! cStreamOutput - for raw dumping of text into the log system.
		virtual HRESULT WriteString(const LOGCHAR_t* pszStr) override;
		virtual HRESULT WriteString(const wchar_t* pszStr) override;

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(cLogMgr);
	};

	//***********************************************************************************
	// Stuff that should stay in release mode.
#define LOGF(_x_)			::Gray::cLogMgr::I().addEventF _x_	// needs attributes and log level.

	// We usually leave this stuff in for release code ?
#ifndef DEBUG_ERR
#define DEBUG_ERR(_x_)		::Gray::cLogMgr::I().addDebugErrorF _x_
#define DEBUG_WARN(_x_)		::Gray::cLogMgr::I().addDebugWarnF _x_
#endif

	// Log messages that compile out when not wanted. only in debug mode.
#if ! defined(DEBUG_MSG) && ! defined(USE_DEBUG_LOG) && ( defined(_DEBUG) || defined(_DEBUG_FAST))
#define USE_DEBUG_LOG
#endif
#ifdef USE_DEBUG_LOG
#define DEBUG_LOGF(_x_)		::Gray::cLogMgr::I().addEventF _x_	// needs attributes and log level.
#define DEBUG_MSG(_x_)		::Gray::cLogMgr::I().addDebugInfoF _x_
#define DEBUG_TRACE(_x_)	::Gray::cLogMgr::I().addDebugTraceF _x_
#else
#define DEBUG_LOGF(_x_)		__noop // compiled out.
#define DEBUG_MSG(_x_)		__noop // compiled out.
#define DEBUG_TRACE(_x_)	__noop // this allows all the variable args to be compiled out
#endif

#ifdef _CPPUNWIND

#define GEXCEP_CATCH_LOG(desc) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )\
	{\
	::Gray::cExceptionHolder exh(ex);\
	::Gray::cLogMgr::I().LogExceptionV(&exh,desc,nullptr);\
	}
	// has 1 arg for context
#define GEXCEP_CATCH_LOG1(desc,arg) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )\
	{\
	::Gray::cExceptionHolder exh(ex);\
	::Gray::cLogMgr::I().LogExceptionF(&exh,desc,arg);\
	}
#else
#define GEXCEP_CATCH_LOG(desc) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )
#define GEXCEP_CATCH_LOG1(desc,arg) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )
#endif
}
#endif	// _INC_cLogMgr_H
