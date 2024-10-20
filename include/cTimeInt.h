//! @file cTimeInt.h
//! Elapsed seconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC), according to the system clock.
//! Real Time, 32 bit (or 64 bit) SIGNED seconds in old UNIX format.
//! Replace the MFC CTime function. Must be usable with file system.
//! Accurate measure of whole seconds.
//! Valid from 1970 to 2038 in unsigned 32 bits. (http://en.wikipedia.org/wiki/Year_2038_problem).
//! @note TIMESEC_t is signed! It really should not be!
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTimeInt_H
#define _INC_cTimeInt_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "cTimeFile.h"

namespace Gray {
/// <summary>
/// Emulate the MFC CTime span functionality
/// </summary>
class GRAYCORE_LINK cTimeSpan {
    TIMESECD_t _nDiffSeconds = 0;
};

/// <summary>
/// the number of seconds elapsed since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC), according to the system clock
/// ASSUME __time64_t is signed! MFC uses __time64_t
/// Emulate the MFC CTime functionality
/// @note 32 bit version of this has a clock rollover in 2038.
/// Same as UNIX_TIMESTAMP() for MySQL
/// TIMESEC_t or time_t stored as a UINT32 32 bits or 64 bits ?
/// -lte- 0 is considered invalid.
/// </summary>
class GRAYCORE_LINK cTimeInt {  /// similar to the MFC CTime and cTimeSpan, not as accurate or large ranged as COleDateTime
 public:
    static const TIMESEC_t k_nZero = static_cast<TIMESEC_t>(0);          /// January 1, 1970 UTC
    static const TIMESEC_t k_nY2K = static_cast<TIMESEC_t>(0x386d4380);  /// The static value for Y2K = January 1, 2000 in UTC/GMT from k_nZero in seconds.
 protected:
    TIMESEC_t _nTimeSec = CastN(TIMESEC_t, 0);  /// Seconds. Essentially the UNIX long time format. (32 bit version is not usable after 2018 unless 64 bit?)
 protected:
    bool InitTimeUnits(const cTimeUnits& rTu);

 public:
    cTimeInt(TIMESEC_t nTime = CastN(TIMESEC_t, 0)) noexcept : _nTimeSec(nTime) {}
    cTimeInt(const cTimeUnits& rTu) {
        InitTimeUnits(rTu);
    }
    cTimeInt(double dTimeDays) noexcept : _nTimeSec(GetTimeFromDays(dTimeDays)) {}
    cTimeInt(const cTimeFile& fileTime) noexcept;
 
    bool operator==(TIMESEC_t nTime) const noexcept {
        return _nTimeSec == nTime;
    }
    bool operator!=(TIMESEC_t nTime) const noexcept {
        return _nTimeSec != nTime;
    }
    bool operator<=(TIMESEC_t nTime) const noexcept {
        return _nTimeSec <= nTime;
    }
    bool operator>=(cTimeInt ttime) const noexcept {
        return _nTimeSec >= ttime._nTimeSec;
    }

    operator TIMESEC_t() const noexcept {
        return _nTimeSec;
    }
    TIMESEC_t GetTime() const noexcept {  // Assume time in seconds. (MFC like)
        return _nTimeSec;
    }
    TIMESEC_t GetTotalSeconds() const noexcept {
        return _nTimeSec;
    }

    /// Get time in seconds from time in days. Opposite of cTimeDouble::GetTimeFromSec()
    static TIMESEC_t GRAYCALL GetTimeFromDays(double dTimeDays) noexcept;

    static cTimeInt GRAYCALL GetTimeFromStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
        // Ignore HRESULT.
        cTimeInt t;
        t.SetTimeStr(pszDateTime, nTimeZone);
        return t;
    }

    void InitTime(TIMESEC_t nTime = k_nZero) noexcept;

