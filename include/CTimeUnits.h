//
//! @file cTimeUnits.h
//! common for cTimeInt, cTimeDouble, cTimeSys
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeUnits_H
#define _INC_cTimeUnits_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "StrArg.h"
#include "StrConst.h"
#include "cDebugAssert.h"
#include <time.h>  // system time_t for count of seconds. int32 or int64.

namespace Gray {
// Base type used for cTimeInt Might be 64 bits ?? or _USE_32BIT_TIME_T
typedef time_t TIMESEC_t;  /// absolute seconds since January 1, 1970. (GMT?)(signed?) NOTE: Changing to __time64_t just adds more range with same values. (>2038)
typedef int TIMESECD_t;    /// signed delta seconds from some epoch. like: std::chrono::seconds

typedef short TIMEVALU_t;  /// Arbitrary time value of type TIMEUNIT_t. (e.g. number of minutes or seconds). Allow negative for null ? Max 16 bits.

/// <summary>
/// Known/Common Time Zones.
/// Generally stored as minute west offset from UTC for specific Time Zones.
/// May already include DST or not. EDT vs EST. Do not assume anything about DST usage / rules.
/// e.g. 45 minutes TZ offset is possible.
/// http://www.timeanddate.com/time/map/
/// </summary>
enum TZ_TYPE {
    TZ_UTC = 0,  /// UTC = never use DST or any TZ offset.
    TZ_GMT = 0,  /// Greenwich mean time. similar to UTC
    TZ_AST = (4 * 60),
    TZ_EDT = (4 * 60),  /// Eastern Daylight time.
    TZ_EST = (5 * 60),  /// Eastern Standard Time Zone. LocalTime+offset=GMT, GMT-offset=Local (seconds)
    TZ_CST = (6 * 60),
    TZ_MST = (7 * 60),
    TZ_PST = (8 * 60),   /// Pacific Standard Time Zone. Default for Windows.
    TZ_MAX = (24 * 60),  /// Max offset. over this is special mapped cTimeZone.
    TZ_LOCAL = 0x7FFF,   /// just use local time zone. might include DST ??
};

/// <summary>
/// Predefined/common time string formats we must be able to parse/supply.
/// Uses similar format tags to strftime() .
/// JavaScript/JSON normal format = ISO8601 = https://en.wikipedia.org/wiki/ISO_8601 e.g. "2012-04-23T18:25:43.511Z"
/// enum cdtdate_format_type {cdtMDY, cdtDAY, cdtMONTH, cdtFULL, cdtEUROPEAN};
/// </summary>
enum class TIMEFORMAT_t {
    _DEFAULT = 0,  /// Default Sortable/linear/readable format. "2008/07/09 13:47:10"
    _DB,           /// Database default but with no TZ. "%04d-%02d-%02d %02d:%02d:%02d"
    _DEFTZ,        /// Sortable Universal/GMT time = "2008-04-10 13:30:00Z"
    _AMERICAN,     /// Typical American style of "07/19/2008 13:47:10"
    _HTTP,         /// HTTP RFC1123 format "Tue, 03 Oct 2000 22:44:56 GMT"
    _SMTP,         /// SMTP wants this format. "7 Aug 2001 10:12:12 GMT"
    _ISO,          /// ISO8601 with no TZ but including the 'T' "2015/01/02T14:03:03"
    _ISO_TZ,       /// ISO8601 plus TZ. "2015/01/02T14:03:03EST" or "2015-11-28T10:16:42+00:00"
    _ASN,          /// No punctuation. e.g. "20150102140303Z"

