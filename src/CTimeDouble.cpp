//
//! @file CTimeDouble.cpp
//! Time in double days. Accurate Measure whole milli seconds
//! 0 = (midnight, 30 December 1899 GMT).
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CTimeDouble.h"
#include "CTimeZone.h"
#include "CString.h"
#include "CTimeInt.h"
#include "CLogMgr.h"
#include <math.h>	// floor()

namespace Gray
{
	const double CTimeDouble::k_nY2K = 36526.0;	//!< The static value for y2k = January 1, 2000 in UTC/GMT
	const double CTimeDouble::k_nY10 = 3650.0;		//!< The first 10 years are sometimes reserved to act as offsets.

	CTimeDouble GRAYCALL CTimeDouble::EncodeSeconds(double s) // static
	{
		//! Encode GMT Time
		return CTimeDouble(s / CTimeUnits::k_nSecondsPerDay);
	}
	CTimeDouble GRAYCALL CTimeDouble::EncodeTime(short h, short m, short s, short ms) // static
	{
		//! Encode GMT Time
		//! Same as MFC COleDateTime::EncodeTime
		return CTimeDouble((double)(h + ((m + (s + (ms / 1000.0)) / 60.0) / 60.0)) / 24.0);
	}

	CTimeDouble GRAYCALL CTimeDouble::EncodeDate(short wYear, short wMonth, short wDay) // static
	{
		//! Encode GMT Date as days since (1899/12/30 midnight GMT) or (1900/1/1 0:0:0 GMT)
		//! same as SystemTimeToVariantTime() but more accurate for mSec

		if (wYear <= 0 || wYear > 9999)
		{
			return 0.0; // "Invalid Year in the date";
		}
		if (wMonth < 1 || wMonth > 12)
		{
			return 0.0; // "Invalid month in the date";
		}
		int nLeapYear = CTimeUnits::IsLeapYear(wYear);
		if (wDay < 1 || wDay > CTimeUnits::k_MonthDays[nLeapYear][wMonth - 1])
		{
			return 0.0; // "Invalid day in the date";
		}

		int iYear = wYear - 1;	// 1999 format. from year 0
		int iDays = (iYear * 365);	// add years

		// like CTimeUnits::GetLeapYearsSince2K
		iDays += (iYear / 4) + (iYear / 400) - (iYear / 100);	// years compensated for leaps.
		iDays -= 693594L;	// Offset so that midnight 12/30/1899 is 0, midnight 12/31/1899 = 1
		iDays += CTimeUnits::k_MonthDaySums[nLeapYear][wMonth - 1];
		iDays += wDay;	// of month

		return CTimeDouble((double)iDays);
	}

