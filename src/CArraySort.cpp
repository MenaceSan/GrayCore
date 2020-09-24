//
//! @file CArraySort.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CArraySort.h"

#if defined(USE_UNITTESTS)
#include "CUnitTest.h"
#include "CLogMgr.h"
#include "CRandomDef.h"
#include "CNewPtr.h"

namespace Gray
{
	class CUnitTestArraySort
	{
	public:
		int m_iSortVal;
		// HASHCODE_t m_nHashCode;
		static int sm_nAllocated;
	public:
		CUnitTestArraySort(int iSortVal)
			: m_iSortVal(iSortVal)
		{
			sm_nAllocated++;
		}
		~CUnitTestArraySort()
		{
			sm_nAllocated--;
		}
		int get_SortValue() const
		{
			return m_iSortVal;
		}
	};

	int CUnitTestArraySort::sm_nAllocated = 0;
}

UNITTEST_CLASS(CArraySort)
{
	UNITTEST_METHOD(CArraySort)
	{
		//! Test sorted arrays. pick a simple template.

		// Test QSort. Create test array of random data then sort it.
		CArrayVal<UINT> aVals;
		for (int i = 0; i < 300; i++)
		{
			aVals.Add(g_Rand.get_RandUns());
		}

		UNITTEST_TRUE(!aVals.isArraySorted());
		aVals.QSort();
		UNITTEST_TRUE(aVals.isArraySorted());

		// Test a list of CNewPtr things sorted.
		UNITTEST_TRUE(CUnitTestArraySort::sm_nAllocated == 0);
		{
			CArraySortFacadeValue< CNewPtr<CUnitTestArraySort>, CUnitTestArraySort*, int > aSortNew;
			aSortNew.Add(new CUnitTestArraySort(1));
			aSortNew.Add(new CUnitTestArraySort(2));
			aSortNew.Add(new CUnitTestArraySort(5));
			aSortNew.Add(new CUnitTestArraySort(4));
			aSortNew.Add(new CUnitTestArraySort(0));
			aSortNew.Add(new CUnitTestArraySort(3));
			UNITTEST_TRUE(aSortNew.isArraySorted());
			UNITTEST_TRUE(aSortNew.GetSize() == 6);
			UNITTEST_TRUE(CUnitTestArraySort::sm_nAllocated == 6);
			UNITTEST_TRUE(aSortNew[3]->m_iSortVal == 3);
			UNITTEST_TRUE(aSortNew[4]->m_iSortVal == 4);

			bool bRet = aSortNew.RemoveKey(4);
			UNITTEST_TRUE(bRet);

			UNITTEST_TRUE(aSortNew.isArraySorted());
			UNITTEST_TRUE(aSortNew.GetSize() == 5);
			UNITTEST_TRUE(CUnitTestArraySort::sm_nAllocated == 5);
			UNITTEST_TRUE(aSortNew[3]->m_iSortVal == 3);
			UNITTEST_TRUE(aSortNew[4]->m_iSortVal == 5);
		}
		UNITTEST_TRUE(CUnitTestArraySort::sm_nAllocated == 0);

	}
};
UNITTEST_REGISTER(CArraySort, UNITTEST_LEVEL_Core);
#endif
