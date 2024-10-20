//! @file cTimeDouble.h
//! Data Time similar to COleDateTime
//! Elapsed days since (midnight, December 30, 1899 GMT).
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTimeDouble_H
#define _INC_cTimeDouble_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "cTimeFile.h"
#include "cTimeUnits.h"

namespace Gray {
/// <summary>
/// Time = Absolute 64 bit double days since (1899/12/30 midnight GMT).
/// same as DATE, COleDateTime
/// same as _WIN32 VariantTimeToSystemTime, same as DATE, COleDateTime
/// double 1.0 = 1 day
/// NOT the same as ::REFTIME which is (double) seconds.
/// </summary>
class GRAYCORE_LINK cTimeDouble {
 protected:
    double _dTimeDays = 0;  /// DATE = days since (midnight, 30 December 1899 GMT), fraction = time of day

 public:
    static const int k_nDaysDiffTimeInt = 25569;  /// days difference from cTimeDouble (1899) to cTimeInt (1970) bases // similar to __linux__ SECS_1601_TO_1970 ?
    static const double k_nY2K;                   /// The static value for y2k = January 1, 2000 in UTC/GMT
    static const double k_nY10;                   /// The first 10 years are sometimes reserved to act as offsets.
    static const int k_nZero = 0;                 /// double cant be used for in-class initializer

 protected:
    bool InitTimeUnits(const cTimeUnits& rTu);
    static void DecodeDate(OUT cTimeUnits& rTu, double dblDays);

 public:
    cTimeDouble(const cTimeDouble& dt) noexcept : _dTimeDays(dt._dTimeDays) {}
    cTimeDouble(const double dTime = k_nZero) noexcept : _dTimeDays(dTime) {}

    static cTimeDouble GRAYCALL EncodeSeconds(double s) noexcept;
    static cTimeDouble GRAYCALL EncodeTime(short h, short m, short s, short ms = 0) noexcept;
    static cTimeDouble GRAYCALL EncodeDate(short year = 0, short month = 0, short day = 0) noexcept;
    static cTimeDouble GRAYCALL GetTimeFromSec(TIMESEC_t nTimeSec) noexcept;

    cTimeDouble(const TIMESEC_t nTimeSec) noexcept : _dTimeDays(GetTimeFromSec(nTimeSec)) {}

    static cTimeDouble GRAYCALL GetTimeFromFile(const cTimeFile& ft) noexcept;
    cTimeDouble(const cTimeFile& ft) noexcept : _dTimeDays(GetTimeFromFile(ft)) {}

    static cTimeDouble GRAYCALL GetTimeFromStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone) {
        // Ignore HRESULT.
        cTimeDouble t;
        t.SetTimeStr(pszDateTime, nTimeZone);
        return t;
    }
    cTimeDouble(const cTimeUnits& rTu) {
        //! like SystemTimeToVariantTime but it ASSUMES GMT
        //!  _dTimeDays = 1 = whole days since 1900
        InitTimeUnits(rTu);
    }

    void InitTime(double dTime = k_nZero) noexcept {
        _dTimeDays = dTime;  /// dTime = 0 = clear to invalid time.
    }
    void InitTimeNow();

    static constexpr bool IsTimeValid(double dTime) {
        return dTime > k_nZero;
    }
    inline bool isTimeValid() const {
        return IsTimeValid(_dTimeDays);
    }
    double get_Double() const noexcept {
        //! Arbitrary units. same as days.
        return _dTimeDays;
    }
    /// <summary>
    /// Get total days since epoch.
    /// </summary>
    double get_Days() const noexcept {
        return _dTimeDays;
    }
    /// <summary>
    /// Get total days since epoch.
    /// </summary>
    operator double() const noexcept {
        return _dTimeDays;
    }

    cTimeDouble operator+(int i) noexcept {
        //! Add days.
        return cTimeDouble(_dTimeDays + i);
    }
    cTimeDouble operator-(int i) noexcept {
        //! Subtract days.
        return cTimeDouble(_dTimeDays - i);
    }
    cTimeDouble operator+(const cTimeDouble& dt) noexcept {
        return cTimeDouble(_dTimeDays + dt._dTimeDays);
    }
    cTimeDouble operator-(const cTimeDouble& dt) noexcept {
        return cTimeDouble(_dTimeDays - dt._dTimeDays);
    }

