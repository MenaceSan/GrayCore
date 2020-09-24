//
//! @file CArrayString.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CArrayString.h"
#include "CArraySortString.h"

#if defined(USE_UNITTESTS)
#include "CUnitTest.h"
#include "CLogMgr.h"
#include "CArraySort.h"

UNITTEST_CLASS(CArraySortString)
{
	UNITTEST_METHOD(CArraySortString)
	{
		//! Test sorted arrays. pick a simple template.

		CArrayString<> arrayUns;
		UNITTEST_TRUE(arrayUns.GetSize() == 0);

		CArraySortStringA array1;
		for (ITERATE_t i = 0; !k_asTextLines[i].isNull(); i++)
		{
			array1.AddStr(k_asTextLines[i]);
		}
		UNITTEST_TRUE(array1.GetSize() == k_TEXTLINES_QTY);

		StrLen_t iLength = 0;
		GRAY_FOREACH(cString, sVal, array1)
		{
			iLength += sVal.GetLength();
		}
		UNITTEST_TRUE(iLength >= 66);

		cStringA* ppData = array1.GetData();
		UNITTEST_TRUE(ppData != nullptr);

		UNITTEST_TRUE(array1.isArraySorted());
	}
};
UNITTEST_REGISTER(CArraySortString, UNITTEST_LEVEL_Core);
#endif
