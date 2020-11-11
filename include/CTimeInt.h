//
//! @file cTimeInt.h
//! Elapsed seconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC), according to the system clock.
//! Real Time, 32 bit (or 64 bit) SIGNED seconds in old UNIX format.
//! Replace the MFC CTime function. Must be usable with file system.
//! Accurate measure of whole seconds.
//! Valid from 1970 to 2038 in unsigned 32 bits. (http://en.wikipedia.org/wiki/Year_2038_problem).
//! @note TIMESEC_t is signed! It really should not be!
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeInt_H
#define _INC_cTimeInt_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "cTimeFile.h"

UNITTEST_PREDEF(cTimeInt)

namespace Gray
{
#ifndef _MFC_VER
	class GRAYCORE_LINK CTime
	{
		//! @class Gray::CTime
		//! TIMESEC_t or time_t stored as a UINT32 32 bits or 64 bits ?
		//! seconds since January 1, 1970 GMT
		//! Emulate the MFC CTime functionality
		//! @note 32 bit version of this has a clock rollover in 2038.
		//! This is actually number of seconds since January 1, 1970 GMT
		//! <= 0 is considered invalid.

	protected:
		TIMESEC_t m_time;	//!< Seconds. Essentially the UNIX long time format. (not usable after 2018 unless 64 bit?)

	public:
		CTime(TIMESEC_t nTime = ((TIMESEC_t)0))
		: m_time(nTime)
		{
		}
		CTime(const cTimeFile& fileTime, int nDST = -1);

		const CTime& operator=(const CTime& timeSrc)
		{
			m_time = timeSrc.m_time;
			return *this;
		}
		const CTime& operator=(TIMESEC_t nTime)
		{
			m_time = nTime;
			return *this;
		}

		bool operator<=(TIMESEC_t nTime) const
		{
			return(m_time <= nTime);
		}
		bool operator==(TIMESEC_t nTime) const
		{
			return(m_time == nTime);
		}
		bool operator!=(TIMESEC_t nTime) const
		{
			return(m_time != nTime);
		}
		bool operator>=(CTime ttime) const
		{
			return(m_time >= ttime.m_time);
		}

		operator TIMESEC_t() const
		{
			return m_time;
		}
		TIMESEC_t GetTime() const	// Assume time in seconds. (MFC like)
		{
			return m_time;
		}
		TIMESEC_t GetTotalSeconds() const
		{
			return m_time;
		}
	};

	class GRAYCORE_LINK cTimeSpan
	{
		//! @class Gray::cTimeSpan
		//! Emulate the MFC CTime functionality
	public:
		int m_nDiffSeconds;
	public:
		cTimeSpan()
		{
		}
	};
#endif

	class GRAYCORE_LINK cTimeInt	//!< similar to the MFC CTime and cTimeSpan, not as accurate or large ranged as COleDateTime
	: public CTime		//!< no need to dupe MFC function.
	{
		//! @class Gray::cTimeInt
		//! the number of seconds elapsed since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC), according to the system clock
		//! ASSUME __time64_t is signed! MFC uses __time64_t
		//! Same as UNIX_TIMESTAMP() for MySQL
		//! @note 32 bit version of this has a clock rollover in 2038.

		typedef CTime SUPER_t;

	public:
		static const TIMESEC_t k_nZero = ((TIMESEC_t)0);		//!< January 1, 1970 UTC
		static const TIMESEC_t k_nY2K = ((TIMESEC_t)0x386d4380);	//!< The static value for Y2K = January 1, 2000 in UTC/GMT from k_nZero in seconds.

	protected:
		bool InitTimeUnits(const cTimeUnits& rTu);

	public:
		cTimeInt()	// init to zero
		{}
		cTimeInt(TIMESEC_t time) : CTime(time)
		{}
		cTimeInt(const cTimeFile& fileTime) : CTime(fileTime, -1)
		{
			//! @note both are UTC so nDST makes no sense. What is MFC thinking ??
		}
		cTimeInt(const cTimeUnits& rTu)
		{
			InitTimeUnits(rTu);
		}
		static TIMESEC_t GRAYCALL GetTimeFromDays(double dTimeDays);
		cTimeInt(double dTimeDays) : CTime(GetTimeFromDays(dTimeDays))
		{
		}

		static cTimeInt GRAYCALL GetTimeFromStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZoneOffset)
		{
			// Ignore HRESULT.
			cTimeInt t;
			t.SetTimeStr(pszDateTime, nTimeZoneOffset);
			return t;
		}

#ifdef _MFC_VER
		TIMESEC_t GetTime() const	// Assume time in total seconds. (MFC like)
		{ return (TIMESEC_t) SUPER_t::GetTime(); } // convert 64 bit time to old 32 bit form?
#endif

