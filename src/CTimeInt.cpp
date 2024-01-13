//
//! @file cTimeInt.cpp
//! Replace the MFC CTime function. Must be usable with file system.
//! Accurate Measure whole seconds
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cLogMgr.h"
#include "cString.h"
#include "cTimeDouble.h"
#include "cTimeInt.h"
#include "cTimeZone.h"

#ifdef UNDER_CE
#include "cTimeFile.h"
#endif
#ifdef __linux__
#include <sys/timeb.h>
#include <sys/types.h>
#endif

namespace Gray {
cTimeInt::cTimeInt(const cTimeFile& fileTime) noexcept {
    //! cTimeFile = 64-bit 100-nanoseconds since January 1, 1601 GMT
    //! convert to TIMESEC_t (1970) (C-runtime local time)
    //! nDST = 0=Standard time is in effect. >0 =Daylight savings time is in effect. <0=default. Automatically computes whether standard time or daylight savings time is in effect.
    //! @note both are UTC so nDST makes no sense. What is MFC thinking ?

    FILETIME_t nTmp = fileTime.get_Val();
    nTmp /= cTimeFile::k_nFreq;  // convert to seconds.
    nTmp -= (cTimeFile::k_nDaysDiffTimeInt * (UINT64)cTimeUnits::k_nSecondsPerDay);
    m_time = (TIMESEC_t)nTmp;
}

TIMESEC_t GRAYCALL cTimeInt::GetTimeFromDays(double dTimeDays) noexcept { // static
    //! Set to time in seconds from time in days.
    //! Opposite of cTimeDouble::GetTimeFromSec()
    return (TIMESEC_t)((dTimeDays - cTimeDouble::k_nDaysDiffTimeInt) * cTimeUnits::k_nSecondsPerDay);
}

cTimeInt GRAYCALL cTimeInt::GetTimeNow() noexcept { // static
    //! @return The current time in seconds since Jan 1 1970 GMT (NOT LOCALIZED)
    //! @note GetCurrentTime() is "#define" by _WIN32 to GetTickCount() so i cant use that name!
    //! NOT adjusted for local time zone or DST.
#if defined(UNDER_CE) || !USE_CRT
    cTimeFile tNow;
    tNow.InitTimeNow();
    return cTimeInt(tNow);
#else
    return ::time(nullptr);
#endif
}

cTimeFile cTimeInt::GetAsFileTime() const noexcept {
    //! @return
    //!  cTimeFile = 64-bit 100-nanosecond since January 1, 1601 GMT
    FILETIME_t nTmp = GetTime();
    nTmp += cTimeFile::k_nDaysDiffTimeInt * (FILETIME_t)cTimeUnits::k_nSecondsPerDay;
    nTmp *= cTimeFile::k_nFreq;  // convert to FILETIME.
    return cTimeFile(nTmp);
}

void cTimeInt::InitTime(TIMESEC_t itime) noexcept {
    //! @arg itime <= 0 = invalid time.
    m_time = itime;
}

void cTimeInt::InitTimeNow() noexcept {
    //! Now();
    InitTime(GetTimeNow().GetTime());
}

void cTimeInt::InitTimeNowPlusSec(TIMESECD_t iOffsetInSeconds) noexcept {
    //! @note Assume iOffset is in seconds
    //! @note ASSUME TIMESEC_t is signed.
    if (iOffsetInSeconds >= INT_MAX) {
        // Set to Max time.
        InitTime(INT_MAX);
        return;
    }
    InitTime(cTimeInt::GetTimeNow().GetTime() + iOffsetInSeconds);
}

//*************************************************************************

bool cTimeInt::InitTimeUnits(const cTimeUnits& rTu) {
    //! Set time in seconds since Jan 1 1970 GMT from cTimeUnits
    //! Similar to the MFC CTime::CTime( const FILETIME& ft, int nDST = -1 )
    //! similar to "::mktime()"
    //! ASSUME _tzset() has been called and _timezone is set.
    //! nTimeZone = how to deal with DST ? TZ_UTC does not use DST. assume all others do.

    if (!rTu.isValidTimeUnits()) {
        InitTime(k_nZero);
        return false;
    }

    if (rTu.m_wYear < 1970)  // Can't be represented by int.
        return false;

    // Calculate elapsed days since base date (midnight, 1/1/70, UTC)
    // 365 days for each elapsed year since 1970, plus one more day for
    // each elapsed leap year. no danger of overflow because of the range
    // check (above) on tmptm1.
    TIMESEC_t nUnits = (rTu.m_wYear - 1970) * 365;
    nUnits += (cTimeUnits::GetLeapYearsSince2K(rTu.m_wYear) + 7);

    // elapsed days to current month (still no possible overflow)
    // Calculate days elapsed minus one, in the given year, to the given
    // month. Check for leap year and adjust if necessary.
    nUnits += cTimeUnits::k_MonthDaySums[cTimeUnits::IsLeapYear(rTu.m_wYear)][rTu.m_wMonth - 1];

    // elapsed days to current date.
    nUnits += rTu.m_wDay - 1;

    // elapsed hours since base date
    nUnits = (nUnits * 24) + rTu.m_wHour;

    // elapsed minutes since base date
    nUnits = (nUnits * 60) + rTu.m_wMinute;

    // elapsed seconds since base date
    nUnits = (nUnits * 60) + rTu.m_wSecond;

    if (rTu.m_nTZ != TZ_UTC) {
        // adjust
        TIMEVALU_t nTimeZone = rTu.m_nTZ;
        if (nTimeZone == TZ_LOCAL) {
            nTimeZone = cTimeZoneMgr::GetLocalMinutesWest();
        }
        nUnits += nTimeZone * 60;  // seconds
        if (rTu.isInDST1()) {        // TODO Does the TZ respect DST ?
            nUnits -= 60 * 60;  // remove added hour.
        }
    }

    InitTime(nUnits);
    return true;
}

bool cTimeInt::GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone) const {
    //! Get cTimeUnits for seconds since Jan 1 1970 GMT
    //! nTimeZone = TZ_UTC, TZ_GMT, TZ_LOCAL (adjust for DST and TZ)
    //! similar to "::gmtime()" or "::localtime()"

    const int k_YEAR_SEC = (365 * cTimeUnits::k_nSecondsPerDay);  // seconds in a typical year

    // Determine the years since 1900. Start by ignoring leap years.
    TIMESEC_t nSeconds = this->GetTime();
    if (nSeconds <= 0)  // not a legal time.
        return false;

    WORD nYears = (WORD)(nSeconds / k_YEAR_SEC);
    nSeconds -= nYears * k_YEAR_SEC;
    nYears += 1970;

    // Correct for elapsed leap years
    nSeconds -= (cTimeUnits::GetLeapYearsSince2K(nYears) + 7) * cTimeUnits::k_nSecondsPerDay;

    // If we have under-flowed the __time64_t range (i.e., if nSeconds < 0),
    // back up one year, adjusting the correction if necessary.
    int islpyr;  // is-current-year-a-leap-year flag
    if (nSeconds < 0) {
        nSeconds += k_YEAR_SEC;
        nYears--;
        islpyr = cTimeUnits::IsLeapYear(nYears);
        if (islpyr > 0) {
            nSeconds += cTimeUnits::k_nSecondsPerDay;
        }
    } else {
        islpyr = cTimeUnits::IsLeapYear(nYears);
    }

    // nYears now holds the value for tm_year. nSeconds now holds the
    // number of elapsed seconds since the beginning of that year.
    rTu.m_wYear = nYears;

    // Determine days since January 1 (0 - 365). This is the nDayOfYear value.
    // Leave nSeconds with number of elapsed seconds in that day.
    int nDayOfYear = (int)(nSeconds / cTimeUnits::k_nSecondsPerDay);
    if (nDayOfYear > 366) {
        ASSERT(0);
        return false;
    }
    nSeconds -= (TIMESEC_t)(nDayOfYear)*cTimeUnits::k_nSecondsPerDay;

    // Determine months since January (0 - 11) and day of month (1 - 31)
    const WORD* pnDays = cTimeUnits::k_MonthDaySums[islpyr];
    WORD nMonth = 1;
    for (; pnDays[nMonth] <= nDayOfYear; nMonth++)
        ;

    rTu.m_wMonth = nMonth;
    rTu.m_wDay = (WORD)(1 + (nDayOfYear - pnDays[nMonth - 1]));

    //  Determine hours since midnight (0 - 23), minutes after the hour
    //  (0 - 59), and seconds after the minute (0 - 59).
    rTu.m_wHour = (WORD)(nSeconds / 3600);
    nSeconds -= (TIMESEC_t)rTu.m_wHour * 3600L;
    rTu.m_wMinute = (WORD)(nSeconds / 60);
    rTu.m_wSecond = (WORD)(nSeconds - (rTu.m_wMinute * 60));

    rTu.AddTZ(nTimeZone);  // adjust for timezone and DST, TZ_GMT = 0
    return true;
}

