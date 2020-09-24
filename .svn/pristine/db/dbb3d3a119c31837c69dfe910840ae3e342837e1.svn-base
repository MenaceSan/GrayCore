//
//! @file CArrayNew.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArrayNew_H
#define _INC_CArrayNew_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArray.h"
#include "CNewPtr.h"

namespace Gray
{
	template<class TYPE>
	class CArrayNew : public CArrayFacade < CNewPtr<TYPE>, TYPE* >
	{
		//! @class Gray::CArrayNew
		//! The point of this type is that the array now OWNS the element.
		//! It will get deleted when the array is deleted.
		//! @note try to use CArraySmart<> instead.
	};
}
#endif
