//
//! @file cTimeUnits.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "StrChar.h"
#include "StrT.h"
#include "cBits.h"
#include "cTimeInt.h"
#include "cTimeUnits.h"
#include "cTimeZone.h"
#ifdef __linux__
#include "cTimeVal.h"
#endif

namespace Gray {
// Stock date time string formats.
const GChar_t cTimeUnits::k_SepsAll[8] = _GT("/ :T.,-");  // All/Any separator that might occur in k_StrFormats.

const GChar_t* cTimeUnits::k_StrFormats[static_cast<int>(TIMEFORMAT_t::_QTY) + 1] = {
    //! strftime() type string formats.
    //! @todo USE k_TimeSeparator

    _GT("%Y/%m/%d %H:%M:%S"),         // TIMEFORMAT_t::_DEFAULT = default Sortable/linear format. "2008/07/09 13:47:10"
    _GT("%Y-%m-%d %H:%M:%S"),         // TIMEFORMAT_t::_DB = Sorted time = "2008-04-10 13:30:00"
    _GT("%Y-%m-%d %H:%M:%S %Z"),      // _DEFTZ = Sorted Universal/GMT time = "2008-04-10 13:30:00Z"
    _GT("%m/%d/%Y %H:%M:%S"),         // TIMEFORMAT_t::_AMERICAN = "07/19/2008 13:47:10"
    _GT("%a, %d %b %Y %H:%M:%S %z"),  // TIMEFORMAT_t::_HTTP = RFC1123 format "Tue, 03 Oct 2000 22:44:56 GMT"
    _GT("%d %b %Y %H:%M:%S %z"),      // TIMEFORMAT_t::_SMTP = SMTP wants this format. "7 Aug 2001 10:12:12 GMT"
    _GT("%Y/%m/%dT%H:%M:%S"),         // TIMEFORMAT_t::_ISO
    _GT("%Y/%m/%dT%H:%M:%S%z"),       // TIMEFORMAT_t::_ISO_TZ
    _GT("%Y%m%d%H%M%S%z"),            // TIMEFORMAT_t::_ASN

    // 01/06/2016, 11:45 AM (-03:00)

    nullptr,
};

const cTimeUnit cTimeUnits::k_Units[static_cast<int>(TIMEUNIT_t::_QTY)] = {
    {_GT("year"), _GT("Y"), 1, 3000, 12, 365 * 24 * 60 * 60, 365.25},  // approximate, depends on leap year.
    {_GT("month"), _GT("M"), 1, 12, 30, 30 * 24 * 60 * 60, 30.43},     // approximate, depends on month
    {_GT("day"), _GT("d"), 1, 31, 24, 24 * 60 * 60, 1.0},
    {_GT("hour"), _GT("h"), 0, 23, 60, 60 * 60, 1.0 / (24.0)},
    {_GT("minute"), _GT("m"), 0, 59, 60, 60, 1.0 / (24.0 * 60.0)},
    {_GT("second"), _GT("s"), 0, 59, 1000, 1, 1.0 / (24.0 * 60.0 * 60.0)},
    {_GT("millisec"), _GT("ms"), 0, 999, 1000, 0, 1.0 / (24.0 * 60.0 * 60.0 * 1000.0)},
    {_GT("microsec"), _GT("us"), 0, 999, 0, 0, 1.0 / (24.0 * 60.0 * 60.0 * 1000.0 * 1000.0)},
    {_GT("TZ"), _GT("TZ"), -24 * 60, 24 * 60, 0, 0, 1.0},  // TIMEUNIT_t::_TZ
};

const BYTE cTimeUnits::k_MonthDays[2][static_cast<int>(TIMEMONTH_t::_QTY)] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},  // normal year // Jan=0
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}   // leap year
};

const WORD cTimeUnits::k_MonthDaySums[2][static_cast<int>(TIMEMONTH_t::_QTY) + 1] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},  // normal year // Jan=0
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}   // leap year
};

const GChar_t* const cTimeUnits::k_MonthName[static_cast<int>(TIMEMONTH_t::_QTY) + 1] = {  // Jan=0
    _GT("January"), _GT("February"), _GT("March"), _GT("April"), _GT("May"), _GT("June"), _GT("July"), _GT("August"), _GT("September"), _GT("October"), _GT("November"), _GT("December"), nullptr};

const GChar_t* const cTimeUnits::k_MonthAbbrev[static_cast<int>(TIMEMONTH_t::_QTY) + 1] = {
    _GT("Jan"), _GT("Feb"), _GT("Mar"), _GT("Apr"), _GT("May"), _GT("Jun"), _GT("Jul"), _GT("Aug"), _GT("Sep"), _GT("Oct"), _GT("Nov"), _GT("Dec"), nullptr,
};

const GChar_t* const cTimeUnits::k_DayName[static_cast<int>(TIMEDOW_t::_QTY) + 1] = {  // Sun=0
    _GT("Sunday"), _GT("Monday"), _GT("Tuesday"), _GT("Wednesday"), _GT("Thursday"), _GT("Friday"), _GT("Saturday"), nullptr};

const GChar_t* const cTimeUnits::k_DayAbbrev[static_cast<int>(TIMEDOW_t::_QTY) + 1] = {  // Sun=0
    _GT("Sun"), _GT("Mon"), _GT("Tue"), _GT("Wed"), _GT("Thu"), _GT("Fri"), _GT("Sat"), nullptr};

const GChar_t cTimeUnits::k_Seps[3] = _GT("/:");  // Normal date string separators. "/:"

GChar_t cTimeUnits::sm_DateSeparator = '/';  /// might be . for Germans,
bool cTimeUnits::sm_time24Mode = false;

//******************************************************************************************

#ifdef _WIN32
cTimeUnits::cTimeUnits(const SYSTEMTIME& sysTime)
    : m_wYear(sysTime.wYear),
      m_wMonth(sysTime.wMonth), // 1 based.
      m_wDay(sysTime.wDay),
      m_wHour(sysTime.wHour),
      m_wMinute(sysTime.wMinute),
      m_wSecond(sysTime.wSecond),
      m_wMillisecond(sysTime.wMilliseconds),
      m_wMicrosecond(0),
      m_nTZ(0) {
    ASSERT(isValidTimeUnits());
}