		static cTimeInt GRAYCALL GetTimeNow();

		static cTimeInt GRAYCALL GetCurrentTime()
		{
			//! Alternate name for MFC.
			//! @note GetCurrentTime() is "#define" by _WIN32 to GetTickCount() so i cant use that name!
			return GetTimeNow();
		}

		void InitTimeNow();
		void InitTimeNowPlusSec(TIMESECD_t iOffsetInSeconds);
		void InitTime(TIMESEC_t nTime = k_nZero);

		cTimeFile GetAsFileTime() const;
		bool GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZoneOffset= TZ_UTC) const;

		// non MFC CTime operations.
		TIMESECD_t GetSecondsSince(const cTimeInt& time) const
		{
			//! difference in seconds,
			//! - = this is in the past. (time in future)
			//! + = this is in the future. (time in past)
			return((TIMESECD_t)(GetTime() - time.GetTime()));
		}
		TIMESECD_t get_TimeTilSec() const
		{
			//! difference in seconds
			//! - = this is in the past.
			//! + = this is in the future.
			cTimeInt timeNow;
			timeNow.InitTimeNow();
			return((TIMESECD_t)(GetTime() - timeNow.GetTime()));
		}
		TIMESECD_t get_AgeSec() const
		{
			//! How old is this? (in seconds)
			//! current time - this time.
			return(-get_TimeTilSec());
		}
		bool isTimeFuture() const
		{
			return((unsigned)GetTime() > (unsigned)GetTimeNow().GetTime());
		}

		static inline bool IsTimeValid(TIMESEC_t nTime)
		{
			return(nTime > k_nZero);
		}
		bool isTimeValid() const
		{
			//! MFC does 64 -> 32 bits.
			return(IsTimeValid((TIMESEC_t)GetTime()));
		}
		int get_TotalDays() const // like in COleDateTimeSpan
		{
			//! Needs to be more consistent than accurate. just for compares.
			//! Should turn over at midnight.
			return((int)(GetTime() / cTimeUnits::k_nSecondsPerDay));
		}

		// to/from strings.
		HRESULT SetTimeStr(const GChar_t* pszTimeDate, TZ_TYPE nTimeZoneOffset = TZ_LOCAL);
		StrLen_t GetTimeFormStr(GChar_t* pszOut, StrLen_t iOutSizeMax, const GChar_t* pszFormat, TZ_TYPE nTimeZoneOffset = TZ_LOCAL) const;
		cString GetTimeFormStr(const GChar_t* pszFormat = nullptr, TZ_TYPE nTimeZoneOffset = TZ_LOCAL) const;
		cString GetTimeFormStr(TIME_FORMAT_TYPE eFormat, TZ_TYPE nTimeZoneOffset = TZ_LOCAL) const
		{
			return GetTimeFormStr((const GChar_t*)eFormat, nTimeZoneOffset);
		}

		static cString GRAYCALL GetTimeSpanStr(TIMESECD_t dwSeconds, TIMEUNIT_TYPE eUnitHigh = TIMEUNIT_Day, int iUnitsDesired = 2, bool bShortText = false);
		static cString GRAYCALL GetTimeDeltaBriefStr(TIMESECD_t dwSeconds);
		static cString GRAYCALL GetTimeDeltaSecondsStr(TIMESECD_t dwSeconds);

		UNITTEST_FRIEND(cTimeInt);
	};

};

#endif // _INC_cTimeInt_H
