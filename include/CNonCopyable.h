//
//! @file cNonCopyable.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cNonCopyable_H
#define _INC_cNonCopyable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	class GRAYCORE_LINK cNonCopyable 
	{
		//! @class Gray::cNonCopyable
		//! Block C++ usage of default copy constructor. base a class on this with protected type.
		//! Similar to use of IS_DELETE = delete
		//! http://stackoverflow.com/questions/4172722/what-is-the-rule-of-three
		//! @note don't use inheritance for templates defined in DLL/SO that might have statics. Use the NonCopyable_IMPL instead. 
		//!  Avoid GRAYCORE_LINK for templates.

	protected:
		//! Force the use of Factory creation via protected constructor.
		inline cNonCopyable() noexcept
		{}
		inline ~cNonCopyable() noexcept
		{}
	
		//! Restrict the copy constructor and assignment operator
		//! __GNUC__ - defaulted and deleted functions only available with -std=c++11 or -std=gnu++11
		//! Make this a macro to avoid linkage inconsistency with use in DLL/SO and templates.
#define NonCopyable_IMPL(_TYPE) private: inline _TYPE(const _TYPE&) IS_DELETE; inline const _TYPE& operator=(const _TYPE&) IS_DELETE;

		NonCopyable_IMPL(cNonCopyable);
	};
} 
#endif // cNonCopyable