bool cTimeUnits::GetSys(SYSTEMTIME& sysTime) const noexcept {
    sysTime.wYear = m_wYear;
    sysTime.wMonth = m_wMonth;             // 1 based.
    sysTime.wDayOfWeek = (WORD)get_DOW();  // Sunday - [0,6] TIMEDOW_t
    sysTime.wDay = m_wDay;
    sysTime.wHour = m_wHour;
    sysTime.wMinute = m_wMinute;
    sysTime.wSecond = m_wSecond;
    sysTime.wMilliseconds = m_wMillisecond;
    return true;
}
void cTimeUnits::SetSys(const SYSTEMTIME& sysTime) {
    m_wYear = sysTime.wYear;
    m_wMonth = sysTime.wMonth;  // 1 based.
    m_wDay = sysTime.wDay;
    m_wHour = sysTime.wHour;
    m_wMinute = sysTime.wMinute;
    m_wSecond = sysTime.wSecond;
    m_wMillisecond = sysTime.wMilliseconds;
    m_wMicrosecond = 0;
    ASSERT(isValidTimeUnits());
}
#endif

void cTimeUnits::SetZeros() {
    cMem::Zero(&m_wYear, static_cast<int>(TIMEUNIT_t::_QTY) * sizeof(m_wYear));
    m_wYear = 1;  // m_uMin
    m_wMonth = 1;
    m_wDay = 1;
}

bool cTimeUnits::InitTimeNow(TZ_TYPE nTimeZone) {
    //! Get the current time, and adjust units for timezone. nDST ??
    //! like _WIN32 GetLocalTime(st), GetSystemTime(st)
    cTimeInt t;
    t.InitTimeNow();
    return t.GetTimeUnits(*this, nTimeZone);
}

COMPARE_TYPE cTimeUnits::Compare(cTimeUnits& b) const {
    //! Compare relevant parts of 2 times.

    if (b.m_nTZ != this->m_nTZ) {
        //! @note TODO DOES NOT FACTOR TIMEUNIT_t::_TZ
    }

    for (int i = static_cast<int>(TIMEUNIT_t::_Year); i <= static_cast<int>(TIMEUNIT_t::_Microsecond); i++) {  // TIMEUNIT_t
        const TIMEVALU_t nThis = GetUnitVal((TIMEUNIT_t)i);
        const TIMEVALU_t nB = b.GetUnitVal((TIMEUNIT_t)i);
        if (nThis != nB) {
            return (nThis > nB) ? COMPARE_Greater : COMPARE_Less;
        }
    }
    return COMPARE_Equal;
}

bool cTimeUnits::isTimeFuture() const {
    cTimeUnits tNow;
    tNow.InitTimeNow((TZ_TYPE)m_nTZ);
    return Compare(tNow) >= COMPARE_Greater;
}

bool cTimeUnits::IsValidUnit(TIMEUNIT_t i) const {
    return GetUnitDef(i).IsInRange(GetUnitVal(i));
}

bool cTimeUnits::isValidTimeUnits() const {
    //! Are the values in valid range ?
    //! @note If we are just using this for time math values may go out of range ?
    if (!isValidMonth()) return false;
    if (m_wDay < 1 || m_wDay > get_DaysInMonth()) return false;
    if (((UINT)m_wHour) > 23) return false;
    if (((UINT)m_wMinute) > 59) return false;
    if (((UINT)m_wSecond) > 59) return false;
    return true;
}

bool cTimeUnits::isReasonableTimeUnits() const {
    //! Is this data reasonable for most purposes?
    if (m_wYear < 1900) return false;
    if (m_wYear > 2500) return false;
    return isValidTimeUnits();
}

int GRAYCALL cTimeUnits::IsLeapYear(TIMEVALU_t wYear) {  // static
    //! 0 or 1 NOT Boolean - for array access.
    //! Every year divisible by 4 is a leap year.
    //! But every year divisible by 100 is NOT a leap year
    //! Unless the year is also divisible by 400, then it is still a leap year.

    if ((wYear & 3) != 0)  // not multiple of 4 = not leap year.
        return 0;
    if ((wYear % 100) == 0)  // multiple of 100. i.e. 1900 is not a leap year.
    {
        if ((wYear % 400) == 0)  // multiple of 400. i.e. 2000 is a leap year.
            return 1;
        return 0;
    }
    return 1;
}

int GRAYCALL cTimeUnits::GetLeapYearsSince2K(TIMEVALU_t wYear) {  // static
    //! calculate the number of leap days/years since Jan 1 of a year to Jan 1 2000.
    //! (Jan 1 2000 = TIMEDOW_Sat) to Jan 1 wYear
    //! can be negative if wYear < 2000
    int iYear = wYear - 2000;
    int iDays = (iYear > 0) ? 1 : 0;
    iYear -= iDays;
    iDays += iYear / 4;    // add multiple 4 year days
    iDays += iYear / 400;  // add multiple 400 years days.
    iDays -= iYear / 100;  // remove multiple 100 years days
    return iDays;
}

/// <summary>
/// day of week for a particular date.
/// </summary>
/// <param name="wYear">2022</param>
/// <param name="wMonth">wMonth = 1 based</param>
/// <param name="wDay">Day of month is 1 based</param>
/// <returns>TIMEDOW_t, 0 = Sunday.</returns>
TIMEDOW_t GRAYCALL cTimeUnits::GetDOW(TIMEVALU_t wYear, TIMEVALU_t wMonth, TIMEVALU_t wDay) {  // static
    ASSERT(wMonth > 0 && wMonth <= 12);
    ASSERT(wDay > 0);
    int iDays = (wYear - 2000);           // should be *365 but since (364%7)==0 we can omit that.
    iDays += GetLeapYearsSince2K(wYear);  // can be negative if wYear < 2000
    iDays += k_MonthDaySums[IsLeapYear(wYear)][wMonth - 1];
    iDays += (wDay - 1) + static_cast<int>(TIMEDOW_t::_Sat);  // (Jan 1 2000 = Sat)
    iDays %= static_cast<int>(TIMEDOW_t::_QTY);               // mod 7
    if (iDays < 0) {
        iDays += static_cast<int>(TIMEDOW_t::_QTY);
    }
    return (TIMEDOW_t)iDays;
}

