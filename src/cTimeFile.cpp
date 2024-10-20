//! @file cTimeFile.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cTimeDouble.h"
#include "cTimeFile.h"  // class implemented
#include "cTimeUnits.h"

namespace Gray {
void cTimeFile::InitTimeNow() noexcept {
    //! Get current UTC time
#ifdef UNDER_CE
    ::GetCurrentFT(static_cast<::FILETIME*>(this));
#elif defined(_WIN32)
    ::GetSystemTimeAsFileTime(static_cast<::FILETIME*>(this));
#else  // __linux__
    cTimeSpec tNow;
    tNow.InitTimeNow1();
    ref_Val() = CvtFileTime(tNow);
#endif
}

cTimeFile GRAYCALL cTimeFile::GetTimeNow() noexcept {  // static
    cTimeFile tNow;
    tNow.InitTimeNow();
    return tNow;
}

void cTimeFile::InitTimeUnits(const cTimeUnits& rTu) {
#ifdef _WIN32
    ::SYSTEMTIME st;
    rTu.GetSys(st);
    SetSys(st, CastN(TZ_TYPE, rTu._nTZ));
#else
    cTimeDouble ti(rTu);
    *this = ti.GetAsFileTime();
#endif
}
bool cTimeFile::GetTimeUnits(OUT cTimeUnits& rTu, TZ_TYPE nTimeZone) const {
#ifdef _WIN32
    ::SYSTEMTIME st;
    const bool bRet = GetSys(st, nTimeZone);
    rTu.SetSys(st);
    rTu._nTZ = CastN(TIMEVALU_t, nTimeZone);
    return bRet;
#else
    const cTimeDouble ti(*this);
    return ti.GetTimeUnits(rTu, nTimeZone);
#endif
}

cString cTimeFile::GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZone) const {
    //! Get the time as a string formatted using "C" strftime()
    //! MFC just calls this "Format"
    cTimeUnits Tu;
    if (!GetTimeUnits(Tu, nTimeZone)) return "";

    GChar_t szBuffer[128];  // Max reasonable size.
    const StrLen_t iLenChars = Tu.GetFormStr(TOSPAN(szBuffer), pszFormat);
    if (iLenChars <= 0) return "";

    return ToSpan(szBuffer, iLenChars);
}

TIMESECD_t cTimeFile::get_AgeSec() const noexcept {
    //! @return age in seconds.
    cTimeFile tNow;
    tNow.InitTimeNow();
    return CastN(TIMESECD_t, (tNow.get_Val() - this->get_Val()) / cTimeFile::k_nFreq);
}
}  // namespace Gray
