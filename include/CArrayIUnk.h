//! @file cArrayIUnk.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArrayIUnk_H
#define _INC_cArrayIUnk_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArray.h"
#include "cIUnkPtr.h"

namespace Gray {
/// <summary>
/// All items in this array are base on IUnknown and maybe on cRefBase
/// The array owns a ref to the object like cIUnkPtr.
/// It will get deleted when all refs are gone.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cArrayIUnk : public cArrayFacade<cSpanX<cIUnkPtr<TYPE>, TYPE*>> {};
}  // namespace Gray
#endif  // _INC_cArrayIUnk_H