int GRAYCALL cTimeUnits::GetDOY(TIMEVALU_t wYear, TIMEVALU_t wMonth, TIMEVALU_t wDay) {  // static
    //! Day of the year. 0 to 365
    //! wMonth = 1 based
    //! wDay = 1 based.
    ASSERT(wMonth > 0 && wMonth <= 12);
    return k_MonthDaySums[IsLeapYear(wYear)][wMonth - 1] + wDay - 1;
}

bool cTimeUnits::isInDST1() const {
    //! Is this date in US DST range? Assuming local time zone honors US DST.
    //! http://www.worldtimezone.com/daylight.html
    //! like the C internal function _isindst(const struct tm *tb)
    //! @note
    //!  rule for years < 1987:
    //!  begin after 2 AM on the last Sunday in April
    //!  end 1 AM on the last Sunday in October.
    //!
    //!  rule for years >= 1987:
    //!  begin after 2 AM on the first Sunday in April
    //!  end 1 AM on the last Sunday in October.
    //!
    //!  rule for years >= 2007:
    //!  begin Second Sunday in March 2AM
    //!  end First Sunday in November 2AM

    TIMEVALU_t wMonthLo;
    TIMEVALU_t wMonthHi;
    if (m_wYear >= 2007) {
        // New US idiot rules.
        wMonthLo = 3;
        wMonthHi = 11;
    } else {
        wMonthLo = 4;
        wMonthHi = 10;
    }

    // If the month is before April or after October, then we know immediately it can't be DST.
    if (m_wMonth < wMonthLo || m_wMonth > wMonthHi) return false;
    // If the month is after April and before October then we know immediately it must be DST.
    if (m_wMonth > wMonthLo && m_wMonth < wMonthHi) return true;

    // Month is April or October see if date falls between appropriate Sundays.
    bool bLow = (m_wMonth < 6);

    // What day (of the month) is the Sunday of interest?
    TIMEVALU_t wHour;
    int iSunday;
    if (m_wYear < 1987) {
        iSunday = 3;  // always last Sunday
        wHour = (bLow) ? 2 : 1;
    } else if (m_wYear < 2007) {
        iSunday = (bLow) ? 1 : 3;  // first or last.
        wHour = (bLow) ? 2 : 1;
    } else { // >= 2007
        iSunday = (bLow) ? 2 : 1;  // second or first.
        wHour = 2;
    }

    int iDayMin;
    switch (iSunday) {
        case 1:  // First Sunday of the month
            iDayMin = 1;
            break;
        case 2:  // Second Sunday of the month
            iDayMin = 8;
            break;
        default:  // Last Sunday in month
            iDayMin = k_MonthDays[IsLeapYear(m_wYear)][m_wMonth - 1] - 6;
            break;
    }

    TIMEDOW_t eDOW = GetDOW(m_wYear, m_wMonth, (TIMEVALU_t)iDayMin);  // sun = 0
    iSunday = iDayMin + ((7 - static_cast<int>(eDOW)) % 7);           // the next Sunday.

    if (bLow) {
        return (m_wDay > iSunday || (m_wDay == iSunday && m_wHour >= wHour));
    } else {
        return (m_wDay < iSunday || (m_wDay == iSunday && m_wHour < wHour));
    }
}

//******************************************************************

void cTimeUnits::put_DosDate(UINT32 ulDosDate) {
    //! unpack 32 bit DosDate format. for ZIP files and old FAT file system.
    //! we could use DosDateTimeToFileTime and LocalFileTimeToFileTime for _WIN32
    //!  0-4 =Day of the month (1-31)
    //!  5-8 =Month (1 = January, 2 = February, and so on)
    //!  9-15 =Year offset from 1980 (add 1980 to get actual year) . 6 bits = 63 years = 2043 failure.
    WORD wFatDate = HIWORD(ulDosDate);

    //  0-4 =Second divided by 2
    //  5-10 =Minute (0-59)
    //  11-15 =Hour (0-23 on a 24-hour clock)
    WORD wFatTime = LOWORD(ulDosDate);

    this->m_wSecond = (TIMEVALU_t)(2 * (wFatTime & 0x1f));  // 2 second accurate.
    this->m_wMinute = (TIMEVALU_t)((wFatTime >> 5) & 0x3f);
    this->m_wHour = (TIMEVALU_t)((wFatTime >> 11));

    this->m_wDay = (TIMEVALU_t)(wFatDate & 0x1f);
    this->m_wMonth = (TIMEVALU_t)((wFatDate >> 5) & 0x0f);
    this->m_wYear = (TIMEVALU_t)(1980 + (wFatDate >> 9));  // up to 2043
}

UINT32 cTimeUnits::get_DosDate() const {
    //! get/pack a 32 bit DOS date format. for ZIP files. and old FAT file system.
    //! ASSUME isValidTimeUnits().
    UINT32 year = (UINT32)this->m_wYear;  // 1980 to 2043
    if (year > 1980)
        year -= 1980;
    else if (year > 80)
        year -= 80;
    return (UINT32)(((this->m_wDay) + (32 * (this->m_wMonth)) + (512 * year)) << 16) | ((this->m_wSecond / 2) + (32 * this->m_wMinute) + (2048 * (UINT32)this->m_wHour));
}

