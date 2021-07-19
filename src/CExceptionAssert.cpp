//
//! @file cExceptionAssert.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cExceptionAssert.h"
#include "cDebugAssert.h"
#include "cLogMgr.h"
#include "StrArg.h"

namespace Gray
{
	void GRAYCALL cException::ThrowEx(const char* pszExp, const cDebugSourceLine src) // static
	{
		//! This assert cannot be ignored. We must throw cExceptionAssert after this. Things will be horribly corrupted if we don't?
		//! Leave this in release code.
		//! Similar to AfxThrowInvalidArgException()

		cLogMgr::I().addEventF(LOG_ATTR_DEBUG | LOG_ATTR_INTERNAL, LOGLEV_CRIT,
			"Assert Throw:'%s' file '%s', line %d",
			LOGSTR(pszExp), LOGSTR(src.m_pszFile), src.m_uLine);

#ifdef _CPPUNWIND
		GRAY_THROW cExceptionAssert(pszExp, LOGLEV_CRIT, src);
#else
		// TODO signal or log instead of throw??
#endif
	}

	cExceptionAssert::cExceptionAssert(const LOGCHAR_t* pExp, LOGLEV_TYPE eLogLevel, const cDebugSourceLine& src)
		: cException("Assert", eLogLevel)
		, m_pExp(pExp)
		, m_Src(src)
	{
	}

	cExceptionAssert::~cExceptionAssert() THROW_DEF
	{
	}

	BOOL cExceptionAssert::GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) // virtual
	{
		UNREFERENCED_PARAMETER(pnHelpContext);
		StrT::sprintfN(lpszError, nLenMaxError,
			_GT("Assert pri=%d:'%s' file '%s', line %d"),
			get_Severity(), StrArg<GChar_t>(m_pExp), StrArg<GChar_t>(m_Src.m_pszFile), m_Src.m_uLine);
		return true;
	}

	void GRAYCALL cExceptionAssert::Throw(const LOGCHAR_t* pExp, const cDebugSourceLine& src) // static
	{
		//! These can get left in release code.
		//! This is similar to _assert() in M$ code.
#if defined(_CPPUNWIND)
		GRAY_THROW cExceptionAssert(pExp, LOGLEV_CRIT, src);
#else
	// system ASSERT ?
#endif
	}
}

extern "C"
{
#ifndef UNDER_CE
	int _cdecl _purecall()
	{
		//! catch this special type of C++ exception as well.
		Gray::cExceptionAssert::Throw("purecall", Gray::cDebugSourceLine("unknown", "", 1));
		return 0;
	}
#endif

#if defined(_MSC_VER) && defined(GRAY_STATICLIB)
	void _cdecl _amsg_exit(int iArg)
	{
		//! overload to try to trap some of the other strange exit conditions !!!
		//! Some strange _MSC_VER stdlib calls use this for really bad stuff.
		//! http://msdn.microsoft.com/en-us/library/ff770579.aspx
		//! DLL returns error LNK2005: __amsg_exit already defined in MSVCRTD.lib(MSVCR80D.dll)
		//! throw this as cExceptionAssert ?? NOT sure this will work?
		UNREFERENCED_PARAMETER(iArg);
		Gray::cExceptionAssert::Throw("_amsg_exit", Gray::cDebugSourceLine("unknown", "", 1));
	}
#endif

#if defined(_MSC_VER) // && defined(GRAY_STATICLIB)
	void _cdecl _assert(void* pExp, void* pFile, unsigned uLine)
	{
		//! Overload the system version of this just in case.
		//! Trap for 3rd party libraries doing funny stuff.
		Gray::cExceptionAssert::Throw((const Gray::LOGCHAR_t*)pExp, Gray::cDebugSourceLine((const char*)pFile, "", (WORD)uLine));
	}
#endif
}	// extern "C"
