//
//! @file cTimeVal.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTimeVal_H
#define _INC_cTimeVal_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cTimeSys.h"
#ifdef _WIN32
#include <winsock2.h>	// timeval
#endif

namespace Gray
{
	class GRAYCORE_LINK cTimeVal : public /* struct*/ timeval
	{
		//! @class Gray::cTimeVal
		//! Wrap the POSIX micro-second clock 'struct timeval'.
		//! Used for "::select()"
		//! _WIN32 defines struct timeval in "Winsock2.h"
		//!  this->tv_usec = cTimeUnits::k_nMicroSeconds = 1/1000000

	public:
		cTimeVal() noexcept
		{
			this->tv_sec = 0;
			this->tv_usec = 0;
		}
		cTimeVal(TIMESYS_t nMilliSeconds) noexcept
		{
			put_mSec(nMilliSeconds);
		}
		cTimeVal(TIMESECD_t nSeconds, int iMicroSecWait) noexcept
		{
			this->tv_sec = nSeconds;
			this->tv_usec = iMicroSecWait;
		}

		TIMESYS_t get_mSec() const noexcept
		{
			return (this->tv_sec * cTimeSys::k_FREQ) + (this->tv_usec / 1000);
		}
		void put_mSec(TIMESYS_t nMilliSeconds) noexcept
		{
			this->tv_sec = nMilliSeconds / cTimeSys::k_FREQ;
			this->tv_usec = (nMilliSeconds % cTimeSys::k_FREQ) * 1000;
		}
		void put_Sec(int nSeconds) noexcept
		{
			this->tv_sec = nSeconds;
			this->tv_usec = 0;
		}
	};
} 
#endif
