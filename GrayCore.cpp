//
//! @file GrayCore.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "GrayCore.h"
#include "CLogMgr.h"
#include "CUnitTest.h"
#include "COSModImpl.h"

#if defined(_CONSOLE)

int _cdecl main(int argc, char* argv[])
{
	// @return APP_EXITCODE_t
	Gray::CAppStateMain inmain(argc, argv);
#ifdef USE_UNITTESTS
	Gray::CUnitTests::I().UnitTests(UNITTEST_LEVEL_Common);
#endif // USE_UNITTESTS
	return APP_EXITCODE_OK;
}

#elif ! defined(GRAY_STATICLIB)	// DLL/SO

Gray::COSModImpl g_Module(GRAY_NAMES "Core");
COSMODULE_IMPL();

#endif
