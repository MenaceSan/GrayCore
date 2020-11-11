//
//! @file cException.h
//! Custom exception classes.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_CException_H
#define _INC_CException_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cExceptionBase.h"
#include "HResult.h"
#include "cUnitTestDecl.h"
#include "cObjectCreator.h"		// DECLARE_DYNAMIC

UNITTEST_PREDEF(cException)

namespace Gray
{
#if defined(_MSC_VER)
#pragma warning(disable:4275)	// non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx 
#endif

	class GRAYCORE_LINK cException : public cExceptionBase
	{
		//! @class Gray::cException
		//! Base for my custom exceptions: cExceptionHResult, cExceptionSystem, cExceptionAssert
		//! System exceptions will of course just throw cExceptionBase
		//! uses GRAY_THROW macro to hide STL vs MFC differences. MFC uses base name CException.

	public:
		const LOGLEV_TYPE m_eSeverity;		//!< how bad is this ?
		const LOGCHAR_t* m_pszDescription;	//!< this pointer should be to something static !?
		static const LOGCHAR_t* k_szDescriptionDefault;

	public:
		cException(const LOGCHAR_t* pszDescription, LOGLEV_TYPE eLogLevel = LOGLEV_ERROR) noexcept
			: m_eSeverity(eLogLevel)
#if ! defined(_MFC_VER) && defined(_MSC_VER) && ! defined(UNDER_CE) // not using _MFC_VER.
			, cExceptionBase(pszDescription)
#endif
			, m_pszDescription((pszDescription == nullptr) ? k_szDescriptionDefault : pszDescription)
		{
		}
		virtual ~cException() THROW_DEF
		{
		}
		LOGLEV_TYPE get_Severity() const
		{
			return m_eSeverity;
		}

		DECLARE_DYNAMIC(cException);	// For MFC support.

		//! overloaded to provide context specific error messages.
		//! @note stupid BOOL return is for MFC compatibility.
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError = cExceptionHolder::k_MSG_MAX_SIZE, UINT* pnHelpContext = nullptr);
		cStringL get_ErrorStr(); // similar to STL what()

#ifndef _MFC_VER	// using _MFC_VER.
		virtual const char* what() const THROW_DEF
		{
			//! STL. store the GetErrorMessage string some place ?
			return m_pszDescription;
		}
#endif // ! _MFC_VER

		UNITTEST_FRIEND(cException);
	};

	class GRAYCORE_LINK cExceptionHResult : public cException
	{
		//! @class Gray::cExceptionHResult
		//! Store some HRESULT error code based exception.
		typedef cException SUPER_t;

	public:
		HRESULT m_hResultCode;	//!< HRESULT S_OK=0, "winerror.h" code. 0x20000000 = start of custom codes. E_FAIL = unknown error

	public:
		cExceptionHResult(HRESULT hResultCode = E_FAIL, const LOGCHAR_t* pszDescription = nullptr, LOGLEV_TYPE eSeverity = LOGLEV_ERROR) THROW_DEF
			: cException(pszDescription, eSeverity)
			, m_hResultCode(hResultCode)
		{
		}
		virtual ~cExceptionHResult() THROW_DEF
		{
		}
		HRESULT get_HResultCode() const
		{
			return m_hResultCode;
		}
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) override;
	};
};

#endif // _INC_CException_H
