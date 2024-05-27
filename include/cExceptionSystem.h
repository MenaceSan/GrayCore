//! @file cExceptionSystem.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cExceptionSystem_H
#define _INC_cExceptionSystem_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cException.h"

#if defined(_CPPUNWIND)

namespace Gray {
/// <summary>
/// Catch and get details on the system exceptions (or Linux signals).
/// e.g. nullptr access, divide by zero, etc.
/// EVENT_E_INTERNALEXCEPTION
/// @note InitForCurrentThread must be called for each thread that wants to receive exceptions like this.
/// uNTStatus = 0xC0000005 = STATUS_ACCESS_VIOLATION
/// </summary>
class GRAYCORE_LINK cExceptionSystem : public cException {
 public:
#ifdef _WIN32
    typedef unsigned int SYSCODE_t;  /// _WIN32 STATUS_ACCESS_VIOLATION uNTStatus code
#else
    typedef int SYSCODE_t;  /// POSIX signal
#endif

 protected:
    SYSCODE_t m_nSystemErrorCode;  /// _WIN32 STATUS_ACCESS_VIOLATION uNTStatus code or iSignal POSIX signal
    void* m_pAddress;              /// source address in the code.

 public:
#ifdef _WIN32
    /// <summary>
    /// _WIN32 gets an exception. from _set_se_translator()
    /// @note use sm_dwCodeStart to offset the exception address to match the MAP file.
    /// is not accurate in debug builds for some stupid reason.
    /// </summary>
    /// <param name="uNTStatus">0xC0000094 = STATUS_INTEGER_DIVIDE_BY_ZERO, 0xC0000005 = STATUS_ACCESS_VIOLATION. DBG_TERMINATE_PROCESS</param>
    /// <param name="pData"></param>
    cExceptionSystem(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData = nullptr);
#else
    cExceptionSystem(SYSCODE_t iSignal);
#endif
    ~cExceptionSystem() THROW_DEF override;

    BOOL GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) override;

    static void GRAYCALL InitForCurrentThread();
#ifdef _WIN32
    static CATTR_NORETURN void _cdecl CatchException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData);  // throw (cExceptionSystem);
    static CATTR_NORETURN void _cdecl CatchTerminate();
    static int _cdecl FilterException(SYSCODE_t uNTStatus, struct _EXCEPTION_POINTERS* pData);  // throw (cExceptionSystem);
#else
    static void __cdecl SignalHandler(SYSCODE_t iSignal);
#endif
};
}  // namespace Gray
#endif
#endif
