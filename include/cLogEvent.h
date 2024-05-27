//! @file cLogEvent.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cLogEvent_H
#define _INC_cLogEvent_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrBuilder.h"
#include "cLogSink.h"
#include "cTimeInt.h"

namespace Gray {
#if 0
enum class LOG_FIELD_t {
	LOG_FIELD_LEVEL,	/// The critical level message prefix. LOGLVL_t
	LOG_FIELD_TIME,		/// The time stamp part of a message.
	LOG_FIELD_MSG,		/// The actual message text part of the log event.
};
#endif

/// <summary>
/// Store a single log event (ref counted) instance for asynchronous processing.
/// TODO store log event as (format,stringarg1,stringargN) and allow translation of the format but assume stringargs are always proper names (not translatable)
/// </summary>
struct GRAYCORE_LINK cLogEvent : public cLogEventParams, public cRefBase {
    TIMESEC_t m_time = 0;                /// when did this happen? as cTimeInt. maybe not set until needed. ! isTimeValid()
    const char* m_pszSubject = nullptr;  /// static allocated general subject matter tag. can be filled in by cLogSubject. Script source ?
    cStringL m_sMsg;                     /// free form message text.

    cLogEvent(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, cStringL sMsg) noexcept : cLogEventParams(uAttrMask, eLogLevel), m_time(0), m_pszSubject(nullptr), m_sMsg(sMsg) {}

    /// take all my attributes and make a single string in normal/default format. adds FILE_EOL.
    void GetFormattedDefault(StrBuilder<LOGCHAR_t>& s) const;
    cStringL get_FormattedDefault() const;
};
typedef cRefPtr<cLogEvent> cLogEventPtr;
}  // namespace Gray
#endif
