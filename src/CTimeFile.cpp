//
//! @file CTimeFile.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CTimeFile.h"   // class implemented
#include "CTimeUnits.h"
#include "CTimeDouble.h"

namespace Gray
{
	void CTimeFile::InitTimeNow()
	{
		//! Get current UTC time
#ifdef UNDER_CE
		::GetCurrentFT(static_cast<FILETIME*>(this));
#elif defined(_WIN32)
		::GetSystemTimeAsFileTime(static_cast<FILETIME*>(this));
#else	// __linux__
		CTimeSpec tNow;
		tNow.InitTimeNow1();
		ref_Val() = CvtFileTime(tNow);
#endif
	}

	CTimeFile GRAYCALL CTimeFile::GetTimeNow()	// static
	{
		//! Get the current time with highest possible accuracy.
		//!  FILETIME_t = 64-bit 100-nanosecond since January 1, 1601 GMT
		CTimeFile tNow;
		tNow.InitTimeNow();
		return tNow;
	}

	void CTimeFile::InitTimeUnits(const CTimeUnits& rTu)
	{
#ifdef _WIN32
		SYSTEMTIME st;
		rTu.GetSys(st);
		SetSys(st, (TZ_TYPE)rTu.m_nTZ);
#else
		CTimeDouble ti(rTu);
		*this = ti.GetAsFileTime();
#endif
	}
	bool CTimeFile::GetTimeUnits(OUT CTimeUnits& rTu, TZ_TYPE nTimeZoneOffset) const
	{
#ifdef _WIN32
		SYSTEMTIME st;
		bool bRet = GetSys(st, nTimeZoneOffset);
		rTu.SetSys(st);
		rTu.m_nTZ = (TIMEUNIT_t)nTimeZoneOffset;
		return bRet;
#else
		CTimeDouble ti(*this);
		return ti.GetTimeUnits(rTu, nTimeZoneOffset);
#endif
	}

	cString CTimeFile::GetTimeFormStr(const GChar_t* pszFormat, TZ_TYPE nTimeZoneOffset) const
	{
		//! Get the time as a string formatted using "C" strftime()
		//! MFC just calls this "Format"

		CTimeUnits Tu;
		if (!GetTimeUnits(Tu, nTimeZoneOffset))
		{
			return "";
		}
		GChar_t szBuffer[256];
		StrLen_t iLenChars = Tu.GetFormStr(szBuffer, STRMAX(szBuffer), pszFormat);
		if (iLenChars <= 0)
		{
			return "";
		}
		return cString(szBuffer, iLenChars);
	}

	TIMESECD_t CTimeFile::get_AgeSec() const
	{
		//! @return age in seconds.
		CTimeFile tNow;
		tNow.InitTimeNow();
		return (TIMESECD_t)((tNow.get_Val() - this->get_Val()) / CTimeFile::k_nFreq);
	}
}

//*****************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CTimeFile)
{
	UNITTEST_METHOD(CTimeFile)
	{

		UINT64 uVal = 0xFFFFFFFFFFFFFFFFULL;
		uVal /= CTimeFile::k_nFreq;	// seconds.
		uVal /= 60 * 60 * 24;		// days.
		uVal /= 365;	// years

		UNITTEST_TRUE(uVal == 58494 );
	}
};
UNITTEST_REGISTER(CTimeFile, UNITTEST_LEVEL_Core);	// UNITTEST_LEVEL_Core
#endif
