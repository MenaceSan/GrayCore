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

namespace Gray {
/// <summary>
/// An array OWNS the elements using cUniquePtr
/// It will get deleted when the array is deleted.
/// @note try to use cArrayRef instead.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cArrayUnique : public cArrayFacade<cSpanX<cUniquePtr<TYPE>, TYPE*>> {};
}  // namespace Gray
#endif
