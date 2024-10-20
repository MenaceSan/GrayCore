//! @file cExceptionAssert.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "PtrCast.h"
#include "StrArg.h"
#include "StrBuilder.h"
#include "cDebugAssert.h"
#include "cExceptionAssert.h"
#include "cLogMgr.h"

namespace Gray {
void GRAYCALL cException::ThrowEx(const char* pszExp, const cDebugSourceLine src) {  // static
    //! This assert cannot be ignored. We must throw cExceptionAssert after this. Things will be horribly corrupted if we don't?
    //! Leave this in release code.
    //! Similar to AfxThrowInvalidArgException()

    cLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLVL_t::_CRIT, "Assert Throw:'%s' file '%s', line %d", LOGSTR(pszExp), LOGSTR(src._pszFile), src._uLine);

#ifdef _CPPUNWIND
    GRAY_THROW cExceptionAssert(pszExp, LOGLVL_t::_CRIT, src);
#else
    // TODO signal or log instead of throw??
#endif
}

cExceptionAssert::cExceptionAssert(const LOGCHAR_t* pExp, LOGLVL_t eLogLevel, const cDebugSourceLine& src) : cException("Assert", eLogLevel), _pExp(pExp), _Src(src) {}

cExceptionAssert::~cExceptionAssert() THROW_DEF {}

BOOL cExceptionAssert::GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) {  // virtual
    UNREFERENCED_PARAMETER(pnHelpContext);
    sb.AddFormat(_GT("Assert pri=%d:'%s' file '%s', line %d"), get_Severity(), StrArg<GChar_t>(_pExp), StrArg<GChar_t>(_Src._pszFile), _Src._uLine);
    return true;
}

void GRAYCALL cExceptionAssert::Throw(const LOGCHAR_t* pExp, const cDebugSourceLine& src) {  // static
    //! These can get left in release code.
    //! This is similar to _assert() in M$ code.
#if defined(_CPPUNWIND)
    GRAY_THROW cExceptionAssert(pExp, LOGLVL_t::_CRIT, src);
#else
    // system ASSERT ?
#endif
}
}  // namespace Gray

extern "C" {
#ifndef UNDER_CE
int _cdecl _purecall() {
    //! catch this special type of C++ exception as well.
    Gray::cExceptionAssert::Throw("purecall", DEBUGSOURCELINE);
    return 0;
}
#endif

#if defined(GRAY_STATICLIB)
void _cdecl _amsg_exit(int iArg) {
    //! overload to try to trap some of the other strange exit conditions !!!
    //! Some strange _MSC_VER stdlib calls use this for really bad stuff.
    //! http://msdn.microsoft.com/en-us/library/ff770579.aspx
    //! DLL returns error LNK2005: __amsg_exit already defined in MSVCRTD.lib(MSVCR80D.dll)
    //! throw this as cExceptionAssert ?? NOT sure this will work?
    UNREFERENCED_PARAMETER(iArg);
    Gray::cExceptionAssert::Throw("_amsg_exit", DEBUGSOURCELINE);
}
#endif

#if defined(_MSC_VER)  // && defined(GRAY_STATICLIB)
void _cdecl _assert(void* pExp, void* pFile, unsigned uLine) {
    //! Overload the system version of this just in case.
    //! Trap for 3rd party libraries doing funny stuff.
    Gray::cExceptionAssert::Throw(Gray::PtrCast<Gray::LOGCHAR_t>(pExp), Gray::cDebugSourceLine(Gray::PtrCast<char>(pFile), "", CastN(WORD, uLine)));
}
#endif
}  // extern "C"
