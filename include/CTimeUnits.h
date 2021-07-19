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

#include "StrConst.h"
#include "StrArg.h"
#include "cDebugAssert.h"
#include "HResult.h"
#include <time.h>	// system time_t for count of seconds. int32 or int64.

namespace Gray
{
	// Base type used for cTimeInt Might be 64 bits ?? or _USE_32BIT_TIME_T
	typedef time_t TIMESEC_t;	//!< absolute seconds since January 1, 1970. (GMT?)(signed) NOTE: Changing to __time64_t just adds more range with same values. (>2038)
	typedef int TIMESECD_t;		//!< signed delta seconds from some epoch. like: std::chrono::seconds

	typedef short TIMEUNIT_t;	//!< Arbitrary time value of type TIMEUNIT_TYPE. (e.g. number of minutes or seconds). Allow negative for null ? Max 16 bits. 

	enum TZ_TYPE
	{
		//! @enum Gray::TZ_TYPE
		//! Known/Common Time Zones. Subtract offset from UTC for specific Time Zones (and maybe DST) in minutes.
		//! e.g. 45 minutes TZ offset is possible.
		//! http://www.timeanddate.com/time/map/

		TZ_UTC = 0,				//!< UTC = never use DST or any TZ offset.
		TZ_GMT = 0,				//!< Greenwich mean time. similar to UTC
		TZ_AST = (4 * 60),
		TZ_EDT = (4 * 60),		//!< Eastern Daylight time.
		TZ_EST = (5 * 60),		//!< Eastern Standard Time Zone. LocalTime+offset=GMT, GMT-offset=Local (seconds)
		TZ_CST = (6 * 60),
		TZ_MST = (7 * 60),
		TZ_PST = (8 * 60),		//!< Pacific Standard Time Zone. Default for Windows.
		TZ_MAX = (24 * 60),		//!< Max offset. over this is special mapped cTimeZone.
		TZ_LOCAL = 0x7FFF,		//!< just use local time zone. might include DST ??
	};

	enum TIME_FORMAT_TYPE
	{
		//! @enum Gray::TIME_FORMAT_TYPE
		//! Predefined/common time string formats we must be able to parse/supply.
		//! Uses similar format tags to strftime() .
		//! JavaScript/JSON normal format = ISO8601 = https://en.wikipedia.org/wiki/ISO_8601 e.g. "2012-04-23T18:25:43.511Z"

		// enum cdtdate_format_type {cdtMDY, cdtDAY, cdtMONTH, cdtFULL, cdtEUROPEAN};

		TIME_FORMAT_DEFAULT = 0,	//!< Default Sortable/linear/readable format. "2008/07/09 13:47:10"
		TIME_FORMAT_DB,				//!< Database default but with no TZ. "%04d-%02d-%02d %02d:%02d:%02d"
		TIME_FORMAT_TZ,				//!< Sortable Universal/GMT time = "2008-04-10 13:30:00Z"

		TIME_FORMAT_AMERICAN,		//!< Typical American style of "07/19/2008 13:47:10"
		TIME_FORMAT_HTTP,			//!< HTTP RFC1123 format "Tue, 03 Oct 2000 22:44:56 GMT"
		TIME_FORMAT_SMTP,			//!< SMTP wants this format. "7 Aug 2001 10:12:12 GMT"

		TIME_FORMAT_ISO,			//!< ISO8601 with no TZ but including the 'T' "2015/01/02T14:03:03"
		TIME_FORMAT_ISO_TZ,			//!< ISO8601 plus TZ. "2015/01/02T14:03:03EST" or "2015-11-28T10:16:42+00:00"
		TIME_FORMAT_ASN,			//!< No punctuation. e.g. "20150102140303Z"

		//! toJSON method: "2012-04-23T18:25:43.511Z" is sortable.
		//! "01/06/2016, 11:45 AM (-03:00)" // from MSSQL samples?
		//! SQL Database normal format. https://www.mssqltips.com/sqlservertip/1145/date-and-time-conversions-using-sql-server/
		//! "06-JAN-16 05.45.00.000000000 PM" = Oracle default.

