//! @file cDebugAssert.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cDebugAssert.h"
#include "cExceptionAssert.h"
#include "cLogMgr.h"
#include "cString.h"

#ifdef __linux__
#include <assert.h>
#endif
#ifdef UNDER_CE
#include <dbgapi.h>
// WinCE doesn't seem to have a macro for assert()?
#define assert(x) ::DbgAssert(#x, __FILE__, __LINE__);
#endif

namespace Gray {
AssertCallback_t* cDebugAssert::sm_pAssertCallback = cDebugAssert::AssertCallbackDefault;  // default = pass to assert.
bool cDebugAssert::sm_bAssertTest = false;

//*************************************************************************

bool CALLBACK cDebugAssert::AssertCallbackDefault(const char* pszExp, const cDebugSourceLine& src) {
    //! sm_pAssertCallback may point to this. Maybe for unit tests.
    //! AssertCallback_t to use the system assert dialog. Use with sm_pAssertCallback.
    //! @return true = continue and ignore the assert.

    // if (sm_pAssertCallback == cDebugAssert::AssertCallbackDefault && !cAppState::isDebuggerPresent()) return;	// Ignore it if no debugger present ?

#if defined(__linux__)
    assert(1);  // __assert_fail(pszExp , src._pszFile, src._uLine, "")
#elif defined(UNDER_CE) || !defined(_DEBUG) || !defined(_MSC_VER)
    ::DebugBreak();  // Do Int 3 - to halt all threads at point.
#elif _MSC_VER >= 1400
    ::_wassert(cStringW(pszExp), cStringW(src._pszFile), src._uLine);
#elif _MSC_VER >= 1300
    ::_assert(pszExp, src._pszFile, src._uLine);
#else
    ::_assert((void*)pszExp, (void*)src._pszFile, src._uLine);
#endif
    // if this returns at all, that means we chose to ignore the assert and continue.
    return true;
}

bool GRAYCALL cDebugAssert::Assert_Fail(const char* pszExp, const cDebugSourceLine src) {
    //! like _assert() is M$ code.
    //! Put something in the log after (or before?) doing normal assert processing. is this too dangerous?
    //! @return false to indicate something failed. stop. (for macro)
    //! @return true = continue and ignore the assert.
    // Watch for sm_bAssertTest

    if (sm_pAssertCallback != nullptr) {  // Divert the assert for testing? else just log it and keep going.
        // do normal assert stuff. OR maybe do special processing for unit tests. May not return.
        if (!sm_pAssertCallback(pszExp, src)) return false;  // AssertCallback_t           
    }

    // Just log it and try to continue.
    cLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLVL_t::_CRIT, "Assert Fail:'%s' file '%s', line %d", LOGSTR(pszExp), LOGSTR(src._pszFile), src._uLine);
    // Flush logs.
    // cExceptionAssert?
    // true = Keep going?
    return false;
}

void GRAYCALL cDebugAssert::ThrowEx_Fail(const char* pszExp, const cDebugSourceLine src) {
    // hidden impl for use by low level functions. This might be normal in unit tests. May be: cDebugAssert::sm_bAssertTest
    cException::ThrowEx(pszExp, src);
}

#if defined(_DEBUG) || defined(_DEBUG_FAST)
bool GRAYCALL cDebugAssert::Debug_Fail(const char* pszExp, const cDebugSourceLine src) noexcept {
    //! A 'softer' version of assert. non-fatal checks. for use in constructors, etc.
    //! @note: always return false to indicate something failed. (for macro)
    // CODEPROFILEFUNC();
#ifdef _DEBUG
    cLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLVL_t::_ERROR, "Check Fail:'%s' file '%s', line %d", LOGSTR(pszExp), LOGSTR(src._pszFile), src._uLine);
#endif
    // Sync the debug thread??
    return false;
}
#endif

}  // namespace Gray
