//
//! @file cTimeSys.h
//! Highest precision timer we can get on this system.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeSys_H
#define _INC_cTimeSys_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cUnitTestDecl.h"
#include "cDebugAssert.h"
#include <time.h> // timespec

UNITTEST_PREDEF(cTimeSys)

namespace Gray
{
	typedef int TIMESECD_t;			//!< signed delta seconds. like TIMESEC_t. redefined in TimeUnits.h.
	typedef float TIMESECF_t;		//!< delta float seconds.

	//! TIMESYS_t = The normal system tick timer. milli-seconds since start of system/app ?
#if 0 // def USE_64BIT
	typedef UINT64	TIMESYS_t;		//!< The absolute system milli-Second tick. (NOT the same as a time range!)
	typedef INT64	TIMESYSD_t;		//!< Time delta. signed milli-Seconds Span. cTimeSys::k_INF = MAILSLOT_WAIT_FOREVER
#else
	typedef UINT32	TIMESYS_t;		//!< The absolute system milli-Second tick. (NOT the same as a time range!)
	typedef INT32	TIMESYSD_t;		//!< Time delta. signed milli-Seconds Span. cTimeSys::k_DMAX, cTimeSys::k_INF = MAILSLOT_WAIT_FOREVER
#endif

#if defined(__linux__)
	class cTimeSpec : public /* struct*/ timespec
	{
		//! @class Gray::cTimeSpec
		//! POSIX CLOCK_MONOTONIC time. (Realtime is from 1970-01-01 UTC)
		//! similar to struct timeval/cTimeVal used for select() but use nanoseconds not microseconds.
		//! No need to USE clock_getres( clockid_t __clock_id, struct timespec *__res) ?
		//! @note link with 'rt' for this.

	public:
		static const UINT k_FREQ = 1000000000;	// billionths of a sec.

		cTimeSpec()
		{
			// undefined. may use clock_gettime() on it.
		}
		cTimeSpec(TIMESYSD_t nMilliSeconds)
		{
			put_mSec(nMilliSeconds);
		}
		cTimeSpec(TIMESECD_t iSeconds, int iNanoSec)
		{
			this->tv_sec = iSeconds;
			this->tv_nsec = iNanoSec;	// nano = billionths of a sec.
		}
		void put_mSec(TIMESYSD_t nMilliSeconds)
		{
			// milliSeconds.
			this->tv_sec = nMilliSeconds / 1000;
			this->tv_nsec = (nMilliSeconds % 1000) * 1000000;	// mSec to nano = billionths of a sec.
		}
		TIMESYS_t get_mSec() const
		{
			//! Get the time as total number of milliSeconds.
			TIMESYS_t nTicks = this->tv_sec * 1000;
			nTicks += this->tv_nsec / 1000000;	// to mSec from nSec
			return(nTicks);
		}
		UINT64 get_nSec() const
		{
			//! Get the time as UINT64 value in nanoseconds (billionths)
			return (((UINT64)this->tv_sec)*cTimeSpec::k_FREQ) + this->tv_nsec;
		}
		void InitTimeNow()
		{
			//! Arbitrary time since system start.
			//! NOT affected by changes to the system time.
			//! @note ASSUME this is FAST!
			::clock_gettime(CLOCK_MONOTONIC, this);
		}
		void InitTimeNow1()
		{
			//! Realtime from 1970-01-01 UTC
			//! Might be affected by changes to the system time.
			::clock_gettime(CLOCK_REALTIME, this);
		}
	};
#endif

	//****************************************************************************

	class GRAYCORE_LINK cTimeSys
	{
		//! @class Gray::cTimeSys
		//! Time in milliseconds from arbitrary/unknown start time.
		//! Unsigned 32 bits will roll every 49.7 days.
		//! _WIN32 = start time = when system was last rebooted.
	public:
		static const TIMESYS_t k_CLEAR = 0;
		static const TIMESYS_t k_FREQ = 1000;		//!< milliSec per Sec
		static const TIMESYS_t k_INF = UINT_MAX;	//!< INFINITE in _WIN32. MAILSLOT_WAIT_FOREVER
		static const TIMESYSD_t k_DMAX = INT_MAX;	//!< Max diff in time.

	private:
		TIMESYS_t m_TimeSys;

	public:
		cTimeSys()
			: m_TimeSys(k_CLEAR)
		{
		}
		cTimeSys(const cTimeSys& t)
			: m_TimeSys(t.m_TimeSys)
		{
		}
		cTimeSys(TIMESYS_t t)
			: m_TimeSys(t)
		{
		}

		static TIMESYS_t inline GetTimeNow()
		{
			//! @note ASSUME this is FAST !
			//! _WIN32 is limited to the resolution of the system timer, which is typically in the range of 10 milliseconds to 16 milliseconds.
#ifdef _WIN32
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724408(v=vs.85).aspx
#if 0 // def USE_64BIT
			return ::GetTickCount64();		// why not use 64 bits ???
#else
			return ::GetTickCount();
#endif
#elif defined(__linux__)
			cTimeSpec tNow;
			tNow.InitTimeNow();
			return tNow.get_mSec();
#else
#error NOOS
#endif
		}

		static unsigned long GRAYCALL WaitSpin(TIMESYSD_t t);