    //! toJSON method: "2012-04-23T18:25:43.511Z" is sortable.
    //! "01/06/2016, 11:45 AM (-03:00)" // from MSSQL samples?
    //! SQL Database normal format. https://www.mssqltips.com/sqlservertip/1145/date-and-time-conversions-using-sql-server/
    //! "06-JAN-16 05.45.00.000000000 PM" = Oracle default.
    _QTY,
};

/// <summary>
/// Days of the week. 0 based.  MFC GetDayOfWeek is +1
/// </summary>
enum class TIMEDOW_t {
    _Sun = 0,  /// 0 based as in SYSTEMTIME.wDayOfWeek, and struct tm.tm_wday
    _Mon,
    _Tue,
    _Wed,
    _Thu,
    _Fri,
    _Sat,
    _QTY,  /// 7
};

/// <summary>
/// Months of the year. 0 based. NOT the same as stored in cTimeUnits!!
/// </summary>
enum class TIMEMONTH_t {
    _Jan = 0,  /// tm.tm_mon, NOT SYSTEMTIME (which is 1 based)
    _Feb,
    _Mar,
    _Apr,
    _May,
    _Jun,
    _Jul,
    _Aug,
    _Sep,
    _Oct,
    _Nov,
    _Dec = 11,
    _QTY = 12,
};

/// <summary>
/// metadata describing ratios between relative time units in cTimeUnits.
/// per TIMEUNIT_t Unit
/// </summary>
struct cTimeUnit {
    const GChar_t* m_pszUnitNameL;  /// long unit name
    const GChar_t* m_pszUnitNameS;  /// short abbreviated unit name
    TIMEVALU_t m_uMin;
    TIMEVALU_t m_uMax;

    WORD m_uSubRatio;           /// How many sub units in this unit. (for absolute units. e.g. not months or years)
    TIMESECD_t m_nUnitSeconds;  /// Total seconds for a unit. (for absolute units)
    double m_dUnitDays;         /// Total days or fractions of a day for the unit. (for absolute units)

    bool IsInRange(TIMEVALU_t v) const {
        return v >= m_uMin && v <= m_uMax;
    }
};

/// <summary>
/// Enumerate TIMEVALU_t (16 bit max) elements of cTimeUnits and cTimeParser
/// </summary>
enum class TIMEUNIT_t {
    _Year = 0,     /// e.g. 2008. (1<=x<=3000)
    _Month,        /// base 1, NOT Base 0 like = TIMEMONTH_t::_Jan. (1<=x<=12)
    _Day,          /// day of month. base 1. (1<=x<=31)
    _Hour,         /// hour of day. 24 hour scale. base 0. (0<=x<=23)
    _Minute,       /// base 0. (0<=x<=59)
    _Second,       /// base 0. (0<=x<=59)
    _Millisecond,  /// 1/1000 = thousandth of a second. (0<=x<=999)
    _Microsecond,  /// millionth of a second. (0<=x<=999)
    _TZ,           /// TZ + DST
    // used for parsing only.
    _DOW,       /// Ignore this for units storage. its redundant.
    _Ignore,   /// Just ignore this duplicate. We have already dealt with it.
    _Numeric,  /// A numeric value of unknown type (parsing).
    _QTY2,     /// END of cTimeParser
};

/// <summary>
/// Decompose/Break time into units in order of size.
/// like: struct tm for POSIX time_t.
/// like: SYSTEMTIME for _WIN32
/// like: TIMESTAMP_STRUCT for SQL_TIMESTAMP, SQL_C_TIMESTAMP, SQL_DATE
/// </summary>
struct GRAYCORE_LINK cTimeUnits {
    TIMEVALU_t m_wYear;   /// Year valid for 1980 to 2043 at least. TIMEUNIT_t::_Year
    TIMEVALU_t m_wMonth;  /// 1 based month of year. Jan=1, to 12=Dec, NOT 0 based like TIMEMONTH_t. TIMEUNIT_t::_Month
    TIMEVALU_t m_wDay;    /// 1 based day of month. 1 to 31. TIMEUNIT_t::_Day

    TIMEVALU_t m_wHour;    /// 0 to 23 for hour of day.
    TIMEVALU_t m_wMinute;  /// 0 to 59
    TIMEVALU_t m_wSecond;  /// 0 to 59

    TIMEVALU_t m_wMillisecond;  /// 1000th = thousandth. 0 to 1000
    TIMEVALU_t m_wMicrosecond;  /// 1000000th = millionth. 0 to 1000. TIMEUNIT_t::_Microsecond

    TIMEVALU_t m_nTZ;  /// TZ_TYPE for m_wMinute offset. TIMEUNIT_t::_TZ

