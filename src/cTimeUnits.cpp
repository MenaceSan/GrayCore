//! @file cTimeUnits.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
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
const GChar_t cTimeUnits::k_SepsAll[8] = _GT("/ :T.,-");  // All/Any separator that might occur in k_aStrFormats.

const GChar_t* const cTimeUnits::k_aStrFormats[static_cast<int>(TIMEFORMAT_t::_QTY) + 1] = {
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

const cTimeUnit cTimeUnits::k_aUnits[static_cast<int>(TIMEUNIT_t::_Ignore)] = {
    {_GT("year"), _GT("Y"), {1, 3000}, 12, 365 * 24 * 60 * 60, 365.25},  // approximate, depends on leap year.
    {_GT("month"), _GT("M"), {1, 12}, 30, 30 * 24 * 60 * 60, 30.43},     // approximate, depends on month
    {_GT("day"), _GT("d"), {1, 31}, 24, 24 * 60 * 60, 1.0},
    {_GT("hour"), _GT("h"), {0, 23}, 60, 60 * 60, 1.0 / (24.0)},
    {_GT("minute"), _GT("m"), {0, 59}, 60, 60, 1.0 / (24.0 * 60.0)},
    {_GT("second"), _GT("s"), {0, 59}, 1000, 1, 1.0 / (24.0 * 60.0 * 60.0)},
    {_GT("millisec"), _GT("ms"), {0, 999}, 1000, 0, 1.0 / (24.0 * 60.0 * 60.0 * 1000.0)},
    {_GT("microsec"), _GT("us"), {0, 999}, 0, 0, 1.0 / (24.0 * 60.0 * 60.0 * 1000.0 * 1000.0)},
    {_GT("TZ"), _GT("TZ"), {-24 * 60, 24 * 60}, 0, 0, 1.0},  // TIMEUNIT_t::_TZ
    {_GT("DOW"), _GT("DOW"), {0, 7}, 0},                     // TIMEDOW_t::_QTY
};

const BYTE cTimeUnits::k_aMonthDays[2][static_cast<int>(TIMEMONTH_t::_QTY)] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},  // normal year // Jan=0
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}   // leap year
};

const WORD cTimeUnits::k_aMonthDaySums[2][static_cast<int>(TIMEMONTH_t::_QTY) + 1] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},  // normal year // Jan=0
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}   // leap year
};

const GChar_t* const cTimeUnits::k_aMonthName[static_cast<int>(TIMEMONTH_t::_QTY)] = {  // Jan=0
    _GT("January"), _GT("February"), _GT("March"), _GT("April"), _GT("May"), _GT("June"), _GT("July"), _GT("August"), _GT("September"), _GT("October"), _GT("November"), _GT("December")};

const GChar_t* const cTimeUnits::k_aMonthAbbrev[static_cast<int>(TIMEMONTH_t::_QTY)] = {_GT("Jan"), _GT("Feb"), _GT("Mar"), _GT("Apr"), _GT("May"), _GT("Jun"), _GT("Jul"), _GT("Aug"), _GT("Sep"), _GT("Oct"), _GT("Nov"), _GT("Dec")};

const GChar_t* const cTimeUnits::k_aDayName[static_cast<int>(TIMEDOW_t::_QTY)] = {  // Sun=0
    _GT("Sunday"), _GT("Monday"), _GT("Tuesday"), _GT("Wednesday"), _GT("Thursday"), _GT("Friday"), _GT("Saturday")};

const GChar_t* const cTimeUnits::k_aDayAbbrev[static_cast<int>(TIMEDOW_t::_QTY)] = {  // Sun=0
    _GT("Sun"), _GT("Mon"), _GT("Tue"), _GT("Wed"), _GT("Thu"), _GT("Fri"), _GT("Sat")};

const GChar_t cTimeUnits::k_Seps[3] = _GT("/:");  // Normal date string separators. "/:"

GChar_t cTimeUnits::sm_DateSeparator = '/';  /// might be . for Germans,
bool cTimeUnits::sm_time24Mode = false;

//******************************************************************************************

