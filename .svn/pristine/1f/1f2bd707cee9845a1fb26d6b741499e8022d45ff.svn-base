//
//! @file CTimeDouble.h
//! Data Time similar to COleDateTime
//! Elapsed days since (midnight, December 30, 1899 GMT).
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CTimeDouble_H
#define _INC_CTimeDouble_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CString.h"
#include "CTimeUnits.h"
#include "CTimeFile.h"

UNITTEST_PREDEF(CTimeDouble)

namespace Gray
{
	class GRAYCORE_LINK CTimeDouble
	{
		//! @class Gray::CTimeDouble
		//! same as DATE, COleDateTime
		//! same as _WIN32 VariantTimeToSystemTime, same as DATE, COleDateTime
		//! Absolute 64 bit double days since (1899/12/30 midnight GMT).
		//! double 1.0 = 1 day
		//! NOT the same as REFTIME which is (double) seconds.
	public:
		static const int k_nDaysDiffTimeInt = 25569;	//!< days difference from CTimeDouble (1899) to CTimeInt (1970) bases // similar to __linux__ SECS_1601_TO_1970 ?
		static const double k_nY2K;		//!< The static value for y2k = January 1, 2000 in UTC/GMT
		static const double k_nY10;		//!< The first 10 years are sometimes reserved to act as offsets.
		static const int k_nZero = 0;	//!< double cant be used for in-class initializer

	protected:
		double	m_dateTime;	//!< DATE = days since (midnight, 30 December 1899 GMT), fraction = time of day

	protected:
		bool InitTimeUnits(const CTimeUnits& rTu);
		void DecodeDate(CTimeUnits& rTu) const;

	public:
		CTimeDouble(const CTimeDouble& dt)
			: m_dateTime(dt.m_dateTime)
		{
		}
		CTimeDouble(const double dTime = k_nZero)
			: m_dateTime(dTime)
		{
		}

		static CTimeDouble GRAYCALL EncodeSeconds(double s);
		static CTimeDouble GRAYCALL EncodeTime(short h, short m, short s, short ms = 0);
		static CTimeDouble GRAYCALL EncodeDate(short year = 0, short month = 0, short day = 0);
		static CTimeDouble GRAYCALL GetTimeFromSec(TIMESEC_t nTimeSec);

		CTimeDouble(const TIMESEC_t nTimeSec)
			: m_dateTime(GetTimeFromSec(nTimeSec))
		{
		}

		static CTimeDouble GRAYCALL GetTimeFromFile(const CTimeFile& ft);
		CTimeDouble(const CTimeFile& ft)
			: m_dateTime(GetTimeFromFile(ft))
		{
		}

		static CTimeDouble GRAYCALL GetTimeFromStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset)
		{
			// Ignore HRESULT.
			CTimeDouble t;
			t.SetTimeStr(pszDateTime, nTimeZoneOffset);
			return t;
		}
		CTimeDouble(const CTimeUnits& rTu)
		{
			//! like SystemTimeToVariantTime but it ASSUMES GMT
			//!  m_dateTime = 1 = whole days since 1900
			InitTimeUnits(rTu);
		}

		void InitTime(double dTime = k_nZero)
		{
			m_dateTime = dTime;	//!< dTime = 0 = clear to invalid time.
		}
		void InitTimeNow();

		static inline bool IsTimeValid(double dTime)
		{
			return(dTime > k_nZero);
		}
		bool isTimeValid() const
		{
			return IsTimeValid(m_dateTime);
		}
		double get_Double() const
		{
			//! Arbitrary units. same as days.
			return m_dateTime;
		}
		double get_Days() const
		{
			//! Get total days since epoch.
			return m_dateTime;
		}
		operator double(void) const
		{
			//! Get total days since epoch.
			return m_dateTime;
		}

		void operator = (const CTimeDouble& date)
		{
			m_dateTime = date.m_dateTime;
		}
		void operator = (const GChar_t* pszDateTime)
		{
			SetTimeStr(pszDateTime, TZ_UTC);
		}

		CTimeDouble operator + (int  i)
		{
			//! Add days.
			return CTimeDouble(m_dateTime + i);
		}
		CTimeDouble operator - (int  i)
		{
			//! Subtract days.
			return CTimeDouble(m_dateTime - i);
		}
		CTimeDouble operator + (const CTimeDouble& dt)
		{
			return CTimeDouble(m_dateTime + dt.m_dateTime);
		}
		CTimeDouble operator - (const CTimeDouble& dt)
		{
			return CTimeDouble(m_dateTime - dt.m_dateTime);
		}

