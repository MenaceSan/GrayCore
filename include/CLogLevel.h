//
//! @file cLogLevel.h
//! Thread safe arrays of stuff.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLogLevel_H
#define _INC_cLogLevel_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray {
typedef char LOGCHAR_t;                                                  /// always just use UTF8 for logs, don't bother with UNICODE.
#define LOGSTR(x) ::Gray::StrArg< ::Gray::LOGCHAR_t>(x)                  /// safe convert wchar_t arguments to char if needed.
#define LOGSTR2(x, y) ::Gray::StrArg< ::Gray::LOGCHAR_t>(x, (RADIX_t)y)  /// for a numeric. safe convert wchar_t arguments to char if needed.

/// <summary>
/// log level = criticalness/importance level of a logged event. higher = more severe/important.
/// Similar to _WIN32 TRACE_LEVEL_VERBOSE "EvnTrace.h", DPFLTR_ERROR_LEVEL, DEBUGLVL_ERROR or .NET System.Diagnostics.EventLogEntryType
/// </summary>
enum class LOGLVL_t : BYTE {
#define LOGLEVELDEF(a, b, c, d) _##a,
#include "cLogLevel.tbl"
#undef LOGLEVELDEF
    _QTY,  /// 8= Filter everything. i want to see nothing.
};

struct GRAYCORE_LINK cLogLevel {                                                        // static
    static const LOGCHAR_t* const k_pszPrefixes[static_cast<int>(LOGLVL_t::_QTY) + 1];  /// LOGLVL_t::_WARN, nullptr term
    static const LOGCHAR_t* GRAYCALL GetPrefixStr(LOGLVL_t eLogLevel);
};
}  // namespace Gray
#endif