	void CTimeDouble::DecodeDate(CTimeUnits& rTu) const
	{
		// Decode just the date portion. NOT time.

		// Round to the second
		const double k_HALF_SECOND = (1.0 / (CTimeUnits::k_nSecondsPerDay * 2));	// Half a second, expressed in days.
		double dblDate = m_dateTime + ((m_dateTime > 0.0) ? k_HALF_SECOND : (-k_HALF_SECOND));

		// Number of days since 1/1/0
		long nDaysAbsolute = (long)dblDate + 693959L; // Add days from 1/1/0 to 12/30/1899

		dblDate = ABS(dblDate);

		// Leap years every 4 yrs except centuries not multiples of 400.
		// Number of 400 year increments since 1/1/0
		long n400Years = (long)(nDaysAbsolute / 146097L);

		// Set nDaysAbsolute to day within 400-year block
		nDaysAbsolute %= 146097L;

		// -1 because first century has extra day
		// Century within 400 year block (0,1,2 or 3)
		long n400Century = (long)((nDaysAbsolute - 1) / 36524L);

		long n4Years;           // Number of 4 year increments since 1/1/0
		long n4Day;             // Day within 4 year block (0 is 1/1/yr1, 1460 is 12/31/yr4)
		bool bLeap4 = true;     // true if 4 year block includes leap year

		// Non-leap century
		// like CTimeUnits::GetLeapYearsSince2K
		if (n400Century != 0)
		{
			// Set nDaysAbsolute to day within century
			nDaysAbsolute = (nDaysAbsolute - 1) % 36524L;

			// +1 because 1st 4 year increment has 1460 days
			n4Years = (long)((nDaysAbsolute + 1) / 1461L);

			if (n4Years != 0)
				n4Day = (long)((nDaysAbsolute + 1) % 1461L);
			else
			{
				bLeap4 = false;
				n4Day = (long)nDaysAbsolute;
			}
		}
		else
		{
			// Leap century - not special case!
			n4Years = (long)(nDaysAbsolute / 1461L);
			n4Day = (long)(nDaysAbsolute % 1461L);
		}

		long n4Yr;              // Year within 4 year block (0,1,2 or 3)
		if (bLeap4)
		{
			// -1 because first year has 366 days
			n4Yr = (n4Day - 1) / 365;

			if (n4Yr != 0)
				n4Day = (n4Day - 1) % 365;
		}
		else
		{
			n4Yr = n4Day / 365;
			n4Day %= 365;
		}

		// n4Day is now 0-based day of year. Save 1-based day of year, year number
		rTu.m_wYear = (WORD)(n400Years * 400 + n400Century * 100 + n4Years * 4 + n4Yr);

		// Handle leap year: before, on, and after Feb. 29.
		if (n4Yr == 0 && bLeap4)
		{
			// Leap Year
			if (n4Day == 59)
			{
				// Feb. 29
				rTu.m_wMonth = 2;
				rTu.m_wDay = 29;
				return;
			}

			// Pretend it's not a leap year for month/day comp.
			if (n4Day >= 60)
				--n4Day;
		}

		// Make n4DaY a 1-based day of non-leap year and compute
		//  month/day for everything but Feb. 29.
		++n4Day;

		// Month number always >= n/32, so save some loop time
		rTu.m_wMonth = (WORD)((n4Day >> 5) + 1);
		for (; n4Day > CTimeUnits::k_MonthDaySums[0][rTu.m_wMonth]; rTu.m_wMonth++)
		{
		}

		rTu.m_wDay = (WORD)(n4Day - CTimeUnits::k_MonthDaySums[0][rTu.m_wMonth - 1]);
	}

	bool CTimeDouble::GetTimeUnits(OUT CTimeUnits& rTu, TZ_TYPE nTimeZoneOffset) const
	{
		//! Convert to CTimeUnits as TZ_UTC = TZ_GMT = NOT localized system time.
		//! Number of days since Dec. 30, 1899
		//! VariantTimeToSystemTime
		//! @return
		//!  month = 1-12
		//!  day =

		DecodeDate(rTu);

		// DecodeTime
		double dTime = m_dateTime - ((int)m_dateTime);	// time portion.

		int secs = int(dTime * CTimeUnits::k_nSecondsPerDay + 0.5);
		rTu.m_wHour = short(secs / (60 * 60));
		secs = secs % (60 * 60);
		rTu.m_wMinute = short(secs / 60);
		rTu.m_wSecond = short(secs % 60);

		rTu.m_wMillisecond = 0;	// lost milliseconds ???
		rTu.m_wMicrosecond = 0;

		rTu.AddTZ(nTimeZoneOffset);
		return true;
	}

	bool CTimeDouble::InitTimeUnits(const CTimeUnits& rTu)
	{
		if (!rTu.isValidTimeUnits())
		{
			return false;
		}
		m_dateTime = EncodeDate(rTu.m_wYear, rTu.m_wMonth, rTu.m_wDay);
		if (isTimeValid())
		{
			m_dateTime += EncodeTime(rTu.m_wHour, rTu.m_wMinute, rTu.m_wSecond, rTu.m_wMillisecond).get_Double();
		}
		if (rTu.m_nTZ != TZ_UTC)
		{
			// adjust timezone.
			TIMEUNIT_t nTimeZoneOffset = rTu.m_nTZ;
			if (nTimeZoneOffset > TZ_MAX)
			{
				nTimeZoneOffset = (TIMEUNIT_t)CTimeZoneMgr::GetLocalTimeZoneOffset();
			}
			m_dateTime += (nTimeZoneOffset / (double)CTimeUnits::k_nMinutesPerDay);
			if (rTu.isInDST())
			{
				m_dateTime -= (1.0 / 24.0);	// remove added hour.
			}
		}
		return true;
	}

	//********************************************

	CTimeFile CTimeDouble::GetAsFileTime() const
	{
		//! convert double time to file system time
		//!  CTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT
		double nTmp = m_dateTime;
		nTmp += 109205;	// magic offset number = 109205 days = 94353120000000000
		nTmp *= (864000000000.0); // days to .1uSec , 10M*60*60*24
		return CTimeFile((FILETIME_t)nTmp);
	}

