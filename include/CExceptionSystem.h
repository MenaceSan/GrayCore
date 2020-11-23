//
//! @file cExceptionSystem.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cExceptionSystem_H
#define _INC_cExceptionSystem_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cException.h"
#include "cUnitTestDecl.h"

#if defined(_CPPUNWIND)

namespace Gray
{
	UNITTEST2_PREDEF(cException);

	class GRAYCORE_LINK cExceptionSystem : public cException
	{
		//! @class Gray::cExceptionSystem
		//! Catch and get details on the system exceptions (or Linux signals).
		//! e.g. nullptr access, divide by zero, etc.
		//! EVENT_E_INTERNALEXCEPTION
		//! @note
		//!  InitForCurrentThread must be called for each thread that wants to receive exceptions like this.
		//! uNTStatus = 0xC0000005 = STATUS_ACCESS_VIOLATION

	public:
#ifdef _WIN32
		typedef unsigned int SYSCODE_t;	//!< _WIN32 STATUS_ACCESS_VIOLATION uNTStatus code
#else
		typedef int SYSCODE_t;	//!< POSIX signal
#endif

	protected:
		SYSCODE_t m_nSystemErrorCode;	//!< _WIN32 STATUS_ACCESS_VIOLATION uNTStatus code or iSignal POSIX signal
		void* m_pAddress;				//!< source address in the code.

	public:
#ifdef _WIN32
		cExceptionSystem(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData = nullptr);
#else
		cExceptionSystem(SYSCODE_t iSignal);
#endif
		virtual ~cExceptionSystem() THROW_DEF;

		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nMaxError, UINT* pnHelpContext) override;

		static void GRAYCALL InitForCurrentThread();
#ifdef _WIN32
		static CATTR_NORETURN void _cdecl CatchException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData); // throw (cExceptionSystem);
		static CATTR_NORETURN void _cdecl CatchTerminate(void);
		static int _cdecl FilterException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData); // throw (cExceptionSystem);
#else
		static void __cdecl SignalHandler(SYSCODE_t iSignal);
#endif
		UNITTEST2_FRIEND(cException);
	};
}
#endif
#endif
