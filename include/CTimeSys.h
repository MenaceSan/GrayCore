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

#include "PtrCast.h"
#include "Index.h"
#include <time.h>  // timespec

#ifdef _WIN32
#include <sysinfoapi.h>  // GetTickCount64() (_WIN32_WINNT >= 0x0600)
#endif

namespace Gray {
typedef int TIMESECD_t;    /// signed delta seconds. like TIMESEC_t. redefined in TimeUnits.h.
typedef float TIMESECF_t;  /// delta float seconds.

//! TIMESYS_t = The normal system tick timer. milli-seconds since start of system/app ?
#ifdef USE_64BIT
typedef UINT64 TIMESYS_t;  /// The absolute system milli-Second tick. (NOT the same as a time range!)
typedef INT64 TIMESYSD_t;  /// Time delta. signed milli-Seconds Span. cTimeSys::k_INF = MAILSLOT_WAIT_FOREVER
#else
typedef UINT32 TIMESYS_t;                                  /// The absolute system milli-Second tick. (NOT the same as a time range!)
typedef INT32 TIMESYSD_t;                                  /// Time delta. signed milli-Seconds Span. cTimeSys::k_DMAX, cTimeSys::k_INF = MAILSLOT_WAIT_FOREVER
#endif

#if defined(__linux__)
/// <summary>
/// POSIX CLOCK_MONOTONIC time. (Realtime is from 1970-01-01 UTC)
/// similar to struct timeval/cTimeVal used for select() but use nanoseconds not microseconds.
/// No need to USE clock_getres( clockid_t __clock_id, struct timespec *__res) ?
/// @note link with 'rt' for this.
/// </summary>
class cTimeSpec : public /* struct*/ timespec {
 public:
    static const UINT k_FREQ = 1000000000;  // billionths of a sec.

    cTimeSpec() noexcept {
        // undefined. may use clock_gettime() on it.
    }
    cTimeSpec(TIMESYSD_t nMilliSeconds) noexcept {
        put_mSec(nMilliSeconds);
    }
    cTimeSpec(TIMESECD_t iSeconds, int iNanoSec) noexcept {
        this->tv_sec = iSeconds;
        this->tv_nsec = iNanoSec;  // nano = billionths of a sec.
    }
    void put_mSec(TIMESYSD_t nMilliSeconds) noexcept {
        // milliSeconds.
        this->tv_sec = nMilliSeconds / 1000;
        this->tv_nsec = (nMilliSeconds % 1000) * 1000000;  // mSec to nano = billionths of a sec.
    }
    TIMESYS_t get_mSec() const noexcept {
        //! Get the time as total number of milliSeconds.
        TIMESYS_t nTicks = this->tv_sec * 1000;
        nTicks += this->tv_nsec / 1000000;  // to mSec from nSec
        return nTicks;
    }
    UINT64 get_nSec() const noexcept {
        //! Get the time as UINT64 value in nanoseconds (billionths)
        return (((UINT64)this->tv_sec) * cTimeSpec::k_FREQ) + this->tv_nsec;
    }
    void InitTimeNow() noexcept {
        //! Arbitrary time since system start.
        //! NOT affected by changes to the system time.
        //! @note ASSUME this is FAST!
        ::clock_gettime(CLOCK_MONOTONIC, this);
    }
    void InitTimeNow1() noexcept {
        //! Real time from 1970-01-01 UTC
        //! Might be affected by changes to the system time.
        ::clock_gettime(CLOCK_REALTIME, this);
    }
};
#endif

//****************************************************************************

/// <summary>
/// Time in milliseconds from arbitrary/unknown start time.
/// Unsigned 32 bits will roll every 49.7 days.
/// _WIN32 = start time = when system was last rebooted.
/// </summary>
class GRAYCORE_LINK cTimeSys {
 public:
    static const TIMESYS_t k_CLEAR = 0;
    static const TIMESYS_t k_FREQ = 1000;      /// milliSec per Sec. TIMESYSD_t
    static const TIMESYS_t k_INF = UINT_MAX;   /// INFINITE in _WIN32. MAILSLOT_WAIT_FOREVER
    static const TIMESYSD_t k_DMAX = INT_MAX;  /// Max delta in time.

 protected:
    TIMESYS_t m_TimeSys;

 public:
    cTimeSys() noexcept : m_TimeSys(k_CLEAR) {}
    cTimeSys(TIMESYS_t t) noexcept : m_TimeSys(t) {}

    bool isTimeValid() const noexcept {
        return m_TimeSys > k_CLEAR;
    }
    TIMESYS_t get_TimeSys() const noexcept {
        return m_TimeSys;
    }
    void InitTime(TIMESYS_t t = k_CLEAR) noexcept {
        m_TimeSys = t;
    }

    // ********************************************************
    // compare to GetTimeNow().

    static TIMESYS_t GRAYCALL GetTimeNow() noexcept;
    static unsigned long GRAYCALL WaitSpin(TIMESYSD_t t);

    void InitTimeNow() noexcept {
        m_TimeSys = GetTimeNow();
    }
    void InitTimeNowPlusSys(TIMESYSD_t iOffset) noexcept {
        m_TimeSys = GetTimeNow() + iOffset;
    }
    void InitTimeNowPlusSec(float fOffsetSec) noexcept {
        InitTimeNowPlusSys(CastN(TIMESYSD_t, fOffsetSec * k_FREQ));
    }
    bool isTimeFuture() const noexcept {
        return m_TimeSys > GetTimeNow();  // GetTimeNow
    }