		TIME_FORMAT_QTY,
	};

	enum TIMEDOW_TYPE
	{
		//! @enum Gray::TIMEDOW_TYPE
		//! DAys of the week. 0 based.
		TIMEDOW_Sun = 0,	//!< 0 based as in SYSTEMTIME.wDayOfWeek, and struct tm.tm_wday
		TIMEDOW_Mon,
		TIMEDOW_Tue,
		TIMEDOW_Wed,
		TIMEDOW_Thu,
		TIMEDOW_Fri,
		TIMEDOW_Sat,
		TIMEDOW_QTY,	//!< 7
	};

	enum TIMEMONTH_TYPE
	{
		//! @enum Gray::TIMEMONTH_TYPE
		//! Months of the year. 0 based.
		TIMEMONTH_Jan = 0,	//!< tm.tm_mon, NOT SYSTEMTIME (which is 1 based)
		TIMEMONTH_Feb,
		TIMEMONTH_Mar,
		TIMEMONTH_Apr,
		TIMEMONTH_May,
		TIMEMONTH_Jun,
		TIMEMONTH_Jul,
		TIMEMONTH_Aug,
		TIMEMONTH_Sep,
		TIMEMONTH_Oct,
		TIMEMONTH_Nov,
		TIMEMONTH_Dec = 11,
		TIMEMONTH_QTY = 12,
	};

	struct CTimeUnit
	{
		//! @struct Gray::CTimeUnit
		//! metadata describing ratios between relative time units in cTimeUnits.
		//! per TIMEUNIT_TYPE Unit
		const GChar_t* m_pszUnitNameL;	//!< long unit name
		const GChar_t* m_pszUnitNameS;	//!< short abbreviated unit name
		TIMEUNIT_t m_uMin;
		TIMEUNIT_t m_uMax;
		WORD m_uSubRatio;				//!< How many sub units in this unit. (for absolute units. e.g. not months or years)
		TIMESECD_t m_nUnitSeconds;		//!< Total seconds for a unit. (for absolute units)
		double m_dUnitDays;				//!< Total days or fractions of a day for the unit. (for absolute units)
	};

	enum TIMEUNIT_TYPE
	{
		//! @enum Gray::TIMEUNIT_TYPE
		//! Enumerate TIMEUNIT_t (16 bit max) elements of cTimeUnits and cTimeParser
		TIMEUNIT_UNUSED = -1,	//!< Marks end.
		TIMEUNIT_Year = 0,		//!< e.g. 2008. (1<=x<=3000)
		TIMEUNIT_Month,			//!< base 1, NOT Base 0 like = TIMEMONTH_Jan. (1<=x<=12)
		TIMEUNIT_Day,			//!< day of month. base 1. (1<=x<=31)
		TIMEUNIT_Hour,			//!< hour of day. 24 hour scale. base 0. (0<=x<=23)
		TIMEUNIT_Minute,		//!< base 0. (0<=x<=59)
		TIMEUNIT_Second,		//!< base 0. (0<=x<=59)
		TIMEUNIT_Millisecond,	//!< 1/1000 = thousandth of a second. (0<=x<=999)
		TIMEUNIT_Microsecond,	//!< millionth of a second. (0<=x<=999)
		TIMEUNIT_TZ,			//!< TZ + DST
		TIMEUNIT_QTY,			//!< END of cTimeUnits
		// used for parsing only.
		TIMEUNIT_DOW,			//!< Ignore this for units storage. its redundant.
		TIMEUNIT_Ignore,		//!< Just ignore this duplicate. We have already dealt with it.
		TIMEUNIT_Numeric,		//!< A numeric value of unknown type (parsing).
		TIMEUNIT_QTY2,			//!< END of cTimeParser
	};

	class GRAYCORE_LINK cTimeUnits
	{
		//! @class Gray::cTimeUnits
		//! Decompose/Break time into units in order of size.
		//! like: struct tm for POSIX time_t.
		//! like: SYSTEMTIME for _WIN32
		//! like: TIMESTAMP_STRUCT for SQL_TIMESTAMP, SQL_C_TIMESTAMP, SQL_DATE