    static const TIMESECD_t k_nSecondsPerDay = (24 * 60 * 60);  /// seconds in a day = 86400
    static const TIMESECD_t k_nSecondsPerHour = (60 * 60);      /// seconds in a hour = 3600
    static const int k_nMinutesPerDay = (24 * 60);              /// minutes in a day
    static const int k_nMicroSeconds = 1000000;                 /// millionth of a second.

    static const cTimeUnit k_Units[static_cast<int>(TIMEUNIT_t::_Ignore)];  /// Metadata for time units.

    static const StrLen_t k_FormStrMax = 256;                 // max reasonable size for time.
    static const GChar_t* k_StrFormats[static_cast<int>(TIMEFORMAT_t::_QTY) + 1];  /// standard strftime() type formats.

    static const BYTE k_MonthDays[2][static_cast<int>(TIMEMONTH_t::_QTY)];         /// Jan=0
    static const WORD k_MonthDaySums[2][static_cast<int>(TIMEMONTH_t::_QTY) + 1];  /// Jan=0

    // may change for language ?
    static const GChar_t* const k_MonthName[static_cast<int>(TIMEMONTH_t::_QTY) + 1];  /// January=0
    static const GChar_t* const k_MonthAbbrev[static_cast<int>(TIMEMONTH_t::_QTY) + 1];  /// Jan=0

    static const GChar_t* const k_DayName[static_cast<int>(TIMEDOW_t::_QTY) + 1];  /// Sunday=0
    static const GChar_t* const k_DayAbbrev[static_cast<int>(TIMEDOW_t::_QTY) + 1];  /// Sun=0

    // internationalization. regional
    static const GChar_t k_TimeSeparator = ':';  /// this is the same for all formats. e.g. 09:00 AM. NOT USED ?
    static const GChar_t k_Seps[3];              // Normal valid time/date string separators. "/:" = all sm_DateSeparator or sm_TimeSeparator
    static const GChar_t k_SepsAll[8];           // All/Any separator that might occur in k_StrFormats.

    // May change for regional preferences ?
    static GChar_t sm_DateSeparator;  /// date separator to use for string creation = '\', '-', '.', but Time is always ':'
    static bool sm_time24Mode;        /// Display time in 24 hour format. default = false

 public:
#ifdef _WIN32
    cTimeUnits(const SYSTEMTIME& sysTime);
    bool GetSys(SYSTEMTIME& sysTime) const noexcept;
    void SetSys(const SYSTEMTIME& sysTime);
#endif
    void SetZeros();
    bool InitTimeNow(TZ_TYPE nTimeZone = TZ_LOCAL);

    COMPARE_TYPE Compare(cTimeUnits& b) const;
    bool isTimeFuture() const;
    bool isTimePast() const {
        //! AKA Expired ?
        return !isTimeFuture();
    }

    /// <summary>
    /// Get calculated Day of Week
    /// </summary>
    /// <returns>TIMEDOW_Sun = 0</returns>
    TIMEDOW_t get_DOW() const {
        return GetDOW(m_wYear, m_wMonth, m_wDay);
    }
    /// <summary>
    /// Get Day of year.
    /// </summary>
    /// <returns>0 based</returns>
    int get_DOY() const {
        return GetDOY(m_wYear, m_wMonth, m_wDay);
    }
    TIMEMONTH_t get_Month() const noexcept {
        return static_cast<TIMEMONTH_t>(m_wMonth - 1);
    }
    static const cTimeUnit& GetUnitDef(TIMEUNIT_t i) {
        ASSERT(IS_INDEX_GOOD_ARRAY(i, cTimeUnits::k_Units));
        return k_Units[static_cast<int>(i)];
    }

    bool IsValidUnit(TIMEUNIT_t i) const;
    bool isValidTimeUnits() const;
    bool isReasonableTimeUnits() const;

