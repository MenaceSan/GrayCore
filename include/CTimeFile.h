//
//! @file cTimeFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeFile_H
#define _INC_cTimeFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include "cString.h"
#include "cTimeUnits.h"
#include "cValArray.h"

#ifdef __linux__
#include "cTimeVal.h"
// from '#include <windef.h>'
struct FILETIME { /// _WIN32 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601.
    UINT64 qwDateTime;  // FILETIME_t
};
#endif

namespace Gray {
typedef UINT64 FILETIME_t;  /// replace FILETIME for 64 bit math. Absolute 64-bit 100-nanosecond since January 1, 1601 GMT

/// <summary>
/// Universal time stamp for a file.
/// 64-bit integer 100-nanosecond intervals since January 1, 1601 GMT. Overflows in ~58494 years.
/// @note FAT32 has time stamps that are only good for ~2 second accuracy.
/// Similar to ATL/MFC CFileTime
/// </summary>
class GRAYCORE_LINK cTimeFile : public ::FILETIME {
 public:
    static const int k_nDaysDiffTimeInt = ((369 * 365) + 89);  /// days difference from FILETIME (1601) to cTimeInt (1970) bases = 134774
    static const int k_nFreq = 10 * 1000000;                   /// 100-nanosecond intervals per second = 10th of a micro second.

 public:
    cTimeFile(FILETIME_t t = 0) noexcept {
        InitTime(t);
    }
    cTimeFile(const ::FILETIME& t) noexcept {
        *static_cast<::FILETIME*>(this) = t;
    }
    cTimeFile(const cTimeUnits& tu) {
        InitTimeUnits(tu);
    }

#ifdef _WIN32
    cTimeFile(const ::SYSTEMTIME& st, TZ_TYPE nTimeZone) {
        SetSys(st, nTimeZone);
    }
    void SetSys(const ::SYSTEMTIME& st, TZ_TYPE nTimeZone) {
        ::SystemTimeToFileTime(&st, this);
        if (nTimeZone == TZ_LOCAL) {
            ::LocalFileTimeToFileTime(this, this);
        }
        ASSERT(nTimeZone == TZ_LOCAL || nTimeZone == TZ_UTC);
    }
    bool GetSys(::SYSTEMTIME& st, TZ_TYPE nTimeZone) const {
        FILETIME ftTmp = *this;
        if (nTimeZone == TZ_LOCAL)  // adjust for TZ and DST
        {
            ::FileTimeToLocalFileTime(this, &ftTmp);
        }
        ::FileTimeToSystemTime(&ftTmp, &st);
        ASSERT(nTimeZone == TZ_LOCAL || nTimeZone == TZ_UTC);
        return true;
    }

#elif defined(__linux__)

    static inline FILETIME_t CvtFileTime(const struct timespec& tSpec) {
        // ASSUME struct timespec/cTimeSpec has same base/EPOCH as time_t cTimeInt
        FILETIME_t nTmp = ((UINT64)k_nDaysDiffTimeInt * (UINT64)cTimeUnits::k_nSecondsPerDay);
        nTmp += ((UINT64)tSpec.tv_sec);
        nTmp *= k_nFreq;
        nTmp += tSpec.tv_nsec / 100;  // add nanoseconds.
        return nTmp;
    }
    cTimeFile(const struct timespec& tSpec) {
        // convert struct timespec/cTimeSpec to 64 bit FILETIME number.
        InitTime(CvtFileTime(tSpec));
    }
    cTimeVal get_TimeVal() const {
        FILETIME_t nTmpSec = this->get_Val() / k_nFreq;                                             // seconds
        return cTimeVal(nTmpSec - (k_nDaysDiffTimeInt * (FILETIME_t)cTimeUnits::k_nSecondsPerDay),  // seconds
                        (this->get_Val() - (nTmpSec * k_nFreq)) / 10);                              // iMicroSecWait
    }
#endif

    FILETIME_t& ref_Val() noexcept {
        //! @return 64-bit integer 100-nanosecond intervals since January 1, 1601 GMT
        //! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
        return *reinterpret_cast<FILETIME_t*>(static_cast<::FILETIME*>(this));
    }
    FILETIME_t get_Val() const noexcept {
        //! @return 64-bit integer 100-nanosecond intervals since January 1, 1601 GMT
        //! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
        return *reinterpret_cast<const FILETIME_t*>(static_cast<const ::FILETIME*>(this));
    }

    FILETIME_t get_FAT32() const noexcept {
        //! get the time truncated to 2 second intervals for FAT32.
        //! 2 second accurate for FAT32
        //! @note This is not really in proper FILETIME_t or FAT32/DosDate format. see cTimeUnits DosDate.
        return get_Val() / (2 * k_nFreq);
    }

    bool isValid() const noexcept {
        if (get_Val() == 0) return false;
        return true;
    }

    void InitTime(FILETIME_t t = 0) noexcept {
        ref_Val() = t;
    }

    void InitTimeUnits(const cTimeUnits& rTu);
    bool GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone) const;

    cString GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZone = TZ_LOCAL) const;

    // ********************************************************
    // compare to GetTimeNow().

    static cTimeFile GRAYCALL GetTimeNow() noexcept;
    void InitTimeNow() noexcept;
    TIMESECD_t get_AgeSec() const noexcept;
};

template <>
inline COMPARE_t cValT::Compare<cTimeFile>(const cTimeFile& t1, const cTimeFile& t2) {
    //! Overload/implementation of cValT::Compare
    //! Use cFileStatus::isSameChangeTime for FAT32 ~2 second
    //! like _WIN32 CompareFileTime() ?
    return cValT::Compare(t1.get_Val(), t2.get_Val());
}

/// <summary>
/// Holds a span of time. Not absolute time.
/// Emulate the MFC CTime functionality
/// </summary>
struct GRAYCORE_LINK cTimeSpanFile {
    INT64 m_nDiffUnits; /// in cTimeFile::k_nFreq units

    cTimeSpanFile(INT64 nDiffUnits = 0) : m_nDiffUnits(nDiffUnits) {}

    cTimeSpanFile(int iDays, int iHours, int iMinutes, int iSeconds) : m_nDiffUnits(iDays) {
        m_nDiffUnits *= 24;
        m_nDiffUnits += iHours;
        m_nDiffUnits *= 60;
        m_nDiffUnits += iMinutes;
        m_nDiffUnits *= 60;
        m_nDiffUnits += iSeconds;
        m_nDiffUnits *= cTimeFile::k_nFreq;
    }

    /// in cTimeFile::k_nFreq units
    INT64 get_Val() const {
        return m_nDiffUnits;
    }
    INT64 GetTotalSeconds() const {
        // MFC like call.
        return ((INT64)m_nDiffUnits) / cTimeFile::k_nFreq;
    }

    // cTimeUnits::GetTimeSpanStr
};
}  // namespace Gray
#endif  // _INC_cTimeFile_H
