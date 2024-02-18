//! @file cDebugAssert.h
//! A very simple header for basic support of asserts.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cDebugAssert_H
#define _INC_cDebugAssert_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray {
/// static_assert = compile time assert. evaluates as a constant at compile time. like BOOST_STATIC_ASSERT
/// @note: Takes a NON string name argument to support old compilers.
#ifndef STATIC_ASSERT
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#define STATIC_ASSERT(exp, name) static_assert(exp, #name)
#elif defined(__GNUC__)
#define STATIC_ASSERT(exp, name) static_assert(exp, #name)  // (since C++11)
#else
#define STATIC_ASSERT(exp, name) typedef int static_assert_##name[(exp) ? 1 : -1]  // This should fail in the compiler. __GNUC__ cant handle this ?
#endif
#endif

/// <summary>
/// a place in the code where something (e.g. assert, exception) occurred. for debugging.
/// like M$ CppUnitTestFramework.__LineInfo
/// </summary>
struct cDebugSourceLine {
    const char* m_pszFile;      /// name of the source file. static text. __FILE__
    const char* m_pszFunction;  /// name of the source function. __func__, __FUNCTION__, __FUNCDNAME__, and __FUNCSIG__. static text.
    WORD m_uLine;               /// line number in the source m_pszFile. (1 based) __LINE__
    cDebugSourceLine(const char* pszFile = "", const char* pszFunction = "", WORD uLine = 0) noexcept : m_pszFile(pszFile), m_pszFunction(pszFunction), m_uLine(uLine) {}
};

//! __FILE__ is valid for __GNUC__ and _MSC_VER.
//! TODO Should we get rid of this if !(defined(_DEBUG) || defined(_DEBUG_FAST))
#define DEBUGSOURCELINE ::Gray::cDebugSourceLine(__FILE__, __FUNCTION__, (WORD)__LINE__)  /// record the file and line this macro is used on.

typedef bool(CALLBACK AssertCallback_t)(const char* pszExp, const cDebugSourceLine& src);  // for redirect of assert in testing.

/// <summary>
/// Log assert events before handling as normal system assert().
/// like M$ test Assert.IsTrue()
/// </summary>
struct GRAYCORE_LINK cDebugAssert {               // static singleton
    static AssertCallback_t* sm_pAssertCallback;  /// redirect callback on Assert_Fail usually used for unit tests.
    static bool sm_bAssertTest;                   ///  Just testing. not a real assert.

    static bool CALLBACK AssertCallbackDefault(const char* pszExp, const cDebugSourceLine& src);

    static bool GRAYCALL Debug_Fail(const char* pszExp, const cDebugSourceLine src) noexcept;
    static bool GRAYCALL Assert_Fail(const char* pszExp, const cDebugSourceLine src);
    static void GRAYCALL ThrowEx_Fail(const char* pszExp, const cDebugSourceLine src);
};

#if defined(_DEBUG) || defined(_DEBUG_FAST)
// debug checks
// Use macros so these can be compiled out.
// overload the assert statements
#undef ASSERT
#define ASSERT(exp) (void)((!!(exp)) || (::Gray::cDebugAssert::Assert_Fail(#exp, DEBUGSOURCELINE), 0))
#define ASSERT_NN(p) ASSERT((p) != nullptr)  // Null check, Cant ignore this !

#undef DEBUG_CHECK
#define DEBUG_CHECK(exp) (void)((!!(exp)) || (::Gray::cDebugAssert::Debug_Fail(#exp, DEBUGSOURCELINE), 0))
#undef DEBUG_ASSERT
#define DEBUG_ASSERT(exp, sDesc) (void)((!!(exp)) || (::Gray::cDebugAssert::Debug_Fail(sDesc, DEBUGSOURCELINE), 0))

#else  // _DEBUG

// NON DEBUG compiles out the checks.
#ifndef ASSERT
#define ASSERT(exp) __noop  // acts like UNREFERENCED_PARAMETER() ?
#endif                      // ASSERT
#define ASSERT_NN(p) __noop
#ifndef DEBUG_CHECK
#define DEBUG_CHECK(exp) __noop
#endif  // DEBUG_CHECK
#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(exp, sDesc) __noop
#endif  // DEBUG_ASSERT

#endif  // ! _DEBUG

#ifndef THROW_IF
#define THROW_IF(exp)                                              \
    if (exp) {                                                     \
        ::Gray::cDebugAssert::ThrowEx_Fail(#exp, DEBUGSOURCELINE); \
    }  // Show the compiler that we wont proceed.
#endif
}  // namespace Gray
#endif  // _INC_cDebugAssert_H
