//
//! @file CInterlockedVal.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "CInterlockedVal.h"

#if defined(USE_UNITTESTS)
#include "CUnitTest.h"
#include "CLogMgr.h"

UNITTEST_CLASS(CInterlockedVal)
{
	UNITTEST_METHOD(CInterlockedVal)
	{
		// See CThread for better testing of this.
		{
			CInterlockedInt32 lVal32;

			lVal32.IncV();
			UNITTEST_TRUE(lVal32 == 1);
			lVal32.DecV();
			UNITTEST_TRUE(lVal32 == 0);

			UNITTEST_TRUE(lVal32.Inc() == 1);
			UNITTEST_TRUE(lVal32.Dec() == 0);
			UNITTEST_TRUE(lVal32.Exchange(123) == 0);
			UNITTEST_TRUE(lVal32.get_Value() == 123);
			UNITTEST_TRUE(lVal32.AddX(10) == 123);
			UNITTEST_TRUE(lVal32.get_Value() == 133);
			UNITTEST_TRUE(lVal32.CompareExchange(233, 0) == 133);
			UNITTEST_TRUE(lVal32.CompareExchange(233, 133) == 133);
			UNITTEST_TRUE(lVal32.get_Value() == 233);
		}

		{
			CInterlockedInt64 lVal64;

			UNITTEST_TRUE(lVal64.Inc() == 1);
			UNITTEST_TRUE(lVal64.Dec() == 0);
			UNITTEST_TRUE(lVal64.Exchange(123) == 0);
			UNITTEST_TRUE(lVal64.get_Value() == 123);
			UNITTEST_TRUE(lVal64.AddX(10) == 123);
			UNITTEST_TRUE(lVal64.get_Value() == 133);
			UNITTEST_TRUE(lVal64.CompareExchange(233, 0) == 133);
			UNITTEST_TRUE(lVal64.CompareExchange(233, 133) == 133);
			UNITTEST_TRUE(lVal64.get_Value() == 233);
		}

		// CInterlockedPtr<> pVal = nullptr;
	}
};
UNITTEST_REGISTER(CInterlockedVal, UNITTEST_LEVEL_Core);
#endif
