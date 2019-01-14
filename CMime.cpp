//
//! @file CMime.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "CMime.h"
#include "StrT.h"

namespace Gray
{

	const char* const CMime::k_aszMimeType[MIME_QTY + 1] =
	{
	#define CMimeType(a,b,c,d,e)	b,
	#include "CMimeTypes.tbl"
	#undef CMimeType
		nullptr,				// MIME_QTY
	};
	const char* const CMime::k_aszMimeExt[(MIME_QTY * 2) + 1] =
	{
	#define CMimeType(a,b,c,d,e)	c, d,
	#include "CMimeTypes.tbl"
	#undef CMimeType
		nullptr,				// MIME_QTY*2
	};

	MIME_TYPE GRAYCALL CMime::FindMimeTypeForExt(const char* pszExt, MIME_TYPE eMimeTypeDefault) // static
	{
		//! For a given file '.ext', find the MIME_TYPE for it. CMIMEType
		//! @note we could check for text files vs binary files ?

		ITERATE_t iType = STR_TABLEFIND_N(pszExt, k_aszMimeExt);
		if (iType < 0)
			return eMimeTypeDefault;
		return (MIME_TYPE)(iType / 2);
	}

	const char* GRAYCALL CMime::GetMimeTypeName(MIME_TYPE eMimeType) // static
	{
		ASSERT(_countof(k_aszMimeType) == MIME_QTY + 1);
		if (IS_INDEX_BAD(eMimeType, MIME_QTY))
		{
			eMimeType = MIME_TEXT;
		}
		return k_aszMimeType[eMimeType];
	}

	MIME_TYPE GRAYCALL CMime::FindMimeTypeName(const char* pszName) // static
	{
		//! NOT exactly the same as Str_TableFindHead() ?
		if (pszName == nullptr)
		{
			return MIME_UNKNOWN;
		}
		for (UINT i = 0; i < _countof(k_aszMimeType) - 1; i++)
		{
			if (!StrT::CmpIN(pszName, k_aszMimeType[i], StrT::Len(k_aszMimeType[i])))
			{
				return (MIME_TYPE)i;
			}
		}
		return MIME_UNKNOWN;
	}
}