    cTimeFile GetAsFileTime() const noexcept;
    bool GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone = TZ_UTC) const;

    // non MFC CTime operations.
    TIMESECD_t GetSecondsSince(const cTimeInt& time) const noexcept {
        //! difference in seconds,
        //! - = this is in the past. (time in future)
        //! + = this is in the future. (time in past)
        return CastN(TIMESECD_t, _nTimeSec - time._nTimeSec);
    }

    static constexpr bool IsTimeValid(TIMESEC_t nTime) noexcept {
        return nTime > k_nZero;
    }
    bool isTimeValid() const noexcept {
        //! MFC does 64 -> 32 bits.
        return IsTimeValid(CastN(TIMESEC_t, _nTimeSec));
    }
    int get_TotalDays() const noexcept {  // like in COleDateTimeSpan
        //! Needs to be more consistent than accurate. just for compares.
        //! Should turn over at midnight.
        return CastN(int, _nTimeSec / cTimeUnits::k_nSecondsPerDay);
    }

    // to/from strings.
    HRESULT SetTimeStr(const GChar_t* pszTimeDate, TZ_TYPE nTimeZone = TZ_LOCAL);
    StrLen_t GetTimeFormStr(cSpanX<GChar_t> ret, const GChar_t* pszFormat, TZ_TYPE nTimeZone = TZ_LOCAL) const;
    cString GetTimeFormStr(const GChar_t* pszFormat = nullptr, TZ_TYPE nTimeZone = TZ_LOCAL) const;
    cString GetTimeFormStr(TIMEFORMAT_t eFormat, TZ_TYPE nTimeZone = TZ_LOCAL) const {
        return GetTimeFormStr(CastNumToPtrT<const GChar_t>(static_cast<int>(eFormat)), nTimeZone);
    }

    /// <summary>
    /// Describe a range of time in text. Get a text description of amount of time (delta)
    /// </summary>
    /// <param name="nSeconds"></param>
    /// <param name="eUnitHigh">the highest unit, TIMEUNIT_t::_Day, TIMEUNIT_t::_Minute</param>
    /// <param name="iUnitsDesired">the number of units up the cTimeUnits::k_Units ladder to go. default=2</param>
    /// <param name="bShortText"></param>
    /// <returns></returns>
    static cString GRAYCALL GetTimeSpanStr(TIMESECD_t dwSeconds, TIMEUNIT_t eUnitHigh = TIMEUNIT_t::_Day, int iUnitsDesired = 2, bool bShortText = false);

    /// <summary>
    /// Describe a range of time in text. Get a short text description of amount of time (delta).
    /// </summary>
    /// <param name="dwSeconds"></param>
    /// <returns>e.g. "2h 2m 2s"</returns>
    static cString GRAYCALL GetTimeDeltaBriefStr(TIMESECD_t dwSeconds);

    /// <summary>
    /// Get the Full time description. (up to hours) NOT days. 
    /// </summary>
    /// <param name="dwSeconds"></param>
    /// <returns>e.g. "x hours and y minutes and z seconds"</returns>
    static cString GRAYCALL GetTimeDeltaSecondsStr(TIMESECD_t dwSeconds);

    // ********************************************************
    // compare to GetTimeNow().

    static cTimeInt GRAYCALL GetTimeNow() noexcept;
    void InitTimeNow() noexcept;
    void InitTimeNowPlusSec(TIMESECD_t iOffsetInSeconds) noexcept;
    bool isTimeFuture() const noexcept {
        return CastN(unsigned, _nTimeSec) > CastN(unsigned, GetTimeNow()._nTimeSec);
    }
    /// <summary>
    /// difference in seconds.
    /// - = this is in the past.
    /// + = this is in the future.
    /// </summary>
    /// <returns>seconds</returns>
    TIMESECD_t get_TimeTilSec() const noexcept {
        cTimeInt timeNow;
        timeNow.InitTimeNow();
        return CastN(TIMESECD_t, _nTimeSec - timeNow._nTimeSec);
    }
    /// <summary>
    /// How old is this? (in seconds)
    /// current time - this time.
    /// </summary>
    /// <returns>seconds</returns>
    TIMESECD_t get_AgeSec() const noexcept {
        return -get_TimeTilSec();
    }
};
}  // namespace Gray
#endif  // _INC_cTimeInt_H
