//
//! @file CTimeVal.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CTimeVal_H
#define _INC_CTimeVal_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CTimeSys.h"
#ifdef _WIN32
#include <winsock2.h>	// timeval
#endif

namespace Gray
{
	class GRAYCORE_LINK CTimeVal : public /* struct*/ timeval
	{
		//! @class Gray::CTimeVal
		//! Wrap the POSIX micro-second clock 'struct timeval'.
		//! Used for "::select()"
		//! _WIN32 defines struct timeval in "Winsock2.h"
		//!  this->tv_usec = CTimeUnits::k_nMicroSeconds = 1/1000000

	public:
		CTimeVal()
		{
			this->tv_sec = 0;
			this->tv_usec = 0;
		}
		CTimeVal(TIMESYS_t dwMilliseconds)
		{
			put_mSec(dwMilliseconds);
		}
		CTimeVal(TIMESECD_t iSecWait, int iMicroSecWait)
		{
			this->tv_sec = iSecWait;
			this->tv_usec = iMicroSecWait;
		}

		TIMESYS_t get_mSec() const
		{
			return (this->tv_sec * CTimeSys::k_FREQ) + (this->tv_usec / 1000);
		}
		void put_mSec(TIMESYS_t dwMilliseconds)
		{
			this->tv_sec = dwMilliseconds / CTimeSys::k_FREQ;
			this->tv_usec = (dwMilliseconds % CTimeSys::k_FREQ) * 1000;
		}
	};
};
#endif
