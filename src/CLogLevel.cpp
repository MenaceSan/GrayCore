//
//! @file cLogLevel.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cLogLevel.h"
#include "StrA.h"
#include "StrU.h"

namespace Gray
{
	const LOGCHAR_t* const cLogLevel::k_pszPrefixes[LOGLEV_QTY + 1] = // LOGLEV_WARN, nullptr term
	{
	#define LOGLEVELDEF(a,b,c,d) c,
	#include "cLogLevel.tbl"
	#undef LOGLEVELDEF
		nullptr,			// LOGLEV_QTY
	};

	const LOGCHAR_t* GRAYCALL cLogLevel::GetPrefixStr(LOGLEV_TYPE eLogLevel) // static
	{
		//! Describe the LOGLEV_TYPE
		if (eLogLevel < LOGLEV_WARN)
			return "";
		if (eLogLevel >= LOGLEV_QTY)
			return "";
		return k_pszPrefixes[eLogLevel];
	}
}