    TIMEVALU_t GetUnitVal(TIMEUNIT_t i) const {
        //! enumerate the time units.
        ASSERT(IS_INDEX_GOOD(i, static_cast<int>(TIMEUNIT_t::_DOW)));
        return (&m_wYear)[static_cast<int>(i)];
    }
    TIMEVALU_t GetUnit0(TIMEUNIT_t i) const {
        // Zero based units.
        return GetUnitVal(i) - GetUnitDef(i).m_uMin;
    }
    void SetUnit(TIMEUNIT_t i, TIMEVALU_t wVal) {
        ASSERT(IS_INDEX_GOOD(i, static_cast<int>(TIMEUNIT_t::_DOW)));
        (&m_wYear)[static_cast<int>(i)] = wVal;
    }

    bool operator==(const cTimeUnits& rTu) const {
        return cMem::IsEqual(this, &rTu, sizeof(rTu));  // All of it ?
    }

    void put_DosDate(UINT32 ulDosDate);
    UINT32 get_DosDate() const;

    void AddMonths(int iMonths);
    void AddDays(int iDays);
    void AddSeconds(TIMESECD_t nSeconds);

    void AddTZ(TZ_TYPE nTimeZone);

    bool isInDST1() const;

    /// <summary>
    /// Get the time as a formatted string using "C" strftime()
    /// build formatted string from cTimeUnits.
    ///  similar to C stdlib strftime() http://linux.die.net/man/3/strftime
    ///  add TZ as postfix if desired??
    ///  used by cTimeDouble::GetTimeFormStr and cTimeInt::GetTimeFormStr
    /// </summary>
    /// <param name="wYear"></param>
    /// <returns>length of string in chars. -lte- 0 = failed.</returns>
    StrLen_t GetFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, const GChar_t* pszFormat) const;
    StrLen_t GetFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, TIMEFORMAT_t eFormat = TIMEFORMAT_t::_DEFAULT) const {
        return GetFormStr(pszOut, iOutSizeMax, CastNumToPtr<GChar_t>(static_cast<int>(eFormat)));
    }
    HRESULT SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset);
    StrLen_t GetTimeSpanStr(GChar_t* pszOut, StrLen_t iOutSizeMax, TIMEUNIT_t eUnitHigh = TIMEUNIT_t::_Day, int iUnitsDesired = 2, bool bShortText = false) const;

    static int GRAYCALL IsLeapYear(TIMEVALU_t wYear);
    static int GRAYCALL GetLeapYearsSince2K(TIMEVALU_t wYear);
    static TIMEDOW_t GRAYCALL GetDOW(TIMEVALU_t wYear, TIMEVALU_t wMonth, TIMEVALU_t wDay);
    static int GRAYCALL GetDOY(TIMEVALU_t wYear, TIMEVALU_t wMonth, TIMEVALU_t wDay);

    TIMESECD_t get_SecondOfDay() const noexcept {
        return m_wSecond + (m_wMinute * 60) + CastN(TIMESECD_t, m_wHour * 60 * 60);
    }
    bool isValidMonth() const noexcept {
        return m_wMonth >= 1 && m_wMonth <= 12;
    }
    TIMEVALU_t get_DaysInMonth() const {
        // How many days in m_wMonth ?
        if (!isValidMonth()) return 0;
        const int iLeapYear = IsLeapYear(m_wYear);
        return k_MonthDays[iLeapYear][m_wMonth - 1];
    }
    TIMEVALU_t get_DayOfYear() const {
        // What day of m_wYear is this ?
        if (!isValidMonth()) return 0;
        const int iLeapYear = IsLeapYear(m_wYear);
        return k_MonthDaySums[iLeapYear][m_wMonth - 1] + (m_wDay - 1);
    }
    /// <summary>
    /// How many days in m_wYear ?
    /// </summary>
    TIMEVALU_t get_DaysInYear() const {
        return IsLeapYear(m_wYear) ? 366 : 365;
    }

    cTimeUnits(void) {
        SetZeros();  // a valid time?
    }
    cTimeUnits(TIMEVALU_t wYear, TIMEVALU_t wMonth, TIMEVALU_t wDay, TIMEVALU_t wHour = 0, TIMEVALU_t wMinute = 0, TIMEVALU_t wSecond = 0, TIMEVALU_t wMilliseconds = 0, TIMEVALU_t wMicroseconds = 0, TZ_TYPE nTZ = TZ_UTC)
        : m_wYear(wYear), m_wMonth(wMonth), m_wDay(wDay), m_wHour(wHour), m_wMinute(wMinute), m_wSecond(wSecond), m_wMillisecond(wMilliseconds), m_wMicrosecond(wMicroseconds), m_nTZ((TIMEVALU_t)nTZ) {}
};