	public:
		TIMEUNIT_t m_wYear;			//!< Year valid for 1980 to 2043 at least. TIMEUNIT_Year
		TIMEUNIT_t m_wMonth;		//!< 1 based month of year. Jan=1, to 12=Dec, NOT TIMEMONTH_TYPE. TIMEUNIT_Month
		TIMEUNIT_t m_wDay;			//!< 1 based day of month. 1 to 31. TIMEUNIT_Day
		TIMEUNIT_t m_wHour;			//!< 0 to 23 for hour of day.
		TIMEUNIT_t m_wMinute;		//!< 0 to 59
		TIMEUNIT_t m_wSecond;		//!< 0 to 59
		TIMEUNIT_t m_wMillisecond;	//!< 1000th = thousandth. 0 to 1000
		TIMEUNIT_t m_wMicrosecond;	//!< 1000000th = millionth. 0 to 1000. TIMEUNIT_Microsecond

		TIMEUNIT_t m_nTZ;			//!< TZ_TYPE for m_wHour. TIMEUNIT_TZ

		static const TIMESECD_t k_nSecondsPerDay = (24 * 60 * 60);		//!< seconds in a day = 86400
		static const TIMESECD_t k_nSecondsPerHour = (60 * 60);		//!< seconds in a hour = 3600
		static const int k_nMinutesPerDay = (24 * 60);		//!< minutes in a day
		static const int k_nMicroSeconds = 1000000;			//!< millionth of a second.

		static const CTimeUnit k_Units[TIMEUNIT_QTY];			//!< Metadata for time units.

		static const StrLen_t k_FormStrMax = 256;	// max reasonable size for time.
		static const GChar_t* k_StrFormats[TIME_FORMAT_QTY + 1];	//!< standard strftime() type formats.

		static const BYTE k_MonthDays[2][TIMEMONTH_QTY];		//!< Jan=0
		static const WORD k_MonthDaySums[2][TIMEMONTH_QTY + 1];	//!< Jan=0

		// may change for language ?
		static const GChar_t* const k_MonthName[TIMEMONTH_QTY + 1];		//!< January=0
		static const GChar_t* const k_MonthAbbrev[TIMEMONTH_QTY + 1];	//!< Jan=0

		static const GChar_t* const k_DayName[TIMEDOW_QTY + 1];		//!< Sunday=0
		static const GChar_t* const k_DayAbbrev[TIMEDOW_QTY + 1];	//!< Sun=0

		static const GChar_t k_TimeSeparator = ':';				//!< this is the same for all formats. e.g. 09:00 AM. NOT USED ?
		static const GChar_t k_Seps[3];	// Normal valid string separators. "/:" = all sm_DateSeparator or sm_TimeSeparator
		static const GChar_t k_SepsAll[8]; 	// All/Any separator that might occur in k_StrFormats.

		// May change for regional preferences ?
		static GChar_t sm_DateSeparator;			//!< date separator to use for string creation = '\', '-', '.', but Time is always ':'
		static bool sm_time24Mode;					//!< Display time in 24 hour format. default = false

	public:
#ifdef _WIN32
		cTimeUnits(const SYSTEMTIME& sysTime);
		bool GetSys(SYSTEMTIME& sysTime) const noexcept;
		void SetSys(const SYSTEMTIME& sysTime);
#endif
		void SetZeros();
		bool InitTimeNow(TZ_TYPE nTimeZoneOffset = TZ_LOCAL);

		COMPARE_TYPE Compare(cTimeUnits& b) const;
		bool isTimeFuture() const;
		bool isTimePast() const
		{
			//! AKA Expired ?
			return !isTimeFuture();
		}

		TIMEDOW_TYPE get_DOW() const  
		{
			//! Calculated Day of Week
			//! @return TIMEDOW_Sun = 0
			return GetDOW(m_wYear, m_wMonth, m_wDay);
		}
		int get_DOY() const
		{
			//! Day of year. 0 based
			return GetDOY(m_wYear, m_wMonth, m_wDay);
		}
		TIMEMONTH_TYPE get_Month() const
		{
			return (TIMEMONTH_TYPE)(m_wMonth - 1);
		}

