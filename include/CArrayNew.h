//
//! @file cArrayNew.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayNew_H
#define _INC_cArrayNew_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cNewPtr.h"

namespace Gray
{
	template<class TYPE>
	class cArrayNew : public cArrayFacade < cNewPtr<TYPE>, TYPE* >
	{
		//! @class Gray::cArrayNew
		//! The point of this type is that the array now OWNS the element.
		//! It will get deleted when the array is deleted.
		//! @note try to use cArrayRef<> instead.
	};
}
#endif
