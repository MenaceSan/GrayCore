//
//! @file cInterlockedVal.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cInterlockedVal.h"

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cLogMgr.h"

UNITTEST_CLASS(cInterlockedVal)
{
	UNITTEST_METHOD(cInterlockedVal)
	{
		// See CThread for better testing of this.
		{
			cInterlockedInt32 lVal32;

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
			cInterlockedInt64 lVal64;

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

		// cInterlockedPtr<> pVal = nullptr;
	}
};
UNITTEST_REGISTER(cInterlockedVal, UNITTEST_LEVEL_Core);
#endif
