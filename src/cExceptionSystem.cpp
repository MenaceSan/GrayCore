//! @file cExceptionSystem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
#include "cAppState.h"
#include "cExceptionSystem.h"
#include "cLogMgr.h"

#if defined(_CPPUNWIND)

#if defined(_MSC_VER)
#include <crtdbg.h>
#include <eh.h>
#else
#include <signal.h>
#endif  // _WIN32

namespace Gray {
#ifdef _WIN32
cExceptionSystem::cExceptionSystem(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData) : cException("SystemException", LOGLVL_t::_CRIT), _nSystemErrorCode(uNTStatus) {
#ifdef _DEBUG
    static const UINT_PTR k_dwCodeStart = 0;  // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
                                              // static const UINT_PTR k_dwCodeStart = 0x06d40; // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
#else
    static const UINT_PTR k_dwCodeStart = 0;  // (UINT32)(BYTE *) &globalstartsymbol;	// used to sync up to my MAP file.
#endif
    //	__asm mov k_dwCodeStart, CODE

    if (_nSystemErrorCode == 0) {
        _nSystemErrorCode = (pData) ? (pData->ExceptionRecord->ExceptionCode) : STATUS_NONCONTINUABLE_EXCEPTION;
    }

    PVOID pAddr = (pData) ? (pData->ExceptionRecord->ExceptionAddress) : 0;
    UINT_PTR nAddr = CastPtrToNum(pAddr);
    nAddr -= k_dwCodeStart;
    _pAddress = (PVOID)(nAddr);
}
#else
cExceptionSystem::cExceptionSystem(SYSCODE_t iSignal) : cException("SystemException", LOGLVL_t::_CRIT), _nSystemErrorCode(iSignal) {
#if 0
	//! get a stack dump.
	void* aStack[25];
	int nSize = ::backtrace(aStack, _countof(aStack));
	char** ppSymbols = ::backtrace_symbols(aStack, nSize);
	for (int i = 0; i < nSize; i++) {
		cout << ppSymbols[i] << endl;
	}
	::free(ppSymbols);
#endif
}
#endif

cExceptionSystem::~cExceptionSystem() THROW_DEF {}

BOOL cExceptionSystem::GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) {  // virtual
    //! @note what module is this in ? in the case of a DLL
    //! look up _nSystemErrorCode codes ??
    sb.AddFormat(_GT("Exception code=0%X, addr=0%p, context=%d"), _nSystemErrorCode, CastPtrToNum(_pAddress), (pnHelpContext != nullptr) ? pnHelpContext : 0);
    return true;
}

#ifdef _WIN32
CATTR_NORETURN void _cdecl cExceptionSystem::CatchException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData) {  // throw (cExceptionSystem)
    //! _WIN32 calls this when it gets a system exception. from _set_se_translator()
    //! NTStatus codes. (from kernel mode)
    //! uNTStatus = 0xc0000094 = STATUS_INTEGER_DIVIDE_BY_ZERO
    //! uNTStatus = 0xC0000005 = STATUS_ACCESS_VIOLATION.
    //! uNTStatus = 0xC0000374 = STATUS_HEAP_CORRUPTION = A heap has been corrupted.
    //! uNTStatus = DBG_TERMINATE_PROCESS
    GRAY_THROW cExceptionSystem(uNTStatus, pData);
    UNREACHABLE_CODE(__noop);
}
int _cdecl cExceptionSystem::FilterException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData) {  // throw (cExceptionSystem); // static
    //! for use with __try and __exception in _WIN32 _MSC_VER. (NOT try/catch)
    UNREFERENCED_PARAMETER(pData);
    if (uNTStatus == 0xC0000374) {
        // NOTE: Win7 can throw this in cases that are harmless!
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;  // not what i wanted.
}
CATTR_NORETURN void _cdecl cExceptionSystem::CatchTerminate() {  // static
    //! A handler can not be found for a thrown exception,
    //! or for some other exceptional circumstance that makes impossible to continue the handling process.
    //! http://www.cplusplus.com/reference/std/exception/set_terminate/
    //! passes along to cAppExitCatcher
    DEBUG_ERR(("cExceptionSystem::CatchTerminate no exception handler found!"));
    cAppState::AbortApp();
    UNREACHABLE_CODE(__noop);
}
#else
void __cdecl cExceptionSystem::SignalHandler(SYSCODE_t iSignal) {  // static
    //! iSignal = SIGSEGV = segmentation fault.
    //! iSignal = SIGFPE = floating point exception
    GRAY_THROW cExceptionSystem(iSignal);
}
#endif

void GRAYCALL cExceptionSystem::InitForCurrentThread() {  // static
    //! Force the system exceptions to be our custom version.
    //! @note InitForCurrentThread must be called for each thread that wants to handle exceptions this way.
#if defined(_MSC_VER)
#if defined(_CPPUNWIND) && !defined(_M_CEE_PURE) && !defined(_MFC_VER)
    ::_set_se_translator(CatchException);  // warning C4535: calling _set_se_translator() requires /EHa ?? (C++->Code Generation->Enable C++ Exceptions = Yes with SEH Exceptions)
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
}  // namespace Gray
#endif  // ! _CPPUNWIND
