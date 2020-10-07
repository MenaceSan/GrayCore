//
//! @file CNonCopyable.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CNonCopyable_H
#define _INC_CNonCopyable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	class GRAYCORE_LINK CNonCopyable // 
	{
		//! @class Gray::CNonCopyable
		//! Block C++ usage of default copy constructor. base a class on this with protected type.
		//! http://stackoverflow.com/questions/4172722/what-is-the-rule-of-three
		//! @note don't use inheritance for templates defined in DLL/SO that might have statics. Use the NonCopyable_IMPL instead. 
		//!  Avoid GRAYCORE_LINK for templates.

	protected:
		//! Force the use of Factory creation via protected constructor.
		CNonCopyable()
		{}
		~CNonCopyable()
		{}
	
		//! Restrict the copy constructor and assignment operator
		//! __GNUC__ - defaulted and deleted functions only available with -std=c++11 or -std=gnu++11
		//! Make this a macro to avoid linkage inconsistency with use in DLL/SO and templates.
#define NonCopyable_IMPL(_TYPE) private: _TYPE(const _TYPE&) IS_DELETE; const _TYPE& operator=(const _TYPE&) IS_DELETE;

		NonCopyable_IMPL(CNonCopyable);
	};
};
#endif // CNonCopyable