//
//! @file GrayCore.cpp
//! @copyright (c) 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "GrayCore.h"
#include "GrayVersion.h"
#include "cLogMgr.h"
#include "cUnitTest.h"
#include "cOSModImpl.h"
#include "SysRes.h"

#if ! USE_CRT
#include "NoCRT.inl"
#endif

namespace Gray
{
	cOSModImpl g_Module(GRAY_NAMES "Core");

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cInterlockedVal<int>;		// Force Instantiation
#ifdef USE_INT64
	template class GRAYCORE_LINK cInterlockedVal<INT64>;	// Force Instantiation 
#endif
#endif
}

#if defined(_CONSOLE)

int _cdecl main(int argc, APP_ARGS_t argv)
{
	// @return APP_EXITCODE_t
	Gray::cAppStateMain inmain(argc, argv);

	cUnitTests& uts = Gray::cUnitTests::I();
	uts.RunUnitTests(UNITTEST_LEVEL_Common);

	return APP_EXITCODE_OK;
}

#elif ! defined(GRAY_STATICLIB)	// DLL/SO

COSMODULE_IMPL(Gray);

#endif
