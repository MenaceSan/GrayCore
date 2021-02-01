//
//! @file StrT.cpp
//! String global functions as a template.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrT.h"
#include "cHeap.h"

namespace Gray
{
	const char StrT::k_szBlockStart[STR_BLOCK_QTY + 1] = "\"{[("; // STR_BLOCK_TYPE
	const char StrT::k_szBlockEnd[STR_BLOCK_QTY + 1] = "\"}])";

	const char StrT::k_szEscEncode[12] = "\'\"\?\\abtnvfr";			// encoded form.
	const char StrT::k_szEscDecode[12] = "\'\"\?\\\a\b\t\n\v\f\r";	// decoded form.
}

//*************************************************************
#include "StrT.inl"

//! force a template function to instantiate for TYPE of char and wchar_t. Explicit instantiation

namespace Gray
{
#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template struct GRAYCORE_LINK StrX<char>;
	template struct GRAYCORE_LINK StrX<wchar_t>;
#endif

	// even GRAY_STATICLIB needs this to expose the symbols. but only in vs2019.
#define TYPE char
#define StrTTbl(returntype, name, args) template GRAYCORE_LINK returntype GRAYCALL StrT::name<TYPE> args ;
#include "StrT.tbl"
#undef TYPE

#define TYPE wchar_t
#include "StrT.tbl"
#undef TYPE
#undef StrTTbl

}