    cTimeDouble& operator+=(int idays) noexcept {  /// days
        //! Add days.
        _dTimeDays += idays;
        return *this;
    }
    cTimeDouble& operator-=(int idays) noexcept {  /// days
        //! Subtract days.
        _dTimeDays -= idays;
        return *this;
    }
    cTimeDouble& operator+=(const cTimeDouble& dt) noexcept {
        _dTimeDays += dt._dTimeDays;
        return *this;
    }
    cTimeDouble& operator-=(const cTimeDouble& dt) noexcept {
        _dTimeDays -= dt._dTimeDays;
        return *this;
    }

    cTimeDouble& operator++() noexcept {  /// Prefix increment
        _dTimeDays += 1;
        return *this;
    }
    cTimeDouble& operator++(int) noexcept {  /// Postfix increment
        _dTimeDays += 1;  // add a day
        return *this;
    }
    cTimeDouble& operator--() noexcept { /// Prefix decrement
        _dTimeDays -= 1;
        return *this;
    }
    cTimeDouble& operator--(int) noexcept { /// Postfix decrement
        _dTimeDays -= 1;
        return *this;
    }

    bool inline operator==(const cTimeDouble& dt2) const {
        return this->_dTimeDays == dt2._dTimeDays;
    }
    bool inline operator<(const cTimeDouble& dt2) const {
        return this->_dTimeDays < dt2._dTimeDays;
    }
    bool inline operator<=(const cTimeDouble& dt2) const {
        return this->_dTimeDays <= dt2._dTimeDays;
    }
    bool inline operator>(const cTimeDouble& dt2) const {
        return this->_dTimeDays > dt2._dTimeDays;
    }
    bool inline operator>=(const cTimeDouble& dt2) const {
        return this->_dTimeDays >= dt2._dTimeDays;
    }

    cTimeFile GetAsFileTime() const noexcept;
    bool GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone) const;

    TIMEDOW_t get_DayOfWeek() const;

    /// <summary>
    /// Get Total days as an integer.
    /// </summary>
    /// <returns></returns>
    unsigned GetDate() const noexcept { /// Numeric date of date object
        return CastN(unsigned, _dTimeDays);
    }

    // to/from strings.
    HRESULT SetTimeStr(const GChar_t* pszDateTime, TZ_TYPE nTimeZone = TZ_UTC);
    cString GetTimeFormStr(const GChar_t* pszFormat = nullptr, TZ_TYPE nTimeZone = TZ_LOCAL) const;
    cString GetTimeFormStr(TIMEFORMAT_t eFormat, TZ_TYPE nTimeZone = TZ_LOCAL) const {
        return GetTimeFormStr(CastNumToPtrT<const GChar_t>(static_cast<int>(eFormat)), nTimeZone);
    }

    static cString GRAYCALL GetTimeSpanStr(double dDays, TIMEUNIT_t eUnitHigh = TIMEUNIT_t::_Day, int iUnitsDesired = 2, bool bShortText = false);

    // ********************************************************
    // compare to GetTimeNow().

    static cTimeDouble GRAYCALL GetTimeNow() noexcept;
    static cTimeDouble GRAYCALL Date() noexcept;  // Now;
    static cTimeDouble GRAYCALL Time() noexcept;  // Now;

    double get_DaysTil() const noexcept {
        //! diff in days
        //! - = this is in the past.
        //! + = this is in the future.
        return get_Double() - GetTimeNow();
    }
    /// <summary>
    /// How old is this? (in days).  current time - this time.
    /// + = this is in the past. - = this is in the future.
    /// </summary>
    /// <returns></returns>
    double get_DaysAge() const noexcept {
        return -get_DaysTil();
    }
};
}  // namespace Gray
#endif  // _INC_cTimeDouble_H
