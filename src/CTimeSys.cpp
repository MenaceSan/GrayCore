//
//! @file cTimeSys.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cTimeSys.h"
#include "cTimeUnits.h"
#include "cTypes.h"

namespace Gray {

#ifdef _WIN32
TIMEPERF_t cTimePerf::sm_nFreq = 0;  /// MUST call static InitFreq()  GRAYCORE_LINK
#endif

bool GRAYCALL cTimePerf::InitFreq() noexcept { // static
    //! need to call this once in _WIN32 to capture the k_nFreq
    if (sm_nFreq != 0) return true;
#if 0  // UNDER_CE
		::timeBeginPeriod(1);
#endif
#ifdef _WIN32
    if (!::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&sm_nFreq))) {
        sm_nFreq = cTimeSys::k_FREQ;  // milliSec freq = default.
        return false;
    }
#endif
    return true;
}

double GRAYCALL cTimePerf::ToDays(TIMEPERF_t t) noexcept { // static
    //! Convert cTimePerf to double days (from arbitrary start time).
    //! @return time in days since some unknown/arbitrary starting point

    DEBUG_CHECK(sm_nFreq > 0);  // ASSUME cTimePerf::InitFreq();
    const double dFreq = (double)sm_nFreq * (double)cTimeUnits::k_nSecondsPerDay;
    double dCount = (double)t;
    double dVal = dCount / dFreq;
    return dVal;
}

void cTimePerf::InitTimeNow() noexcept {
    //! QueryPerformanceCounter() is better than 'rdtsc' for multi core.
    //! available >= Windows 2000
#ifdef _WIN32
    if (!::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_nTime))) {
        m_nTime = cTimeSys::GetTimeNow();
    }
#else  // __linux__
    cTimeSpec tNow;
    tNow.InitTimeNow();
    m_nTime = tNow.get_nSec();  // nano-sec.
#endif
}

//*************************************************************************
TIMESYS_t GRAYCALL cTimeSys::GetTimeNow() noexcept { // static
    //! @note ASSUME this is FAST !
    //! _WIN32 is limited to the resolution of the system timer, which is typically in the range of 10 milliseconds to 16 milliseconds.
#ifdef _WIN32
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724408(v=vs.85).aspx
#ifdef USE_64BIT
    return CastN(TIMESYS_t, ::GetTickCount64());  // why not always use 64 bits ? what header is this in ? fails for DotNetX ??
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

unsigned long GRAYCALL cTimeSys::WaitSpin(TIMESYSD_t nmSecs) { // static
    //! Wait a certain amount of time. Spin instead of sleeping.
    //! Do NOT yield time to other threads. (NOT Sleep)

    VOLATILE unsigned long i = 0;  // for busy-waiting
    VOLATILE unsigned long j = 0;  // to prevent optimization

    cTimeSys t(cTimeSys::GetTimeNow());
    while (t.get_AgeSys() < nmSecs) {
        i++;
    }

    j = i;
    return j;
}

bool cTimerSys::OnTickCheck(TIMESYS_t now) noexcept {
    const TIMESYSD_t diff = CastN(TIMESYSD_t, m_TimeSys - now);
    if (diff < 0)  // wait
        return false;
    if (m_TimeSys == k_CLEAR || m_TimeSys == k_INF)  // invalid!
        return false;
    if (_Freq == 0) {
        m_TimeSys = k_INF;
        return true;  // trigger now but never again.
    }
    if (diff <= _Freq) {
        // schedule again for the future.
        m_TimeSys += _Freq;
        return true;
    }
    // Missed multiple ticks!?? go again ASAP!
    m_TimeSys = now + 1;
    return true;
}
}  // namespace Gray
