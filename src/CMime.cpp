//
//! @file cMime.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cMime.h"
#include "StrT.h"

namespace Gray
{
	const cMime cMime::k_Type[MIME_QTY] = // static
	{
	#define cMimeType(a,b,c,d,e)	{ b, c, d },
	#include "cMimeTypes.tbl"
	#undef cMimeType
	};

	MIME_TYPE GRAYCALL cMime::FindMimeTypeForExt(const char* pszExt, MIME_TYPE eMimeTypeDefault) // static
	{
		//! For a given file '.ext', find the MIME_TYPE for it. read from cMimeTypes.tbl
		//! @note we could check for text files vs binary files ?

		for (COUNT_t i = 0; i < _countof(k_Type); i++)
		{
			if (StrT::CmpI(pszExt, k_Type[i].m_pExt))
				return (MIME_TYPE)i;
			if (StrT::CmpI(pszExt, k_Type[i].m_pExt2))
				return (MIME_TYPE)i;
		}

		return eMimeTypeDefault;
	}

	const char* GRAYCALL cMime::GetMimeTypeName(MIME_TYPE eMimeType) // static
	{
		if (IS_INDEX_BAD(eMimeType, MIME_QTY))
		{
			eMimeType = MIME_TEXT;
		}
		return k_Type[eMimeType].m_pszName;
	}

	MIME_TYPE GRAYCALL cMime::FindMimeTypeName(const char* pszName) // static
	{
		//! NOT exactly the same as Str_TableFindHead() ?
		if (pszName == nullptr)
		{
			return MIME_UNKNOWN;
		}
		for (COUNT_t i = 0; i < _countof(k_Type); i++)
		{
			const char* pszNamePrefix = k_Type[i].m_pszName;
			if (StrT::StartsWithI(pszName, pszNamePrefix))
			{
				return (MIME_TYPE)i;
			}
		}
		return MIME_UNKNOWN;
	}
}
