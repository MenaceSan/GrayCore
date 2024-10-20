//! @file cTimeDouble.cpp
//! Time in double days. Accurate Measure whole milli seconds
//! 0 = (midnight, 30 December 1899 GMT).
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
#include "cLogMgr.h"
#include "cString.h"
#include "cTimeDouble.h"
#include "cTimeInt.h"
#include "cTimeZone.h"
#include <math.h>  // floor()

namespace Gray {
const double cTimeDouble::k_nY2K = 36526.0;  /// The static value for y2k = January 1, 2000 in UTC/GMT
const double cTimeDouble::k_nY10 = 3650.0;   /// The first 10 years are sometimes reserved to act as offsets.

cTimeDouble GRAYCALL cTimeDouble::EncodeSeconds(double s) noexcept {  // static
    //! Encode GMT Time
    return cTimeDouble(s / cTimeUnits::k_nSecondsPerDay);
}
cTimeDouble GRAYCALL cTimeDouble::EncodeTime(short h, short m, short s, short ms) noexcept {  // static
    //! Encode GMT Time
    //! Same as MFC COleDateTime::EncodeTime
    return cTimeDouble((double)(h + ((m + (s + (ms / 1000.0)) / 60.0) / 60.0)) / 24.0);
}

cTimeDouble GRAYCALL cTimeDouble::EncodeDate(short wYear, short wMonth, short wDay) noexcept {  // static
    //! Encode GMT Date as days since (1899/12/30 midnight GMT) or (1900/1/1 0:0:0 GMT)
    //! same as SystemTimeToVariantTime() but more accurate for mSec

    if (wYear <= 0 || wYear > 9999) return 0.0;  // "Invalid Year in the date";
    if (wMonth < 1 || wMonth > 12) return 0.0;   // "Invalid month in the date";

    const int nLeapYear = cTimeUnits::IsLeapYear(wYear);
    if (wDay < 1 || wDay > cTimeUnits::k_aMonthDays[nLeapYear][wMonth - 1]) {
        return 0.0;  // "Invalid day in the date";
    }

    const int iYear = wYear - 1;  // 1999 format. from year 0
    int iDays = (iYear * 365);    // add years

    // like cTimeUnits::GetLeapYearsSince2K
    iDays += (iYear / 4) + (iYear / 400) - (iYear / 100);  // years compensated for leaps.
    iDays -= 693594L;                                      // Offset so that midnight 12/30/1899 is 0, midnight 12/31/1899 = 1
    iDays += cTimeUnits::k_aMonthDaySums[nLeapYear][wMonth - 1];
    iDays += wDay;  // of month

    return cTimeDouble((double)iDays);
}

void cTimeDouble::DecodeDate(OUT cTimeUnits& rTu, double dblDate) {  // static
    // Decode just the date portion. NOT time.
    // Round to the second
    const double k_HALF_SECOND = (1.0 / (cTimeUnits::k_nSecondsPerDay * 2));  // Half a second, expressed in days.
    dblDate += (dblDate > 0.0) ? k_HALF_SECOND : (-k_HALF_SECOND);

    // Number of days since 1/1/0
    long nDaysAbsolute = (long)dblDate + 693959L;  // Add days from 1/1/0 to 12/30/1899

    dblDate = cValT::Abs(dblDate);

    // Leap years every 4 yrs except centuries not multiples of 400.
    // Number of 400 year increments since 1/1/0
    const long n400Years = (long)(nDaysAbsolute / 146097L);

    // Set nDaysAbsolute to day within 400-year block
    nDaysAbsolute %= 146097L;

    // -1 because first century has extra day
    // Century within 400 year block (0,1,2 or 3)
    const long n400Century = (long)((nDaysAbsolute - 1) / 36524L);

    long n4Years;        // Number of 4 year increments since 1/1/0
    long n4Day;          // Day within 4 year block (0 is 1/1/yr1, 1460 is 12/31/yr4)
    bool bLeap4 = true;  // true if 4 year block includes leap year

    // Non-leap century
    // like cTimeUnits::GetLeapYearsSince2K
    if (n400Century != 0) {
        // Set nDaysAbsolute to day within century
        nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

        // +1 because 1st 4 year increment has 1460 days
        n4Years = (long)((nDaysAbsolute + 1) / 1461L);

        if (n4Years != 0)
            n4Day = (long)((nDaysAbsolute + 1) % 1461L);
        else {
            bLeap4 = false;
            n4Day = (long)nDaysAbsolute;
        }
    } else {
        // Leap century - not special case!
        n4Years = (long)(nDaysAbsolute / 1461L);
        n4Day = (long)(nDaysAbsolute % 1461L);
    }

    long n4Yr;  // Year within 4 year block (0,1,2 or 3)
    if (bLeap4) {
        // -1 because first year has 366 days
        n4Yr = (n4Day - 1) / 365;
        if (n4Yr != 0) n4Day = (n4Day - 1) % 365;
    } else {
        n4Yr = n4Day / 365;
        n4Day %= 365;
    }

    // n4Day is now 0-based day of year. Save 1-based day of year, year number
    rTu._wYear = (WORD)(n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr);

    // Handle leap year: before, on, and after Feb. 29.
    if (n4Yr == 0 && bLeap4) {
        // Leap Year
        if (n4Day == 59) {
            // Feb. 29
            rTu._wMonth = 2;
            rTu._wDay = 29;
            return;
        }

        // Pretend it's not a leap year for month/day comp.
        if (n4Day >= 60) --n4Day;
    }

    // Make n4DaY a 1-based day of non-leap year and compute
    //  month/day for everything but Feb. 29.
    ++n4Day;

    // Month number always >= n/32, so save some loop time
    rTu._wMonth = (WORD)((n4Day >> 5) + 1);
    for (; n4Day > cTimeUnits::k_aMonthDaySums[0][rTu._wMonth]; rTu._wMonth++) {
    }

    rTu._wDay = (WORD)(n4Day - cTimeUnits::k_aMonthDaySums[0][rTu._wMonth - 1]);
}

bool cTimeDouble::GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone) const {
    //! Convert to cTimeUnits as TZ_UTC = TZ_GMT = NOT localized system time.
    //! Number of days since Dec. 30, 1899
    //! VariantTimeToSystemTime
    //! @return
    //!  month = 1-12
    //!  day =

    double dTime = _dTimeDays;

    DecodeDate(rTu, dTime);

    // DecodeTime
    dTime -= ((int)_dTimeDays);  // get time portion only.

    int secs = int(dTime * cTimeUnits::k_nSecondsPerDay + 0.5);
    rTu._wHour = short(secs / (60 * 60));
    secs = secs % (60 * 60);
    rTu._wMinute = short(secs / 60);
    rTu._wSecond = short(secs % 60);

    rTu._wMillisecond = 0;  // lost milliseconds ???
    rTu._wMicrosecond = 0;

    rTu.AddTZ(nTimeZone);
    return true;
}