void cTimeUnits::AddMonths(int iMonths) {
    //! Add months to this structure. months are not exact time measures, but there are always 12 per year.
    //! @arg iMonths = 0 based. Can be <0
    iMonths += m_wMonth - 1;
    m_wMonth = (TIMEVALU_t)(1 + (iMonths % 12));
    m_wYear = (TIMEVALU_t)(m_wYear + (iMonths / 12));  // adjust years.
}

void cTimeUnits::AddDays(int iDays) {
    //! Add Days. Adjust for the fact that months and years are not all the same number of days.
    //! @arg iDays can be negative.
    //! Do we cross years ?
    for (;;) {
        // Use GetLeapYearsSince2K to optimize this? don't bother, its never that big.
        int iDayOfYear = get_DayOfYear();
        int iDaysInYear = get_DaysInYear();
        int iDays2 = iDayOfYear + iDays;
        if (iDays2 >= iDaysInYear) {  // advance to next year.
            ASSERT(iDays > 0);
            iDays -= iDaysInYear - iDayOfYear;
            m_wYear++;
            m_wMonth = 1;
            m_wDay = 1;
        } else if (iDays2 < 0) { // previous year.
            ASSERT(iDays < 0);
            iDays += iDaysInYear;
            m_wYear--;
            m_wMonth = 12;
            m_wDay = 31;
        } else
            break;
    }
    // Do we cross months?
    for (;;) {
        int iDayOfMonth = (m_wDay - 1);
        int iDaysInMonth = get_DaysInMonth();
        int iDays2 = iDayOfMonth + iDays;
        if (iDays2 >= iDaysInMonth) { // next month.
            ASSERT(iDays > 0);
            iDays -= iDaysInMonth - iDayOfMonth;
            m_wMonth++;
            m_wDay = 1;
        } else if (iDays2 < 0) {  // previous month
            ASSERT(iDays < 0);
            iDays += iDayOfMonth;
            m_wMonth--;
            m_wDay = get_DaysInMonth();
        } else {
            m_wDay += (TIMEVALU_t)iDays;
            break;
        }
    }
}

void cTimeUnits::AddSeconds(TIMESECD_t nSeconds) {
    //! Add TimeUnits with seconds. handles very large values of seconds.
    //! Used to adjust for TZ and DST.
    //! nSeconds can be negative.

    int nSecondOfDay = get_SecondOfDay();
    int nSeconds2 = nSeconds + nSecondOfDay;
    int iDays = 0;
    if (nSeconds2 >= k_nSecondsPerDay)  // Test days as special case since months are not uniform.
    {
        // Cross into next day(s). TIMEUNIT_t::_Day
        ASSERT(nSeconds > 0);
        iDays = nSeconds2 / k_nSecondsPerDay;
    do_days:
        AddDays(iDays);
        nSeconds2 -= iDays * k_nSecondsPerDay;
    } else if (nSeconds2 < 0) {
        // Back to previous day(s). TIMEUNIT_t::_Day
        ASSERT(nSeconds < 0);
        iDays = (nSeconds2 / k_nSecondsPerDay) - 1;
        goto do_days;
    }

    int iTicksA = cValT::Abs(nSeconds2);
    ASSERT(iTicksA < k_nSecondsPerDay);
    int iUnitDiv = 60 * 60;

    for (int i = static_cast<int>(TIMEUNIT_t::_Hour); i <= static_cast<int>(TIMEUNIT_t::_Second); i++) {
        TIMEVALU_t iUnits = (TIMEVALU_t)(nSeconds2 / iUnitDiv);
        SetUnit((TIMEUNIT_t)i, iUnits);
        ASSERT(IsValidUnit((TIMEUNIT_t)i));
        nSeconds2 -= iUnits * iUnitDiv;
        iUnitDiv /= 60;
    }
}

void cTimeUnits::AddTZ(TZ_TYPE nTimeZone) {
    //! add TZ Offset in minutes.
    //! TODO Get rid of this and replace with SetTZ() and do the offset in advance.

    TIMEVALU_t offset = CastN(TIMEVALU_t, nTimeZone);
    if (nTimeZone == TZ_LOCAL) {
        offset = cTimeZoneMgr::GetLocalMinutesWest();
    }

    m_nTZ = offset;
    if (nTimeZone == TZ_UTC) return;  // adjust for timezone and DST, TZ_GMT = 0

    if (isInDST1()) offset -= 60;  // subtract hour.

    // adjust for TZ/DST offsets. may be new day or year ?
    AddSeconds(-(offset * 60));
}

//******************************************************************

StrLen_t cTimeUnits::GetTimeSpanStr(GChar_t* pszOut, StrLen_t iOutSizeMax, TIMEUNIT_t eUnitHigh, int iUnitsDesired, bool bShortText) const {
    //! A delta/span time string. from years to milliseconds.
    //! Get a text description of amount of time span (delta)
    //! @arg
    //!  eUnitHigh = the highest unit, TIMEUNIT_t::_Day, TIMEUNIT_t::_Minute
    //!  iUnitsDesired = the number of units up the cTimeUnits::k_Units ladder to go. default=2
    //! @return
    //!  length of string in chars

    if (iUnitsDesired < 1) {
        iUnitsDesired = 1;  // must have at least 1.
    }
    if (IS_INDEX_BAD_ARRAY(eUnitHigh, cTimeUnits::k_Units)) {
        eUnitHigh = TIMEUNIT_t::_Day;  // days is highest unit by default. months is not accurate!
    }

    int iUnitsPrinted = 0;
    StrLen_t iOutLen = 0;
    UINT64 nUnits = 0;
    int i = static_cast<int>(TIMEUNIT_t::_Year);  // 0
    for (; i < static_cast<int>(eUnitHigh); i++) {
        nUnits = (nUnits + GetUnit0((TIMEUNIT_t)i)) * k_Units[i].m_uSubRatio;
    }

    for (; i < static_cast<int>(TIMEUNIT_t::_Microsecond); i++) {  // highest to lowest.
        nUnits += GetUnit0((TIMEUNIT_t)i);
        if (!nUnits) continue;  // just skip empty units.

        if (iOutLen) {
            iOutLen += StrT::CopyLen(pszOut + iOutLen, _GT(" "), iOutSizeMax - iOutLen);  // " and ";
        }
        if (bShortText) {
            iOutLen += StrT::sprintfN(pszOut + iOutLen, iOutSizeMax - iOutLen, _GT("%u%s"), (int)nUnits, StrArg<GChar_t>(cTimeUnits::k_Units[i].m_pszUnitNameS));
        } else if (nUnits == 1) {
            iOutLen += StrT::CopyLen(pszOut + iOutLen, _GT("1 "), iOutSizeMax - iOutLen);
            iOutLen += StrT::CopyLen(pszOut + iOutLen, cTimeUnits::k_Units[i].m_pszUnitNameL, iOutSizeMax - iOutLen);
        } else {
            iOutLen += StrT::sprintfN(pszOut + iOutLen, iOutSizeMax - iOutLen, _GT("%u %ss"), (int)nUnits, cTimeUnits::k_Units[i].m_pszUnitNameL);
        }

        if (++iUnitsPrinted >= iUnitsDesired)  // only print iUnitsDesired most significant units of time
            break;
        nUnits = 0;
    }

    if (iUnitsPrinted == 0) {
        // just 0
        iOutLen = StrT::CopyLen(pszOut, bShortText ? _GT("0s") : _GT("0 seconds"), iOutSizeMax);
    }

    return iOutLen;
}

