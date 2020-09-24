//
//! @file CLogLevel.h
//! Thread safe arrays of stuff.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CLogLevel_H
#define _INC_CLogLevel_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	typedef char LOGCHAR_t;	//!< always just use UTF8 for logs, don't bother with UNICODE.
#define LOGSTR(x) ::Gray::StrArg< ::Gray::LOGCHAR_t >(x)		//!< safe convert wchar_t arguments to char if needed.
#define LOGSTR2(x,y) ::Gray::StrArg< ::Gray::LOGCHAR_t >(x,(RADIX_t)y)	//!< for a numeric. safe convert wchar_t arguments to char if needed.
#define LOGERR(hRes) LOGSTR(CStringL::GetErrorString(hRes))		//!< Used to supply "ERR='%s'"

	template <typename _TYPE_CH> class GRAYCORE_LINK cStringT;	//!< forward declare this.
	typedef cStringT<LOGCHAR_t> CStringL;	//!< Log string.

	enum LOGLEV_TYPE
	{
		//! @enum Gray::LOGLEV_TYPE
		//! log level = criticalness/importance level of a logged event. higher = more severe/important.
		//! Similar to _WIN32 DPFLTR_ERROR_LEVEL = DEBUGLVL_ERROR or .NET System.Diagnostics.EventLogEntryType
#define LOGLEVELDEF(a,b,c,d) LOGLEV_##a,
#include "CLogLevel.tbl"
#undef LOGLEVELDEF
		LOGLEV_QTY,		//!< 8= Filter everything. i want to see nothing.
	};

	struct GRAYCORE_LINK CLogLevel	// static
	{
		static const LOGCHAR_t* const k_pszPrefixes[LOGLEV_QTY + 1];	//!< LOGLEV_WARN, nullptr term
		static const LOGCHAR_t* GRAYCALL GetPrefixStr(LOGLEV_TYPE eLogLevel);
	};
};
#endif
