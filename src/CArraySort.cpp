//
//! @file cArraySort.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cArraySort.h"

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cLogMgr.h"
#include "cRandomDef.h"
#include "cNewPtr.h"

namespace Gray
{
	class cUnitTestArraySort
	{
	public:
		int m_iSortVal;
		// HASHCODE_t m_nHashCode;
		static int sm_nAllocated;
	public:
		cUnitTestArraySort(int iSortVal)
			: m_iSortVal(iSortVal)
		{
			sm_nAllocated++;
		}
		~cUnitTestArraySort()
		{
			sm_nAllocated--;
		}
		int get_SortValue() const
		{
			return m_iSortVal;
		}
	};

	int cUnitTestArraySort::sm_nAllocated = 0;
}

UNITTEST_CLASS(cArraySort)
{
	UNITTEST_METHOD(cArraySort)
	{
		//! Test sorted arrays. pick a simple template.

		// Test QSort. Create test array of random data then sort it.
		cArrayVal<UINT> aVals;
		for (int i = 0; i < 300; i++)
		{
			aVals.Add(g_Rand.get_RandUns());
		}

		UNITTEST_TRUE(!aVals.isArraySorted());
		aVals.QSort();
		UNITTEST_TRUE(aVals.isArraySorted());

		// Test a list of cNewPtr things sorted.
		UNITTEST_TRUE(cUnitTestArraySort::sm_nAllocated == 0);
		{
			cArraySortFacadeValue< cNewPtr<cUnitTestArraySort>, cUnitTestArraySort*, int > aSortNew;
			aSortNew.Add(new cUnitTestArraySort(1));
			aSortNew.Add(new cUnitTestArraySort(2));
			aSortNew.Add(new cUnitTestArraySort(5));
			aSortNew.Add(new cUnitTestArraySort(4));
			aSortNew.Add(new cUnitTestArraySort(0));
			aSortNew.Add(new cUnitTestArraySort(3));
			UNITTEST_TRUE(aSortNew.isArraySorted());
			UNITTEST_TRUE(aSortNew.GetSize() == 6);
			UNITTEST_TRUE(cUnitTestArraySort::sm_nAllocated == 6);
			UNITTEST_TRUE(aSortNew[3]->m_iSortVal == 3);
			UNITTEST_TRUE(aSortNew[4]->m_iSortVal == 4);

			bool bRet = aSortNew.RemoveKey(4);
			UNITTEST_TRUE(bRet);

			UNITTEST_TRUE(aSortNew.isArraySorted());
			UNITTEST_TRUE(aSortNew.GetSize() == 5);
			UNITTEST_TRUE(cUnitTestArraySort::sm_nAllocated == 5);
			UNITTEST_TRUE(aSortNew[3]->m_iSortVal == 3);
			UNITTEST_TRUE(aSortNew[4]->m_iSortVal == 5);
		}
		UNITTEST_TRUE(cUnitTestArraySort::sm_nAllocated == 0);

	}
};
UNITTEST_REGISTER(cArraySort, UNITTEST_LEVEL_Core);
#endif
