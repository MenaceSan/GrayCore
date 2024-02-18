//! @file cTimeZone.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTimeZone_H
#define _INC_cTimeZone_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrConst.h"
#include "cTimeUnits.h"

namespace Gray {
/// <summary>
/// Enumerate the DST rules that might exist in the world.
/// Try to be historically accurate. May or may not use DST.
/// http://www.worldtimezone.com/daylight.html
/// </summary>
enum class TZ_DSTRULE_t {
    _NONE = 0,  /// don't use DST at all. UTC. 
    _AMERICAN,  /// use the American rules for DST. As they existed at the time.
    // _MOROCCO // is insane.
};

/// <summary>
/// TimeZone/DST offset rules for a political region by name.
/// For display of time as a string.
/// Similar to _WIN32 GetTimeZoneInformation(TIME_ZONE_INFORMATION)
/// </summary>
struct cTimeZone {
    const GChar_t* m_pszTimeZoneName;  /// Short Name. EST, PST, etc.
    const GChar_t* m_pszTimeZoneDesc;  /// Long name and description.

    TIMEVALU_t m_nTimeZoneOffset;  /// offset from UTC/GMT in minutes. Pure geography, NOT DST.
    TZ_DSTRULE_t m_eDSTRule;       /// does it have/use a DST calculation? needs cTimeUnits.
};

/// <summary>
/// Manage the collection of time zones. We need to make this configurable since it may change over time.
/// @todo Manage dynamic list of TZ from file or db.
/// </summary>
class GRAYCORE_LINK cTimeZoneMgr { // : public cSingleton<cTimeZoneMgr>
 private:
    static bool sm_bInitTimeZoneSet;  /// Have i called tzset() ? So I know this computers local time zone.
 public:
    static const cTimeZone k_TimeZones[];  /// Fixed/Default array of world time zones. terminated by name = nullptr;
 public:
    /// Get minutes west.
    static TIMEVALU_t GRAYCALL GetLocalMinutesWest();
    static TIMEVALU_t GRAYCALL GetOffsetMinutes(TZ_TYPE nTimeZone);

    static const cTimeZone* GRAYCALL FindTimeZone(TZ_TYPE nTimeZone);
    static const cTimeZone* GRAYCALL FindTimeZone(const GChar_t* pszName);
    static const cTimeZone* GRAYCALL FindTimeZoneHead(const GChar_t* pszName);
};
}  // namespace Gray
#endif