		bool isTimeValid() const noexcept
		{
			return(m_TimeSys > k_CLEAR);
		}
		TIMESYS_t get_TimeSys() const noexcept
		{
			return m_TimeSys;
		}

		void InitTime(TIMESYS_t t = k_CLEAR) noexcept
		{
			m_TimeSys = t;
		}
		void InitTimeNow()
		{
			m_TimeSys = GetTimeNow();
		}
		void InitTimeNowPlusSys(TIMESYSD_t iOffset)
		{
			m_TimeSys = GetTimeNow() + iOffset;
		}
		void InitTimeNowPlusSec(float fOffsetSec)
		{
			InitTimeNowPlusSys((TIMESYSD_t)(fOffsetSec * k_FREQ));
		}
		bool isTimeFuture() const
		{
			return(m_TimeSys > GetTimeNow()); // GetTimeNow
		}

		TIMESYSD_t get_TimeTilSys() const
		{
			//! How long until this time (msec)
			//! @return >0 = m_TimeSys is in the future.
			if (m_TimeSys == 0)
				return -k_DMAX;
			if (m_TimeSys == k_INF)
				return k_DMAX;
			return((TIMESYSD_t)(m_TimeSys - GetTimeNow()));
		}
		TIMESYSD_t get_AgeSys() const
		{
			//! How long ago was this ? (was TIMESYS_GetAge(x))
			//! @return signed TIMESYS_t (mSec)
			//! <0 = t is in the future.
			//! >0 = t is in the past.
			if (m_TimeSys == 0)
				return k_DMAX;
			if (m_TimeSys == k_INF)
				return -k_DMAX;
			return((TIMESYSD_t)(GetTimeNow() - m_TimeSys));
		}

		TIMESECF_t get_TimeTilSecF() const
		{
			//! in float seconds.
			return(get_TimeTilSys() / (TIMESECF_t)k_FREQ);
		}
		TIMESECF_t get_AgeSecF() const
		{
			//! in float seconds.
			return(get_AgeSys() / (TIMESECF_t)k_FREQ);
		}
		TIMESECD_t get_AgeSec() const
		{
			//! How old is this? (in seconds)
			//! current time - this time.
			return(get_AgeSys() / k_FREQ);
		}
		UNITTEST_FRIEND(cTimeSys);
	};

	//****************************************************************************

#ifdef _WIN32
	typedef LONGLONG TIMEPERF_t;	//!< INT64 == LONGLONG  The system very high precision performance timer.
#else
	typedef UINT64 TIMEPERF_t;		//!< The system very high precision performance timer. cTimeSpec
#endif

	class GRAYCORE_LINK cTimePerf
	{
		//! @class Gray::cTimePerf
		//! Very high rate timer. 64 bit. like the X86 'rdtsc' instruction.
		//! TIMEPERF_t = The system very high precision performance timer. Maybe nSec ?

	public:
		TIMEPERF_t m_nTime;				//!< Arbitrary start time in k_nFreq units. 64 byte unsigned type.
#ifdef _WIN32
		static TIMEPERF_t k_nFreq;		//!< The frequency might change depending on the machine. Must call InitFreq()
#else // __linux__
		static const TIMEPERF_t k_nFreq = cTimeSpec::k_FREQ;	//!< nanosecond accurate. for __linux__ using cTimeSpec
#endif

	public:
		cTimePerf(TIMEPERF_t nTime = 0)
			: m_nTime(nTime)
		{
			//! default = init to 0.
		}
		cTimePerf(int nTime)
			: m_nTime(nTime)
		{
			//! default = init to 0. Allow constants to not have a convert.
		}
		cTimePerf(bool bTrue)
		{
			// Indicate I want the current time.
			if (bTrue)
				InitTimeNow();
			else
				m_nTime = 0;
		}

		bool isTimeValid() const
		{
			return m_nTime != 0;
		}

		static void GRAYCALL InitFreq();
		void InitTimeNow();

		TIMEPERF_t get_Perf() const
		{
			//! Get the time stamp.
			return m_nTime;
		}
		TIMEPERF_t GetAgeDiff(cTimePerf tStop) const
		{
			//! how long ago was this ?
			return tStop.m_nTime - this->m_nTime;
		}
		TIMEPERF_t get_AgePerf() const
		{
			//! how long ago was this ?
			cTimePerf tStop(true);
			return GetAgeDiff(tStop);
		}

		static inline double GRAYCALL ToSeconds(TIMEPERF_t t)
		{
			return ((double)t) / ((double)k_nFreq);
		}
		double get_Seconds() const
		{
			//! convert arbitrary start time to seconds (type = double) TIMESECF_t
			// ASSERT( k_nFreq != 0 );
			return ToSeconds(m_nTime);
		}
		double get_AgeSeconds() const
		{
			//! how long ago was this ? TIMESECF_t
			TIMEPERF_t tDiff = get_AgePerf();
			// ASSERT( k_nFreq != 0 );
			return ToSeconds(tDiff);
		}

		static double GRAYCALL ToDays(TIMEPERF_t t);
		double get_Days() const
		{
			//! Convert cTimePerf to double days (from arbitrary start time).
			//! @return time in days since some unknown/arbitrary starting point
			return ToDays(m_nTime);
		}
	};
};
#endif // _INC_cTimeSys_H
