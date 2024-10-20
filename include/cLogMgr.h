//! @file cLogMgr.h
//! Message / Event Log Macros. (Debug Mostly)
//! similar to Log4J (with appenders) formatters per sink to control actual format?
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cLogMgr_H
#define _INC_cLogMgr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "IUnknown.h"
#include "cArrayIUnk.h"
#include "cArrayRef.h"
#include "cDebugAssert.h"
#include "cException.h"
#include "cLogSink.h"
#include "cSingleton.h"
#include "cTimeSys.h"
#include "cTimeUnits.h"
#include "cTypeInfo.h"

// #define _DEBUG_FAST	// put debug in release mode optimized code.

namespace Gray {
/// <summary>
/// A nexus for routing log messages. (may have sub sinks)
/// can submit directly instead of using cLogSubject = default
/// Like Log4J
/// Actual cLogEvent may be routed or filtered to multiple Sink/destinations/appender from here.
/// Array of attached Sinks to say where the logged events go.
/// addEvent() is multi thread safe.
/// </summary>
class GRAYCORE_LINK cLogNexus : public cLogProcessor {
 protected:
    cArrayIUnk<cLogSink> _aSinks;      /// where do the log messages go? child sinks. Protect with _LockLog.

 public:
    cLogEventParams _LogFilter;         /// Union filter of what goes out to ALL sinks
    cLogThrottle _LogThrottle;          /// Measure how fast messages are going.
    mutable cThreadLockableX _LockLog;  /// serialize multiple threads for _aSinks

 public:
    cLogNexus(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_ALL_MASK, LOGLVL_t eLogLevel = LOGLVL_t::_ANY);
 
    /// <summary>
    /// Is this a cLogNexus or just a cLogProcessor?
    /// </summary>
    const cLogNexus* get_ThisLogNexus() const noexcept override {
        return this;
    }

    /// <summary>
    /// fast pre-check.
    /// @note Check IsLogged(x) before generating the message! for speed
    /// </summary>
    bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept override {
        return _LogFilter.IsLogged(uAttrMask, eLogLevel);
    }

    /// <summary>
    /// add a new log event and send/route it to all applicable sinks.
    /// </summary>
    /// <param name="pEvent"></param>
    /// <returns>-lt- 0 = failed, 0=not processed by anyone, # = number of processors.</returns>
    HRESULT addEvent(cLogEvent& rEvent) noexcept override;

    HRESULT FlushLogs() override;

    // manage sinks. ASSUME _LockLog
    cLogSink* EnumSinks(ITERATE_t i) {
        return _aSinks.GetAtCheck(i);
    }
    const cLogSink* EnumSinks(ITERATE_t i) const {
        return _aSinks.GetAtCheck(i);
    }

    /// <summary>
    /// Does this cLogNexus contain this cLogSink?
    /// </summary>
    /// <param name="pSink">what are we trying to find?</param>
    /// <param name="bDescend">will descend into child cLogNexus as well.</param>
    /// <returns></returns>
    bool HasSink(cLogSink* pSink, bool bDescend = false) const;

    /// <summary>
    /// Add Newest first.
    /// </summary>
    /// <param name="pSink"></param>
    /// <returns></returns>
    HRESULT AddSink(cLogSink* pSink);

    /// <summary>
    /// Remove this sink. 
    /// </summary>
    /// <param name="pSink"></param>
    /// <param name="bDescend">descend into child cLogNexus as well.</param>
    /// <returns></returns>
    bool RemoveSink(cLogSink* pSink, bool bDescend = false);

    /// <summary>
    /// is there a sink/appender of this type already installed?
    /// </summary>
    /// <param name="rType"></param>
    /// <param name="bDescend">search child cLogNexus?</param>
    /// <returns></returns>
    cLogSink* FindSinkType(const TYPEINFO_t& rType, bool bDescend = false) const;

    bool RemoveSinkType(const TYPEINFO_t& rType, bool bDescend = true) {
        return RemoveSink(FindSinkType(rType, bDescend), bDescend);
    }
};

/// <summary>
/// The root log message cLogNexus. take in all log messages and decide where they go.
/// Only one default singleton log manager per system.
/// </summary>
class GRAYCORE_LINK cLogMgr final : public cSingleton<cLogMgr>, public cLogNexus {
    static TIMESECD_t sm_TimePrevException;  /// throttle/don't flood log with exceptions().

public:
    DECLARE_cSingleton(cLogMgr);