StrLen_t cTimeUnits::GetFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, const GChar_t* pszFormat) const {
    //! Get the time as a formatted string using "C" strftime()
    //!  build formatted string from cTimeUnits.
    //!  similar to C stdlib strftime() http://linux.die.net/man/3/strftime
    //!  add TZ as postfix if desired??
    //!  used by cTimeDouble::GetTimeFormStr and cTimeInt::GetTimeFormStr
    //! @return
    //!  length of string in chars. <= 0 = failed.

    if (PtrCastToNum(pszFormat) < static_cast<int>(TIMEFORMAT_t::_QTY)) {  // IS_INTRESOURCE()
        pszFormat = k_StrFormats[PtrCastToNum(pszFormat)];
    }

    GChar_t szTmp[2];

    StrLen_t iOut = 0;
    for (StrLen_t i = 0; iOut < iOutSizeMax; i++) {
        GChar_t ch = pszFormat[i];
        if (ch == '\0') break;
        if (ch != '%') {
            pszOut[iOut++] = ch;
            continue;
        }

        ch = pszFormat[++i];
        if (ch == '\0') break;
        if (ch == '#')  // As in the printf function, the # flag may prefix/modify any formatting code
        {
            ch = pszFormat[++i];
            if (ch == '\0') break;
        }

        const GChar_t* pszVal = nullptr;
        int iValPad = 0;
        short wVal = 0;
        switch (ch) {
            case '/':
                szTmp[0] = sm_DateSeparator;
                szTmp[1] = '\0';
                pszVal = szTmp;
                break;
            case '%':
                pszVal = _GT("%");
                break;

            case 'y':  // Year without century, as decimal number (00 to 99)
                wVal = m_wYear % 100;
                iValPad = 2;
                break;
            case 'Y':  // Year with century, as decimal number
                wVal = m_wYear;
                iValPad = 0;
                break;

            case 'b':  // Abbreviated month name
            case 'h':
                if (m_wMonth == 0) break;
                pszVal = k_MonthAbbrev[m_wMonth - 1];
                break;
            case 'B':  // Full month name
                if (m_wMonth == 0) break;
                pszVal = k_MonthName[m_wMonth - 1];
                break;
            case 'm':  // Month as decimal number (01 to 12)
                wVal = m_wMonth;
                iValPad = 2;
                break;
            case 'd':  // Day of month as decimal number (01 to 31)
                wVal = m_wDay;
                iValPad = 2;
                break;

            case 'a':
                pszVal = k_DayAbbrev[static_cast<int>(get_DOW())];
                break;
            case 'A':
                pszVal = k_DayName[static_cast<int>(get_DOW())];
                break;
            case 'w':  // Weekday as decimal number (0 to 6; Sunday is 0)
                wVal = static_cast<WORD>(get_DOW());
                iValPad = 0;
                break;
            case 'j':  // Day of year as decimal number (001 to 366)
                wVal = static_cast<WORD>(get_DOW());
                iValPad = 3;
                break;

            case 'H':  // Hour in 24-hour format (00 to 23)
                wVal = m_wHour;
                iValPad = 2;
                break;
            case 'k':
                wVal = m_wHour;
                iValPad = 0;
                break;
            case 'I':  // Hour in 12-hour format (01 to 12)
                wVal = (m_wHour % 12);
                iValPad = 2;
                if (!wVal) wVal = 12;
                break;
            case 'p':  // Current locale's A.M./P.M. indicator for 12-hour clock
                pszVal = (m_wHour < 12) ? _GT("AM") : _GT("PM");
                break;
            case 'M':  // Minute as decimal number (00 to 59)
                wVal = m_wMinute;
                iValPad = 2;
                break;
            case 'S':  // Second as decimal number (00 to 59)
                wVal = m_wSecond;
                iValPad = 2;
                break;

            case 'Z': {  // Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZone((TZ_TYPE)m_nTZ);
                if (pTZ != nullptr) {
                    pszVal = pTZ->m_pszTimeZoneName;
                } else {
                    pszVal = _GT("");  // +00 for timezone.
                }
                break;
            }

            case 'z': {
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZone((TZ_TYPE)m_nTZ);
                if (pTZ != nullptr) {
                    pszVal = pTZ->m_pszTimeZoneName;
                } else {
                    pszVal = _GT("");
                }
                break;
            }

                // TIMEUNIT_t::_Millisecond
                // TIMEUNIT_t::_Microsecond

            case 'F':  // equiv to "%Y-%m-%d" for ISO
            case 'c':  // Date and time representation appropriate for locale. NOT SUPPORTED.
            case 'U':  // Week of year as decimal number, with Sunday as first day of week (00 to 53)
            case 'W':  // Week of year as decimal number, with Monday as first day of week (00 to 53)
            case 'x':  // Date representation for current locale
            case 'X':  // Time representation for current locale
            default:
                ASSERT(0);
                break;
        }

        GChar_t* pszOutCur = pszOut + iOut;
        StrLen_t iOutSizeLeft = iOutSizeMax - iOut;
        if (pszVal != nullptr) {
            iOut += StrT::CopyLen(pszOutCur, pszVal, iOutSizeLeft);
        } else if (iValPad > 0 && iValPad < iOutSizeLeft) {
            // right padded number.
            GChar_t* pszMSD = StrT::ULtoARev(wVal, pszOutCur, iValPad + 1, 10, 'A');
            while (pszMSD > pszOutCur) {
                *(--pszMSD) = '0';
            }
            iOut += iValPad;
        } else {
            iOut += StrT::UtoA(wVal, pszOutCur, iOutSizeLeft);
        }
    }

    pszOut[iOut] = '\0';
    return iOut;
}

