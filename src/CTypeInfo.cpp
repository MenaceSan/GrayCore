//
//! @file cTypeInfo.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cTypeInfo.h"
#include "cTypes.h"

namespace Gray
{

#ifndef _MSC_VER	// __GNUC__
#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h)  template<> const _TYPE cTypeLimit<_TYPE>::k_Min = e; template<> const _TYPE cTypeLimit<_TYPE>::k_Max = f; 
#include "cTypes.tbl"
#undef CTYPE_DEF
#endif

	const char* cTypeInfo::GetMemberName(int i) const
	{
		//! read the vptr/vtable to get a list of names of the virtual exposed members.
		//! @todo Get List of members.
		//! @return nullptr = end of list.
		if (i < 0)
			return nullptr;
		if (i > 1)
			return nullptr;

		// _MSC_VER
		// __GNUC__

		return "test";
	}
}