	CTimeDouble GRAYCALL CTimeDouble::GetTimeNow() // static
	{
		//!  CTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT/UTC
		return GetTimeFromFile(CTimeFile::GetTimeNow());
	}

	CTimeDouble CTimeDouble::Date() // static
	{
		//! Get just whole days portion for now.
		double dDays = GetTimeNow();
		return double(int(dDays));
	}

	CTimeDouble CTimeDouble::Time() // static
	{
		//! Time of day now = 0 to 1
		double dDays = GetTimeNow();
		return dDays - int(dDays);
	}

	//*******************************************************************

	CTimeDouble GRAYCALL CTimeDouble::GetTimeFromFile(const CTimeFile& ft) // static
	{
		//! CTimeFile = 64-bit 100-nanosec since January 1, 1601 GMT
		//! double = days since (midnight, 30 December 1899 GMT)
		FILETIME_t tmp = ft.get_Val();
		double dTimeDays = ((double)tmp) / (864000000000.0); // .1uSec -> days 10M*60*60*24
		dTimeDays -= 109205; // magic offset number = 109205 days = 94353120000000000
		return dTimeDays;
	}

	CTimeDouble CTimeDouble::GetTimeFromSec(TIMESEC_t nTimeSec) // static
	{
		//! convert TIMESEC_t (seconds) to double (days)
		//! Opposite of CTimeInt::GetTimeFromDays()
		double dTimeDays = (double)nTimeSec;
		dTimeDays /= CTimeUnits::k_nSecondsPerDay;
		dTimeDays += CTimeDouble::k_nDaysDiffTimeInt;
		return dTimeDays;
	}

	void CTimeDouble::InitTimeNow()
	{
		m_dateTime = GetTimeNow();
	}

	TIMEDOW_TYPE CTimeDouble::get_DayOfWeek(void) const
	{
		//! Sunday is 0, TIMEDOW_TYPE
		//! MFC does (sun=1) but we don't
		int idays = (int)m_dateTime;
		return (TIMEDOW_TYPE)((idays - 1) % TIMEDOW_QTY);
	}

	//***********************************************************************
	// string routines

