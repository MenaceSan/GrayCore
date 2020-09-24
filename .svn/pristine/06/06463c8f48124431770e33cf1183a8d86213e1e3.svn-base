//
//! @file CExceptionSystem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CExceptionSystem.h"
#include "CLogMgr.h"
#include "CAppState.h"

#if defined(_CPPUNWIND)

#if defined(_MSC_VER)
#include <crtdbg.h>
#include <eh.h>
#else
#include <signal.h>
#endif // _WIN32

namespace Gray
{
#ifdef _WIN32
	cExceptionSystem::cExceptionSystem(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData)
		: cException("SystemException", LOGLEV_CRIT)
		, m_nSystemErrorCode(uNTStatus)
		, m_pAddress(0)
	{
		//! _WIN32 gets an exception. from _set_se_translator()
		//! @arg uNTStatus = 0xC0000094 = STATUS_INTEGER_DIVIDE_BY_ZERO
		//! uNTStatus = 0xC0000005 = STATUS_ACCESS_VIOLATION.
		//! uNTStatus = DBG_TERMINATE_PROCESS
		//! @note use sm_dwCodeStart to offset the exception address to match the MAP file.
		//!  is not accurate in debug builds for some stupid reason.

#ifdef _DEBUG
		static const UINT_PTR k_dwCodeStart = 0; // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
		// static const UINT_PTR k_dwCodeStart = 0x06d40; // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
#else
		static const UINT_PTR k_dwCodeStart = 0; // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
#endif
	//	__asm mov k_dwCodeStart, CODE

		if (m_nSystemErrorCode == 0)
		{
			m_nSystemErrorCode = (pData) ? (pData->ExceptionRecord->ExceptionCode) : STATUS_NONCONTINUABLE_EXCEPTION;
		}

		PVOID pAddr = (pData) ? (pData->ExceptionRecord->ExceptionAddress) : 0;
		UINT_PTR dwAddr = (UINT_PTR)pAddr;
		dwAddr -= k_dwCodeStart;
		m_pAddress = (PVOID)(dwAddr);
	}
#else
	cExceptionSystem::cExceptionSystem(SYSCODE_t iSignal)
		: cException("SystemException", LOGLEV_CRIT)
		, m_nSystemErrorCode(iSignal)
		, m_pAddress(0)
	{
#if 0
		//! get a stack dump.
		void* aStack[25];
		int nSize = ::backtrace(aStack, _countof(aStack));
		char** ppSymbols = ::backtrace_symbols(aStack, nSize);
		for (int i = 0; i < nSize; i++)
		{
			cout << ppSymbols[i] << endl;
		}
		free(ppSymbols);
#endif
	}
#endif

	cExceptionSystem::~cExceptionSystem() THROW_DEF
	{
	}

	BOOL cExceptionSystem::GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) // virtual
	{
		//! @note what module is this in ? in the case of a DLL
		//! look up m_nSystemErrorCode codes ??
		StrT::sprintfN(lpszError, nLenMaxError, _GT("Exception code=0%X, addr=0%x, context=%d"),
			m_nSystemErrorCode, (UINT_PTR)m_pAddress, (pnHelpContext != nullptr) ? pnHelpContext : 0);
		return true;
	}

#ifdef _WIN32
	CATTR_NORETURN void _cdecl cExceptionSystem::CatchException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData) // throw (cExceptionSystem)
	{
		//! _WIN32 calls this when it gets a system exception. from _set_se_translator()
		//! NTStatus codes. (from kernel mode)
		//! uNTStatus = 0xc0000094 = STATUS_INTEGER_DIVIDE_BY_ZERO
		//! uNTStatus = 0xC0000005 = STATUS_ACCESS_VIOLATION.
		//! uNTStatus = 0xC0000374 = STATUS_HEAP_CORRUPTION = A heap has been corrupted.
		//! uNTStatus = DBG_TERMINATE_PROCESS
		GRAY_THROW cExceptionSystem(uNTStatus, pData);
		UNREACHABLE_CODE(__noop);
	}
	int _cdecl cExceptionSystem::FilterException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData) // throw (cExceptionSystem); // static
	{
		//! for use with __try and __exception in _WIN32 _MSC_VER. (NOT try/catch)
		UNREFERENCED_PARAMETER(pData);
		if (uNTStatus == 0xC0000374)
		{
			// NOTE: Win7 can throw this in cases that are harmless!
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;	// not what i wanted.
	}
	CATTR_NORETURN void _cdecl cExceptionSystem::CatchTerminate(void) // static
	{
		//! A handler can not be found for a thrown exception,
		//! or for some other exceptional circumstance that makes impossible to continue the handling process.
		//! http://www.cplusplus.com/reference/std/exception/set_terminate/
		//! passes along to CAppExitCatcher
		DEBUG_ERR(("cExceptionSystem::CatchTerminate no exception handler found!"));
		CAppState::AbortApp();
		UNREACHABLE_CODE(__noop);
	}
#else
	void __cdecl cExceptionSystem::SignalHandler(SYSCODE_t iSignal) // static
	{
		//! iSignal = SIGSEGV = segmentation fault.
		//! iSignal = SIGFPE = floating point exception
		GRAY_THROW cExceptionSystem(iSignal);
	}
#endif

	void GRAYCALL cExceptionSystem::InitForCurrentThread() // static
	{
		//! Force the system exceptions to be our custom version.
		//! @note InitForCurrentThread must be called for each thread that wants to handle exceptions this way.
#if defined(_MSC_VER)
#if defined(_CPPUNWIND) && ! defined(_M_CEE_PURE) && ! defined(_MFC_VER)
		::_set_se_translator(CatchException); // warning C4535: calling _set_se_translator() requires /EHa ??
#endif
		::set_terminate(CatchTerminate);
#else
	// TODO: In __linux__, if we get an access violation, an exception isn't thrown.
	// Instead, we get a SIGSEGV, and the process cores.
	// The following code takes care of this for us. signals are set for all threads.
	// sigthreadmask signal(SIGSEGV, SignalHandler);
	// sigthreadmask signal(SIGFPE, SignalHandler);
#endif
#ifdef _WIN32
	// don't put up a dialog automatically.
		::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOOPENFILEERRORBOX);
#endif
	}
}

//***************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(cExceptionSystem)
{
	UNITTEST_METHOD(cExceptionSystem)
	{
#ifdef _MSC_VER
		__try
		{
			// do bad stuff.

			// nullptr reference.

			// divide by zero. RPC_S_ZERO_DIVIDE
		}
		__except (cExceptionSystem::FilterException(GetExceptionCode(), GetExceptionInformation()))
		{
			// like catch
			//-- display the fatal error message
#if 0
			MessageBox(0, "Exception was caught here!",
				"Unexpected Error", MB_OK);
#endif

			DEBUG_MSG(("Exception Test"));
		}
#endif

		// TODO a try/catch block as well.

		// cExceptionSystem::InitForCurrentThread()

	}
};
UNITTEST_REGISTER(cExceptionSystem, UNITTEST_LEVEL_Core);
#endif

#endif	// ! _CPPUNWIND
