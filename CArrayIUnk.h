//
//! @file CArrayIUnk.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArrayIUnk_H
#define _INC_CArrayIUnk_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArray.h"
#include "CIUnkPtr.h"

namespace Gray
{
	template<class TYPE>
	class CArrayIUnk : public CArrayFacade < CIUnkPtr<TYPE>, TYPE* >
	{
		//! @class Gray::CArrayIUnk
		//! All items in this array are base on IUnknown and maybe on CSmartBase
		//! The array owns a ref to the object like CIUnkPtr.
		//! It will get deleted when all refs are gone.
	};
};
#endif // _INC_CArrayIUnk_H
