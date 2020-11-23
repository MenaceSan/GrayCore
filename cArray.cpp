//
//! @file cArray.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cArray.h"
#include "cString.h"
#include "cArraySortString.h"

namespace Gray
{

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK CArray < cStringT<char>, const char* >;
	template class GRAYCORE_LINK cArrayTyped < cStringT<char>, const char* >;

	template class GRAYCORE_LINK cArrayString < char >;
	template class GRAYCORE_LINK cArrayString < wchar_t >;
#endif

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cArraySorted < cStringT<char>, const char*, const char* >;
	template class GRAYCORE_LINK cArraySorted < cStringT<wchar_t>, const wchar_t*, const wchar_t* >;
	template class GRAYCORE_LINK cArraySortString < char >;
	template class GRAYCORE_LINK cArraySortString < wchar_t >;
#endif

}
