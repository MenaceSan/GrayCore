//
//! @file cExceptionBase.h
//! Wrap base exception classes:
//! STL uses exception&
//! MFC uses CException* (must call Delete()?)
//! like C#/CLR uses Exception^
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cExceptionBase_H
#define _INC_cExceptionBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"
#include "StrT.h"
#include "cObject.h"
#include "cLogLevel.h"
#include "HResult.h"

#include <setjmp.h>
#if defined(_CPPUNWIND) && ! defined(_MFC_VER)	// NOT using _MFC_VER.
#include <exception> // use STL based std::exception class.
#endif

namespace Gray
{
	class cExceptionJmp
	{
		//! @class Gray::cExceptionJump
		//! Wrap the c setjmp(), longjmp() to make something like an exception. DANGER: NO unwind.
		//! https://en.wikipedia.org/wiki/Setjmp.h

		::jmp_buf _buf;		//! hold results of setjmp()

		int Init()
		{
			//! @return 0 = do nothing. setup ok. 1 = this is a jump return.
			return ::setjmp(OUT _buf);
		}

		void Jump(int ret)
		{
			//! @arg ret = return value to Init(). 0 = call again?, > 0 = errors.
			::longjmp(_buf, ret);
			// no return.
		}
	};

#if ! defined(_CPPUNWIND)	// like STL _HAS_EXCEPTIONS ?
	class cExceptionBase	// stub this out if no throws allowed.
	{
		//! @class Gray::cExceptionBase
		//! Stub out common base for exceptions. Normally this would be for MFC opr STL/std style execptions.
	public:
		virtual ~cExceptionBase()
		{
		}
		virtual const char* what() const = 0;	// can throw ? strange. THROW_DEF
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

}

#endif // _INC_cExceptionBase_H
