//
//! @file CHashTable.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CHashTable.h"

#if USE_UNITTESTS
#include "CUnitTest.h"
#include "CNewPtr.h"
#include "CLogMgr.h"

namespace Gray
{
	class CUnitTestHashStruct
	{
	private:
		HASHCODE_t m_nHashCode;
	public:
		CUnitTestHashStruct(HASHCODE_t nHashCode = 0)
		: m_nHashCode(nHashCode)
		{
		}
		HASHCODE_t get_HashCode() const
		{
			return m_nHashCode;
		}
	};

	class CUnitTestHashSmart : public CSmartBase, public CUnitTestHashStruct
	{
	public:
		CUnitTestHashSmart(HASHCODE_t nHashCode = 0)
		: CUnitTestHashStruct(nHashCode)
		{
		}
		HASHCODE_t get_HashCode() const	// resolve ambiguous.
		{
			return CUnitTestHashStruct::get_HashCode();
		}
	};
};

UNITTEST_CLASS(CHashTableT)
{
	UNITTEST_METHOD(CHashTableT)
	{
		//! @todo add and remove stuff. CUnitTestHashSmart

		CHashTableStruct<CUnitTestHashStruct, HASHCODE_t, 5> hashtable1;
		UNITTEST_TRUE(hashtable1.get_HashArrayQty() == 32);
		hashtable1.Add(CUnitTestHashStruct(123));
		// UNITTEST_TRUE(hashtable1.FindArgForKey(123) == t1.get_Ptr());

		CHashTableSmart<CUnitTestHashSmart, HASHCODE_t, 5> hashtable2;
		UNITTEST_TRUE(hashtable1.get_HashArrayQty() == 32);
		CSmartPtr<CUnitTestHashSmart> t2( new CUnitTestHashSmart(123));
		hashtable2.Add(t2);
		UNITTEST_TRUE(hashtable2.FindArgForKey(123) == t2);
	}
};
UNITTEST_REGISTER(CHashTableT, UNITTEST_LEVEL_Core);
#endif
