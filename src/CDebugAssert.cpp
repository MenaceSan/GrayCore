//
//! @file CDebugAssert.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CDebugAssert.h"
#include "CLogMgr.h"
#include "CString.h"
#include "CExceptionAssert.h"

#ifdef UNDER_CE
#include <dbgapi.h>
// WinCE doesn't seem to have a macro for assert()?
#define assert(x) ::DbgAssert( #x, __FILE__, __LINE__ );
#endif

namespace Gray
{
	AssertCallback_t* CDebugAssert::sm_pAssertCallback = CDebugAssert::Assert_System;	// default = pass to assert.
	bool CDebugAssert::sm_bAssertTest = false;

	//*************************************************************************

	bool CALLBACK CDebugAssert::Assert_System(const char* pszExp, const CDebugSourceLine& src) // static
	{
		//! AssertCallback_t to use the system assert dialog. Use with sm_pAssertCallback.
		//! @return true = continue and ignore the assert.

		// if (sm_pAssertCallback == CDebugAssert::Assert_System && !CAppState::isDebuggerPresent()) return;	// Ignore it if no debugger present ?

#if defined(__linux__)
		__assert_fail(pszExp, src.m_pszFile, src.m_uLine, "");
#elif defined(UNDER_CE) || ! defined(_DEBUG) || ! defined(_MSC_VER)
		::DebugBreak();	// Do Int 3 - to halt all threads at point.
#elif _MSC_VER >= 1400
		_wassert(cStringW(pszExp), cStringW(src.m_pszFile), src.m_uLine);
#elif _MSC_VER >= 1300
		_assert(pszExp, src.m_pszFile, src.m_uLine);
#else
		_assert((void*)pszExp, (void*)src.m_pszFile, src.m_uLine);
#endif
		// if this returns at all, that means we chose to ignore the assert and continue.
		return true;
	}

	bool GRAYCALL CDebugAssert::Assert_Fail(const char* pszExp, const CDebugSourceLine src)	// static
	{
		//! like _assert() is M$ code.
		//! Put something in the log after (or before?) doing normal assert processing. is this too dangerous?
		//! @return false to indicate something failed. stop. (for macro)
		//! @return true = continue and ignore the assert.
		// Watch for sm_bAssertTest

		if (sm_pAssertCallback != nullptr)	// Divert the assert for testing? else just log it and keep going.
		{
			// maybe do special processing for unit tests.
			if (!sm_pAssertCallback(pszExp, src)) // AssertCallback_t
				return false;
		}

		// Just log it and try to continue.
		CLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLEV_CRIT,
			"Assert Fail:'%s' file '%s', line %d",
			LOGSTR(pszExp), LOGSTR(src.m_pszFile), src.m_uLine);
		// Flush logs.
		// cExceptionAssert?
		// true = Keep going?
		return false;
	}

	void GRAYCALL CDebugAssert::Assert_Throw(const char* pszExp, const CDebugSourceLine src)
	{
		//! This assert cannot be ignored. We must throw cExceptionAssert after this. Things will be horribly corrupted if we don't?
		//! Leave this in release code.
		//! Similar to AfxThrowInvalidArgException()

		if (sm_bAssertTest)	// Allow me to test the throw.
		{
			sm_bAssertTest = false;	// Test complete.
		}
		else
		{
			CLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLEV_CRIT,
				"Assert Throw:'%s' file '%s', line %d",
				LOGSTR(pszExp), LOGSTR(src.m_pszFile), src.m_uLine);
		}

#ifdef _CPPUNWIND
		GRAY_THROW cExceptionAssert(pszExp, LOGLEV_CRIT, src);
#else
		// TODO signal or log instead of throw??
#endif
	}

	bool GRAYCALL CDebugAssert::Debug_Fail(const char* pszExp, const CDebugSourceLine src) // static
	{
		//! A 'softer' version of assert. non-fatal checks. for use in constructors, etc.
		//! @note: always return false to indicate something failed. (for macro)
		// CODEPROFILEFUNC();
#ifdef _DEBUG
		CLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLEV_ERROR,
			"Check Fail:'%s' file '%s', line %d",
			LOGSTR(pszExp), LOGSTR(src.m_pszFile), src.m_uLine);
#endif
		// Sync the debug thread??
		return false;
	}
}
