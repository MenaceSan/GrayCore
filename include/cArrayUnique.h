//
//! @file cArrayUnique.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayUnique_H
#define _INC_cArrayUnique_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cUniquePtr.h"

namespace Gray
{
	template<class TYPE>
	class cArrayUnique : public cArrayFacade < cUniquePtr<TYPE>, TYPE* >
	{
		//! @class Gray::cArrayUnique
		//! The point of this type is that the array now OWNS the element. using cUniquePtr<>
		//! It will get deleted when the array is deleted.
		//! @note try to use cArrayRef<> instead.
	};
}
#endif
