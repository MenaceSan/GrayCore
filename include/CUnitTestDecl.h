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

#define UNITTEST_N(n) n##Tests  /// Pick a unique name for the unit test class. Assume in same namespace as what it is testing (n).

//! Define this in the class body to be unit tested. Allow the unit test to access private/protected stuff.
#define UNITTEST_FRIEND(n) friend struct UNITTEST_N(n);  // assume test code is in same namespace and may get internal access to things.

#endif  // _INC_cUnitTestDecl_H
