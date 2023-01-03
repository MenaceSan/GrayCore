//
//! @file cTimeDouble.h
//! Data Time similar to COleDateTime
//! Elapsed days since (midnight, December 30, 1899 GMT).
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeDouble_H
#define _INC_cTimeDouble_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "cTimeUnits.h"
#include "cTimeFile.h"

namespace Gray
{
	class GRAYCORE_LINK cTimeDouble
	{
		//! @class Gray::cTimeDouble
		//! same as DATE, COleDateTime
		//! same as _WIN32 VariantTimeToSystemTime, same as DATE, COleDateTime
		//! Absolute 64 bit double days since (1899/12/30 midnight GMT).
		//! double 1.0 = 1 day
		//! NOT the same as REFTIME which is (double) seconds.
	public:
		static const int k_nDaysDiffTimeInt = 25569;	//!< days difference from cTimeDouble (1899) to cTimeInt (1970) bases // similar to __linux__ SECS_1601_TO_1970 ?
		static const double k_nY2K;		//!< The static value for y2k = January 1, 2000 in UTC/GMT
		static const double k_nY10;		//!< The first 10 years are sometimes reserved to act as offsets.
		static const int k_nZero = 0;	//!< double cant be used for in-class initializer

	protected:
		double	m_dateTime;	//!< DATE = days since (midnight, 30 December 1899 GMT), fraction = time of day

	protected:
		bool InitTimeUnits(const cTimeUnits& rTu);
		void DecodeDate(cTimeUnits& rTu) const;

	public:
		cTimeDouble(const cTimeDouble& dt) noexcept
			: m_dateTime(dt.m_dateTime)
		{
		}
		cTimeDouble(const double dTime = k_nZero) noexcept
			: m_dateTime(dTime)
		{
		}

		static cTimeDouble GRAYCALL EncodeSeconds(double s) noexcept;
		static cTimeDouble GRAYCALL EncodeTime(short h, short m, short s, short ms = 0) noexcept;
		static cTimeDouble GRAYCALL EncodeDate(short year = 0, short month = 0, short day = 0) noexcept;
		static cTimeDouble GRAYCALL GetTimeFromSec(TIMESEC_t nTimeSec) noexcept;

		cTimeDouble(const TIMESEC_t nTimeSec)
			: m_dateTime(GetTimeFromSec(nTimeSec))
		{
		}

		static cTimeDouble GRAYCALL GetTimeFromFile(const cTimeFile& ft) noexcept;
		cTimeDouble(const cTimeFile& ft)
			: m_dateTime(GetTimeFromFile(ft))
		{
		}

		static cTimeDouble GRAYCALL GetTimeFromStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset)
		{
			// Ignore HRESULT.
			cTimeDouble t;
			t.SetTimeStr(pszDateTime, nTimeZoneOffset);
			return t;
		}
		cTimeDouble(const cTimeUnits& rTu)
		{
			//! like SystemTimeToVariantTime but it ASSUMES GMT
			//!  m_dateTime = 1 = whole days since 1900
			InitTimeUnits(rTu);
		}

		void InitTime(double dTime = k_nZero) noexcept
		{
			m_dateTime = dTime;	//!< dTime = 0 = clear to invalid time.
		}
		void InitTimeNow();

		static constexpr bool IsTimeValid(double dTime)
		{
			return(dTime > k_nZero);
		}
		inline bool isTimeValid() const
		{
			return IsTimeValid(m_dateTime);
		}
		double get_Double() const noexcept
		{
			//! Arbitrary units. same as days.
			return m_dateTime;
		}
		double get_Days() const noexcept
		{
			//! Get total days since epoch.
			return m_dateTime;
		}
		operator double(void) const noexcept
		{
			//! Get total days since epoch.
			return m_dateTime;
		}

		void operator = (const cTimeDouble& date) noexcept
		{
			m_dateTime = date.m_dateTime;
		}
		void operator = (const GChar_t* pszDateTime)
		{
			SetTimeStr(pszDateTime, TZ_UTC);
		}

		cTimeDouble operator + (int  i) noexcept
		{
			//! Add days.
			return cTimeDouble(m_dateTime + i);
		}
		cTimeDouble operator - (int  i) noexcept
		{
			//! Subtract days.
			return cTimeDouble(m_dateTime - i);
		}
		cTimeDouble operator + (const cTimeDouble& dt) noexcept
		{
			return cTimeDouble(m_dateTime + dt.m_dateTime);
		}
		cTimeDouble operator - (const cTimeDouble& dt) noexcept
		{
			return cTimeDouble(m_dateTime - dt.m_dateTime);
		}

		cTimeDouble& operator += (int idays) noexcept	//!< days
		{
			//! Add days.
			m_dateTime += idays;
			return *this;
		}
		cTimeDouble& operator -= (int idays) noexcept	//!< days
		{
			//! Subtract days.
			m_dateTime -= idays;
			return *this;
		}
		cTimeDouble& operator += (const cTimeDouble& dt) noexcept
		{
			m_dateTime += dt.m_dateTime;
			return *this;
		}
		cTimeDouble& operator -= (const cTimeDouble& dt) noexcept
		{
			m_dateTime -= dt.m_dateTime;
			return *this;
		}

		cTimeDouble& operator ++ () noexcept    //!< Prefix increment
		{
			m_dateTime += 1;
			return *this;
		}
		cTimeDouble& operator ++ (int) noexcept //!< Postfix increment
		{
			m_dateTime += 1; // add a day
			return *this;
		}
		cTimeDouble& operator -- () noexcept   //!< Prefix decrement
		{
			m_dateTime -= 1;
			return *this;
		}
		cTimeDouble& operator -- (int) noexcept  //!< Postfix decrement
		{
			m_dateTime -= 1;
			return *this;
		}

		friend bool operator <  (const cTimeDouble& dt1, const cTimeDouble& dt2);
		friend bool operator <= (const cTimeDouble& dt1, const cTimeDouble& dt2);
		friend bool operator >  (const cTimeDouble& dt1, const cTimeDouble& dt2);
		friend bool operator >= (const cTimeDouble& dt1, const cTimeDouble& dt2);
		friend bool operator == (const cTimeDouble& dt1, const cTimeDouble& dt2);
		friend bool operator != (const cTimeDouble& dt1, const cTimeDouble& dt2);

		static cTimeDouble GRAYCALL GetTimeNow();
		static cTimeDouble GRAYCALL Date();
		static cTimeDouble GRAYCALL Time();

		cTimeFile GetAsFileTime() const;
		bool GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZoneOffset) const;

		TIMEDOW_TYPE get_DayOfWeek() const;

		unsigned GetDate() const noexcept	//!< Numeric date of date object
		{
			//! Get Total days as an integer.
			return CastN(unsigned, m_dateTime);
		}

		double get_DaysTil() const
		{
			//! diff in days
			//! - = this is in the past.
			//! + = this is in the future.
			return get_Double() - GetTimeNow();
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
	};

	bool inline operator < (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return(dt1.m_dateTime < dt2.m_dateTime);
	}
	bool inline operator <= (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return((dt1.m_dateTime <= dt2.m_dateTime));
	}
	bool inline operator > (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return(dt1.m_dateTime > dt2.m_dateTime);
	}
	bool inline operator >= (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return((dt1.m_dateTime >= dt2.m_dateTime));
	}
	bool inline operator == (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return(dt1.m_dateTime == dt2.m_dateTime);
	}
	bool inline operator != (const cTimeDouble& dt1, const cTimeDouble& dt2)
	{
		return(dt1.m_dateTime != dt2.m_dateTime);
	}
}
#endif // _INC_cTimeDouble_H