		bool IsValidUnit(TIMEUNIT_TYPE i) const;
		bool isValidTimeUnits() const;
		bool isReasonableTimeUnits() const;

		TIMEUNIT_t GetUnit(TIMEUNIT_TYPE i) const
		{
			//! enumerate the time units.
			ASSERT(IS_INDEX_GOOD(i, TIMEUNIT_QTY));
			return (&m_wYear)[i];
		}
		TIMEUNIT_t GetUnit0(TIMEUNIT_TYPE i) const
		{
			// Zero based units.
			return GetUnit(i) - k_Units[i].m_uMin;
		}
		void SetUnit(TIMEUNIT_TYPE i, TIMEUNIT_t wVal)
		{
			ASSERT(IS_INDEX_GOOD(i, TIMEUNIT_QTY));
			(&m_wYear)[i] = wVal;
		}

		bool operator == (const cTimeUnits& rTu) const
		{
			return cMem::IsEqual(this, &rTu, sizeof(rTu)) ;	// All of it ?
		}

		void put_DosDate(UINT32 ulDosDate);
		UINT32 get_DosDate() const;

		void AddMonths(int iMonths);
		void AddDays(int iDays);
		void AddSeconds(TIMESECD_t nSeconds);
		void AddTZ(TZ_TYPE nTimeZoneOffset);

		bool isInDST() const;

