//
//! @file cException.h
//! Custom exception classes.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cException_H
#define _INC_cException_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cExceptionBase.h"
#include "HResult.h"
#include "cObject.h" 		// DECLARE_DYNAMIC()
#include "cString.h"

namespace Gray
{
	typedef cStringT<LOGCHAR_t> cStringL;	//!< Log string.

#if defined(_MSC_VER)
#pragma warning(disable:4275)	// non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx 
#endif
	class GRAYCORE_LINK cExceptionHolder : public cPtrFacade < cExceptionBase >
	{
		//! @class Gray::cExceptionHolder
		//! Holds/Wraps an exception in a uniform way, and hides the fact that it is a pointer (MFC) or a reference (STL).
		//! make sure we call Delete() when we are done with this.
		//! ONLY useful because MFC passes all exceptions by pointer and STL does not.

	public:
		static const StrLen_t k_MSG_MAX_SIZE = 1024;	//!< arbitrary max message size.

	private:
		bool m_bDeleteEx;	//!< i must delete this. Always true for MFC ?

	public:
		cExceptionHolder() noexcept
			: m_bDeleteEx(false)
		{
		}
		explicit cExceptionHolder(cExceptionBase* pEx, bool bDeleteEx = true) noexcept
			: cPtrFacade<cExceptionBase>(pEx)
			, m_bDeleteEx(bDeleteEx)
		{
			//! Normal usage for _MFC_VER.
		}
		explicit cExceptionHolder(cExceptionBase& ex) noexcept
			: cPtrFacade<cExceptionBase>(&ex)
			, m_bDeleteEx(false)
		{
			//! Normal STL usage.
		}
		~cExceptionHolder() noexcept
		{
			//! basically an auto_ptr
			if (m_bDeleteEx && m_p != nullptr) // make sure DetachException() wasn't called.
			{
#ifdef _MFC_VER	// using _MFC_VER.
				m_p->Delete();
#else
				delete m_p;
#endif
			}
		}
		void AttachException(cExceptionBase* pEx, bool bDeleteEx)
		{
			ASSERT(m_p == nullptr);
			m_p = pEx;
			m_bDeleteEx = bDeleteEx;
		}
		cException* get_Ex() const;	// is Custom?

		BOOL GetErrorMessage(LOGCHAR_t* lpszError, StrLen_t nLenMaxError = k_MSG_MAX_SIZE) const;
		cStringL get_ErrorStr() const;
		LOGLEV_TYPE get_Severity() const;
	};

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
#if ! defined(_MFC_VER) && defined(_MSC_VER) && defined(_CPPUNWIND) // not using _MFC_VER.
			, cExceptionBase(pszDescription)
#endif
			, m_pszDescription((pszDescription == nullptr) ? k_szDescriptionDefault : pszDescription)
		{
		}
		virtual ~cException() THROW_DEF
		{
		}
		LOGLEV_TYPE get_Severity() const noexcept
		{
			return m_eSeverity;
		}

		DECLARE_DYNAMIC(cException);	// For MFC support.

		//! overloaded to provide context specific error messages. In English or default OS language. (as GChar_t)
		//! @note stupid BOOL return is for MFC compatibility.
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError = cExceptionHolder::k_MSG_MAX_SIZE, UINT* pnHelpContext = nullptr);
		cStringL get_ErrorStr(); // similar to STL what()

#ifndef _MFC_VER	// using _MFC_VER.
		virtual const char* what() const THROW_DEF
		{
			//! for STL. store the GetErrorMessage string some place ?
			return m_pszDescription;
		}
#endif // ! _MFC_VER

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
		HRESULT get_HResultCode() const noexcept
		{
			return m_hResultCode;
		}
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) override;
	};
} 

#endif // _INC_cException_H
