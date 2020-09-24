//
//! @file CLogMgr.h
//! Message / Event Log Macros. (Debug Mostly)
//! similar to Log4J (with appenders) formatters per appender to control actual format?
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CLogMgr_H
#define _INC_CLogMgr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CLogAppender.h"
#include "CDebugAssert.h"
#include "CTimeSys.h"
#include "IUnknown.h"
#include "CSingleton.h"
#include "CTimeUnits.h"
#include "CArraySmart.h"
#include "CArrayIUnk.h"
#include "CException.h"
#include "HResult.h"
#include "CUnitTestDecl.h"
#include "CTypeInfo.h"

// #define _DEBUG_FAST	// put debug in release mode optimized code.
UNITTEST_PREDEF(CLogMgr)

namespace Gray
{
	class GRAYCORE_LINK CLogSubject : public CLogProcessor
	{
		//! @class Gray::CLogSubject
		//! A logger dedicated to a certain subject matter.
		//! declared statically for each subject we might want to log.
		//! equivalent to the Logger in Log4J
		//! all log messages should enter the log system here ideally. NOT through CLogMgr::I() directly.
		//! ideally subjects are hierarchical ALA Log4J
		//! e.g. "Root.Server.Clients.Login" so they can be filter via hierarchy wildcards.

	public:
		const char* m_pszSubject;	//!< static string. general subject matter tag.
	public:
		CLogSubject(const char* pszSubject);
		virtual ~CLogSubject();

		virtual HRESULT addEvent(CLogEvent* pEvent) override; // ILogProcessor
	};

	class GRAYCORE_LINK CLogNexus
		: public CLogProcessor	// can submit directly instead of using CLogSubject = default
	{
		//! @class Gray::CLogNexus
		//! A nexus for routing log messages. (may have sub appenders)
		//! Like Log4J
		//! Actual CLogEvent may be routed or filtered to multiple destinations/Appenders from here.
		//! Array of attached appenders to say where the logged events go.
		//! addEvent() is multi thread safe.

	public:
		CLogEventParams m_LogFilter;	//!< Union filter what goes out to ALL appenders
		CLogThrottle m_LogThrottle;		//!< Measure how fast messages are going.
		mutable CThreadLockCount m_LockLog;			//!< serialize multiple threads for m_aAppenders

	protected:
		CArrayIUnk<CLogAppender> m_aAppenders;		//!< where do the log messages go? child appenders.

	public:
		CLogNexus(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_ALL_MASK, LOGLEV_TYPE eLogLevel = LOGLEV_ANY);
		virtual ~CLogNexus();

		virtual const CLogNexus* get_ThisLogNexus() const override
		{
			//! Is this a log nexus or just a processor?
			return this;
		}
		virtual bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLEV_TYPE eLogLevel) const override // fast pre-check.
		{
			//! @note Check IsLogged(x) before generating the message! for speed
			return m_LogFilter.IsLogged(uAttrMask, eLogLevel);
		}
		virtual HRESULT addEvent(CLogEvent* pEvent) override;
		virtual HRESULT FlushLogs() override;

		// manage appenders
		CLogAppender* EnumAppender(ITERATE_t i)
		{
			return m_aAppenders.GetAtCheck(i);
		}
		const CLogAppender* EnumAppender(ITERATE_t i) const
		{
			return m_aAppenders.GetAtCheck(i);
		}
		bool HasAppender(CLogAppender* pAppender, bool bDescend = false) const;
		HRESULT AddAppender(CLogAppender* pAppender);
		bool RemoveAppender(CLogAppender* pAppender, bool bDescend = false);
		CLogAppender* FindAppenderType(const TYPEINFO_t& rType, bool bDescend = false) const;

		bool RemoveAppenderType(const TYPEINFO_t& rType, bool bDescend=true)
		{
			return RemoveAppender(FindAppenderType(rType, bDescend), bDescend);
		}
	};

	class GRAYCORE_LINK CLogMgr
		: public CSingleton < CLogMgr >
		, public CLogNexus
	{
		//! @class Gray::CLogMgr
		//! The root log message nexus. take in all log messages and decide where they go.
		//! Only one default singleton log manager per system.
		friend class CSingleton < CLogMgr >;

	private:
		static TIMESECD_t sm_TimePrevException;	//!< throttle/don't flood log with exceptions().

	protected:
		CLogMgr();
		virtual ~CLogMgr();

	public:

#ifdef _CPPUNWIND
		// Logging of cException.
		// has no arguments
		void LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs);
		void _cdecl LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...);
#endif

		//! CStreamOutput - for raw dumping of text into the log system.
		virtual HRESULT WriteString(const LOGCHAR_t* pszStr) override;
		virtual HRESULT WriteString(const wchar_t* pszStr) override;

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(CLogMgr);
	};

	//***********************************************************************************
	// Stuff that should stay in release mode.
#define LOGF(_x_)			::Gray::CLogMgr::I().addEventF _x_	// needs attributes and log level.

	// We usually leave this stuff in for release code ?
#ifndef DEBUG_ERR
#define DEBUG_ERR(_x_)		::Gray::CLogMgr::I().addDebugErrorF _x_
#define DEBUG_WARN(_x_)		::Gray::CLogMgr::I().addDebugWarnF _x_
#endif

	// Log messages that compile out when not wanted. only in debug mode.
#if ! defined(DEBUG_MSG) && ! defined(USE_DEBUG_LOG) && ( defined(_DEBUG) || defined(_DEBUG_FAST))
#define USE_DEBUG_LOG
#endif
#ifdef USE_DEBUG_LOG
#define DEBUG_LOGF(_x_)		::Gray::CLogMgr::I().addEventF _x_	// needs attributes and log level.
#define DEBUG_MSG(_x_)		::Gray::CLogMgr::I().addDebugInfoF _x_
#define DEBUG_TRACE(_x_)	::Gray::CLogMgr::I().addDebugTraceF _x_
#else
#define DEBUG_LOGF(_x_)		__noop // compiled out.
#define DEBUG_MSG(_x_)		__noop // compiled out.
#define DEBUG_TRACE(_x_)	__noop // this allows all the variable args to be compiled out
#endif

#ifdef _CPPUNWIND

#define GEXCEP_CATCH_LOG(desc) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )\
	{\
	::Gray::cExceptionHolder exh(ex);\
	::Gray::CLogMgr::I().LogExceptionV(&exh,desc,nullptr);\
	}
	// has 1 arg for context
#define GEXCEP_CATCH_LOG1(desc,arg) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )\
	{\
	::Gray::cExceptionHolder exh(ex);\
	::Gray::CLogMgr::I().LogExceptionF(&exh,desc,arg);\
	}
#else
#define GEXCEP_CATCH_LOG(desc) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )
#define GEXCEP_CATCH_LOG1(desc,arg) GRAY_TRY_CATCH( ::Gray::cExceptionBase, ex )
#endif
};
#endif	// _INC_CLogMgr_H