		StrLen_t GetFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, const GChar_t* pszFormat) const;
		StrLen_t GetFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, TIME_FORMAT_TYPE eFormat = TIME_FORMAT_DEFAULT) const
		{
			return GetFormStr(pszOut, iOutSizeMax, (const GChar_t*)eFormat);
		}
		HRESULT SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset);
		StrLen_t GetTimeSpanStr(GChar_t* pszOut, StrLen_t iOutSizeMax, TIMEUNIT_TYPE eUnitHigh = TIMEUNIT_Day, int iUnitsDesired = 2, bool bShortText = false) const;

		static int GRAYCALL IsLeapYear(TIMEUNIT_t wYear);
		static int GRAYCALL GetLeapYearsSince2K(TIMEUNIT_t wYear);
		static TIMEDOW_TYPE GRAYCALL GetDOW(TIMEUNIT_t wYear, TIMEUNIT_t wMonth, TIMEUNIT_t wDay);
		static int GRAYCALL GetDOY(TIMEUNIT_t wYear, TIMEUNIT_t wMonth, TIMEUNIT_t wDay);

		TIMESECD_t get_SecondOfDay() const noexcept
		{
			return m_wSecond + (m_wMinute * 60) + (TIMESECD_t)(m_wHour * 60 * 60);
		}
		bool isValidMonth() const noexcept
		{
			return m_wMonth >= 1 && m_wMonth <= 12;
		}
		TIMEUNIT_t get_DaysInMonth() const
		{
			// How many days in m_wMonth ?
			if (!isValidMonth())
				return 0;
			int iLeapYear = IsLeapYear(m_wYear);
			return k_MonthDays[iLeapYear][m_wMonth - 1];
		}
		TIMEUNIT_t get_DayOfYear() const
		{
			// What day of m_wYear is this ?
			if (!isValidMonth())
				return 0;
			int iLeapYear = IsLeapYear(m_wYear);
			return k_MonthDaySums[iLeapYear][m_wMonth - 1] + (m_wDay - 1);
		}
		TIMEUNIT_t get_DaysInYear() const
		{
			// How many days in m_wYear ?
			return (IsLeapYear(m_wYear) == 0) ? 365 : 366;
		}

		cTimeUnits(void)
		{
			SetZeros();	// a valid time?
		}
		cTimeUnits(TIMEUNIT_t wYear, TIMEUNIT_t wMonth, TIMEUNIT_t wDay, TIMEUNIT_t wHour = 0, TIMEUNIT_t wMinute = 0, TIMEUNIT_t wSecond = 0, TIMEUNIT_t wMilliseconds = 0, TIMEUNIT_t wMicroseconds = 0, TZ_TYPE nTZ = TZ_UTC)
			: m_wYear(wYear)
			, m_wMonth(wMonth)
			, m_wDay(wDay)
			, m_wHour(wHour)
			, m_wMinute(wMinute)
			, m_wSecond(wSecond)
			, m_wMillisecond(wMilliseconds)
			, m_wMicrosecond(wMicroseconds)
			, m_nTZ((TIMEUNIT_t)nTZ)
		{
		}
	};

	//*******************************************************
	struct cTimeParserUnit
	{
		//! @struct Gray::cTimeParserUnit
		//! Helper for parsing time units from string.

		TIMEUNIT_TYPE m_Type;	//!< What type of field does this look like. best guess. TIMEUNIT_Sec
		TIMEUNIT_t m_nValue;	//!< Value we read from the field.	<0 = null/omitted.
		StrLen_t m_iOffsetSep;	//!< End of the type info and start of the separator.
		GChar_t m_Separator;		//!< What sort of separator follows ? ":T /.,-"

		void Init()
		{
			m_Type = TIMEUNIT_UNUSED;
			m_nValue = -1;		// not set yet.
			m_iOffsetSep = -1;
			m_Separator = -1;	// not set yet.
		}
		TIMEUNIT_TYPE get_HashCode() const noexcept	// should be only one of each type.
		{
			return m_Type;
		}
		TIMEUNIT_t get_SortValue() const noexcept
		{
			return m_nValue;
		}
	};

	class GRAYCORE_LINK cTimeParser
	{
		//! @class Gray::cTimeParser
		//! Try to interpret/parse a string as a date/time.
		//! Hold result of the first parsing pass to (perhaps) process the time string as cTimeUnits.

	public:
		cTimeParserUnit m_Unit[TIMEUNIT_QTY2];		//!< space for parsed results.
		int m_iUnitsParsed;		//!< m_Unit used. <TIMEUNIT_QTY2
		int m_iUnitsMatched;	//!< m_iUnitsMatched <= m_iUnitsParsed and all m_Type are set. No use of TIMEUNIT_Numeric

	public:
		static bool GRAYCALL TestMatchUnit(const cTimeParserUnit& u, TIMEUNIT_TYPE t);
		static TIMEUNIT_TYPE GRAYCALL GetTypeFromFormatCode(GChar_t ch);
		int FindType(TIMEUNIT_TYPE t) const;
		void SetUnitFormats(const GChar_t* pszFormat);

		StrLen_t ParseNamedUnit(const GChar_t* pszName);
		HRESULT ParseString(const GChar_t* pszTimeString, const GChar_t* pszSeparators = nullptr);
		bool isMatched() const
		{
			return m_iUnitsMatched > 0;
		}
		StrLen_t GetMatchedLength() const
		{
			//! How much of the parsed string was consumed by the match?
			ASSERT(m_iUnitsMatched <= m_iUnitsParsed);
			if (m_iUnitsMatched <= 0)
				return 0;
			int i = m_iUnitsMatched - 1;
			ASSERT(IS_INDEX_GOOD_ARRAY(i, m_Unit));
			return m_Unit[i].m_iOffsetSep;
		}

		bool TestMatchFormat(const cTimeParser& parserFormat, bool bTrimJunk = false);
		bool TestMatch(const GChar_t* pszFormat);
		bool TestMatches(const GChar_t** ppStrFormats = nullptr);

		HRESULT GetTimeUnits(OUT cTimeUnits& tu) const;

		cTimeParser()
			: m_iUnitsParsed(0)
			, m_iUnitsMatched(0)
		{
			m_Unit[0].Init();
		}
		cTimeParser(const GChar_t* pszTimeString)
			: m_iUnitsParsed(0)
			, m_iUnitsMatched(0)
		{
			ParseString(pszTimeString);
		}
		cTimeParser(const GChar_t* pszTimeString, const GChar_t** ppStrFormats)
			: m_iUnitsParsed(0)
			, m_iUnitsMatched(0)
		{
			// Matches
			ParseString(pszTimeString);
			TestMatches(ppStrFormats);
		}
	};
}
#endif
