//! @file cException.h
//! Custom exception classes.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cException_H
#define _INC_cException_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cExceptionBase.h"
#include "cObject.h"  // DECLARE_DYNAMIC()
#include "cString.h"

namespace Gray {
typedef cStringT<LOGCHAR_t> cStringL;  /// Log string.

#if defined(_MSC_VER)
#pragma warning(disable : 4275)  // non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx
#endif
/// <summary>
/// Holds/Wraps an exception in a uniform way, and hides the fact that it is a pointer (MFC) or a reference (STL).
/// make sure we call Delete() when we are done with this.
/// ONLY useful because MFC passes all exceptions by pointer and STL does not.
/// </summary>
class GRAYCORE_LINK cExceptionHolder : public cPtrFacade<cExceptionBase> {
    typedef cPtrFacade<cExceptionBase> SUPER_t;

 public:
    static const StrLen_t k_MSG_MAX_SIZE = 1024;  /// arbitrary max message size.
 private:
    bool _isDeleteReq = false;  /// i must delete this. Always true for MFC ? IsOwned

 public:
    cExceptionHolder() noexcept {}
    /// Normal usage for _MFC_VER.
    explicit cExceptionHolder(cExceptionBase* pEx, bool bDeleteEx = true) noexcept : cPtrFacade<cExceptionBase>(pEx), _isDeleteReq(bDeleteEx) {}
    /// Normal STL usage.
    explicit cExceptionHolder(cExceptionBase& ex) noexcept : cPtrFacade<cExceptionBase>(&ex) {}

    /// basically an auto_ptr
    ~cExceptionHolder() noexcept {
        if (_isDeleteReq && this->isValidPtr()) {  // make sure DetachException() wasn't called.
#ifdef _MFC_VER                                    // using _MFC_VER Exceptions.
            get_Ptr()->Delete();
#else
            delete get_Ptr();
#endif
        }
    }
    void AttachException(cExceptionBase* pEx, bool bDeleteEx) {
        ASSERT(!this->isValidPtr());
        AttachPtr(pEx);
        _isDeleteReq = bDeleteEx;
    }
    using SUPER_t::DetachPtr;  // dangerous but allow this.

    /// <summary>
    /// Get the cException version of the exception if it is based on it. Use dynamic_cast.
    /// </summary>
    /// <returns>nullptr if a native MFC "::CException" or STL "stl::exception".</returns>
    cException* get_Ex() const;  // is Custom?

    BOOL GetErrorMessage(StrBuilder<LOGCHAR_t>& sb) const;
    cStringL get_ErrorStr() const;
    LOGLVL_t get_Severity() const;
};

/// <summary>
/// Base for my custom exceptions: cExceptionHResult, cExceptionSystem, cExceptionAssert
/// System/External exceptions will of course just throw cExceptionBase
/// uses GRAY_THROW macro to hide STL vs MFC differences. MFC uses base name CException.
/// </summary>
struct GRAYCORE_LINK cException : public cExceptionBase {
    const LOGLVL_t _eSeverity;         /// how bad is this ?
    const LOGCHAR_t* _pszDescription;  /// this pointer should be to something static !?
    static const LOGCHAR_t k_szDescriptionDefault[];

    cException(const LOGCHAR_t* pszDescription, LOGLVL_t eLogLevel = LOGLVL_t::_ERROR) noexcept
        : _eSeverity(eLogLevel),
#if !defined(_MFC_VER) && defined(_MSC_VER) && defined(_CPPUNWIND)  // not using _MFC_VER.
          cExceptionBase(pszDescription),
#endif
          _pszDescription((pszDescription == nullptr) ? k_szDescriptionDefault : pszDescription) {
    }

    LOGLVL_t get_Severity() const noexcept {
        return _eSeverity;
    }

    DECLARE_DYNAMIC(cException);  // For MFC support.

    //! overloaded to provide context specific error messages. In English or default OS language. (as GChar_t)
    //! @note stupid BOOL return is for MFC compatibility.
    virtual BOOL GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext = nullptr);
    cStringL get_ErrorStr();  // similar to STL what()

#ifndef _MFC_VER  // using _MFC_VER.
    const char* what() const noexcept override {
        //! for STL. store the GetErrorMessage string some place ?
        return _pszDescription;
    }
#endif  // ! _MFC_VER

    static CATTR_NORETURN void GRAYCALL ThrowEx(const char* pszExp, const cDebugSourceLine src);
};

/// <summary>
///  Store some HRESULT error code based exception.
/// </summary>
struct GRAYCORE_LINK cExceptionHResult : public cException {
    typedef cException SUPER_t;
    HRESULT _hResultCode;  /// HRESULT S_OK=0, "winerror.h" code. 0x20000000 = start of custom codes. E_FAIL = unknown error

    cExceptionHResult(HRESULT hResultCode = E_FAIL, const LOGCHAR_t* pszDescription = nullptr, LOGLVL_t eSeverity = LOGLVL_t::_ERROR) THROW_DEF : SUPER_t(pszDescription, eSeverity), _hResultCode(hResultCode) {}

    HRESULT get_HResultCode() const noexcept {
        return _hResultCode;
    }
    BOOL GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) override;
};
}  // namespace Gray
#endif  // _INC_cException_H
