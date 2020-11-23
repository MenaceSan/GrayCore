//
//! @file cUnitTestDecl.h
//! Included from header file to minimally declare a unit test.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cUnitTestDecl_H
#define _INC_cUnitTestDecl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

#define UNITTEST_N(n)			n##Tests			//!< Pick a unique name for the unit test class. Assume in same namespace as what it is testing (n). 

	// New way. external test code.
#define UNITTEST2_PREDEF(n)		class UNITTEST_N(n);	// assume test code is in same namespace
#define UNITTEST2_FRIEND(n)		friend class UNITTEST_N(n);	// assume test code is in global namespace ? or same namespace ?

namespace Gray
{
	// Non M$ test register.
	class GRAYCORE_LINK cUnitTestRegister;
#define UNITTEST_EXT(n)			g_pUnitTest_##n		//!< a base pointer to cUnitTestRegister for UNITTEST_N(n)
}

// TODO Stop using below here.

//! Define this outside namespace so M$ UnitTests can pick it up.
//! Define this at the top of the header file declaring the class to be unit tested.
#define UNITTEST_PREDEF(n)		class UNITTEST_N(n); extern ::Gray::cUnitTestRegister* UNITTEST_EXT(n);	 // In Global namespace.

//! Define this in the class body to be unit tested. Allow the unit test to access private stuff.
#define UNITTEST_FRIEND(n)		friend class ::UNITTEST_N(n);		 // In Global namespace.

#endif	// _INC_cUnitTestDecl_H