#ifdef _WIN32
cTimeUnits::cTimeUnits(const SYSTEMTIME& sysTime)
    : _wYear(sysTime.wYear),
      _wMonth(sysTime.wMonth),  // 1 based.
      _wDay(sysTime.wDay),
      _wHour(sysTime.wHour),
      _wMinute(sysTime.wMinute),
      _wSecond(sysTime.wSecond),
      _wMillisecond(sysTime.wMilliseconds),
      _wMicrosecond(0),
      _nTZ(0) {
    ASSERT(isValidTimeUnits());
}

bool cTimeUnits::GetSys(SYSTEMTIME& sysTime) const noexcept {
    sysTime.wYear = _wYear;
    sysTime.wMonth = _wMonth;              // 1 based.
    sysTime.wDayOfWeek = (WORD)get_DOW();  // Sunday - [0,6] TIMEDOW_t
    sysTime.wDay = _wDay;
    sysTime.wHour = _wHour;
    sysTime.wMinute = _wMinute;
    sysTime.wSecond = _wSecond;
    sysTime.wMilliseconds = _wMillisecond;
    return true;
}
void cTimeUnits::SetSys(const SYSTEMTIME& sysTime) {
    _wYear = sysTime.wYear;
    _wMonth = sysTime.wMonth;  // 1 based.
    _wDay = sysTime.wDay;
    _wHour = sysTime.wHour;
    _wMinute = sysTime.wMinute;
    _wSecond = sysTime.wSecond;
    _wMillisecond = sysTime.wMilliseconds;
    _wMicrosecond = 0;
    ASSERT(isValidTimeUnits());
}
#endif

void cTimeUnits::SetZeros() {
    cMem::Zero(&_wYear, static_cast<int>(TIMEUNIT_t::_DOW) * sizeof(_wYear));
    _wYear = 1;  // _nMin
    _wMonth = 1;
    _wDay = 1;
}

bool cTimeUnits::InitTimeNow(TZ_TYPE nTimeZone) {
    //! Get the current time, and adjust units for timezone. nDST ??
    //! like _WIN32 GetLocalTime(st), GetSystemTime(st)
    cTimeInt t;
    t.InitTimeNow();
    return t.GetTimeUnits(*this, nTimeZone);
}

