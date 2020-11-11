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
	TIMEPERF_t cTimePerf::k_nFreq = 0;	//!< MUST call InitFreq()
#endif

	void GRAYCALL cTimePerf::InitFreq() // static
	{
		//! need to call this once in _WIN32 to capture the k_nFreq
#if 0	// UNDER_CE
		::timeBeginPeriod(1);
#endif
#ifdef _WIN32
		if (!::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&k_nFreq)))
		{
			k_nFreq = cTimeSys::k_FREQ;	// milliSec freq = default.
		}
#endif
	}

	double GRAYCALL cTimePerf::ToDays(TIMEPERF_t t) // static
	{
		//! Convert cTimePerf to double days (from arbitrary start time).
		//! @return time in days since some unknown/arbitrary starting point
		ASSERT(k_nFreq > 0);	// MUST call cTimePerf::InitFreq()
		const double dFreq = (double)k_nFreq * (double)cTimeUnits::k_nSecondsPerDay;
		double dCount = (double)t;
		double dVal = dCount / dFreq;
		return dVal;
	}

	void cTimePerf::InitTimeNow()
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

//*************************************************************************
#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cThreadLock.h"
#include "cLogMgr.h"

UNITTEST_CLASS(cTimeSys)
{
	void TestTimerPerf()
	{
		//
		// Allow one failure for possible counter wrapping.
		// On a 4Ghz 32-bit machine the cycle counter wraps about once per second;
		// since the whole test is about 10ms, it shouldn't happen twice in a row.
		//
		int hardfail = 0;
		bool bSuccess = false;
		while (!bSuccess)
		{
			UNITTEST_TRUE(hardfail <= 1);

			// Get a reference ratio cycles/ms
			TIMEPERF_t ratio = 0;

			// Check that the ratio is mostly constant
			bSuccess = true;
			for (unsigned long millisecs = 1; millisecs <= 4; millisecs++)
			{
				cTimePerf tStart(true);
				unsigned long iCount = cTimeSys::WaitSpin(millisecs);
				UNITTEST_TRUE(iCount > 0);

				TIMEPERF_t cycles = tStart.get_AgePerf();
				if (ratio <= 0)
				{
					ratio = cycles / millisecs;
					continue;
				}
				// Allow variation up to 20%
				if (cycles / millisecs < ratio - ratio / 5 ||
					cycles / millisecs > ratio + ratio / 5)
				{
					hardfail++;
					ratio = 0;
					bSuccess = false;
					break;
				}
			}
		}
	}

	UNITTEST_METHOD(cTimeSys)
	{
		cTimePerf::InitFreq();

		cTimePerf tStart0(true);
		cTimeSys tNow = cTimeSys::GetTimeNow();
		TIMEPERF_t tDiff0 = tStart0.get_AgePerf();
		UNITTEST_TRUE(tDiff0 >= 0);	// e.g. 2	// NOTE This can be so fast it looks like 0 ?
		UNITTEST_TRUE(tNow.get_TimeSys() >= 1);

		cTimePerf tStart1(true);
		cThreadId::SleepCurrent(0);
		TIMEPERF_t tDiff1 = tStart1.get_AgePerf();
		UNITTEST_TRUE(tDiff1 >= 1);

		cTimePerf tStart2(true);
		cThreadId::SleepCurrent(1);
		TIMEPERF_t tDiff2 = tStart2.get_AgePerf();
		UNITTEST_TRUE(tDiff2 > 1);

		const TIMESYSD_t k_nLen = 200;
		const TIMESYSD_t k_nLenVar = 20;	// Allow variation up to 10%

		for (int tocks = 1; tocks <= 3; tocks++)
		{
			cTimeSys hires = cTimeSys::GetTimeNow();
			cThreadId::SleepCurrent((int)(k_nLen * tocks));
			TIMESYSD_t nAge = hires.get_AgeSys();
			UNITTEST_TRUE(nAge >= (k_nLen - k_nLenVar) * tocks);
			UNITTEST_TRUE(nAge <= (k_nLen + k_nLenVar) * tocks);
		}
	}
};
UNITTEST_REGISTER(cTimeSys, UNITTEST_LEVEL_Core);	// UNITTEST_LEVEL_Core
#endif
