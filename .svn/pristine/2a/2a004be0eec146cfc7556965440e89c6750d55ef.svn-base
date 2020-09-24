//
//! @file CTimeZone.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CTimeZone.h"
#include "StrT.h"

namespace Gray
{
	const CTimeZone CTimeZoneMgr::k_TimeZones[] = 	//!< terminated by name = nullptr;
	{
		{ _GT("Z"), _GT(""), TZ_UTC, TZ_DSTRULE_NONE },
		{ _GT("G"), _GT(""), TZ_UTC, TZ_DSTRULE_NONE },
		{ _GT("UTC"), _GT(""), TZ_UTC, TZ_DSTRULE_NONE },
		{ _GT("GMT"), _GT(""), TZ_UTC, TZ_DSTRULE_NONE },
		{ _GT("EST"), _GT("Eastern"), TZ_EST, TZ_DSTRULE_AMERICAN },
		{ _GT("CST"), _GT("Central"), TZ_CST, TZ_DSTRULE_AMERICAN },
		{ _GT("MST"), _GT("Mountain"), TZ_MST, TZ_DSTRULE_AMERICAN },
		{ _GT("PST"), _GT("Pacific"), TZ_PST, TZ_DSTRULE_AMERICAN },
		{ nullptr, nullptr, TZ_UTC, TZ_DSTRULE_NONE },
	};

	bool CTimeZoneMgr::sm_bInitTimeZoneSet = false;	//!< Have I called tzset() ?

	//******************************************************************************************

	TZ_TYPE GRAYCALL CTimeZoneMgr::GetLocalTimeZoneOffset() // static
	{
		//! Get the time zone offset (for this local system) in minutes.
		//! The difference in minutes between UTC and local time
		//! This DOES NOT include DST offset if current.

		if (!sm_bInitTimeZoneSet)
		{
			// Must call this once to get timezone from environment variables.
			// Reads the timezone from the system. environment variables ?
			sm_bInitTimeZoneSet = true;
#ifdef __linux__
			::tzset();
#elif ! defined(UNDER_CE)
			::_tzset();
#endif
		}

#if defined(_WIN32)

#ifdef UNDER_CE
		// dwRet = TIME_ZONE_ID_DAYLIGHT, TIME_ZONE_ID_STANDARD, TIME_ZONE_ID_UNKNOWN=0
		TIME_ZONE_INFORMATION tzi;
		DWORD dwRet = ::GetTimeZoneInformation(&tzi);
		return (TZ_TYPE)(tzi.Bias);	// minutes.

#elif defined(_MSC_VER) && _MSC_VER >= 1400
		long nTimeZoneOffset = TZ_UTC;
		if (_get_timezone(&nTimeZoneOffset))  //  in seconds between UTC and local time as an integer.
		{
			// error;
			return TZ_UTC;
		}
		return (TZ_TYPE)(nTimeZoneOffset / 60);	// to minutes
#else
		// assume _tzset(); called.
		return (TZ_TYPE)(_timezone / 60);	// minutes
#endif

#else	// __linux__
		return (TZ_TYPE)(timezone / 60);		// minutes
#endif

#if 0 // defined(__linux__)
		CTimeVal tv;
		struct timezone tz;
		CMem::Zero(&tz, sizeof(tz));
		gettimeofday(&tv, &tz);
		return (TZ_TYPE)tz.tz_minuteswest;
#endif
	}

	const CTimeZone* GRAYCALL CTimeZoneMgr::FindTimeZone(TZ_TYPE nTimeZoneOffset) // static
	{
		//! Get a block describing the time zone. (by offset)
		//! geographic timezone does not include offset for DST.
		for (UINT i = 0; i < _countof(k_TimeZones) - 1; i++)
		{
			if (nTimeZoneOffset == k_TimeZones[i].m_nTimeZoneOffset)
				return &k_TimeZones[i];
		}
		return nullptr;
	}
	const CTimeZone* GRAYCALL CTimeZoneMgr::FindTimeZone(const GChar_t* pszName) // static
	{
		//! Get a block describing the time zone. (by name)
		for (UINT i = 0; i < _countof(k_TimeZones) - 1; i++)
		{
			if (!StrT::CmpI(pszName, k_TimeZones[i].m_pszTimeZoneName))
				return &k_TimeZones[i];
		}
		return nullptr;
	}
	const CTimeZone* GRAYCALL CTimeZoneMgr::FindTimeZoneHead(const GChar_t* pszName) // static
	{
		// like FindTimeZone() but doesn't have to be a full string.
		for (UINT i = 0; i < _countof(k_TimeZones); i++)
		{
			if (!StrT::CmpHeadI(pszName, k_TimeZones[i].m_pszTimeZoneName))
				return &k_TimeZones[i];
		}
		return nullptr;
	}
}