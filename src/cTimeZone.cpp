//! @file cTimeZone.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cTimeZone.h"

namespace Gray {
const cTimeZone cTimeZoneMgr::k_TimeZones[] = {
    /// terminated by name = nullptr;
    {_GT("Z"), _GT(""), TZ_UTC, TZ_DSTRULE_t::_NONE},
    {_GT("G"), _GT(""), TZ_UTC, TZ_DSTRULE_t::_NONE},
    {_GT("UTC"), _GT(""), TZ_UTC, TZ_DSTRULE_t::_NONE},
    {_GT("GMT"), _GT(""), TZ_UTC, TZ_DSTRULE_t::_NONE},
    {_GT("EST"), _GT("Eastern"), TZ_EST, TZ_DSTRULE_t::_AMERICAN},
    {_GT("CST"), _GT("Central"), TZ_CST, TZ_DSTRULE_t::_AMERICAN},
    {_GT("MST"), _GT("Mountain"), TZ_MST, TZ_DSTRULE_t::_AMERICAN},
    {_GT("PST"), _GT("Pacific"), TZ_PST, TZ_DSTRULE_t::_AMERICAN},
    {nullptr, nullptr, TZ_UTC, TZ_DSTRULE_t::_NONE},
};

bool cTimeZoneMgr::sm_bInitTimeZoneSet = false;  /// Have I called tzset() ?

//******************************************************************************************

TIMEVALU_t GRAYCALL cTimeZoneMgr::GetLocalMinutesWest() {  // static
    //! Get the time zone offset (for this local system) in minutes (west).
    //! The difference in minutes between UTC and local time
    //! This DOES NOT include DST offset if current.

    if (!sm_bInitTimeZoneSet) {
        // Must call this once to get timezone from environment variables.
        // Reads the timezone from the system. environment variables ?
        sm_bInitTimeZoneSet = true;
#ifdef __linux__
        ::tzset();
#elif USE_CRT && !defined(UNDER_CE)
        ::_tzset();
#else
        // Get TZ from some other place ???
#endif
    }

#if defined(_WIN32)

#if defined(UNDER_CE) || !USE_CRT
    // dwRet = TIME_ZONE_ID_DAYLIGHT, TIME_ZONE_ID_STANDARD, TIME_ZONE_ID_UNKNOWN=0
    ::TIME_ZONE_INFORMATION tzi;
    DWORD dwRet = ::GetTimeZoneInformation(&tzi);
    return (TIMEVALU_t)(tzi.Bias);  // minutes.

#elif defined(_MSC_VER) && _MSC_VER >= 1400
    long nTimeZone = TZ_UTC;
    if (::_get_timezone(&nTimeZone)) {  //  in seconds between UTC and local time as an integer.
        // error;
        return TZ_UTC;
    }
    return CastN(TIMEVALU_t, nTimeZone / 60);  // to minutes
#else
    // assume _tzset(); called.
    return CastN(TIMEVALU_t, ::_timezone / 60);  // minutes
#endif

#else  // __linux__
    return CastN(TIMEVALU_t, ::timezone / 60);  // minutes
#endif

#if 0  // defined(__linux__)
		cTimeVal tv;
		struct timezone tz;
		cMem::Zero(&tz, sizeof(tz));
		::gettimeofday(&tv, &tz);
		return CastN(TIMEVALU_t,tz.tz_minuteswest);
#endif
}

TIMEVALU_t GRAYCALL cTimeZoneMgr::GetOffsetMinutes(TZ_TYPE nTimeZone) {  // static
    if (nTimeZone == TZ_LOCAL) {
        return GetLocalMinutesWest();
    }
    return CastN(TIMEVALU_t, nTimeZone);
}

const cTimeZone* GRAYCALL cTimeZoneMgr::FindTimeZone(TZ_TYPE nTimeZone) {  // static
    //! Get a block describing the time zone. (by offset)
    //! geographic timezone does not include offset for DST.
    for (UINT i = 0; i < _countof(k_TimeZones) - 1; i++) {
        if (nTimeZone == k_TimeZones[i]._nTimeZoneOffset) return &k_TimeZones[i];
    }
    return nullptr;
}
const cTimeZone* GRAYCALL cTimeZoneMgr::FindTimeZone(const GChar_t* pszName) {  // static
    //! Get a block describing the time zone. (by name)
    for (UINT i = 0; i < _countof(k_TimeZones) - 1; i++) {
        if (!StrT::CmpI(pszName, k_TimeZones[i]._pszTimeZoneName)) return &k_TimeZones[i];
    }
    return nullptr;
}
const cTimeZone* GRAYCALL cTimeZoneMgr::FindTimeZoneHead(const GChar_t* pszName) {  // static
    // like FindTimeZone() but doesn't have to be a full string.
    for (UINT i = 0; i < _countof(k_TimeZones); i++) {
        if (!StrT::CmpHeadI(pszName, k_TimeZones[i]._pszTimeZoneName)) return &k_TimeZones[i];
    }
    return nullptr;
}
}  // namespace Gray