//******************************************************************

HRESULT cTimeUnits::SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
    //! set cTimeUnits from a string.
    //! @arg
    //!  pszDateTime = "2008/10/23 12:0:0 PM GMT"
    //!  rnTimeZoneOffset = if found a time zone indicator in the string. do not set if nothing found.
    //! @note
    //!  Must support all regular TIMEFORMAT_t::_QTY types.
    //! @return
    //!  > 0 length = OK, <=0 = doesn't seem like a valid datetime.
    //! e.g. "Sat, 07 Aug 2004 01:20:20", ""
    //! toJSON method: "2012-04-23T18:25:43.511Z" is sortable.

    SetZeros();
    cTimeParser parser;
    HRESULT hRes = parser.ParseString(pszDateTime, nullptr);
    if (FAILED(hRes)) return hRes;
    if (!parser.TestMatches())  // try all formats i know.
        return MK_E_SYNTAX;
    m_nTZ = (TIMEVALU_t)nTimeZone;  // allowed to be overridden by cTimeParser.GetTimeUnits
    hRes = parser.GetTimeUnits(*this);
    if (m_nTZ == TZ_LOCAL) {
        m_nTZ = cTimeZoneMgr::GetLocalMinutesWest();
    }
    return hRes;
}

//******************************************************************************************

StrLen_t cTimeParser::ParseNamedUnit(const GChar_t* pszName) {
    // Get values for named units. TIMEUNIT_t::_Month, TIMEUNIT_t::_TZ or TIMEUNIT_t::_QTY (for DOW)
    ITERATE_t iStart = STR_TABLEFIND_NH(pszName, cTimeUnits::k_MonthName);
    if (iStart >= 0) {
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_Month;
        m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)(iStart + 1);
        return StrT::Len(cTimeUnits::k_MonthName[iStart]);
    }
    iStart = STR_TABLEFIND_NH(pszName, cTimeUnits::k_MonthAbbrev);
    if (iStart >= 0) {
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_Month;
        m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)(iStart + 1);
        return StrT::Len(cTimeUnits::k_MonthAbbrev[iStart]);
    }

    iStart = STR_TABLEFIND_NH(pszName, cTimeUnits::k_DayName);
    if (iStart >= 0) {
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_DOW;  // Temporary for DOW
        m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)iStart;
        return StrT::Len(cTimeUnits::k_DayName[iStart]);
    }
    iStart = STR_TABLEFIND_NH(pszName, cTimeUnits::k_DayAbbrev);
    if (iStart >= 0) {
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_DOW;  // Temporary for DOW
        m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)iStart;
        return StrT::Len(cTimeUnits::k_DayAbbrev[iStart]);
    }

    const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZoneHead(pszName);
    if (pTZ != nullptr) {
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_TZ;
        m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)(pTZ->m_nTimeZoneOffset);
        return StrT::Len(pTZ->m_pszTimeZoneName);
    }

    // AM / PM
    if (!StrT::CmpHeadI(_GT("PM"), pszName)) {
        // Add 12 hours.
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_Hour;
        m_Unit[m_iUnitsParsed].m_nValue = 12;
        return 2;
    }
    if (!StrT::CmpHeadI(_GT("AM"), pszName)) {
        // Add 0 hours.
        m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_Hour;
        m_Unit[m_iUnitsParsed].m_nValue = 0;
        return 2;
    }

    return 0;
}

