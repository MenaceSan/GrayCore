//
//! @file CFloat.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CFloat.h"
#include "CArray.h"

#if USE_UNITTESTS
#include "CLogMgr.h"
#include "CUnitTest.h"

UNITTEST_CLASS(CFloat32)
{
	UNITTEST_METHOD(CFloat32)
	{
		// Test basic assumptions about floats.
		CFloat32 f1(65536);
		CFloat32 f2;
		f2.m_v.u_f = 65536;
		UNITTEST_TRUE(f1.m_v.u_f == f2.m_v.u_f);

		f1.put_Bits(CFloat32::k_uOne);
		UNITTEST_TRUE(1.0f == f1.m_v.u_f);
		f2.put_Bits(CFloat32::k_uTwo);
		UNITTEST_TRUE(2.0f == f2.m_v.u_f);

		CFloat64 d1(65536);
		CFloat64 d2;
		d2.m_v.u_d = 65536;
		UNITTEST_TRUE(d1.m_v.u_d == d2.m_v.u_d);

		CFloat64 dx1(CTypeLimit<double>::k_Max);
		UNITTEST_TRUE(dx1.m_v.u_d > 10);
		CFloat64 dx2(CTypeLimit<double>::k_Min);
		UNITTEST_TRUE(dx2.m_v.u_d < -10);

	}
};
UNITTEST_REGISTER(CFloat32, UNITTEST_LEVEL_Core);	// 

#endif