COMPRET_t cTimeUnits::Compare(cTimeUnits& b) const {
    //! Compare relevant parts of 2 times.

    if (b._nTZ != this->_nTZ) {
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
    tNow.InitTimeNow((TZ_TYPE)_nTZ);
    return Compare(tNow) >= COMPARE_Greater;
}

bool cTimeUnits::IsValidUnit(TIMEUNIT_t i) const {
    return GetUnitDef(i).IsInRange(GetUnitVal(i));
}

bool cTimeUnits::isValidTimeUnits() const {
    //! Are the values in valid range ? like GetUnitDef(i).IsInRange().
    //! @note If we are just using this for time math values may go out of range ?
    if (!isValidMonth()) return false;
    if (_wDay < 1 || _wDay > get_DaysInMonth()) return false;
    if (_wHour > 23) return false;
    if (_wMinute > 59) return false;
    if (_wSecond > 59) return false;
    return true;
}

bool cTimeUnits::isReasonableTimeUnits() const {
    //! Is this data reasonable for most purposes?
    if (_wYear < 1900) return false;
    if (_wYear > 2500) return false;
    return isValidTimeUnits();
}

int GRAYCALL cTimeUnits::IsLeapYear(TIMEVALU_t wYear) {  // static
    if (wYear & 3) return 0;                             // not multiple of 4 = not leap year.
    if (wYear % 100) return 1;                           // multiple of 4 but not multiple of 100 = is leap year.
    if (wYear % 400) return 0;                           // multiple of 100 but not 400 = NOT. i.e. 1900 is not a leap year.
    return 1;                                            // multiple of 400. i.e. 2000 is a leap year.
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
    iDays += k_aMonthDaySums[IsLeapYear(wYear)][wMonth - 1];
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
    return k_aMonthDaySums[IsLeapYear(wYear)][wMonth - 1] + wDay - 1;
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
    if (_wYear >= 2007) {
        // New US idiot rules.
        wMonthLo = 3;
        wMonthHi = 11;
    } else {
        wMonthLo = 4;
        wMonthHi = 10;
    }

    // If the month is before April or after October, then we know immediately it can't be DST.
    if (_wMonth < wMonthLo || _wMonth > wMonthHi) return false;
    // If the month is after April and before October then we know immediately it must be DST.
    if (_wMonth > wMonthLo && _wMonth < wMonthHi) return true;

    // Month is April or October see if date falls between appropriate Sundays.
    bool bLow = (_wMonth < 6);

    // What day (of the month) is the Sunday of interest?
    TIMEVALU_t wHour;
    int iSunday;
    if (_wYear < 1987) {
        iSunday = 3;  // always last Sunday
        wHour = (bLow) ? 2 : 1;
    } else if (_wYear < 2007) {
        iSunday = (bLow) ? 1 : 3;  // first or last.
        wHour = (bLow) ? 2 : 1;
    } else {                       // >= 2007
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
            iDayMin = k_aMonthDays[IsLeapYear(_wYear)][_wMonth - 1] - 6;
            break;
    }

    TIMEDOW_t eDOW = GetDOW(_wYear, _wMonth, (TIMEVALU_t)iDayMin);  // sun = 0
    iSunday = iDayMin + ((7 - static_cast<int>(eDOW)) % 7);         // the next Sunday.

    if (bLow) {
        return (_wDay > iSunday || (_wDay == iSunday && _wHour >= wHour));
    } else {
        return (_wDay < iSunday || (_wDay == iSunday && _wHour < wHour));
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

    this->_wSecond = (TIMEVALU_t)(2 * (wFatTime & 0x1f));  // 2 second accurate.
    this->_wMinute = (TIMEVALU_t)((wFatTime >> 5) & 0x3f);
    this->_wHour = (TIMEVALU_t)((wFatTime >> 11));

    this->_wDay = (TIMEVALU_t)(wFatDate & 0x1f);
    this->_wMonth = (TIMEVALU_t)((wFatDate >> 5) & 0x0f);
    this->_wYear = (TIMEVALU_t)(1980 + (wFatDate >> 9));  // up to 2043
}

UINT32 cTimeUnits::get_DosDate() const {
    //! get/pack a 32 bit DOS date format. for ZIP files. and old FAT file system.
    //! ASSUME isValidTimeUnits().
    UINT32 year = (UINT32)this->_wYear;  // 1980 to 2043
    if (year > 1980)
        year -= 1980;
    else if (year > 80)
        year -= 80;
    return (UINT32)(((this->_wDay) + (32 * (this->_wMonth)) + (512 * year)) << 16) | ((this->_wSecond / 2) + (32 * this->_wMinute) + (2048 * (UINT32)this->_wHour));
}

void cTimeUnits::AddMonths(int iMonths) {
    //! Add months to this structure. months are not exact time measures, but there are always 12 per year.
    //! @arg iMonths = 0 based. Can be <0
    iMonths += _wMonth - 1;
    _wMonth = (TIMEVALU_t)(1 + (iMonths % 12));
    _wYear = (TIMEVALU_t)(_wYear + (iMonths / 12));  // adjust years.
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
            _wYear++;
            _wMonth = 1;
            _wDay = 1;
        } else if (iDays2 < 0) {  // previous year.
            ASSERT(iDays < 0);
            iDays += iDaysInYear;
            _wYear--;
            _wMonth = 12;
            _wDay = 31;
        } else
            break;
    }
    // Do we cross months?
    for (;;) {
        int iDayOfMonth = (_wDay - 1);
        int iDaysInMonth = get_DaysInMonth();
        int iDays2 = iDayOfMonth + iDays;
        if (iDays2 >= iDaysInMonth) {  // next month.
            ASSERT(iDays > 0);
            iDays -= iDaysInMonth - iDayOfMonth;
            _wMonth++;
            _wDay = 1;
        } else if (iDays2 < 0) {  // previous month
            ASSERT(iDays < 0);
            iDays += iDayOfMonth;
            _wMonth--;
            _wDay = get_DaysInMonth();
        } else {
            _wDay += (TIMEVALU_t)iDays;
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

    const int iTicksA = cValT::Abs(nSeconds2);
    ASSERT(iTicksA < k_nSecondsPerDay);
    UNREFERENCED_PARAMETER(iTicksA);

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

    _nTZ = offset;
    if (nTimeZone == TZ_UTC) return;  // adjust for timezone and DST, TZ_GMT = 0

    if (isInDST1()) offset -= 60;  // subtract hour.

    // adjust for TZ/DST offsets. may be new day or year ?
    AddSeconds(-(offset * 60));
}

//******************************************************************

StrLen_t cTimeUnits::GetTimeSpanStr(cSpanX<GChar_t> ret, TIMEUNIT_t eUnitHigh, int iUnitsDesired, bool bShortText) const {
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
    if (IS_INDEX_BAD_ARRAY(eUnitHigh, cTimeUnits::k_aUnits)) {
        eUnitHigh = TIMEUNIT_t::_Day;  // days is highest unit by default. months is not accurate!
    }

    int iUnitsPrinted = 0;
    UINT64 nUnits = 0;
    int i = static_cast<int>(TIMEUNIT_t::_Year);  // 0
    for (; i < static_cast<int>(eUnitHigh); i++) {
        nUnits = (nUnits + GetUnit0((TIMEUNIT_t)i)) * k_aUnits[i]._uSubRatio;
    }

    StrBuilder<GChar_t> sb(ret);

    for (; i < static_cast<int>(TIMEUNIT_t::_Microsecond); i++) {  // highest to lowest.
        nUnits += GetUnit0((TIMEUNIT_t)i);
        if (!nUnits) continue;  // just skip empty units.

        sb.AddSep(' ');  // " and ";

        if (bShortText) {
            sb.Printf(_GT("%u%s"), (int)nUnits, StrArg<GChar_t>(cTimeUnits::k_aUnits[i]._pszUnitNameS));
        } else if (nUnits == 1) {
            sb.AddStr(_GT("1 "));
            sb.AddStr(cTimeUnits::k_aUnits[i]._pszUnitNameL);
        } else {
            sb.Printf(_GT("%u %ss"), (int)nUnits, cTimeUnits::k_aUnits[i]._pszUnitNameL);
        }

        if (++iUnitsPrinted >= iUnitsDesired)  // only print iUnitsDesired most significant units of time
            break;
        nUnits = 0;
    }

    if (iUnitsPrinted == 0) {
        // just 0
        return StrT::CopyPtr(ret, bShortText ? _GT("0s") : _GT("0 seconds"));
    }

    return sb.get_Length();
}

StrLen_t cTimeUnits::GetFormStr(cSpanX<GChar_t> ret, const GChar_t* pszFormat) const {
    if (CastPtrToNum(pszFormat) < static_cast<int>(TIMEFORMAT_t::_QTY)) {  // IS_INTRESOURCE()
        pszFormat = k_aStrFormats[CastPtrToNum(pszFormat)];
    }

    GChar_t szTmp[2];
    StrBuilder<GChar_t> sb(ret);

    for (StrLen_t i = 0; !sb.isOverflow(); i++) {
        GChar_t ch = pszFormat[i];
        if (ch == '\0') break;
        if (ch != '%') {
            sb.AddChar(ch);
            continue;
        }

        ch = pszFormat[++i];
        if (ch == '\0') break;
        if (ch == '#') {  // As in the printf function, the # flag may prefix/modify any formatting code
            ch = pszFormat[++i];
            if (ch == '\0') break;
        }

        const GChar_t* pszVal = nullptr;
        int iValWidth = 0;
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
                wVal = _wYear % 100;
                iValWidth = 2;
                break;
            case 'Y':  // Year with century, as decimal number
                wVal = _wYear;
                iValWidth = 0;
                break;

            case 'b':  // Abbreviated month name
            case 'h':
                if (_wMonth == 0) break;
                pszVal = k_aMonthAbbrev[_wMonth - 1];
                break;
            case 'B':  // Full month name
                if (_wMonth == 0) break;
                pszVal = k_aMonthName[_wMonth - 1];
                break;
            case 'm':  // Month as decimal number (01 to 12)
                wVal = _wMonth;
                iValWidth = 2;
                break;
            case 'd':  // Day of month as decimal number (01 to 31)
                wVal = _wDay;
                iValWidth = 2;
                break;

            case 'a':
                pszVal = k_aDayAbbrev[static_cast<int>(get_DOW())];
                break;
            case 'A':
                pszVal = k_aDayName[static_cast<int>(get_DOW())];
                break;
            case 'w':  // Weekday as decimal number (0 to 6; Sunday is 0)
                wVal = static_cast<WORD>(get_DOW());
                iValWidth = 0;
                break;
            case 'j':  // Day of year as decimal number (001 to 366)
                wVal = static_cast<WORD>(get_DOW());
                iValWidth = 3;
                break;

            case 'H':  // Hour in 24-hour format (00 to 23)
                wVal = _wHour;
                iValWidth = 2;
                break;
            case 'k':
                wVal = _wHour;
                iValWidth = 0;
                break;
            case 'I':  // Hour in 12-hour format (01 to 12)
                wVal = (_wHour % 12);
                iValWidth = 2;
                if (!wVal) wVal = 12;
                break;
            case 'p':  // Current locale's A.M./P.M. indicator for 12-hour clock
                pszVal = (_wHour < 12) ? _GT("AM") : _GT("PM");
                break;
            case 'M':  // Minute as decimal number (00 to 59)
                wVal = _wMinute;
                iValWidth = 2;
                break;
            case 'S':  // Second as decimal number (00 to 59)
                wVal = _wSecond;
                iValWidth = 2;
                break;

            case 'Z': {  // Either the time-zone name or time zone abbreviation, depending on registry settings; no characters if time zone is unknown
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZone((TZ_TYPE)_nTZ);
                if (pTZ != nullptr) {
                    pszVal = pTZ->_pszTimeZoneName;
                } else {
                    pszVal = _GT("");  // +00 for timezone.
                }
                break;
            }

            case 'z': {
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZone((TZ_TYPE)_nTZ);
                if (pTZ != nullptr) {
                    pszVal = pTZ->_pszTimeZoneName;
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

        if (pszVal != nullptr) {
            sb.AddStr(pszVal);
        } else if (iValWidth > 0 && iValWidth < sb.get_WriteSpaceQty()) {
            // right padded number.
            GChar_t* pszOutCur = sb.GetWritePrep(iValWidth);
            cSpanX<GChar_t> spanMSD = StrNum::ULtoARev(wVal, pszOutCur, iValWidth, 10, 'A');
            GChar_t* pszMSD = spanMSD.get_PtrWork();
            while (pszMSD > pszOutCur) {
                *(--pszMSD) = '0';
            }
            sb.AdvanceWrite(iValWidth);
        } else {
            sb.AdvanceWrite(StrT::UtoA(wVal, sb.get_SpanWrite()));
        }
    }

    return sb.get_Length();
}

//******************************************************************

HRESULT cTimeUnits::SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
    //! parse cTimeUnits from a string.
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
    hRes = parser.TestMatches();  // try all formats i know.
    if (FAILED(hRes)) return hRes;

    _nTZ = (TIMEVALU_t)nTimeZone;  // allowed to be overridden by cTimeParser.GetTimeUnits
    hRes = parser.GetTimeUnits(*this);
    if (_nTZ == TZ_LOCAL) _nTZ = cTimeZoneMgr::GetLocalMinutesWest();

    return hRes;
}

//******************************************************************************************

StrLen_t cTimeParser::ParseNamedUnit(const GChar_t* pszName) {
    auto& unitX = _Unit[_nUnitsParsed];

    // Get values for named units. TIMEUNIT_t::_Year to TIMEUNIT_t::_TZ
    ITERATE_t iStart = StrT::SpanFindHead(pszName, TOSPAN(cTimeUnits::k_aMonthName));
    if (iStart >= 0) {
        unitX._eType = TIMEUNIT_t::_Month;
        unitX._nValue = (TIMEVALU_t)(iStart + 1);
        return StrT::Len(cTimeUnits::k_aMonthName[iStart]);
    }
    iStart = StrT::SpanFindHead(pszName, TOSPAN(cTimeUnits::k_aMonthAbbrev));
    if (iStart >= 0) {
        unitX._eType = TIMEUNIT_t::_Month;
        unitX._nValue = (TIMEVALU_t)(iStart + 1);
        return StrT::Len(cTimeUnits::k_aMonthAbbrev[iStart]);
    }

    iStart = StrT::SpanFindHead(pszName, TOSPAN(cTimeUnits::k_aDayName));
    if (iStart >= 0) {
        unitX._eType = TIMEUNIT_t::_DOW;  // Temporary for DOW
        unitX._nValue = (TIMEVALU_t)iStart;
        return StrT::Len(cTimeUnits::k_aDayName[iStart]);
    }
    iStart = StrT::SpanFindHead(pszName, TOSPAN(cTimeUnits::k_aDayAbbrev));
    if (iStart >= 0) {
        unitX._eType = TIMEUNIT_t::_DOW;  // Temporary for DOW
        unitX._nValue = (TIMEVALU_t)iStart;
        return StrT::Len(cTimeUnits::k_aDayAbbrev[iStart]);
    }

    const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZoneHead(pszName);
    if (pTZ != nullptr) {
        unitX._eType = TIMEUNIT_t::_TZ;
        unitX._nValue = (TIMEVALU_t)(pTZ->_nTimeZoneOffset);
        return StrT::Len(pTZ->_pszTimeZoneName);
    }

    // AM / PM
    if (!StrT::CmpHeadI(_GT("PM"), pszName)) {
        // Add 12 hours.
        unitX._eType = TIMEUNIT_t::_Hour;
        unitX._nValue = 12;
        return 2;
    }
    if (!StrT::CmpHeadI(_GT("AM"), pszName)) {
        // Add 0 hours.
        unitX._eType = TIMEUNIT_t::_Hour;
        unitX._nValue = 0;
        return 2;
    }

    return 0;
}

HRESULT cTimeParser::ParseString(const GChar_t* pszTimeString, const GChar_t* pszSeparators) {
    //! parse the pszTimeString to look for things that look like a date time.
    //! parse 3 types of things: Separators, numbers and unit names (e.g. Sunday).
    //! @return _nUnitsParsed
    //! @todo parse odd time zone storage .  (-03:00)

    if (pszTimeString == nullptr) return E_POINTER;

    const GChar_t* pszSepFind = nullptr;
    bool bNeedSep = false;
    _nUnitsParsed = 0;

    if (pszSeparators == nullptr) pszSeparators = cTimeUnits::k_SepsAll;

    StrLen_t i = 0;
    for (;;) {
        const StrLen_t iStart = i;
        i += StrT::GetNonWhitespaceN(pszTimeString + i);
        GChar_t ch = pszTimeString[i];
        pszSepFind = StrT::FindChar(pszSeparators, ch);
        if (pszSepFind != nullptr) {  // its a legal separator char?
        do_sep:
            _Unit[_nUnitsParsed].SetSep(i, ch);
            if (!bNeedSep) break;  // Was just empty!? NOT ALLOWED.

            bNeedSep = false;
            _nUnitsParsed++;
            if (_nUnitsParsed >= (int)_countof(_Unit)) break;
            if (ch == '\0') break;  // done.

            i++;
            continue;
        }

        if (bNeedSep) {                                                      // must complete the previous first.
            if (iStart == i) {                                               // needed a space separator but didn't get one.
                if (ch == 'T' && StrChar::IsDigitA(pszTimeString[i + 1])) {  // ISO can use this as a separator.
                    goto do_sep;
                }

                //! @todo parse odd time zone storage .  (-03:00)
                // Check for terminating TZ with no separator.
                const cTimeZone* pTZ = cTimeZoneMgr::FindTimeZoneHead(pszTimeString + i);
                if (pTZ != nullptr) {
                    ch = ' ';  // Insert fake separator.
                    i--;
                    goto do_sep;
                }
                break;  // quit.
            }

            // Was whitespace separator.
            _Unit[_nUnitsParsed].SetSep(iStart, ' ');
            bNeedSep = false;
            _nUnitsParsed++;
            if (_nUnitsParsed >= (int)_countof(_Unit)) break;
        }
        if (ch == '\0') break;  // done.

        _Unit[_nUnitsParsed].InitUnit();
        const GChar_t* pszStart = pszTimeString + i;

        if (StrChar::IsDigitA(ch)) {
            // We found a number. good. use it.
            _Unit[_nUnitsParsed]._eType = TIMEUNIT_t::_Numeric;  // this just means its a number. don't care what kind yet. resolve later.
            const GChar_t* pszEnd = nullptr;
            _Unit[_nUnitsParsed]._nValue = (TIMEVALU_t)StrT::toI(pszStart, &pszEnd, 10);
            if (pszStart >= pszEnd || pszEnd == nullptr) break;
            i += cValSpan::Diff(pszEnd, pszStart);
            bNeedSep = true;
            continue;

        } else if (StrChar::IsAlpha(ch)) {  // specific named units . DOW, TZ, Month.
            const int iLen = ParseNamedUnit(pszStart);
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

    for (i = 0; i < _nUnitsParsed; i++) {
        if (iMonthFound < 0 && _Unit[i]._eType == TIMEUNIT_t::_Month) {
            iMonthFound = i;
        }
        if (iYearFound < 0 && _Unit[i]._eType == TIMEUNIT_t::_Numeric && _Unit[i]._nValue > 366) {
            // This must be the year.
            iYearFound = i;
            _Unit[i]._eType = TIMEUNIT_t::_Year;
        }
        if (iHourFound < 0 && _Unit[i]._eType == TIMEUNIT_t::_Numeric && _Unit[i]._chSeparator == ':') {
            iHourFound = i;
            _Unit[i]._eType = TIMEUNIT_t::_Hour;
            i++;
            _Unit[i]._eType = TIMEUNIT_t::_Minute;
            if (i + 1 < _nUnitsParsed && _Unit[i + 1]._eType == TIMEUNIT_t::_Numeric && _Unit[i]._chSeparator == ':') {
                i++;
                _Unit[i]._eType = TIMEUNIT_t::_Second;
                if (i + 1 < _nUnitsParsed && _Unit[i + 1]._eType == TIMEUNIT_t::_Numeric && _Unit[i]._chSeparator == '.') {
                    i++;
                    _Unit[i]._eType = TIMEUNIT_t::_Millisecond;
                }
            }
        }
        if (iHourFound >= 0 && _Unit[i]._eType == TIMEUNIT_t::_Hour) {  // PM ?
            _Unit[i]._eType = TIMEUNIT_t::_Ignore;                      // Ignore this from now on.
            if (_Unit[iHourFound]._nValue < 12) {
                _Unit[iHourFound]._nValue += _Unit[i]._nValue;  // Add 'PM'
            }
        }
    }

    // Fail is there are type dupes.
    // Find day if month is found ?

    // We are not reading a valid time/date anymore. done. stop.
    _Unit[_nUnitsParsed]._eType = TIMEUNIT_t::_QTY2;  // end
    return _nUnitsParsed;
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
    return TIMEUNIT_t::_QTY2;  // bad
}

int cTimeParser::FindType(TIMEUNIT_t t) const {
    // is this TIMEUNIT_t already used ?
    for (int i = 0; i < _nUnitsParsed; i++) {
        if (_Unit[i]._eType == t) return i;
    }
    return -1;  // TIMEUNIT_t not used.
}

void cTimeParser::SetUnitFormats(const GChar_t* pszFormat) {
    //! Similar to ParseString but assumes we just want to set units from a format string.
    _nUnitsParsed = 0;
    int i = 0;
    for (;;) {  // TIMEUNIT_t::_QTY2
        GChar_t ch = pszFormat[i];
        if (ch == '\0') break;
        if (ch != '%') break;
        const TIMEUNIT_t eType = GetTypeFromFormatCode(pszFormat[i + 1]);
        _Unit[_nUnitsParsed]._eType = eType;
        if (IS_INDEX_BAD(eType, TIMEUNIT_t::_Numeric)) break;  // this should not happen ?! bad format string!

        _Unit[_nUnitsParsed]._nValue = -1;  // set later.
        ch = pszFormat[i + 2];
        _Unit[_nUnitsParsed].SetSep(i + 2, ch);
        _nUnitsParsed++;
        if (ch == '\0') break;
        i += 3;
        i += StrT::GetNonWhitespaceN(pszFormat + i);
    }
}

bool GRAYCALL cTimeParser::TestMatchUnit(const cTimeParserUnit& u, TIMEUNIT_t t) {  // static
    ASSERT(IS_INDEX_GOOD(u._eType, TIMEUNIT_t::_QTY2));
    ASSERT(IS_INDEX_GOOD(t, TIMEUNIT_t::_Numeric));
    if (!cTimeUnits::GetUnitDef(t).IsInRange(u._nValue)) return false;
    if (t == u._eType) return true;  // exact type match is good.
    // TIMEUNIT_t::_Numeric is parsed wildcard (i don't know yet) type.
    // known types must match. TIMEUNIT_t::_Month or TIMEUNIT_t::_DOW
    if (u._eType == TIMEUNIT_t::_Numeric) return true;  // It looks like a match ? I guess.
    return false;                                       // not a match.
}

bool cTimeParser::TestMatchFormat(const cTimeParser& parserFormat, bool bTrimJunk) {
    //! Does parserFormat fit with data in _Unit ?
    //! Does this contain compatible units with parserFormat? if so fix _Unit types!
    //! @arg bTrimJunk = any unrecognized stuff beyond parserFormat can just be trimmed.

    ASSERT(_nUnitsParsed <= (int)_countof(_Unit));
    if (_nUnitsParsed <= 1) return false;

    int iUnitsMatched = 0;
    for (; iUnitsMatched < _nUnitsParsed && iUnitsMatched < parserFormat._nUnitsParsed; iUnitsMatched++) {  // TIMEUNIT_t::_QTY2
        if (!TestMatchUnit(_Unit[iUnitsMatched], parserFormat._Unit[iUnitsMatched]._eType))                 // not all parserFormat matched.
            return false;
    }

    // All _nUnitsParsed matches parserFormat, but is there more ?

    if (_nUnitsParsed > parserFormat._nUnitsParsed) {
        // More arguments than the template supplies . is this OK?
        // As long as the extra units are assigned and not duplicated we are good.
        for (; iUnitsMatched < _nUnitsParsed; iUnitsMatched++) {
            TIMEUNIT_t t = _Unit[iUnitsMatched]._eType;
            if (t == TIMEUNIT_t::_Numeric) {  // cant determine type.
                if (bTrimJunk) break;
                return false;
            }
            if (parserFormat.FindType(t) >= 0) return false;  // duped.
                 
            if (FindType(t) < iUnitsMatched) return false;  // duped.
                
        }
    }

    if (iUnitsMatched < parserFormat._nUnitsParsed) {
        if (parserFormat._nUnitsParsed <= 3)  // must have at least 3 parsed units to be valid.
            return false;
        if (iUnitsMatched < 3) return false;
        // int iLeft = parserFormat._nUnitsParsed - _nUnitsParsed;
    }

    // Its a match, so fix the ambiguous types.
    for (int i = 0; i < iUnitsMatched && i < parserFormat._nUnitsParsed; i++) {
        if (_Unit[i]._eType == TIMEUNIT_t::_Numeric) _Unit[i]._eType = parserFormat._Unit[i]._eType;
    }
    _nUnitsMatched = iUnitsMatched;
    return true;  // its compatible!
}

bool cTimeParser::TestMatch(const GChar_t* pszFormat) {
    //! Does pszFormat fit with data in _Unit ?
    if (pszFormat == nullptr) return false;
    if (_nUnitsParsed <= 1) return false;
    cTimeParser t1;
    t1.SetUnitFormats(pszFormat);
    return TestMatchFormat(t1);
}

HRESULT cTimeParser::TestMatches(const GChar_t* const* ppStrFormats) {
    //! Try standard k_aStrFormats to match.
    if (_nUnitsParsed <= 1) return false;
    if (ppStrFormats == nullptr) ppStrFormats = cTimeUnits::k_aStrFormats;

    for (int i = 0; ppStrFormats[i] != nullptr; i++) {
        if (TestMatch(ppStrFormats[i])) return i;  // does it match this format ?
    }

    // If all units have assignments then no match is needed ?? NO _QTY, _UNK ?
    return MK_E_SYNTAX;
}

HRESULT cTimeParser::GetTimeUnits(OUT cTimeUnits& tu) const {
    //! Make a valid cTimeUnits class from what we already parsed. If i can.
    if (!isMatched()) return MK_E_SYNTAX;
    ASSERT(_nUnitsMatched <= _countof(_Unit));
    for (int i = 0; i < _nUnitsMatched; i++) {              // <TIMEUNIT_t::_QTY2
        if (_Unit[i]._eType >= TIMEUNIT_t::_DOW) continue;  // TIMEUNIT_t::_DOW, TIMEUNIT_t::_Ignore ignored.
        tu.SetUnit(_Unit[i]._eType, _Unit[i]._nValue);
    }
    return GetMatchedLength();
}
}  // namespace Gray
