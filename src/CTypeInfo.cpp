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

#ifdef _CPPRTTI

	const void** GRAYCALL cTypeInfo::Get_vtable(void* p) // static 
	{
		// Get the vtable of the object.
		// Assume this is a 'new' returned pointer.

		return nullptr;
	}

	const char* GRAYCALL cTypeInfo::GetMemberName_TODO(const void** vtable, int i)   // static
	{
		//! read the __vfptr/vtable AKA _vptr, vftable to get a list of names of the virtual exposed members.
		//! @todo enum List of members.
		//! @return nullptr = end of list.
		if (i < 0)
			return nullptr;
		if (i > 1)
			return nullptr;

		// _MSC_VER
		// __GNUC__

		return "test";
	}
#endif

}
