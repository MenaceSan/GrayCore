//
//! @file CDebugAssert.h
//! A very simple header for basic support of asserts.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_CDebugAssert_H
#define _INC_CDebugAssert_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	//! compile time assert. evaluates as a constant at compile time. like BOOST_STATIC_ASSERT
#ifndef CASSERT
#if defined(_MSC_VER) && ( _MSC_VER >= 1600 )
#define CASSERT(exp, name) static_assert(exp,#name)
#elif defined(__GNUC__)
#define CASSERT(exp, name) static_assert(exp,#name) // (since C++11)
#else
#define CASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]	// This should fail in the compiler. __GNUC__ cant handle this ?
#endif
#endif

	class CDebugSourceLine
	{
		//! @class Gray::CDebugSourceLine
		//! a place in the code where something occurred. for debugging
		//! like CppUnitTestFramework.__LineInfo
	public:
		const char* m_pszFile;	//!< name of the source file. static text. __FILE__
		const char* m_pszFunction;	// __func__, __FUNCTION__, __FUNCDNAME__, and __FUNCSIG__
		WORD m_uLine;			//!< line in the source file. (1 based) __LINE__

	public:
		CDebugSourceLine(const char* pszFile = "", const char* pszFunction = "", WORD uLine = 0)
		: m_pszFile(pszFile)
		, m_pszFunction(pszFunction)
		, m_uLine(uLine)
		{
		}
	};

	//! __FILE__ is valid for __GNUC__ and _MSC_VER.
	//! ?? Should we get rid of this if !(defined(_DEBUG) || defined(_DEBUG_FAST))
#define DEBUGSOURCELINE ::Gray::CDebugSourceLine( __FILE__, __FUNCTION__, (WORD) __LINE__ )	//!< record the file and line this macro is used on.

	typedef bool (CALLBACK AssertCallback_t)(const char* pszExp, const CDebugSourceLine& src);	// for redirect.

	struct GRAYCORE_LINK CDebugAssert	// static
	{
		//! @struct Gray::CDebugAssert
		//! Log assert events before handling as normal system assert().
		//! like M$ Assert.IsTrue()

		static AssertCallback_t* sm_pAssertCallback;		//!< redirect callback on Assert_Fail usually used for unit tests.
		static bool sm_bAssertTest;		// Just testing. 

		static bool CALLBACK Assert_System(const char* pszExp, const CDebugSourceLine& src);
		static bool GRAYCALL Assert_Fail(const char* pszExp, const CDebugSourceLine src);
		static CATTR_NORETURN void GRAYCALL Assert_Throw(const char* pszExp, const CDebugSourceLine src);
		static bool GRAYCALL Debug_Fail(const char* pszExp, const CDebugSourceLine src);
	};

#define ASSERT_THROW(exp)		if (!(exp)) { ::Gray::CDebugAssert::Assert_Throw(#exp, DEBUGSOURCELINE ); } // Show the compiler that we wont proceed.

#define ASSERT_N(exp) ASSERT_THROW(exp)	// Null check, Cant ignore this !

#if defined(_DEBUG) || defined(_DEBUG_FAST)
	// debug checks
	// Use macros so these can be compiled out.
	// overload the assert statements
#undef ASSERT
#define ASSERT(exp)				(void)((!!(exp)) || (::Gray::CDebugAssert::Assert_Fail(#exp, DEBUGSOURCELINE ), 0))
#undef DEBUG_CHECK
#define DEBUG_CHECK(exp)		(void)((!!(exp)) || (::Gray::CDebugAssert::Debug_Fail(#exp, DEBUGSOURCELINE ), 0))
#undef DEBUG_ASSERT
#define DEBUG_ASSERT(exp,sDesc)	(void)((!!(exp)) || (::Gray::CDebugAssert::Debug_Fail(sDesc, DEBUGSOURCELINE ), 0))

#else	// _DEBUG

	// NON DEBUG compiles out the checks.
#ifndef ASSERT
#define ASSERT(exp)			__noop	// acts like UNREFERENCED_PARAMETER() ?
#endif	// ASSERT
#ifndef DEBUG_CHECK
#define DEBUG_CHECK(exp)	__noop
#endif	// DEBUG_CHECK
#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT(exp,sDesc)	__noop
#endif // DEBUG_ASSERT

#endif	// ! _DEBUG

};

#endif // _INC_CDebugAssert_H
