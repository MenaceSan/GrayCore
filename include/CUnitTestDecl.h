//
//! @file CUnitTestDecl.h
//! Included from header file to minimally declare a unit test.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_CUnitTestDecl_H
#define _INC_CUnitTestDecl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

#if USE_UNITTESTS

namespace Gray
{
	class GRAYCORE_LINK CUnitTestRegister;
}

#define UNITTEST_N(n)			CUnitTest_##n			//!< Pick a unique name for the test class. In Global namespace.
#define UNITTEST_EXT(n)			g_pCUnitTest_##n		//!< a base pointer to CUnitTestRegister for UNITTEST_N(n)

//! Define this outside namespace so M$ UnitTests can pick it up.
//! Define this at the top of the header file declaring the class to be unit tested.
#define UNITTEST_PREDEF(n)		class UNITTEST_N(n); extern ::Gray::CUnitTestRegister* UNITTEST_EXT(n);	 // In Global namespace.

//! Define this in the class body to be unit tested. Allow the unit test to access private stuff.
#define UNITTEST_FRIEND(n)		friend class ::UNITTEST_N(n);

#else

#define UNITTEST_PREDEF(n)		// stub this out. not __noop.
#define UNITTEST_FRIEND(n)		// stub this out. not __noop.

#endif

#endif
