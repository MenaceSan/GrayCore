//! @file GrayCore.cpp
//! @copyright (c) 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "GrayCore.h"
#include "GrayVersion.h"
#include "SysRes.h"
#include "cLogMgr.h"
#include "cOSModImpl.h"
#include "cUnitTest.h"

#if !USE_CRT
#include "NoCRT.inl"
#endif

namespace Gray {
cOSModImpl g_Module(GRAY_NAMES "Core");
va_list GRAYCORE_LINK k_va_list_empty = {};  // For faking out the va_list. __GNUC__ doesn't allow a pointer to va_list. So use this to simulate nullptr.

template class GRAYCORE_LINK cInterlockedVal<int>;  // Force implementation/instantiate for DLL/SO.
#if defined(USE_INT64) && !defined(__GNUC__)
template class GRAYCORE_LINK cInterlockedVal<INT64>;  // Force implementation/instantiate for DLL/SO.
#endif
}  // namespace Gray

#if defined(_CONSOLE)

int _cdecl main(int argc, APP_ARGS_t argv) {
    // @return APP_EXITCODE_t
    Gray::cAppStateMain inmain(argc, argv);

    cUnitTests& uts = Gray::cUnitTests::I();
    uts.RunUnitTests(UNITTEST_LEVEL_t::_Common);

    return APP_EXITCODE_t::_OK;
}

#elif !defined(GRAY_STATICLIB)  // DLL/SO

cOSModImpl_DEF(Gray);

#endif