//*******************************************************

/// <summary>
/// Helper for parsing time units from string.
/// </summary>
struct cTimeParserUnit {
    TIMEUNIT_t m_Type;   /// What type of field/unit does this look like. best guess. TIMEUNIT_t::_Sec
    TIMEVALU_t m_nValue;    /// Value we read from the field.	<0 = null/omitted.
    StrLen_t m_iOffsetSep;  /// End of the type info and start of the separator.
    GChar_t m_Separator;    /// What sort of separator follows ? ":T /.,-"

    void Init() {
        m_Type = TIMEUNIT_t::_QTY2;
        m_nValue = -1;  // not set yet.
        m_iOffsetSep = -1;
        m_Separator = -1;  // not set yet.
    }
    TIMEUNIT_t get_HashCode() const noexcept {  // should be only one of each type.
        return m_Type;
    }
    TIMEVALU_t get_SortValue() const noexcept {
        return m_nValue;
    }
};

/// <summary>
/// Try to interpret/parse a string as a date/time.
/// Hold result of the first parsing pass to (perhaps) process the time string as cTimeUnits.
/// </summary>
class GRAYCORE_LINK cTimeParser {
 public:
    cTimeParserUnit m_Unit[static_cast<int>(TIMEUNIT_t::_QTY2)];  /// space for parsed results.
    int m_iUnitsParsed;                     /// m_Unit used. <TIMEUNIT_t::_QTY2
    int m_iUnitsMatched;                    /// m_iUnitsMatched <= m_iUnitsParsed and all m_Type are set. No use of TIMEUNIT_t::_Numeric

 public:
    /// <summary>
    /// is the value and type in cTimeParserUnit compatible with TIMEUNIT_t t?
    /// </summary>
    static bool GRAYCALL TestMatchUnit(const cTimeParserUnit& u, TIMEUNIT_t t);
    static TIMEUNIT_t GRAYCALL GetTypeFromFormatCode(GChar_t ch);
    int FindType(TIMEUNIT_t t) const;
    void SetUnitFormats(const GChar_t* pszFormat);

    StrLen_t ParseNamedUnit(const GChar_t* pszName);
    HRESULT ParseString(const GChar_t* pszTimeString, const GChar_t* pszSeparators = nullptr);
    bool isMatched() const noexcept {
        return m_iUnitsMatched > 0;
    }
    StrLen_t GetMatchedLength() const {
        //! How much of the parsed string was consumed by the match?
        ASSERT(m_iUnitsMatched <= m_iUnitsParsed);
        if (m_iUnitsMatched <= 0) return 0;
        int i = m_iUnitsMatched - 1;
        ASSERT(IS_INDEX_GOOD_ARRAY(i, m_Unit));
        return m_Unit[i].m_iOffsetSep;
    }

    bool TestMatchFormat(const cTimeParser& parserFormat, bool bTrimJunk = false);
    bool TestMatch(const GChar_t* pszFormat);
    HRESULT TestMatches(const GChar_t** ppStrFormats = nullptr);

    HRESULT GetTimeUnits(OUT cTimeUnits& tu) const;

    cTimeParser() : m_iUnitsParsed(0), m_iUnitsMatched(0) {
        m_Unit[0].Init();
    }
    cTimeParser(const GChar_t* pszTimeString) : m_iUnitsParsed(0), m_iUnitsMatched(0) {
        ParseString(pszTimeString);
    }
    cTimeParser(const GChar_t* pszTimeString, const GChar_t** ppStrFormats) : m_iUnitsParsed(0), m_iUnitsMatched(0) {
        // Matches
        ParseString(pszTimeString);
        TestMatches(ppStrFormats);
    }
};
}  // namespace Gray
#endif