		CTimeDouble& operator += (int idays)	//!< days
		{
			//! Add days.
			m_dateTime += idays;
			return *this;
		}
		CTimeDouble& operator -= (int idays)	//!< days
		{
			//! Subtract days.
			m_dateTime -= idays;
			return *this;
		}
		CTimeDouble& operator += (const CTimeDouble& dt)
		{
			m_dateTime += dt.m_dateTime;
			return *this;
		}
		CTimeDouble& operator -= (const CTimeDouble& dt)
		{
			m_dateTime -= dt.m_dateTime;
			return *this;
		}

		CTimeDouble& operator ++ ()     //!< Prefix increment
		{
			m_dateTime += 1;
			return *this;
		}
		CTimeDouble& operator ++ (int)  //!< Postfix increment
		{
			m_dateTime += 1; // add a day
			return *this;
		}
		CTimeDouble& operator -- ()     //!< Prefix decrement
		{
			m_dateTime -= 1;
			return *this;
		}
		CTimeDouble& operator -- (int)  //!< Postfix decrement
		{
			m_dateTime -= 1;
			return *this;
		}

		friend bool operator <  (const CTimeDouble &dt1, const CTimeDouble &dt2);
		friend bool operator <= (const CTimeDouble &dt1, const CTimeDouble &dt2);
		friend bool operator >  (const CTimeDouble &dt1, const CTimeDouble &dt2);
		friend bool operator >= (const CTimeDouble &dt1, const CTimeDouble &dt2);
		friend bool operator == (const CTimeDouble &dt1, const CTimeDouble &dt2);
		friend bool operator != (const CTimeDouble &dt1, const CTimeDouble &dt2);

		static CTimeDouble GRAYCALL GetTimeNow();
		static CTimeDouble GRAYCALL Date();
		static CTimeDouble GRAYCALL Time();

		CTimeFile GetAsFileTime() const;
		bool GetTimeUnits(OUT CTimeUnits& rTu, TZ_TYPE nTimeZoneOffset) const;

		TIMEDOW_TYPE get_DayOfWeek() const;      //!< MFC GetDayOfWeek is +1

		unsigned GetDate() const             //!< Numeric date of date object
		{
			//! Get Total days as an integer.
			return unsigned(m_dateTime);
		}

		double get_DaysTil() const
		{
			//! diff in days
			//! - = this is in the past.
			//! + = this is in the future.
			return(get_Double() - GetTimeNow());
		}
		double get_DaysAge() const
		{
			//! How old is this? (in days)
			//! current time - this time.
			//! + = this is in the past.
			//! - = this is in the future.
			return(-get_DaysTil());
		}

		// to/from strings.
		HRESULT SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset = TZ_UTC);
		cString GetTimeFormStr(const GChar_t* pszFormat = nullptr, TZ_TYPE nTimeZoneOffset = TZ_LOCAL) const;
		cString GetTimeFormStr(TIME_FORMAT_TYPE eFormat, TZ_TYPE nTimeZoneOffset = TZ_LOCAL) const
		{
			return GetTimeFormStr((const GChar_t*)eFormat, nTimeZoneOffset);
		}

		static cString GRAYCALL GetTimeSpanStr(double dDays, TIMEUNIT_TYPE eUnitHigh = TIMEUNIT_Day, int iUnitsDesired = 2, bool bShortText = false);

		UNITTEST_FRIEND(CTimeDouble);
	};

	bool inline operator < (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return(dt1.m_dateTime < dt2.m_dateTime);
	}
	bool inline operator <= (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return((dt1.m_dateTime <= dt2.m_dateTime));
	}
	bool inline operator > (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return(dt1.m_dateTime > dt2.m_dateTime);
	}
	bool inline operator >= (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return((dt1.m_dateTime >= dt2.m_dateTime));
	}
	bool inline operator == (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return(dt1.m_dateTime == dt2.m_dateTime);
	}
	bool inline operator != (const CTimeDouble &dt1, const CTimeDouble &dt2)
	{
		return(dt1.m_dateTime != dt2.m_dateTime);
	}
};
#endif // _INC_CTimeDouble_H