    /// <summary>
    /// How long until this time (msec)
    /// </summary>
    /// <returns>-gt- 0 = m_TimeSys is in the future.</returns>
    TIMESYSD_t get_TimeTilSys() const noexcept {
        if (m_TimeSys == 0) return -k_DMAX;
        if (m_TimeSys == k_INF) return k_DMAX;
        return CastN(TIMESYSD_t, m_TimeSys - GetTimeNow());
    }
    /// <summary>
    /// How long ago was this ? (was TIMESYS_GetAge(x))
    /// </summary>
    /// <returns>signed TIMESYS_t (mSec). -lt- 0 = t is in the future. -gt- 0 = t is in the past.</returns>
    TIMESYSD_t get_AgeSys() const noexcept {
        if (m_TimeSys == 0) return k_DMAX;
        if (m_TimeSys == k_INF) return -k_DMAX;
        return CastN(TIMESYSD_t, GetTimeNow() - m_TimeSys);
    }

    TIMESECF_t get_TimeTilSecF() const noexcept {
        //! in float seconds.
        return get_TimeTilSys() / CastN(TIMESECF_t, k_FREQ);
    }
    TIMESECF_t get_AgeSecF() const noexcept {
        //! in float seconds.
        return get_AgeSys() / CastN(TIMESECF_t, k_FREQ);
    }
    TIMESECD_t get_AgeSec() const noexcept {
        //! How old is this? (in seconds)
        //! current time - this time.
        return CastN(TIMESECD_t, get_AgeSys() / k_FREQ);
    }
};

/// <summary>
/// a repeating timer. Next time some event should occur.
/// </summary>
class GRAYCORE_LINK cTimerSys : public cTimeSys {
    TIMESECD_t _Freq;  // next tick.
 public:
    void put_Freq(TIMESECD_t freq) noexcept {
        _Freq = freq;
    }
    void Init(TIMESYS_t now, TIMESECD_t freq) noexcept {
        m_TimeSys = now;
        _Freq = freq;  // occur again ?
    }
    void InitFreq(TIMESECD_t freq) noexcept {
        InitTimeNowPlusSys(freq);
        _Freq = freq;  // occur again ?
    }
    void Clear() noexcept {
        Init(k_INF, 0);
    }

    bool OnTickCheck(TIMESYS_t now) noexcept;
};

//****************************************************************************

#ifdef _WIN32
typedef LONGLONG TIMEPERF_t;  /// INT64 == LONGLONG  The system very high precision performance timer.
#else
typedef UINT64 TIMEPERF_t;                                 /// The system very high precision performance timer. cTimeSpec
#endif

/// <summary>
/// Very high rate timer. unknown epoch. 64 bit. like the X86 'rdtsc' instruction.
/// TIMEPERF_t = The system very high precision performance timer. Maybe nSec ?
/// </summary>
class GRAYCORE_LINK cTimePerf {
 public:
    TIMEPERF_t m_nTime;  /// Arbitrary start time in k_nFreq units. 64 byte unsigned type.
#ifdef _WIN32
    static TIMEPERF_t sm_nFreq;  /// The frequency might change depending on the machine. Must call InitFreq()
#else                            // __linux__
    static const TIMEPERF_t sm_nFreq = cTimeSpec::k_FREQ;  /// nanosecond accurate. for __linux__ using cTimeSpec
#endif

 public:
    cTimePerf(TIMEPERF_t nTime = 0) noexcept : m_nTime(nTime) {
        //! default = init to 0.
    }
    cTimePerf(int nTime) noexcept : m_nTime(nTime) {
        //! default = init to 0. Allow constants to not have a convert.
    }
    cTimePerf(bool bTrue) noexcept {
        // Indicate I want the current time.
        if (bTrue)
            InitTimeNow();
        else
            m_nTime = 0;  // The test is turned off. don't record time.
    }

    bool isTimeValid() const noexcept {
        return m_nTime != 0;
    }

    static bool GRAYCALL InitFreq() noexcept;
    void InitTimeNow() noexcept;

    TIMEPERF_t get_Perf() const noexcept {
        //! Get the time stamp.
        return m_nTime;
    }
    TIMEPERF_t GetAgeDiff(cTimePerf tStop) const noexcept {
        //! how long ago was this ?
        return tStop.m_nTime - this->m_nTime;
    }
    TIMEPERF_t get_AgePerf() const noexcept {
        //! how long ago was this ?
        const cTimePerf tStop(true);
        return GetAgeDiff(tStop);
    }

    static inline double GRAYCALL ToSeconds(TIMEPERF_t t) noexcept {
        return ((double)t) / ((double)sm_nFreq);
    }
    /// <summary>
    /// convert arbitrary start time to seconds (type = double) TIMESECF_t
    /// Use only as a differential.
    /// </summary>
    double get_Seconds() const noexcept {
        // ASSERT( k_nFreq != 0 );
        return ToSeconds(m_nTime);
    }
    double get_AgeSeconds() const noexcept {
        //! how long ago was this ? TIMESECF_t
        const TIMEPERF_t tDiff = get_AgePerf();
        // ASSERT( k_nFreq != 0 );
        return ToSeconds(tDiff);
    }

    static double GRAYCALL ToDays(TIMEPERF_t t) noexcept;
    double get_Days() const noexcept {
        //! Convert cTimePerf to double days (from arbitrary start time).
        //! @return time in days since some unknown/arbitrary starting point
        return ToDays(m_nTime);
    }
};
}  // namespace Gray
#endif  // _INC_cTimeSys_H
