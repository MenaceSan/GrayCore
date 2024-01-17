//
//! @file cLogSink.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogSink_H
#define _INC_cLogSink_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogLevel.h"
#include "cRefPtr.h"
#include "cStream.h"
#include "cString.h"
#include "cThreadLock.h"

namespace Gray {
struct cLogEvent;

typedef cStringT<LOGCHAR_t> cStringL;                        /// Log string.
#define LOGERR(hRes) LOGSTR(cStringL::GetErrorStr(hRes))  /// Used to supply "ERR='%s'"

/// <summary>
/// Log event attributes. special controls for odd logged events.
/// bitmask so event can have multi attributes
/// </summary>
enum LOG_ATTR_TYPE_ : UINT32 {
    LOG_ATTR_0 = 0,

    //! Adding new attributes (low values) here can be used to add more categories.
    //!  but use cLogSubject for that for much more flexibility.

    LOG_ATTR_INIT = 0x00100000,      /// startup/exit stuff. category.
    LOG_ATTR_SCRIPT = 0x00200000,    /// from some sort of scripted code exec. category.
    LOG_ATTR_NET = 0x00400000,       /// from network activity. category. (watch out that this is not also sent on the network as it might cause feedback)
    LOG_ATTR_INTERNAL = 0x00800000,  /// Do not echo this message as it may relate to my own logging internals (i.e.feedback loop)

    LOG_ATTR_ODD = 0x01000000,      /// This is odd/unusual behavior for client. category. Probably not a code problem but a user/security/integrity problem.
    LOG_ATTR_DEBUG = 0x02000000,    /// Unclassified debug stuff. category.
    LOG_ATTR_TEMP = 0x04000000,     /// Real time status (don't bother to log permanently)

    // Bit Flags to control Sink
    LOG_ATTR_PRINT = 0x10000000,      /// The equiv of a printf() to console.
    LOG_ATTR_PRINTBACK = 0x20000000,  /// Back up over the last LOG_ATTR_PRINT. append to the last.
    LOG_ATTR_NOCRLF = 0x40000000,     /// Don't add a FILE_EOL (CR NL) to the end of this string. this is a partial message.

    // LOG_ATTR_QUESTION = 0x123123,	/// This is a question that we hope will get an answer through some async callback. cLogSinkConsole

    LOG_ATTR_CUST_MASK = 0x000FFFFF,
    LOG_ATTR_BASE_MASK = 0xFFF00000,
    LOG_ATTR_ALL_MASK = 0xFFFFFFFF
};
typedef UINT32 LOG_ATTR_MASK_t;  // mask of LOG_ATTR_TYPE_

//***********************************************************************************

/// <summary>
/// Filtering parameters associated with a particular log event instance.
/// </summary>
class GRAYCORE_LINK cLogEventParams {
    LOG_ATTR_MASK_t m_uAttrMask = LOG_ATTR_0;  /// Special attributes for the event. (regardless of level) similar to pszSubject?
    LOGLVL_t m_eLogLevel = LOGLVL_t::_ANY;     /// Min Importance level to see. 0 = LOGLVL_t::_ANY = not important.

 public:
    cLogEventParams(LOG_ATTR_MASK_t uAttrMask = LOG_ATTR_0, LOGLVL_t eLogLevel = LOGLVL_t::_TRACE) noexcept
        : m_uAttrMask(uAttrMask),  // Level of log detail messages. IsLogMsg()
          m_eLogLevel(eLogLevel)   // Importance level.
    {}

    LOG_ATTR_MASK_t get_LogAttrMask() const noexcept {
        return m_uAttrMask;
    }
    void put_LogAttrMask(LOG_ATTR_MASK_t uAttrMask) noexcept {
        //! What types of info do we want to filter for.
        m_uAttrMask = uAttrMask;
    }
    bool IsLogAttrMask(LOG_ATTR_MASK_t uAttrMask) const noexcept {
        return cBits::HasMask(m_uAttrMask, uAttrMask);
    }

    LOGLVL_t get_LogLevel() const noexcept {
        return m_eLogLevel; // Min level to show.
    }
    void put_LogLevel(LOGLVL_t eLogLevel) noexcept {
        //! What level of importance do we want to filter for.
        m_eLogLevel = eLogLevel;
    }
    bool IsLoggedLevel(LOGLVL_t eLogLevel) const noexcept {
        //! level = LOGLVL_t::_INFO (higher is more important
        return eLogLevel >= m_eLogLevel;
    }

    /// Would/should this message be logged? LOG_ATTR_0
    bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept {
        if (!IsLoggedLevel(eLogLevel)) return false;
        if (uAttrMask != 0 && !IsLogAttrMask(uAttrMask)) return false;
        return true;
    }
};

/// <summary>
/// Parameters for time throttle of log messages. Queue messages up if they are coming too fast.
/// </summary>
struct GRAYCORE_LINK cLogThrottle {
    // throttle.
    float m_fLogThrottle;             /// how fast sent to me? messages/sec
    mutable TIMESYS_t m_TimeLogLast;  /// Last time period for throttling (1 sec).
    mutable UINT m_nQtyLogLast;       /// Qty of messages since m_TimeLogLast.

    cLogThrottle() noexcept;