//**************************************************************************
// String formatting

StrLen_t cTimeInt::GetTimeFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, const GChar_t* pszFormat, TZ_TYPE nTimeZone) const {
    // TODO look for %z or %Z to preserve timezone.
    //! MFC just calls this "Format"
    cTimeUnits Tu;
    if (!GetTimeUnits(Tu, nTimeZone)) {
        return 0;
    }
    return Tu.GetFormStr(pszOut, iOutSizeMax, pszFormat);
}

cString cTimeInt::GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZone) const {
    //! Get the time as a string formatted using "C" strftime()
    //! Opposite of SetTimeStr()
    //! MFC just calls this "Format"
    //! @arg
    //!  pszFormat = (const char*) TIME_FORMAT_DEFAULT
    //!  nTimeZone = (seconds) what TZ was this recorded in (_timezone), TZ_UTC, TZ_GMT, TZ_EST, TZ_LOCAL

    GChar_t szTemp[cTimeUnits::k_FormStrMax];  // estimate reasonable max size.
    StrLen_t iLenChars = GetTimeFormStr(szTemp, STRMAX(szTemp), pszFormat, nTimeZone);
    if (iLenChars <= 0) {
        return "";
    }
    return cString(szTemp, iLenChars);
}

HRESULT cTimeInt::SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
    //! Read the full date format (from Web pages etc)
    //! and make it into a cTimeInt value. (local TZ)
    //! @arg nTimeZone = (seconds) what TZ was this recorded in (_timezone) (typically TZ_EST)
    //!    ?? we have no idea is our local offset for DST is the same as encoded!
    //!	did the creator of pszDateTime adjust for DST ? tm_isdst
    //! @return
    //!  m_time = the number of seconds elapsed since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC), according to the system clock
    //!  true = OK
    //! e.g. "Sat, 07 Aug 2004 01:20:20", ""

    if (pszDateTime == nullptr) return E_POINTER;

    if (!StrT::CmpI(pszDateTime, _GT("now"))) {
        InitTimeNow();  // Sets the current date & time
        return 3;
    }

    cTimeUnits Tu;
    HRESULT hRes = Tu.SetTimeStr(pszDateTime, nTimeZone);
    if (hRes <= 0) {
        return 0;
    }

    InitTimeUnits(Tu);
    return hRes;
}

//******************************************************************************

cString GRAYCALL cTimeInt::GetTimeSpanStr(TIMESECD_t nSeconds, TIMEUNIT_t eUnitHigh, int iUnitsDesired, bool bShortText) { // static
    if (nSeconds <= 0) return bShortText ? _GT("0s") : _GT("0 seconds");
    cTimeUnits tu;  // 0
    tu.AddSeconds(nSeconds);
    GChar_t szMsg[256];
    tu.GetTimeSpanStr(szMsg, STRMAX(szMsg), eUnitHigh, iUnitsDesired, bShortText);
    return szMsg;
}
cString GRAYCALL cTimeInt::GetTimeDeltaBriefStr(TIMESECD_t dwSeconds) { // static
    return GetTimeSpanStr(dwSeconds, TIMEUNIT_t::_Day, 4, true);
}
cString GRAYCALL cTimeInt::GetTimeDeltaSecondsStr(TIMESECD_t dwSeconds) { // static
    return GetTimeSpanStr(dwSeconds, TIMEUNIT_t::_Hour, 3, false);
}
}  // namespace Gray
