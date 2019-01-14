//
//! @file CExceptionBase.h
//! Wrap base exception classes:
//! STL uses exception&
//! MFC uses CException* (must call Delete()?)
//! like C#/CLR uses Exception^
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_CExceptionBase_H
#define _INC_CExceptionBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CPtrFacade.h"
#include "StrT.h"
#include "CObject.h"
#include "CLogLevel.h"
#ifndef _MFC_VER	// NOT using _MFC_VER.
#include <exception> // use STL based exception class.
#endif

namespace Gray
{

#if ! defined(_CPPUNWIND)	// STL _HAS_EXCEPTIONS
	class cExceptionBase	// stub this out if no throws allowed.
	{
		//! @class Gray::cExceptionBase
		//! Common base for STL or MFC.
	public:
		virtual ~cExceptionBase()
		{
		}
		virtual const char* what() const THROW_DEF = 0;
	};
#elif defined(_MFC_VER)
	// MFC throw a pointer to this structure to track an error. must call Delete() to destruct.
	// NOTE: cannot be instantiated on the stack. MUST be "new CException" as per MFC rules.
	// base for to MFC CFileException is CException
	typedef ::CException cExceptionBase;			//!< assume ALL exceptions are of this base. MFC CException
#else
	// throw a reference to this structure to track an error. auto destruct as normal class.
	typedef std::exception cExceptionBase;			//!< assume ALL exceptions are of this base. STL std::exception
#endif

	class GRAYCORE_LINK cException;	// Base for my custom exceptions. Also based on cExceptionBase

	class GRAYCORE_LINK cExceptionHolder : public CPtrFacade < cExceptionBase >
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
		cExceptionHolder()
			: m_bDeleteEx(false)
		{
		}
		explicit cExceptionHolder(cExceptionBase* pEx, bool bDeleteEx=true)
			: CPtrFacade<cExceptionBase>(pEx)
			, m_bDeleteEx(bDeleteEx)
		{
			//! Normal usage for _MFC_VER.
		}
		explicit cExceptionHolder(cExceptionBase& ex)
			: CPtrFacade<cExceptionBase>(&ex)
			, m_bDeleteEx(false)
		{
			//! Normal STL usage.
		}
		~cExceptionHolder()
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
		void AttachException(cExceptionBase* pEx, bool bDeleteEx )
		{
			ASSERT(m_p == nullptr);
			m_p = pEx;
			m_bDeleteEx = bDeleteEx;
		}
		cException* get_Ex() const;	// is Custom?

		BOOL GetErrorMessage(LOGCHAR_t* lpszError, StrLen_t nLenMaxError = k_MSG_MAX_SIZE) const;
		CStringL get_ErrorStr() const;
		LOGLEV_TYPE get_Severity() const;
	};

	// Abstract the several different types of exception handling as macros.
#if ! defined(_CPPUNWIND)	// No exception allowed. use STL _HAS_EXCEPTIONS ?
	// #define GRAY_THROW		// no throws allowed.
#define GRAY_TRY				if (true) {
#define GRAY_TRY_CATCH(c,ex)	} else if (false) {
#define GRAY_TRY_CATCHALL		} else {
#define GRAY_TRY_END			}
#elif defined(_MFC_VER)
	// MFC TRY/CATCH/AND_CATCH/END_CATCH
#define GRAY_THROW				throw new // throw dynamic "new" pointer (MFC). This is usually BAD practice in modern times.
#define GRAY_TRY				TRY
#define GRAY_TRY_CATCH(c,ex)	CATCH(c,ex)		// c=cExceptionBase
#define GRAY_TRY_CATCHALL		CATCH(CException,ex)	// CATCH_ALL is weird. needs END_CATCH_ALL
#define GRAY_TRY_END			END_CATCH
#elif defined(_MSC_VER)
	// STL xstddef _TRY_BEGIN/_CATCH/_CATCH_ALL/_CATCH_END
#define GRAY_THROW				throw	// throw reference (STL)
#define GRAY_TRY 				_TRY_BEGIN
#define GRAY_TRY_CATCH(c,ex)	_CATCH(c& ex)	// c=cExceptionBase
#define GRAY_TRY_CATCHALL		_CATCH_ALL
#define GRAY_TRY_END			_CATCH_END
#else	// __linux__
#define GRAY_THROW				throw	// throw reference (STL)
#define GRAY_TRY				try {
#define GRAY_TRY_CATCH(c,ex)	} catch (c& ex) {	// c=cExceptionBase
#define GRAY_TRY_CATCHALL		} catch (...) {
#define GRAY_TRY_END			}
#endif

};

#endif // _INC_CExceptionBase_H