	HRESULT CTimeDouble::SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset)
	{
		//! Set the time/date from a string date time .
		//! Attempt to determine format from the string/numbers given.

		if (pszDateTime == nullptr)
			return E_POINTER;

		if (!StrT::CmpI(pszDateTime, _GT("now")))
		{
			InitTimeNow();   // Sets the current date & time
			return 3;
		}
		if (!StrT::CmpI(pszDateTime, _GT("today")))
		{
			m_dateTime = Date();        // Sets the current date
			return 5;
		}
		if (!StrT::CmpI(pszDateTime, _GT("time")))
		{
			m_dateTime = Time();        // Sets the current time
			return 4;
		}

		CTimeUnits Tu;
		HRESULT hRes = Tu.SetTimeStr(pszDateTime, nTimeZoneOffset);
		if (hRes <= 0)
		{
			m_dateTime = 0;
			return 0;
		}

		m_dateTime = CTimeDouble(Tu);
		return hRes;
	}

	cString CTimeDouble::GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZoneOffset) const
	{
		//! Get the time as a string formatted using "C" strftime()
		//! MFC just calls this "Format"
		CTimeUnits Tu;
		if (!GetTimeUnits(Tu, nTimeZoneOffset))
		{
			return "";
		}
		GChar_t szBuffer[256];
		StrLen_t iLenChars = Tu.GetFormStr(szBuffer, STRMAX(szBuffer), pszFormat);
		if (iLenChars <= 0)
		{
			return "";
		}
		return cString(szBuffer, iLenChars);
	}

	//*************************************************

	cString GRAYCALL CTimeDouble::GetTimeSpanStr(double dDays, TIMEUNIT_TYPE eUnitHigh, int iUnitsDesired, bool bShortText) // static
	{
		//! Describe a delta/span of time from days to milliseconds.
		//! iUnitHigh = 0 = days.
		//! @todo MERGE THIS WITH CTimeUnits::GetTimeSpanStr version?!

		if (dDays <= 0)
		{
			return bShortText ? _GT("0s") : _GT("0 seconds");
		}
		if (iUnitsDesired < 1)
		{
			iUnitsDesired = 1;	// must have at least 1.
		}

		const int kUnitMax = _countof(CTimeUnits::k_Units) - 1; // Skip TIMEUNIT_TZ of course.
		if (IS_INDEX_BAD(eUnitHigh, kUnitMax))
		{
			// Just take a default.
			eUnitHigh = TIMEUNIT_Day;	// days is highest unit by default.
		}

		int iUnitsPrinted = 0;
		GChar_t szMsg[256];
		szMsg[0] = '\0';
		StrLen_t iMsgLen = 0;

		bool bMostSignificantFound = false;
		UINT i = (UINT)eUnitHigh;
		for (; i < kUnitMax - 1; i++)
		{
			const double dUnits = CTimeUnits::k_Units[i].m_dUnitDays;
			if (!bMostSignificantFound)
			{
				if (dDays < dUnits)
					continue;
				bMostSignificantFound = true;
			}

			if (i >= TIMEUNIT_Second)	// sub seconds print as a decimal.
				break;
			const int nQtyOfUnit = (int)floor(dDays / dUnits);

			if (iMsgLen)
			{
				iMsgLen += StrT::CopyLen(szMsg + iMsgLen, _GT(" "), STRMAX(szMsg) - iMsgLen); // " and ";
			}
			if (bShortText)
			{
				iMsgLen += StrT::sprintfN(szMsg + iMsgLen, STRMAX(szMsg) - iMsgLen,
					_GT("%u%s"), nQtyOfUnit, StrArg<GChar_t>(CTimeUnits::k_Units[i].m_pszUnitNameS));
			}
			else if (nQtyOfUnit == 1)
			{
				iMsgLen += StrT::CopyLen(szMsg + iMsgLen, _GT("1 "), STRMAX(szMsg) - iMsgLen);
				iMsgLen += StrT::CopyLen(szMsg + iMsgLen, CTimeUnits::k_Units[i].m_pszUnitNameL, STRMAX(szMsg) - iMsgLen);
			}
			else
			{
				iMsgLen += StrT::sprintfN(szMsg + iMsgLen, STRMAX(szMsg) - iMsgLen,
					_GT("%u %ss"), nQtyOfUnit, StrArg<GChar_t>(CTimeUnits::k_Units[i].m_pszUnitNameL));
			}
			iUnitsPrinted++;
			if (iUnitsPrinted >= iUnitsDesired)		// only print iUnitsDesired most significant units of time
				break;

			dDays -= nQtyOfUnit * dUnits;
			if (dDays <= 0)
				break;
		}

		if (iUnitsDesired > 0 &&
			iUnitsPrinted < iUnitsDesired &&
			dDays > 0 &&
			i >= TIMEUNIT_Second)
		{
			// remainder is always decimal.
			iMsgLen += StrT::sprintfN(szMsg + iMsgLen, STRMAX(szMsg) - iMsgLen, _GT(" %g %s%s"),
				dDays / CTimeUnits::k_Units[i].m_dUnitDays,
				bShortText ? CTimeUnits::k_Units[i].m_pszUnitNameS : CTimeUnits::k_Units[i].m_pszUnitNameL,
				bShortText ? "" : "s"	// plural.
			);
		}

		return szMsg;
	}
}