bool cTimeDouble::InitTimeUnits(const cTimeUnits& rTu) {
    if (!rTu.isValidTimeUnits()) return false;

    _dTimeDays = EncodeDate(rTu._wYear, rTu._wMonth, rTu._wDay);
    if (isTimeValid()) {
        _dTimeDays += EncodeTime(rTu._wHour, rTu._wMinute, rTu._wSecond, rTu._wMillisecond).get_Double();
    }
    if (rTu._nTZ != TZ_UTC) {
        // adjust timezone.
        TIMEVALU_t nTimeZoneOffset = rTu._nTZ;
        if (nTimeZoneOffset > TZ_MAX) {
            nTimeZoneOffset = cTimeZoneMgr::GetLocalMinutesWest();
        }
        _dTimeDays += (nTimeZoneOffset / (double)cTimeUnits::k_nMinutesPerDay);
        if (rTu.isInDST1()) {
            _dTimeDays -= (1.0 / 24.0);  // remove added hour.
        }
    }
    return true;
}

//********************************************

cTimeFile cTimeDouble::GetAsFileTime() const noexcept {
    //! convert double time to file system time
    //!  cTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT
    double nTmp = _dTimeDays;
    nTmp += 109205;            // magic offset number = 109205 days = 94353120000000000
    nTmp *= (864000000000.0);  // days to .1uSec , 10M*60*60*24
    return cTimeFile((FILETIME_t)nTmp);
}

cTimeDouble GRAYCALL cTimeDouble::GetTimeNow() noexcept {  // static
    //!  cTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT/UTC
    return GetTimeFromFile(cTimeFile::GetTimeNow());
}

cTimeDouble cTimeDouble::Date() noexcept {  // static
    //! Get just whole days portion for now.
    const double dDays = GetTimeNow();
    return double(int(dDays));
}

cTimeDouble cTimeDouble::Time() noexcept {  // static
    //! Time of day now = 0 to 1
    const double dDays = GetTimeNow();
    return dDays - int(dDays);
}

//*******************************************************************

cTimeDouble GRAYCALL cTimeDouble::GetTimeFromFile(const cTimeFile& ft) noexcept {  // static
    //! cTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT
    //! double = days since (midnight, 30 December 1899 GMT)
    const FILETIME_t tmp = ft.get_Val();
    double dTimeDays = ((double)tmp) / (864000000000.0);  // .1uSec -> days 10M*60*60*24
    dTimeDays -= 109205;                                  // magic offset number = 109205 days = 94353120000000000
    return dTimeDays;
}

cTimeDouble cTimeDouble::GetTimeFromSec(TIMESEC_t nTimeSec) noexcept {  // static
    //! convert TIMESEC_t (seconds) to double (days)
    //! Opposite of cTimeInt::GetTimeFromDays()
    double dTimeDays = (double)nTimeSec;
    dTimeDays /= cTimeUnits::k_nSecondsPerDay;
    dTimeDays += cTimeDouble::k_nDaysDiffTimeInt;
    return dTimeDays;
}

