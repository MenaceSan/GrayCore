//
//! @file GrayCore.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "GrayCore.h"
#include "cLogMgr.h"
#include "cUnitTest.h"
#include "cOSModImpl.h"

namespace Gray
{
	cOSModImpl g_Module(GRAY_NAMES "Core");
}

#if defined(_CONSOLE)

int _cdecl main(int argc, APP_ARGS_t argv)
{
	// @return APP_EXITCODE_t
	Gray::cAppStateMain inmain(argc, argv);
#if USE_UNITTESTS
	Gray::cUnitTests::I().UnitTests(UNITTEST_LEVEL_Common);
#endif // USE_UNITTESTS
	return APP_EXITCODE_OK;
}

#elif ! defined(GRAY_STATICLIB)	// DLL/SO

COSMODULE_IMPL(Gray);

#endif