 protected:
    cLogMgr() noexcept;
    ~cLogMgr();
    void ReleaseModuleChildren(::HMODULE hMod) override;

 public:
#ifdef _CPPUNWIND
    /// <summary>
    /// An exception occurred. record it. Logging of cException.
    /// </summary>
    void LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ::va_list vargs) noexcept;
    void _cdecl LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...) noexcept;
#endif
};

/// <summary>
/// A source logger dedicated to a certain subject matter that feeds into cLogMgr.
/// declared statically for each subject we might want to log.
/// equivalent to the Logger in Log4J
/// all log messages should enter the log system here ideally. NOT through cLogMgr::I() directly.
/// ideally subjects are hierarchical ALA Log4J
/// e.g. "Root.Server.Clients.Login" so they can be filter via hierarchy wildcard.
/// </summary>
struct GRAYCORE_LINK cLogSubject : public cLogProcessor {
    typedef cLogProcessor SUPER_t;
    const char* _pszSubject;  /// static string. general subject matter tag.
    cLogSubject(const char* pszSubject);
    /// <summary>
    /// Check cLogMgr levels
    /// </summary>
    bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept override;
    /// <summary>
    /// Add Source event to the logging system with attached subject.
    /// push to the logging system for delivery to log sinks.
    /// Prefix the event with the subject.
    /// </summary>
    /// <param name="rEvent"></param>
    /// <returns>-lt- 0 = failed, 0=not processed by anyone, # = number of processors.</returns>
    HRESULT addEvent(cLogEvent& rEvent) noexcept override;  // ILogProcessor
};

//***********************************************************************************
// Stuff that should stay in release mode.
#define LOGF(_x_) ::Gray::cLogMgr::I().addEventF _x_  // needs attributes and log level.

// We usually leave this stuff in for release code ?
#ifndef DEBUG_ERR
#define DEBUG_ERR(_x_) ::Gray::cLogMgr::I().addDebugErrorF _x_
#define DEBUG_WARN(_x_) ::Gray::cLogMgr::I().addDebugWarnF _x_
#endif

// Log messages that compile out when not wanted. only in debug mode.
#if !defined(DEBUG_MSG) && !defined(USE_DEBUG_LOG) && (defined(_DEBUG) || defined(_DEBUG_FAST))
#define USE_DEBUG_LOG
#endif
#ifdef USE_DEBUG_LOG
#define DEBUG_LOGF(_x_) ::Gray::cLogMgr::I().addEventF _x_  // needs attributes and log level.
#define DEBUG_MSG(_x_) ::Gray::cLogMgr::I().addDebugInfoF _x_
#define DEBUG_TRACE(_x_) ::Gray::cLogMgr::I().addDebugTraceF _x_
#else
#define DEBUG_LOGF(_x_) __noop   // compiled out.
#define DEBUG_MSG(_x_) __noop    // compiled out.
#define DEBUG_TRACE(_x_) __noop  // this allows all the variable args to be compiled out
#endif

#ifdef _CPPUNWIND

#define GEXCEP_CATCH_LOG(desc)                                   \
    GRAY_TRY_CATCH(::Gray::cExceptionBase, ex) {                 \
        ::Gray::cExceptionHolder exh(ex);                        \
        ::Gray::cLogMgr::I().LogExceptionV(&exh, desc, nullptr); \
    }
// has 1 arg for context
#define GEXCEP_CATCH_LOG1(desc, arg)                         \
    GRAY_TRY_CATCH(::Gray::cExceptionBase, ex) {             \
        ::Gray::cExceptionHolder exh(ex);                    \
        ::Gray::cLogMgr::I().LogExceptionF(&exh, desc, arg); \
    }
#else
#define GEXCEP_CATCH_LOG(desc) GRAY_TRY_CATCH(::Gray::cExceptionBase, ex)
#define GEXCEP_CATCH_LOG1(desc, arg) GRAY_TRY_CATCH(::Gray::cExceptionBase, ex)
#endif
}  // namespace Gray
#endif  // _INC_cLogMgr_H
