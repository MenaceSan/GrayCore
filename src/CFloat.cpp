//
//! @file cFloat.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cFloat.h"
#include "cArray.h"

#if USE_UNITTESTS
#include "cLogMgr.h"
#include "cUnitTest.h"

UNITTEST_CLASS(cFloat32)
{
	UNITTEST_METHOD(cFloat32)
	{
		// Test basic assumptions about floats.
		cFloat32 f1(65536);
		cFloat32 f2;
		f2.m_v.u_f = 65536;
		UNITTEST_TRUE(f1.m_v.u_f == f2.m_v.u_f);

		f1.put_Bits(cFloat32::k_uOne);
		UNITTEST_TRUE(1.0f == f1.m_v.u_f);
		f2.put_Bits(cFloat32::k_uTwo);
		UNITTEST_TRUE(2.0f == f2.m_v.u_f);

		cFloat64 d1(65536);
		cFloat64 d2;
		d2.m_v.u_d = 65536;
		UNITTEST_TRUE(d1.m_v.u_d == d2.m_v.u_d);

		cFloat64 dx1(CTypeLimit<double>::k_Max);
		UNITTEST_TRUE(dx1.m_v.u_d > 10);
		cFloat64 dx2(CTypeLimit<double>::k_Min);
		UNITTEST_TRUE(dx2.m_v.u_d < -10);

	}
};
UNITTEST_REGISTER(cFloat32, UNITTEST_LEVEL_Core);	// 

#endif