    float get_LogThrottle() const noexcept {
        //! messages/sec
        return m_fLogThrottle;
    }
};

//***********************************************************************

/// <summary>
/// All events funnel through addEvent(). Sources flow into cLogMgr and Sinks flow to some output device.
/// </summary>
DECLARE_INTERFACE(ILogProcessor) {
    IGNORE_WARN_INTERFACE(ILogProcessor);
    /// <summary>
    /// Would/should this message be logged? fast pre-check. can call before building complex message.
    /// </summary>
    /// <param name="uAttrMask">LOG_ATTR_MASK_t</param>
    /// <param name="eLogLevel">LOGLVL_t</param>
    virtual bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept = 0;

    /// <summary>
    /// Push the message where it is supposed to go. NEVER throw!
    /// </summary>
    /// <param name="rEvent">cLogEvent ref counted</param>
    /// <returns> -gt- 0 = i handled this. -lt- 0 = failed = don't process this sink anymore.
    /// </returns>
    virtual HRESULT addEvent(cLogEvent & rEvent) noexcept = 0;
};

class cLogNexus;

/// <summary>
/// abstract base class. Build/submit a log message cLogEvent to be submitted to the log system. Source or Sink.
/// </summary>
struct GRAYCORE_LINK cLogProcessor : public ILogProcessor {  // for WriteString raw dump messages into the system.
    virtual ~cLogProcessor() {}

    /// Is this a cLogNexus or just a cLogProcessor ? like dynamic_cast
    virtual const cLogNexus* get_ThisLogNexus() const noexcept {
        return nullptr;
    }

    /// <summary>
    /// should this message be logged? should i bother building it ? fast. default = yes.
    /// </summary>
    bool IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept override {  // fast pre-check.
        UNREFERENCED_PARAMETER(uAttrMask);
        UNREFERENCED_PARAMETER(eLogLevel);
        return true;
    }

    /// <summary>
    /// Override this to flush logs for this processor.
    /// </summary>
    virtual HRESULT FlushLogs() {
        return S_OK;
    }

    // Helpers.

    /// <summary>
    /// Dispatch the event to all matching sink. noexcept = logger should never throw !
    /// ASSUME new line will be added.
    /// @note This can be called by multiple threads!
    /// @note This could be called in odd interrupt context so don't use dynamic stuff
    /// </summary>
    /// <param name="uAttrMask">LOG_ATTR_DEBUG|LOG_ATTR_INTERNAL</param>
    /// <param name="eLogLevel">LOGLVL_t::_TRACE, LOGLVL_t::_ERROR</param>
    /// <returns>-lt- 0 = failed, 0=not processed by anyone</returns>
    HRESULT addEventS(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, cStringL sMsg) noexcept;

    HRESULT addEventV(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, const LOGCHAR_t* pszFormat, va_list vargs) noexcept;

    // Variadic helpers.
    HRESULT _cdecl addEventF(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, const LOGCHAR_t* pszFormat, ...) noexcept {
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(uAttrMask, eLogLevel, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }

    HRESULT _cdecl addInfoF(const LOGCHAR_t* pszFormat, ...) {
        //! @return <0 = failed, 0=not processed by anyone, # = number of processors.
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(LOG_ATTR_0, LOGLVL_t::_INFO, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }

    HRESULT _cdecl addDebugErrorF(const LOGCHAR_t* pszFormat, ...) {
        //! Add message with LOG_ATTR_DEBUG
        //! @return <0 = failed, 0=not processed by anyone, # = number of processors.
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLVL_t::_ERROR, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }
    HRESULT _cdecl addDebugWarnF(const LOGCHAR_t* pszFormat, ...) {
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLVL_t::_WARN, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }
    HRESULT _cdecl addDebugInfoF(const LOGCHAR_t* pszFormat, ...) {
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLVL_t::_INFO, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }
    HRESULT _cdecl addDebugTraceF(const LOGCHAR_t* pszFormat, ...) {
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hRes = addEventV(LOG_ATTR_DEBUG, LOGLVL_t::_TRACE, pszFormat, vargs);
        va_end(vargs);
        return hRes;
    }
};

/// <summary>
/// Abstract Base class for the destination for a log message.
/// Messages can be routed to several different places depending on filtering, app config, etc.
/// overload these to implement which messages go here.
/// </summary>
struct GRAYCORE_LINK cLogSink : public IUnknown, public cLogProcessor {
    friend class cLogNexus;

    ~cLogSink() override {
        RemoveSinkThis();
    }

    /// <summary>
    /// Remove myself from the list of valid sink in cLogMgr.
    /// will descend into child cLogNexus as well.
    /// called on destruct.
    /// </summary>
    bool RemoveSinkThis();
};

/// <summary>
/// Send logged messages out to the debug system. OutputDebugString()
/// No filter and take default formatted string
/// </summary>
class GRAYCORE_LINK cLogSinkDebug : public cLogSink, public cRefBase, public cStreamOutput {
    mutable cThreadLockCount m_Lock;  // prevent multi thread mixing of messages.
 public:
    static HRESULT GRAYCALL AddSinkCheck(cLogNexus* pLogger = nullptr);
    HRESULT WriteString(const LOGCHAR_t* pszMsg) override;
    HRESULT addEvent(cLogEvent& rEvent) noexcept override;
    IUNKNOWN_DISAMBIG(cRefBase);
};

/// <summary>
/// @todo Append (or cache) detailed messages here and hold them until some error triggers them.
/// Once some error triggers, then we can emit all these detail messages to some file for processing.
/// If no trigger occurs in time then trash these messages.
/// </summary>
class GRAYCORE_LINK cLogSinkCache : public cLogSink, public cRefBase {
    TIMESYS_t m_nCacheHold;  /// How long to hold messages. toss detail messages if nothing special happens.
};
}  // namespace Gray

#endif