void cTimeDouble::InitTimeNow() {
    _dTimeDays = GetTimeNow();
}

TIMEDOW_t cTimeDouble::get_DayOfWeek() const {
    //! Sunday is 0, TIMEDOW_t
    //! MFC does (sun=1) but we don't
    const int idays = (int)_dTimeDays;
    return CastN(TIMEDOW_t, (idays - 1) % static_cast<int>(TIMEDOW_t::_QTY));
}

//***********************************************************************
// string routines

HRESULT cTimeDouble::SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
    //! Set the time/date from a string date time .
    //! Attempt to determine format from the string/numbers given.

    if (pszDateTime == nullptr) return E_POINTER;

    if (!StrT::CmpI(pszDateTime, _GT("now"))) {
        InitTimeNow();  // Sets the current date & time
        return 3;
    }
    if (!StrT::CmpI(pszDateTime, _GT("today"))) {
        _dTimeDays = Date();  // Sets the current date
        return 5;
    }
    if (!StrT::CmpI(pszDateTime, _GT("time"))) {
        _dTimeDays = Time();  // Sets the current time
        return 4;
    }

    cTimeUnits Tu;
    const HRESULT hRes = Tu.SetTimeStr(pszDateTime, nTimeZone);
    if (hRes <= 0) {
        _dTimeDays = 0;
        return 0;
    }

    _dTimeDays = cTimeDouble(Tu);
    return hRes;
}

cString cTimeDouble::GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZone) const {
    //! Get the time as a string formatted using "C" strftime()
    //! MFC just calls this "Format"
    cTimeUnits Tu;
    if (!GetTimeUnits(Tu, nTimeZone)) return "";

    GChar_t szBuffer[256];
    const StrLen_t iLenChars = Tu.GetFormStr(TOSPAN(szBuffer), pszFormat);
    if (iLenChars <= 0) return "";

    return ToSpan(szBuffer, iLenChars);
}

//*************************************************

cString GRAYCALL cTimeDouble::GetTimeSpanStr(double dDays, TIMEUNIT_t eUnitHigh, int iUnitsDesired, bool bShortText) {  // static
    //! Describe a delta/span of time from days to milliseconds.
    //! iUnitHigh = 0 = days.
    //! @todo MERGE THIS WITH cTimeUnits::GetTimeSpanStr version?!

    if (dDays <= 0) return bShortText ? _GT("0s") : _GT("0 seconds");
    if (iUnitsDesired < 1) iUnitsDesired = 1;  // must have at least 1.

    static constexpr UINT kUnitMax = CastN(int, TIMEUNIT_t::_Millisecond); 
    if (IS_INDEX_BAD(eUnitHigh, kUnitMax+1)) {
        eUnitHigh = TIMEUNIT_t::_Day;  // days is highest unit by default. // Just take a default.
    }

    int iUnitsPrinted = 0;
    GChar_t szMsg[256];
    StrBuilder<GChar_t> sb(TOSPAN(szMsg));

    bool bMostSignificantFound = false;
    UINT i = CastN(UINT, eUnitHigh);
    for (; i <= kUnitMax; i++) {
        const double dUnits = cTimeUnits::k_aUnits[i]._dUnitDays;
        if (!bMostSignificantFound) {
            if (dDays < dUnits) continue;
            bMostSignificantFound = true;
        }

        if (i >= static_cast<int>(TIMEUNIT_t::_Second)) break;  // sub seconds print as a decimal _Millisecond.

        const int nQtyOfUnit = (int)(dDays / dUnits);  // same as ::floor()

        sb.AddSep(' ');  // " and ";

        if (bShortText) {
            sb.Printf(_GT("%u%s"), nQtyOfUnit, StrArg<GChar_t>(cTimeUnits::k_aUnits[i]._pszUnitNameS));
        } else if (nQtyOfUnit == 1) {
            sb.AddStr(_GT("1 "));
            sb.AddStr(cTimeUnits::k_aUnits[i]._pszUnitNameL);
        } else {
            sb.Printf(_GT("%u %ss"), nQtyOfUnit, StrArg<GChar_t>(cTimeUnits::k_aUnits[i]._pszUnitNameL));
        }

        if (++iUnitsPrinted >= iUnitsDesired) break;  // only print iUnitsDesired most significant units of time

        dDays -= nQtyOfUnit * dUnits;
        if (dDays <= 0) break;
    }

    if (iUnitsDesired > 0 && iUnitsPrinted < iUnitsDesired && dDays > 0 && i >= static_cast<int>(TIMEUNIT_t::_Second)) {
        // remainder is always decimal _Millisecond.
        sb.Printf(_GT(" %g %s%s"), dDays / cTimeUnits::k_aUnits[i]._dUnitDays, bShortText ? cTimeUnits::k_aUnits[i]._pszUnitNameS : cTimeUnits::k_aUnits[i]._pszUnitNameL,
                  bShortText ? "" : "s"  // plural.
        );
    }

    return szMsg;
}
}  // namespace Gray
