//
//! @file cThreadArrayString.h
//! thread safe array of strings (and sorted strings)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadArrayString_H
#define _INC_cThreadArrayString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cThreadLock.h"
#include "cArrayString.h"

namespace Gray
{
	template<class TYPE>
	class cThreadArraySortString
	: protected cArraySortString < TYPE >
	{
		//! @class Gray::cThreadArraySortString
		//! Thread safe array of strings
		//! Create an alpha sorted string lookup table. CASE IGNORED !
	public:
		mutable cThreadLockCount m_Lock;
	};
};

#endif