HRESULT cTimeParser::ParseString(const GChar_t* pszTimeString, const GChar_t* pszSeparators) {
    //! parse the pszTimeString to look for things that look like a date time.
    //! parse 3 types of things: Separators, numbers and unit names (e.g. Sunday).
    //! @return m_iUnitsParsed
    //! @todo parse odd time zone storage .  (-03:00)

    if (pszTimeString == nullptr) return E_POINTER;

    const GChar_t* pszSepFind = nullptr;
    bool bNeedSep = false;
    m_iUnitsParsed = 0;

    if (pszSeparators == nullptr) pszSeparators = cTimeUnits::k_SepsAll;

    int i = 0;
    for (;;) {
        int iStart = i;
        i += StrT::GetNonWhitespaceI(pszTimeString + i);
        GChar_t ch = pszTimeString[i];

        if ((pszSepFind = StrT::FindChar(pszSeparators, ch)) != nullptr)  // its a legal separator char?
        {
        do_sep:
            m_Unit[m_iUnitsParsed].m_iOffsetSep = i;
            m_Unit[m_iUnitsParsed].m_Separator = ch;
            if (!bNeedSep) {
                // Was just empty!? NOT ALLOWED.
                break;
            }
            bNeedSep = false;
            m_iUnitsParsed++;
            if (m_iUnitsParsed >= (int)_countof(m_Unit)) break;
            if (ch == '\0') break;  // done.
                
            i++;
            continue;
        }

        if (bNeedSep) { // must complete the previous first.
            if (iStart == i) {  // needed a space separator but didn't get one.
                if (ch == 'T' && StrChar::IsDigitA(pszTimeString[i + 1])) { // ISO can use this as a separator.
                    goto do_sep;
                }

                //! @todo parse odd time zone storage .  (-03:00)

                // Check for terminating TZ with no separator.
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZoneHead(pszTimeString + i);
                if (pTZ != nullptr) {
                    // Insert fake separator.
                    ch = ' ';
                    i--;
                    goto do_sep;
                }
                break;  // quit.
            }

            // Was whitespace separator.
            m_Unit[m_iUnitsParsed].m_iOffsetSep = iStart;
            m_Unit[m_iUnitsParsed].m_Separator = ' ';
            bNeedSep = false;
            m_iUnitsParsed++;
            if (m_iUnitsParsed >= (int)_countof(m_Unit)) break;
        }
        if (ch == '\0') break;  // done.

        m_Unit[m_iUnitsParsed].Init();
        const GChar_t* pszStart = pszTimeString + i;

        if (StrChar::IsDigitA(ch)) {
            // We found a number. good. use it.
            m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_Numeric;  // this just means its a number. don't care what kind yet. resolve later.
            const GChar_t* pszEnd = nullptr;
            m_Unit[m_iUnitsParsed].m_nValue = (TIMEVALU_t)StrT::toI(pszStart, &pszEnd, 10);
            if (pszStart >= pszEnd || pszEnd == nullptr) break;
            i += StrT::Diff(pszEnd, pszStart);
            bNeedSep = true;
            continue;

        } else if (StrChar::IsAlpha(ch)) {  // specific named units . DOW, TZ, Month.
            int iLen = ParseNamedUnit(pszStart);
            if (iLen <= 0) break;
            i += iLen;
            bNeedSep = true;
            continue;
        }

        // nothing more so stop.
        break;
    }

    // The End. post process.
    // Identify stuff near : as Time. Always in hour:min:sec format.
    int iHourFound = -1;
    int iYearFound = -1;
    int iMonthFound = -1;

    for (i = 0; i < m_iUnitsParsed; i++) {
        if (iMonthFound < 0 && m_Unit[i].m_Type == TIMEUNIT_t::_Month) {
            iMonthFound = i;
        }
        if (iYearFound < 0 && m_Unit[i].m_Type == TIMEUNIT_t::_Numeric && m_Unit[i].m_nValue > 366) {
            // This must be the year.
            iYearFound = i;
            m_Unit[i].m_Type = TIMEUNIT_t::_Year;
        }
        if (iHourFound < 0 && m_Unit[i].m_Type == TIMEUNIT_t::_Numeric && m_Unit[i].m_Separator == ':') {
            iHourFound = i;
            m_Unit[i].m_Type = TIMEUNIT_t::_Hour;
            i++;
            m_Unit[i].m_Type = TIMEUNIT_t::_Minute;
            if (i + 1 < m_iUnitsParsed && m_Unit[i + 1].m_Type == TIMEUNIT_t::_Numeric && m_Unit[i].m_Separator == ':') {
                i++;
                m_Unit[i].m_Type = TIMEUNIT_t::_Second;
                if (i + 1 < m_iUnitsParsed && m_Unit[i + 1].m_Type == TIMEUNIT_t::_Numeric && m_Unit[i].m_Separator == '.') {
                    i++;
                    m_Unit[i].m_Type = TIMEUNIT_t::_Millisecond;
                }
            }
        }
        if (iHourFound >= 0 && m_Unit[i].m_Type == TIMEUNIT_t::_Hour) { // PM ?
            m_Unit[i].m_Type = TIMEUNIT_t::_Ignore;  // Ignore this from now on.
            if (m_Unit[iHourFound].m_nValue < 12) {
                m_Unit[iHourFound].m_nValue += m_Unit[i].m_nValue;  // Add 'PM'
            }
        }
    }

    // Fail is there are type dupes.
    // Find day if month is found ?

    // We are not reading a valid time/date anymore. done. stop.
    m_Unit[m_iUnitsParsed].m_Type = TIMEUNIT_t::_UNUSED;  // end
    return m_iUnitsParsed;
}

TIMEUNIT_t GRAYCALL cTimeParser::GetTypeFromFormatCode(GChar_t ch) {  // static
    switch (ch) {
        case 'y':  // Year without century, as decimal number (00 to 99)
        case 'Y':  // Year with century, as decimal number
            return TIMEUNIT_t::_Year;
        case 'b':  // Abbreviated month name
        case 'h':
        case 'B':  // Full month name
        case 'm':  // Month as decimal number (01 to 12)
            return TIMEUNIT_t::_Month;
        case 'd':  // Day of month as decimal number (01 to 31)
            return TIMEUNIT_t::_Day;

        case 'H':  // Hour in 24-hour format (00 to 23)
        case 'I':  // Hour in 12-hour format (01 to 12)
            return TIMEUNIT_t::_Hour;
        case 'M':  // Minute as decimal number (00 to 59)
            return TIMEUNIT_t::_Minute;
        case 'S':  // Second as decimal number (00 to 59)
            return TIMEUNIT_t::_Second;

        case 'Z':  // Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown
        case 'z':
            return TIMEUNIT_t::_TZ;

        case 'a':
        case 'A':                        // Day of week.
        case 'w':                        // Weekday as decimal number (0 to 6; Sunday is 0)
            return TIMEUNIT_t::_DOW;     // No equiv. ignore.
        case 'j':                        // Day of year as decimal number (001 to 366)
        case 'p':                        // Current locale's A.M./P.M. indicator for 12-hour clock
            return TIMEUNIT_t::_Ignore;  // No equiv. ignore.
    }
    ASSERT(0);
    return TIMEUNIT_t::_UNUSED;  // bad
}

int cTimeParser::FindType(TIMEUNIT_t t) const {
    // is this TIMEUNIT_t already used ?
    for (int i = 0; i < m_iUnitsParsed; i++) {
        if (m_Unit[i].m_Type == t) return i;
    }
    return -1;  // TIMEUNIT_t not used.
}

