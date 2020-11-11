//
//! @file cArrayIUnk.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayIUnk_H
#define _INC_cArrayIUnk_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cIUnkPtr.h"

namespace Gray
{
	template<class TYPE>
	class cArrayIUnk : public cArrayFacade < cIUnkPtr<TYPE>, TYPE* >
	{
		//! @class Gray::cArrayIUnk
		//! All items in this array are base on IUnknown and maybe on cRefBase
		//! The array owns a ref to the object like cIUnkPtr.
		//! It will get deleted when all refs are gone.
	};
}
#endif // _INC_cArrayIUnk_H
