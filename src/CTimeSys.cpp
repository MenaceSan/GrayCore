//
//! @file cTimeSys.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cTimeSys.h"
#include "cTimeUnits.h"
#include "cTypes.h"

namespace Gray
{

#ifdef _WIN32
	TIMEPERF_t cTimePerf::sm_nFreq = 0;	//!< MUST call static InitFreq()  GRAYCORE_LINK
#endif

	bool GRAYCALL cTimePerf::InitFreq() noexcept // static
	{
		//! need to call this once in _WIN32 to capture the k_nFreq
		if (sm_nFreq != 0)
			return true;
#if 0	// UNDER_CE
		::timeBeginPeriod(1);
#endif
#ifdef _WIN32
		if (!::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&sm_nFreq)))
		{
			sm_nFreq = cTimeSys::k_FREQ;	// milliSec freq = default.
			return false;
		}
#endif
		return true;
	}

	double GRAYCALL cTimePerf::ToDays(TIMEPERF_t t) noexcept // static
	{
		//! Convert cTimePerf to double days (from arbitrary start time).
		//! @return time in days since some unknown/arbitrary starting point
		
		DEBUG_CHECK(sm_nFreq > 0);	// ASSUME cTimePerf::InitFreq();
		const double dFreq = (double)sm_nFreq * (double)cTimeUnits::k_nSecondsPerDay;
		double dCount = (double)t;
		double dVal = dCount / dFreq;
		return dVal;
	}

	void cTimePerf::InitTimeNow() noexcept
	{
		//! QueryPerformanceCounter() is better than 'rdtsc' for multi core.
		//! available >= Windows 2000
#ifdef _WIN32
		if (!::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_nTime)))
		{
			m_nTime = cTimeSys::GetTimeNow();
		}
#else	// __linux__
		cTimeSpec tNow;
		tNow.InitTimeNow();
		m_nTime = tNow.get_nSec();	// nano-sec.
#endif
		}

	//*************************************************************************

	unsigned long GRAYCALL cTimeSys::WaitSpin(TIMESYSD_t nmSecs) // static
	{
		//! Wait a certain amount of time. Spin instead of sleeping.
		//! Do NOT yield time to other threads. (NOT Sleep)

		VOLATILE unsigned long i = 0; // for busy-waiting
		VOLATILE unsigned long j = 0; // to prevent optimization

		cTimeSys t(cTimeSys::GetTimeNow());
		while (t.get_AgeSys() < nmSecs)
		{
			i++;
		}

		j = i;
		return j;
	}
	}