void cTimeParser::SetUnitFormats(const GChar_t* pszFormat) {
    //! Similar to ParseString but assumes we just want to set units from a format string.
    m_iUnitsParsed = 0;
    int i = 0;
    for (;;) {  // TIMEUNIT_t::_QTY2
        GChar_t ch = pszFormat[i];
        if (ch == '\0') break;
        if (ch != '%') break;
        TIMEUNIT_t eType = GetTypeFromFormatCode(pszFormat[i + 1]);
        m_Unit[m_iUnitsParsed].m_Type = eType;
        if (IS_INDEX_BAD(eType, TIMEUNIT_t::_Numeric))  // this should not happen ?! bad format string!
            break;
        m_Unit[m_iUnitsParsed].m_nValue = -1;  // set later.
        m_Unit[m_iUnitsParsed].m_iOffsetSep = i + 2;
        ch = pszFormat[i + 2];
        m_Unit[m_iUnitsParsed].m_Separator = ch;
        m_iUnitsParsed++;
        if (ch == '\0') break;
        i += 3;
        i += StrT::GetNonWhitespaceI(pszFormat + i);
    }
}

bool GRAYCALL cTimeParser::TestMatchUnit(const cTimeParserUnit& u, TIMEUNIT_t t) {  // static
    ASSERT(IS_INDEX_GOOD(u.m_Type, TIMEUNIT_t::_QTY2));
    ASSERT(IS_INDEX_GOOD(t, TIMEUNIT_t::_Numeric)); 
    if (!cTimeUnits::GetUnitDef(t).IsInRange(u.m_nValue)) return false;
    if (t == u.m_Type) return true;  // exact type match is good.
    // TIMEUNIT_t::_Numeric is parsed wildcard (i don't know yet) type.
    // known types must match. TIMEUNIT_t::_Month or TIMEUNIT_t::_DOW
    if (u.m_Type == TIMEUNIT_t::_Numeric) return true;  // It looks like a match ? I guess.
    return false;                                       // not a match.
}

bool cTimeParser::TestMatchFormat(const cTimeParser& parserFormat, bool bTrimJunk) {
    //! Does parserFormat fit with data in m_Units ?
    //! Does this contain compatible units with parserFormat? if so fix m_Unit types!
    //! @arg bTrimJunk = any unrecognized stuff beyond parserFormat can just be trimmed.

    ASSERT(m_iUnitsParsed <= (int)_countof(m_Unit));
    if (m_iUnitsParsed <= 1) return false;

    int iUnitsMatched = 0;
    for (; iUnitsMatched < m_iUnitsParsed && iUnitsMatched < parserFormat.m_iUnitsParsed; iUnitsMatched++) { // TIMEUNIT_t::_QTY2
        if (!TestMatchUnit(m_Unit[iUnitsMatched], parserFormat.m_Unit[iUnitsMatched].m_Type))  // not all parserFormat matched.
            return false;
    }

    // All m_iUnitsParsed matches parserFormat, but is there more ?

    if (m_iUnitsParsed > parserFormat.m_iUnitsParsed) {
        // More arguments than the template supplies . is this OK?
        // As long as the extra units are assigned and not duplicated we are good.
        for (; iUnitsMatched < m_iUnitsParsed; iUnitsMatched++) {
            TIMEUNIT_t t = m_Unit[iUnitsMatched].m_Type;
            if (t == TIMEUNIT_t::_Numeric) { // cant determine type.
                if (bTrimJunk) break;
                return false;
            }
            if (parserFormat.FindType(t) >= 0)  // duped.
                return false;
            if (FindType(t) < iUnitsMatched)  // duped.
                return false;
        }
    }

    if (iUnitsMatched < parserFormat.m_iUnitsParsed) {
        if (parserFormat.m_iUnitsParsed <= 3)  // must have at least 3 parsed units to be valid.
            return false;
        if (iUnitsMatched < 3) return false;
        // int iLeft = parserFormat.m_iUnitsParsed - m_iUnitsParsed;
    }

    // Its a match, so fix the ambiguous types.
    for (int i = 0; i < iUnitsMatched && i < parserFormat.m_iUnitsParsed; i++) {
        if (m_Unit[i].m_Type == TIMEUNIT_t::_Numeric) m_Unit[i].m_Type = parserFormat.m_Unit[i].m_Type;
    }
    m_iUnitsMatched = iUnitsMatched;
    return true;  // its compatible!
}

bool cTimeParser::TestMatch(const GChar_t* pszFormat) {
    //! Does pszFormat fit with data in m_Units ?
    if (pszFormat == nullptr) return false;
    if (m_iUnitsParsed <= 1) return false;
    cTimeParser t1;
    t1.SetUnitFormats(pszFormat);
    return TestMatchFormat(t1);
}

bool cTimeParser::TestMatches(const GChar_t** ppStrFormats) {
    //! Try standard k_StrFormats to match.
    if (m_iUnitsParsed <= 1) return false;
    if (ppStrFormats == nullptr) {
        ppStrFormats = cTimeUnits::k_StrFormats;
    }
    for (int i = 0; ppStrFormats[i] != nullptr; i++) {
        if (TestMatch(ppStrFormats[i])) return true;
    }

    // If all units have assignments then no match is needed ??
    return false;
}

HRESULT cTimeParser::GetTimeUnits(OUT cTimeUnits& tu) const {
    //! Make a valid cTimeUnits class from what we already parsed. If i can.
    if (!isMatched()) return MK_E_SYNTAX;
    for (int i = 0; i < m_iUnitsMatched; i++) {  // <TIMEUNIT_t::_QTY2
        if (m_Unit[i].m_Type >= TIMEUNIT_t::_QTY) {
            continue;  // TIMEUNIT_t::_DOW, TIMEUNIT_t::_Ignore ignored.
        }
        tu.SetUnit(m_Unit[i].m_Type, m_Unit[i].m_nValue);
    }
    return GetMatchedLength();
}
}  // namespace Gray