//*****************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CTimeDouble)
{
	bool UnitTest_1(const CTimeUnits& rTu) // static
	{
		CTimeUnits Tu2;
		CTimeInt timeTest(rTu);
		timeTest.GetTimeUnits(Tu2, TZ_UTC);
		UNITTEST_TRUE(rTu == Tu2);

		CTimeDouble datetimeTest(rTu);
		datetimeTest.GetTimeUnits(Tu2, TZ_UTC);
		UNITTEST_TRUE(rTu == Tu2);

#if 0
		int iTimeDays = timeTest.GetTime();
		iTimeDays /= CTimeUnits::k_nSecondsPerDay;
		// TIMEDOUBLE_TO_TIME_DAY
		double dDaysDiff = datetimeTest.get_Days() - iTimeDays;
#endif

		// InitTimeDouble()
		CTimeInt timeTest2(datetimeTest);
		UNITTEST_TRUE(timeTest2 == timeTest);

		// GetTimeFromSec()
		CTimeDouble datetimeTest2 = CTimeDouble::GetTimeFromSec(timeTest.GetTime());
		double dSecsDiff = datetimeTest2.get_Days() - datetimeTest.get_Days();
		dSecsDiff = ABS(dSecsDiff) / CTimeUnits::k_nSecondsPerDay;
		UNITTEST_TRUE(dSecsDiff <= 0.0001); // close?

#ifdef _WIN32
		// VariantTime is the same as COleDateTime compare
		SYSTEMTIME sysTime;
		rTu.GetSys(sysTime);
		double dTest;
		::SystemTimeToVariantTime(&sysTime, &dTest);
		UNITTEST_TRUE(dTest == datetimeTest.get_Double());

		::VariantTimeToSystemTime(dTest, &sysTime);
		UNITTEST_TRUE(rTu == CTimeUnits(sysTime));
#endif
		return true;
	}

	UNITTEST_METHOD(CTimeDouble)
	{
		CTimeDouble tnow1;
		tnow1.InitTimeNow();
		UNITTEST_TRUE(tnow1.isTimeValid());
		CTimeUnits tn;
		UNITTEST_TRUE(tnow1.GetTimeUnits(tn, TZ_LOCAL));
		UNITTEST_TRUE(tn.isReasonableTimeUnits());

		sm_pLog->addDebugInfoF("Current local time is '%s'", LOGSTR(tnow1.GetTimeFormStr(TIME_FORMAT_TZ, TZ_LOCAL)));

		CTimeUnits tu0(1900, 1, 1, 0);	// 0
		// CTimeUnits tu0( 1899,  12, 31, 24 );	// midnight
		CTimeDouble t0(tu0);
		UNITTEST_TRUE(t0.get_Days() == 2);

		CTimeDouble y2k;
		HRESULT hRes = y2k.SetTimeStr(_GT("2000/1/1 00:00:00"), TZ_UTC);
		UNITTEST_TRUE(hRes > 0);
		double dd = y2k.get_Days();
		UNITTEST_TRUE(dd == CTimeDouble::k_nY2K);
		hRes = y2k.SetTimeStr(_GT("2000/1/1 01:00:00"), TZ_UTC);
		UNITTEST_TRUE(hRes > 0);
		dd = y2k.get_Days() - (CTimeDouble::k_nY2K + 1.0 / 24.0);
		UNITTEST_TRUE(ABS(dd) <= 0.0001);

		// Try a bunch of samples dates to convert back and forth.
		// Valid from 1970 to 2038
		static const CTimeUnits k_sysTimes[] = // DOW is tested elsewhere
		{
			CTimeUnits(1971, 2, 1, 12),
			CTimeUnits(1986, 2, 1, 12),
			CTimeUnits(2000, 1, 2, 12),
			CTimeUnits(2004, 8, 2, 12),		// Mon = 38201.5
			CTimeUnits(2004, 12, 30, 12),	// Fri = 38251
			CTimeUnits(2005, 1, 1, 12),		// Sun = 38253
			CTimeUnits(2005, 12, 28, 12),	// Wed = 38713.5
			CTimeUnits(2005, 12, 30, 12),	// Fri
			CTimeUnits(2005, 12, 11, 12),	// Sat
			CTimeUnits(2006, 1, 1, 12),		// Sun
			CTimeUnits(2020, 2, 1, 12),
			CTimeUnits(2037, 2, 1, 12),
		};
		for (UINT i = 0; i < _countof(k_sysTimes); i++)
		{
			UnitTest_1(k_sysTimes[i]);
		}

		CThreadId::SleepCurrent(1);	// may not be accurate enough to advance without sleep?
		CTimeDouble tnow2;
		tnow2.InitTimeNow();
		UNITTEST_TRUE(tnow2.isTimeValid());
		UNITTEST_TRUE(tnow2 > tnow1);

		double dDays = 1 * CTimeUnits::k_Units[TIMEUNIT_Day].m_dUnitDays
			+ 2 * CTimeUnits::k_Units[TIMEUNIT_Hour].m_dUnitDays
			+ 3 * CTimeUnits::k_Units[TIMEUNIT_Minute].m_dUnitDays
			+ 4 * CTimeUnits::k_Units[TIMEUNIT_Second].m_dUnitDays
			+ 5 * CTimeUnits::k_Units[TIMEUNIT_Millisecond].m_dUnitDays
			+ 6 * CTimeUnits::k_Units[TIMEUNIT_Microsecond].m_dUnitDays
			;

		cString s1 = CTimeDouble::GetTimeSpanStr(dDays, TIMEUNIT_Day, 6);
		UNITTEST_TRUE(!s1.Compare(_GT("1 day 2 hours 3 minutes 4.00501 seconds")));
	}
};
UNITTEST_REGISTER(CTimeDouble, UNITTEST_LEVEL_Core);
#endif