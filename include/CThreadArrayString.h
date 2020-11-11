//
//! @file CThreadArrayString.h
//! thread safe array of strings (and sorted strings)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_CThreadArrayString_H
#define _INC_CThreadArrayString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cThreadLock.h"
#include "cArrayString.h"

namespace Gray
{
	template<class TYPE>
	class CThreadArraySortString
	: protected cArraySortString < TYPE >
	{
		//! @class Gray::CThreadArraySortString
		//! Thread safe array of strings
		//! Create an alpha sorted string lookup table. CASE IGNORED !
	public:
		mutable cThreadLockCount m_Lock;
	};
};

#endif
