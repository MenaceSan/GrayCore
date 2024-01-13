//
//! @file cThreadArrayString.h
//! thread safe array of strings (and sorted strings)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadArrayString_H
#define _INC_cThreadArrayString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayString.h"
#include "cThreadLock.h"

namespace Gray {
/// <summary>
/// Thread safe array of strings. Create an alpha sorted string lookup table. CASE IGNORED !
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cThreadArraySortString : protected cArraySortString<TYPE> {
 public:
    mutable cThreadLockCount m_Lock;
};
}  // namespace Gray
#endif
